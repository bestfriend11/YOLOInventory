#include "SYIFragmentDashboard.h"

#include "YIAffix.h"
#include "YIAffixAsset.h"
#include "YIFragmentAsset.h"
#include "YIInventoryBlueprintLibrary.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "PropertyCustomizationHelpers.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/AppStyle.h"
#include "FileHelpers.h"

namespace
{
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

TSharedRef<SWidget> SYIFragmentDashboard::BuildAssetPanelWidget()
{
	auto AddFragment = [this](const UScriptStruct* FragmentStruct)
	{
		UObject* Asset = SelectedAsset.Get();
		if (!Asset || !FragmentStruct)
		{
			return;
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
		}
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
				SNew(SObjectPropertyEntryBox)
				.AllowedClass(UObject::StaticClass())
				.OnShouldFilterAsset_Lambda([](const FAssetData& AssetData)
				{
					const UClass* AssetClass = AssetData.GetClass();
					if (!AssetClass)
					{
						return true;
					}
					return !(AssetClass->IsChildOf(UYIAffixAsset::StaticClass())
						|| AssetClass->IsChildOf(UYIFragmentAsset::StaticClass()));
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
				SNew(STextBlock)
				.Text(NSLOCTEXT("YOLOInventory", "FragmentDashboard_AddPreset", "Add authoring fragments"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 2.f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("YOLOInventory", "FragmentDashboard_AddStaticAffix", "Add Static Affix Fragment"))
				.OnClicked_Lambda([AddFragment]()
				{
					AddFragment(FYIStaticAffixDefinitionFragment::StaticStruct());
					return FReply::Handled();
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 2.f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("YOLOInventory", "FragmentDashboard_BuildSnapshot", "Build Runtime Snapshot (Preview)"))
				.OnClicked_Lambda([this]()
				{
					UYIAffixAsset* AffixAsset = Cast<UYIAffixAsset>(SelectedAsset.Get());
					if (!AffixAsset)
					{
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
						// We only need package-dirty behavior when authoring data changes.
						// Snapshot preview is runtime data; it is rendered in Preview panel.
					}
					return FReply::Handled();
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("YOLOInventory", "FragmentDashboard_Save", "Save Selected Affix"))
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
				.Text(NSLOCTEXT("YOLOInventory", "FragmentDashboard_MappingHelp", "Author static affix data in DefinitionFragments, then generate runtime snapshots when rolling item instances."))
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

	if (!SelectedAsset.IsValid())
	{
		return NSLOCTEXT("YOLOInventory", "FragmentDashboard_NoSelection", "Select an Affix Asset or Fragment Asset to inspect its fragment data.");
	}
	return FText::FromString(SelectedAsset->GetClass()->GetName());
}
