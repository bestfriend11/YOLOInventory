#include "SYIFragmentDashboard.h"

#include "YIAffix.h"
#include "YIAffixAsset.h"
#include "YIFragmentAsset.h"
#include "YIFragmentPoolAsset.h"
#include "YIItemTraitAsset.h"
#include "YIFragmentAssetFactory.h"
#include "YIFragmentPoolAssetFactory.h"
#include "YIItemTraitAssetFactory.h"
#include "YIInventoryBlueprintLibrary.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "PropertyCustomizationHelpers.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Styling/AppStyle.h"
#include "FileHelpers.h"
#include "UObject/UObjectIterator.h"

namespace
{
static UScriptStruct* YIResolveStructFromPath(const FString& StructPath)
{
	if (StructPath.IsEmpty())
	{
		return nullptr;
	}
	if (UScriptStruct* Found = FindObject<UScriptStruct>(nullptr, *StructPath))
	{
		return Found;
	}
	return LoadObject<UScriptStruct>(nullptr, *StructPath);
}

static FString YIMakeReadableFragmentName(const UScriptStruct* FragmentStruct)
{
	if (!FragmentStruct)
	{
		return TEXT("Fragment");
	}

	const FString MetaDisplayName = FragmentStruct->GetMetaData(TEXT("DisplayName"));
	if (!MetaDisplayName.IsEmpty())
	{
		return MetaDisplayName;
	}

	FString Name = FragmentStruct->GetName();
	Name.RemoveFromStart(TEXT("FYI"));
	Name.RemoveFromEnd(TEXT("DefinitionFragment"));
	Name.RemoveFromEnd(TEXT("Fragment"));
	Name = Name.TrimStartAndEnd();
	return Name.IsEmpty() ? FragmentStruct->GetName() : Name;
}

static void YICollectFragmentStructOptions(const UScriptStruct* BaseStruct, TArray<TSharedPtr<FString>>& OutPaths)
{
	OutPaths.Reset();
	if (!BaseStruct)
	{
		return;
	}

	TArray<FString> Paths;
	for (TObjectIterator<UScriptStruct> It; It; ++It)
	{
		UScriptStruct* Struct = *It;
		if (!Struct || Struct == BaseStruct)
		{
			continue;
		}
		if (Struct->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) || Struct->IsNative() == false)
		{
			continue;
		}
		if (!Struct->IsChildOf(BaseStruct))
		{
			continue;
		}
		Paths.AddUnique(Struct->GetPathName());
	}

	Paths.Sort([](const FString& A, const FString& B)
	{
		UScriptStruct* SA = YIResolveStructFromPath(A);
		UScriptStruct* SB = YIResolveStructFromPath(B);
		return YIMakeReadableFragmentName(SA) < YIMakeReadableFragmentName(SB);
	});

	for (const FString& Path : Paths)
	{
		OutPaths.Add(MakeShared<FString>(Path));
	}
}

static bool YIFragmentDashboard_SaveObjectPackage(UObject* ObjectToSave)
{
	if (!ObjectToSave)
	{
		return false;
	}

	UPackage* Package = ObjectToSave->GetOutermost();
	if (!Package)
	{
		return false;
	}

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(Package);
	const bool bCheckDirty = false;
	const bool bPromptToSave = true;
	return FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave) == FEditorFileUtils::PR_Success;
}
}

void SYIFragmentDashboard::Construct(const FArguments& InArgs)
{
	LayoutMode = InArgs._LayoutMode;
	RefreshFragmentStructOptions();
	LastActionStatus = NSLOCTEXT("YOLOInventory", "FragmentDashboard_SelectAssetHint", "Select a Fragment Asset or Item Trait Asset to author fragments. Legacy affix assets are optional.");

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bAllowSearch = true;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::ObjectsUseNameArea;
	DetailsView = PropertyEditorModule.CreateDetailView(DetailsArgs);

	if (LayoutMode == EYIFragmentDashboardLayout::AssetListOnly)
	{
		ChildSlot
		[
			GetAssetPanelWidget()
		];
		return;
	}

	ChildSlot
	[
		SNew(SSplitter)
		+ SSplitter::Slot().Value(0.30f)
		[
			GetAssetPanelWidget()
		]
		+ SSplitter::Slot().Value(0.45f)
		[
			GetDetailsPanelWidget()
		]
		+ SSplitter::Slot().Value(0.25f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(0.45f)
			[
				GetMappingPanelWidget()
			]
			+ SVerticalBox::Slot().FillHeight(0.55f).Padding(0.f, 6.f, 0.f, 0.f)
			[
				GetPreviewPanelWidget()
			]
		]
	];
}

void SYIFragmentDashboard::OpenAsset(UObject* Asset)
{
	SetSelectedAsset(Asset);
}

TSharedRef<SWidget> SYIFragmentDashboard::GetAssetPanelWidget() const
{
	SYIFragmentDashboard* Self = const_cast<SYIFragmentDashboard*>(this);
	if (!Self->AssetPanelWidget.IsValid())
	{
		Self->AssetPanelWidget = Self->BuildAssetPanelWidget();
	}
	return Self->AssetPanelWidget.ToSharedRef();
}

TSharedRef<SWidget> SYIFragmentDashboard::GetDetailsPanelWidget() const
{
	SYIFragmentDashboard* Self = const_cast<SYIFragmentDashboard*>(this);
	if (!Self->DetailsPanelWidget.IsValid())
	{
		Self->DetailsPanelWidget = Self->BuildDetailsPanelWidget();
	}
	return Self->DetailsPanelWidget.ToSharedRef();
}

TSharedRef<SWidget> SYIFragmentDashboard::GetMappingPanelWidget() const
{
	SYIFragmentDashboard* Self = const_cast<SYIFragmentDashboard*>(this);
	if (!Self->MappingPanelWidget.IsValid())
	{
		Self->MappingPanelWidget = Self->BuildMappingPanelWidget();
	}
	return Self->MappingPanelWidget.ToSharedRef();
}

TSharedRef<SWidget> SYIFragmentDashboard::GetPreviewPanelWidget() const
{
	SYIFragmentDashboard* Self = const_cast<SYIFragmentDashboard*>(this);
	if (!Self->PreviewPanelWidget.IsValid())
	{
		Self->PreviewPanelWidget = Self->BuildPreviewPanelWidget();
	}
	return Self->PreviewPanelWidget.ToSharedRef();
}

void SYIFragmentDashboard::SaveCurrentAssetFromToolbar()
{
	YIFragmentDashboard_SaveObjectPackage(SelectedAsset.Get());
}

void SYIFragmentDashboard::RefreshFragmentStructOptions()
{
	YICollectFragmentStructOptions(FYIItemDefinitionFragmentBase::StaticStruct(), ItemDefinitionFragmentStructOptions);
	YICollectFragmentStructOptions(FYIAffixDefinitionFragmentBase::StaticStruct(), AffixDefinitionFragmentStructOptions);

	if (!SelectedItemDefinitionFragmentStructOption.IsValid()
		|| !ItemDefinitionFragmentStructOptions.ContainsByPredicate([this](const TSharedPtr<FString>& Entry)
		{
			return Entry.IsValid() && SelectedItemDefinitionFragmentStructOption.IsValid() && *Entry == *SelectedItemDefinitionFragmentStructOption;
		}))
	{
		SelectedItemDefinitionFragmentStructOption = ItemDefinitionFragmentStructOptions.Num() > 0
			? ItemDefinitionFragmentStructOptions[0]
			: nullptr;
	}

	if (!SelectedAffixDefinitionFragmentStructOption.IsValid()
		|| !AffixDefinitionFragmentStructOptions.ContainsByPredicate([this](const TSharedPtr<FString>& Entry)
		{
			return Entry.IsValid() && SelectedAffixDefinitionFragmentStructOption.IsValid() && *Entry == *SelectedAffixDefinitionFragmentStructOption;
		}))
	{
		SelectedAffixDefinitionFragmentStructOption = AffixDefinitionFragmentStructOptions.Num() > 0
			? AffixDefinitionFragmentStructOptions[0]
			: nullptr;
	}
}

void SYIFragmentDashboard::SetActionStatus(const FText& InStatus, bool bIsError)
{
	LastActionStatus = InStatus;
	bLastActionError = bIsError;
}

TSharedRef<SWidget> SYIFragmentDashboard::BuildAssetPanelWidget()
{
	auto CanAuthorItemDefinitionFragments = [this]() -> bool
	{
		const UObject* Asset = SelectedAsset.Get();
		return Asset && (Asset->IsA<UYIItemTraitAsset>() || Asset->IsA<UYIFragmentAsset>());
	};

	auto CanAuthorAffixDefinitionFragments = [this]() -> bool
	{
		const UObject* Asset = SelectedAsset.Get();
		return Asset && (Asset->IsA<UYIAffixAsset>() || Asset->IsA<UYIFragmentAsset>());
	};

	auto AddItemDefinitionFragment = [this]() -> FReply
	{
		UObject* Asset = SelectedAsset.Get();
		if (!Asset)
		{
			SetActionStatus(NSLOCTEXT("YOLOInventory", "FragmentDashboard_NoAsset", "Select an asset first."), true);
			return FReply::Handled();
		}
		if (!SelectedItemDefinitionFragmentStructOption.IsValid())
		{
			SetActionStatus(NSLOCTEXT("YOLOInventory", "FragmentDashboard_NoItemFragmentType", "No item-definition fragment type is available."), true);
			return FReply::Handled();
		}

		UScriptStruct* FragmentStruct = YIResolveStructFromPath(*SelectedItemDefinitionFragmentStructOption);
		if (!FragmentStruct)
		{
			SetActionStatus(NSLOCTEXT("YOLOInventory", "FragmentDashboard_ItemFragmentInvalid", "Selected item fragment type is invalid."), true);
			return FReply::Handled();
		}

		bool bAdded = false;
		if (UYIItemTraitAsset* TraitAsset = Cast<UYIItemTraitAsset>(Asset))
		{
			bAdded = TraitAsset->FindOrAddDefinitionFragmentByStruct(FragmentStruct) != nullptr;
		}
		else if (UYIFragmentAsset* FragmentAsset = Cast<UYIFragmentAsset>(Asset))
		{
			bAdded = FragmentAsset->FindOrAddItemDefinitionFragmentByStruct(FragmentStruct) != nullptr;
		}

		if (bAdded)
		{
			Asset->Modify();
			Asset->MarkPackageDirty();
			if (DetailsView.IsValid())
			{
				DetailsView->SetObject(Asset, true);
			}
			SetActionStatus(FText::Format(
				NSLOCTEXT("YOLOInventory", "FragmentDashboard_ItemFragmentAdded", "Added item fragment: {0}"),
				FText::FromString(YIMakeReadableFragmentName(FragmentStruct))), false);
		}
		else
		{
			SetActionStatus(NSLOCTEXT("YOLOInventory", "FragmentDashboard_ItemFragmentAddFailed", "Could not add item fragment to selected asset type."), true);
		}

		return FReply::Handled();
	};

	auto AddAffixDefinitionFragment = [this]() -> FReply
	{
		UObject* Asset = SelectedAsset.Get();
		if (!Asset)
		{
			SetActionStatus(NSLOCTEXT("YOLOInventory", "FragmentDashboard_NoAsset", "Select an asset first."), true);
			return FReply::Handled();
		}
		if (!SelectedAffixDefinitionFragmentStructOption.IsValid())
		{
			SetActionStatus(NSLOCTEXT("YOLOInventory", "FragmentDashboard_NoAffixFragmentType", "No affix-definition fragment type is available."), true);
			return FReply::Handled();
		}

		UScriptStruct* FragmentStruct = YIResolveStructFromPath(*SelectedAffixDefinitionFragmentStructOption);
		if (!FragmentStruct)
		{
			SetActionStatus(NSLOCTEXT("YOLOInventory", "FragmentDashboard_AffixFragmentInvalid", "Selected affix fragment type is invalid."), true);
			return FReply::Handled();
		}

		bool bAdded = false;
		if (UYIAffixAsset* AffixAsset = Cast<UYIAffixAsset>(Asset))
		{
			bAdded = AffixAsset->FindOrAddDefinitionFragmentByStruct(FragmentStruct) != nullptr;
		}
		else if (UYIFragmentAsset* FragmentAsset = Cast<UYIFragmentAsset>(Asset))
		{
			bAdded = FragmentAsset->FindOrAddAffixDefinitionFragmentByStruct(FragmentStruct) != nullptr;
		}

		if (bAdded)
		{
			Asset->Modify();
			Asset->MarkPackageDirty();
			if (DetailsView.IsValid())
			{
				DetailsView->SetObject(Asset, true);
			}
			SetActionStatus(FText::Format(
				NSLOCTEXT("YOLOInventory", "FragmentDashboard_AffixFragmentAdded", "Added affix fragment: {0}"),
				FText::FromString(YIMakeReadableFragmentName(FragmentStruct))), false);
		}
		else
		{
			SetActionStatus(NSLOCTEXT("YOLOInventory", "FragmentDashboard_AffixFragmentAddFailed", "Could not add affix fragment to selected asset type."), true);
		}

		return FReply::Handled();
	};

	auto CreateFragmentAsset = [this]() -> FReply
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		UYIFragmentAssetFactory* Factory = NewObject<UYIFragmentAssetFactory>();
		UObject* NewAsset = AssetToolsModule.Get().CreateAsset(TEXT("NewFragmentAsset"), TEXT("/Game"), UYIFragmentAsset::StaticClass(), Factory);
		if (NewAsset)
		{
			SetSelectedAsset(NewAsset);
			SetActionStatus(NSLOCTEXT("YOLOInventory", "FragmentDashboard_CreatedFragmentAsset", "Created Fragment Asset. Add fragments in this panel and edit values in Details."), false);
		}
		else
		{
			SetActionStatus(NSLOCTEXT("YOLOInventory", "FragmentDashboard_CreateFragmentAssetFailed", "Failed to create Fragment Asset."), true);
		}
		return FReply::Handled();
	};

	auto CreateItemTraitAsset = [this]() -> FReply
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		UYIItemTraitAssetFactory* Factory = NewObject<UYIItemTraitAssetFactory>();
		UObject* NewAsset = AssetToolsModule.Get().CreateAsset(TEXT("NewItemTraitAsset"), TEXT("/Game"), UYIItemTraitAsset::StaticClass(), Factory);
		if (NewAsset)
		{
			SetSelectedAsset(NewAsset);
			SetActionStatus(NSLOCTEXT("YOLOInventory", "FragmentDashboard_CreatedTraitAsset", "Created Item Trait Asset. Add fragments in this panel and edit values in Details."), false);
		}
		else
		{
			SetActionStatus(NSLOCTEXT("YOLOInventory", "FragmentDashboard_CreateTraitAssetFailed", "Failed to create Item Trait Asset."), true);
		}
		return FReply::Handled();
	};

	auto CreateFragmentPoolAsset = [this]() -> FReply
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		UYIFragmentPoolAssetFactory* Factory = NewObject<UYIFragmentPoolAssetFactory>();
		UObject* NewAsset = AssetToolsModule.Get().CreateAsset(TEXT("NewFragmentPoolAsset"), TEXT("/Game"), UYIFragmentPoolAsset::StaticClass(), Factory);
		if (NewAsset)
		{
			SetSelectedAsset(NewAsset);
			SetActionStatus(NSLOCTEXT("YOLOInventory", "FragmentDashboard_CreatedPoolAsset", "Created Fragment Pool Asset. Fill entries to drive strategy-based runtime generation."), false);
		}
		else
		{
			SetActionStatus(NSLOCTEXT("YOLOInventory", "FragmentDashboard_CreatePoolAssetFailed", "Failed to create Fragment Pool Asset."), true);
		}
		return FReply::Handled();
	};

	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(8.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("YOLOInventory", "FragmentDashboard_Title", "Fragment Editor"))
				.Font(FAppStyle::Get().GetFontStyle("BoldFont"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 2.f, 0.f)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory", "FragmentDashboard_CreateFragmentAsset", "Create Fragment Asset"))
					.ToolTipText(NSLOCTEXT("YOLOInventory", "FragmentDashboard_CreateFragmentAsset_TT", "Create a generic fragment authoring asset (no C++ required)."))
					.OnClicked_Lambda([CreateFragmentAsset]() { return CreateFragmentAsset(); })
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f, 0.f, 0.f, 0.f)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory", "FragmentDashboard_CreateTraitAsset", "Create Item Trait Asset"))
					.ToolTipText(NSLOCTEXT("YOLOInventory", "FragmentDashboard_CreateTraitAsset_TT", "Create a reusable item-definition fragment bundle asset."))
					.OnClicked_Lambda([CreateItemTraitAsset]() { return CreateItemTraitAsset(); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("YOLOInventory", "FragmentDashboard_CreatePoolAsset", "Create Fragment Pool Asset"))
				.ToolTipText(NSLOCTEXT("YOLOInventory", "FragmentDashboard_CreatePoolAsset_TT", "Create a neutral pool asset for runtime fragment roll strategies."))
				.OnClicked_Lambda([CreateFragmentPoolAsset]() { return CreateFragmentPoolAsset(); })
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(UObject::StaticClass())
				.OnShouldFilterAsset_Lambda([this](const FAssetData& AssetData)
				{
					const UClass* AssetClass = AssetData.GetClass();
					if (!AssetClass)
					{
						return true;
					}
					const bool bIsLegacyAffix = AssetClass->IsChildOf(UYIAffixAsset::StaticClass());
					if (bIsLegacyAffix && !bShowLegacyAffixAuthoring)
					{
						return true;
					}
					return !(bIsLegacyAffix
						|| AssetClass->IsChildOf(UYIFragmentAsset::StaticClass())
						|| AssetClass->IsChildOf(UYIFragmentPoolAsset::StaticClass())
						|| AssetClass->IsChildOf(UYIItemTraitAsset::StaticClass()));
				})
				.ObjectPath_Lambda([this]()
				{
					return SelectedAsset.IsValid() ? SelectedAsset->GetPathName() : FString();
				})
				.OnObjectChanged_Lambda([this](const FAssetData& AssetData)
				{
					SetSelectedAsset(AssetData.GetAsset());
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]() { return bShowLegacyAffixAuthoring ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
				{
					bShowLegacyAffixAuthoring = (NewState == ECheckBoxState::Checked);
					if (!bShowLegacyAffixAuthoring && SelectedAsset.IsValid() && SelectedAsset->IsA<UYIAffixAsset>())
					{
						SetSelectedAsset(nullptr);
					}
				})
				[
					SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "FragmentDashboard_ShowLegacyAffixes", "Show legacy affix assets"))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					if (const UObject* Asset = SelectedAsset.Get())
					{
						return FText::Format(
							NSLOCTEXT("YOLOInventory", "FragmentDashboard_SelectedType", "Selected: {0}"),
							FText::FromString(Asset->GetClass()->GetName()));
					}
					return NSLOCTEXT("YOLOInventory", "FragmentDashboard_SelectedTypeNone", "Selected: <None>");
				})
				.ColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.75f, 0.75f)))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("YOLOInventory", "FragmentDashboard_AddPreset", "Add authoring fragments"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 2.f)
			[
				SNew(SVerticalBox)
				.Visibility_Lambda([CanAuthorItemDefinitionFragments]()
				{
					return CanAuthorItemDefinitionFragments() ? EVisibility::Visible : EVisibility::Collapsed;
				})
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 2.f)
				[
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&ItemDefinitionFragmentStructOptions)
					.OnComboBoxOpening_Lambda([this]()
					{
						RefreshFragmentStructOptions();
					})
					.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewItem, ESelectInfo::Type)
					{
						if (NewItem.IsValid())
						{
							SelectedItemDefinitionFragmentStructOption = NewItem;
						}
					})
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
					{
						const FString Path = InItem.IsValid() ? *InItem : FString();
						return SNew(STextBlock).Text(FText::FromString(YIMakeReadableFragmentName(YIResolveStructFromPath(Path))));
					})
					.Content()
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							if (!SelectedItemDefinitionFragmentStructOption.IsValid())
							{
								return NSLOCTEXT("YOLOInventory", "FragmentDashboard_SelectItemFragment", "Select item fragment");
							}
							return FText::FromString(YIMakeReadableFragmentName(YIResolveStructFromPath(*SelectedItemDefinitionFragmentStructOption)));
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory", "FragmentDashboard_AddItemFragment", "Add Item Fragment"))
					.OnClicked_Lambda([AddItemDefinitionFragment]()
					{
						return AddItemDefinitionFragment();
					})
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 2.f)
			[
				SNew(SVerticalBox)
				.Visibility_Lambda([CanAuthorAffixDefinitionFragments]()
				{
					return CanAuthorAffixDefinitionFragments() ? EVisibility::Visible : EVisibility::Collapsed;
				})
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 2.f)
				[
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&AffixDefinitionFragmentStructOptions)
					.OnComboBoxOpening_Lambda([this]()
					{
						RefreshFragmentStructOptions();
					})
					.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewItem, ESelectInfo::Type)
					{
						if (NewItem.IsValid())
						{
							SelectedAffixDefinitionFragmentStructOption = NewItem;
						}
					})
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
					{
						const FString Path = InItem.IsValid() ? *InItem : FString();
						return SNew(STextBlock).Text(FText::FromString(YIMakeReadableFragmentName(YIResolveStructFromPath(Path))));
					})
					.Content()
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							if (!SelectedAffixDefinitionFragmentStructOption.IsValid())
							{
								return NSLOCTEXT("YOLOInventory", "FragmentDashboard_SelectAffixFragment", "Select affix fragment");
							}
							return FText::FromString(YIMakeReadableFragmentName(YIResolveStructFromPath(*SelectedAffixDefinitionFragmentStructOption)));
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory", "FragmentDashboard_AddAffixFragment", "Add Legacy Affix Fragment"))
					.OnClicked_Lambda([AddAffixDefinitionFragment]()
					{
						return AddAffixDefinitionFragment();
					})
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 2.f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("YOLOInventory", "FragmentDashboard_BuildSnapshot", "Build Runtime Snapshot (Preview)"))
				.IsEnabled_Lambda([this]()
				{
					return Cast<UYIAffixAsset>(SelectedAsset.Get()) != nullptr;
				})
				.OnClicked_Lambda([this]()
				{
					UYIAffixAsset* AffixAsset = Cast<UYIAffixAsset>(SelectedAsset.Get());
					if (!AffixAsset)
					{
						SetActionStatus(NSLOCTEXT("YOLOInventory", "FragmentDashboard_SnapshotNeedsAffix", "Runtime snapshot preview requires an Affix Asset selection."), true);
						return FReply::Handled();
					}

					FYIAffixInstance Snapshot;
					const bool bBuilt = UYIInventoryBlueprintLibrary::BuildAffixSnapshot(
						AffixAsset,
						1,
						1337,
						true,
						Snapshot);
					if (bBuilt)
					{
						SetActionStatus(NSLOCTEXT("YOLOInventory", "FragmentDashboard_SnapshotBuilt", "Runtime snapshot built. Check Preview panel for rolled values."), false);
					}
					else
					{
						SetActionStatus(NSLOCTEXT("YOLOInventory", "FragmentDashboard_SnapshotFailed", "Runtime snapshot build failed for selected Affix Asset."), true);
					}
					return FReply::Handled();
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Text_Lambda([this]() { return LastActionStatus; })
				.ColorAndOpacity_Lambda([this]()
				{
					return bLastActionError
						? FSlateColor(FLinearColor(0.9f, 0.35f, 0.35f))
						: FSlateColor(FLinearColor(0.35f, 0.75f, 0.45f));
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("YOLOInventory", "FragmentDashboard_Save", "Save Selected Asset"))
				.OnClicked_Lambda([this]()
				{
					SaveCurrentAssetFromToolbar();
					return FReply::Handled();
				})
			]
		];
}

TSharedRef<SWidget> SYIFragmentDashboard::BuildDetailsPanelWidget()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(2.f)
		[
			DetailsView.IsValid()
				? StaticCastSharedRef<SWidget>(DetailsView.ToSharedRef())
				: SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "FragmentDashboard_DetailsUnavailable", "Details view unavailable."))
		];
}

TSharedRef<SWidget> SYIFragmentDashboard::BuildMappingPanelWidget()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(8.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("YOLOInventory", "FragmentDashboard_MappingTitle", "Fragment Authoring Notes"))
				.Font(FAppStyle::Get().GetFontStyle("BoldFont"))
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Text(NSLOCTEXT("YOLOInventory", "FragmentDashboard_MappingHelp", "Definition fragments are static authoring data stored on assets. Runtime/dynamic fragments are created per item instance at generation/equip time and are not authored directly here."))
			]
		];
}

TSharedRef<SWidget> SYIFragmentDashboard::BuildPreviewPanelWidget()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(8.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Text_Lambda([this]()
				{
					return BuildFragmentSummaryText();
				})
			]
		];
}

void SYIFragmentDashboard::SetSelectedAsset(UObject* InAsset)
{
	SelectedAsset = InAsset;
	RefreshFragmentStructOptions();
	SetActionStatus(
		InAsset
			? FText::Format(NSLOCTEXT("YOLOInventory", "FragmentDashboard_SelectedAssetStatus", "Selected {0}. Choose fragment type and click Add."),
				FText::FromString(InAsset->GetName()))
			: NSLOCTEXT("YOLOInventory", "FragmentDashboard_SelectAssetStatus", "Select an asset to start authoring fragments."),
		false);
	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(InAsset);
	}
}

FText SYIFragmentDashboard::BuildFragmentSummaryText() const
{
	if (const UYIAffixAsset* AffixAsset = Cast<UYIAffixAsset>(SelectedAsset.Get()))
	{
		FYIAffixResolvedDefinitionData Effective;
		AffixAsset->GetEffectiveDefinitionData(Effective);

		FYIAffixInstance Snapshot;
		const bool bSnapshotBuilt = UYIInventoryBlueprintLibrary::BuildAffixSnapshot(AffixAsset, 1, 1337, true, Snapshot);

		FString Summary = FString::Printf(TEXT("Definition Fragments: %d"), AffixAsset->DefinitionFragments.Num());
		for (const FInstancedStruct& Fragment : AffixAsset->DefinitionFragments)
		{
			const UScriptStruct* FragmentStruct = Fragment.GetScriptStruct();
			Summary += TEXT("\n- ");
			Summary += FragmentStruct ? FragmentStruct->GetName() : TEXT("<Invalid Fragment>");
		}
		Summary += FString::Printf(TEXT("\n\nEffective Tier: %d\nEffective Min/Max: %.2f / %.2f"),
			Effective.Tier,
			Effective.MinValue,
			Effective.MaxValue);
		if (bSnapshotBuilt)
		{
			Summary += FString::Printf(TEXT("\nSnapshot RolledValue: %.2f\nSnapshot Seed: %d"),
				Snapshot.RolledValue,
				Snapshot.Seed);
		}
		else
		{
			Summary += TEXT("\nSnapshot build failed.");
		}

		return FText::FromString(Summary);
	}

	if (const UYIFragmentAsset* FragmentAsset = Cast<UYIFragmentAsset>(SelectedAsset.Get()))
	{
		FString Summary = FString::Printf(
			TEXT("Fragment Asset\nItem Fragments: %d\nAffix Fragments: %d"),
			FragmentAsset->ItemDefinitionFragments.Num(),
			FragmentAsset->AffixDefinitionFragments.Num());
		Summary += TEXT("\n\nAffix Fragment Types:");
		for (const FInstancedStruct& Fragment : FragmentAsset->AffixDefinitionFragments)
		{
			const UScriptStruct* FragmentStruct = Fragment.GetScriptStruct();
			Summary += TEXT("\n- ");
			Summary += FragmentStruct ? FragmentStruct->GetName() : TEXT("<Invalid Fragment>");
		}
		Summary += TEXT("\n\nItem Fragment Types:");
		for (const FInstancedStruct& Fragment : FragmentAsset->ItemDefinitionFragments)
		{
			const UScriptStruct* FragmentStruct = Fragment.GetScriptStruct();
			Summary += TEXT("\n- ");
			Summary += FragmentStruct ? FragmentStruct->GetName() : TEXT("<Invalid Fragment>");
		}
		return FText::FromString(Summary);
	}

	if (const UYIItemTraitAsset* TraitAsset = Cast<UYIItemTraitAsset>(SelectedAsset.Get()))
	{
		FString Summary = FString::Printf(
			TEXT("Item Trait Asset\nDefinition Fragments: %d"),
			TraitAsset->DefinitionFragments.Num());
		Summary += TEXT("\n\nDefinition Fragment Types:");
		for (const FInstancedStruct& Fragment : TraitAsset->DefinitionFragments)
		{
			const UScriptStruct* FragmentStruct = Fragment.GetScriptStruct();
			Summary += TEXT("\n- ");
			Summary += FragmentStruct ? FragmentStruct->GetName() : TEXT("<Invalid Fragment>");
		}
		return FText::FromString(Summary);
	}

	if (const UYIFragmentPoolAsset* PoolAsset = Cast<UYIFragmentPoolAsset>(SelectedAsset.Get()))
	{
		FString Summary = FString::Printf(
			TEXT("Fragment Pool Asset\nEntries: %d"),
			PoolAsset->Entries.Num());
		for (const FYIFragmentPoolEntry& Entry : PoolAsset->Entries)
		{
			Summary += FString::Printf(
				TEXT("\n- %s (Weight=%.2f, Level=%d..%d)"),
				*Entry.FragmentAsset.ToSoftObjectPath().GetAssetName(),
				Entry.Weight,
				Entry.MinLevel,
				Entry.MaxLevel);
		}
		return FText::FromString(Summary);
	}

	if (!SelectedAsset.IsValid())
	{
		return NSLOCTEXT("YOLOInventory", "FragmentDashboard_NoSelection", "Select a Fragment Asset or Item Trait Asset to inspect fragment data. Enable legacy affix visibility if needed.");
	}
	return FText::FromString(SelectedAsset->GetClass()->GetName());
}
