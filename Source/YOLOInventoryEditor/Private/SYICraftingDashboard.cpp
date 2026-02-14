#include "SYICraftingDashboard.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "YIAffixAsset.h"
#include "YIInventoryBlueprintLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"
#include "InputCoreTypes.h"
#include "FileHelpers.h"

namespace
{
static bool YICrafting_SaveObjectPackage(UObject* ObjectToSave)
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
	const bool bPromptToSave = false;
	return FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave) == FEditorFileUtils::PR_Success;
}
}

void SYICraftingDashboard::Construct(const FArguments& InArgs)
{
	RefreshAffixEntries();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(6)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Padding(8)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(2)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("YOLOInventory", "Craft_Header", "Craft Specific Item (Definition + Predefined Affixes)"))
					.Font(FAppStyle::Get().GetFontStyle("BoldFont"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)
					[
						SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Craft_Bag", "Target Bag"))
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2)
					[
						SNew(SObjectPropertyEntryBox)
						.AllowedClass(UYIInventoryBag::StaticClass())
						.ObjectPath_Lambda([this]()
						{
							return TargetBag.IsValid() ? TargetBag->GetPathName() : FString();
						})
						.OnObjectChanged_Lambda([this](const FAssetData& AssetData)
						{
							SetTargetBag(Cast<UYIInventoryBag>(AssetData.GetAsset()));
						})
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10, 2, 2, 2)
					[
						SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Craft_Item", "Item Definition"))
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2)
					[
						SNew(SObjectPropertyEntryBox)
						.AllowedClass(UYIItemDefinition::StaticClass())
						.ObjectPath_Lambda([this]()
						{
							return TargetItemDefinition.IsValid() ? TargetItemDefinition->GetPathName() : FString();
						})
						.OnObjectChanged_Lambda([this](const FAssetData& AssetData)
						{
							TargetItemDefinition = Cast<UYIItemDefinition>(AssetData.GetAsset());
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)
					[
						SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Craft_Count", "Count"))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2)
					[
						SNew(SNumericEntryBox<int32>)
						.MinValue(1)
						.MaxValue(999)
						.Value_Lambda([this]() -> TOptional<int32> { return ItemCount; })
						.OnValueChanged_Lambda([this](int32 NewValue) { ItemCount = FMath::Clamp(NewValue, 1, 999); })
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10, 2, 2, 2)
					[
						SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Craft_Level", "Roll Level"))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2)
					[
						SNew(SNumericEntryBox<int32>)
						.MinValue(1)
						.MaxValue(9999)
						.Value_Lambda([this]() -> TOptional<int32> { return RollLevel; })
						.OnValueChanged_Lambda([this](int32 NewValue) { RollLevel = FMath::Max(1, NewValue); })
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10, 2, 2, 2)
					[
						SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Craft_Seed", "Seed"))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2)
					[
						SNew(SNumericEntryBox<int32>)
						.MinValue(1)
						.MaxValue(2147483647)
						.Value_Lambda([this]() -> TOptional<int32> { return RollSeed; })
						.OnValueChanged_Lambda([this](int32 NewValue) { RollSeed = FMath::Clamp(NewValue, 1, 2147483647); })
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(10, 2, 2, 2)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([this]() { return bIncludeTemplateAffixes ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState NewState)
						{
							bIncludeTemplateAffixes = (NewState == ECheckBoxState::Checked);
						})
						[
							SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Craft_IncludeImplicit", "Include Template/Implicit Affixes"))
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(2)
					[
						SNew(SButton)
						.Text(NSLOCTEXT("YOLOInventory", "Craft_Add", "Add Crafted Item To Bag"))
						.OnClicked(this, &SYICraftingDashboard::AddCraftedItemToBag)
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2)
					[
						SNew(SButton)
						.Text(NSLOCTEXT("YOLOInventory", "Craft_SaveBag", "Save Bag"))
						.OnClicked(this, &SYICraftingDashboard::SaveCurrentBag)
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2)
					[
						SNew(SButton)
						.Text(NSLOCTEXT("YOLOInventory", "Craft_ClearAffixes", "Clear Selected Affixes"))
						.OnClicked_Lambda([this]()
						{
							for (const TSharedPtr<FCraftAffixEntry>& Entry : AllAffixEntries)
							{
								if (Entry.IsValid())
								{
									Entry->bSelected = false;
								}
							}
							if (AffixListView.IsValid())
							{
								AffixListView->RequestListRefresh();
							}
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2)
					[
						SNew(SButton)
						.Text(NSLOCTEXT("YOLOInventory", "Craft_RefreshAffixes", "Refresh Affixes"))
						.OnClicked_Lambda([this]()
						{
							RefreshAffixEntries();
							return FReply::Handled();
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(2)
				[
					SNew(STextBlock)
					.Text_Lambda([this]()
					{
						return StatusText.IsEmpty()
							? NSLOCTEXT("YOLOInventory", "Craft_Status_Default", "Pick bag + item, select affixes, then add to bag.")
							: StatusText;
					})
				]
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(6, 0)
		[
			SNew(SSearchBox)
			.HintText(NSLOCTEXT("YOLOInventory", "Craft_AffixSearch", "Search affixes..."))
			.OnTextChanged_Lambda([this](const FText& NewText)
			{
				SearchText = NewText;
				ApplySearchFilter();
			})
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(6, 0, 6, 6)
		[
			SAssignNew(AffixListView, SListView<TSharedPtr<FCraftAffixEntry>>)
			.ListItemsSource(&FilteredAffixEntries)
			.SelectionMode(ESelectionMode::None)
			.OnGenerateRow(this, &SYICraftingDashboard::MakeAffixRow)
			.HeaderRow
			(
				SNew(SHeaderRow)
				+ SHeaderRow::Column("Use").DefaultLabel(NSLOCTEXT("YOLOInventory", "Craft_ColUse", "Use")).FixedWidth(48.f)
				+ SHeaderRow::Column("Name").DefaultLabel(NSLOCTEXT("YOLOInventory", "Craft_ColName", "Affix")).FillWidth(0.45f)
				+ SHeaderRow::Column("Kind").DefaultLabel(NSLOCTEXT("YOLOInventory", "Craft_ColKind", "Kind")).FillWidth(0.15f)
				+ SHeaderRow::Column("Tier").DefaultLabel(NSLOCTEXT("YOLOInventory", "Craft_ColTier", "Tier")).FillWidth(0.10f)
				+ SHeaderRow::Column("Power").DefaultLabel(NSLOCTEXT("YOLOInventory", "Craft_ColPower", "Power")).FillWidth(0.10f)
				+ SHeaderRow::Column("Path").DefaultLabel(NSLOCTEXT("YOLOInventory", "Craft_ColPath", "Asset")).FillWidth(0.20f)
			)
		]
	];
}

void SYICraftingDashboard::OpenAsset(UObject* Asset)
{
	if (UYIInventoryBag* Bag = Cast<UYIInventoryBag>(Asset))
	{
		SetTargetBag(Bag);
		return;
	}

	if (UYIItemDefinition* Def = Cast<UYIItemDefinition>(Asset))
	{
		TargetItemDefinition = Def;
		return;
	}

	if (UYIAffixAsset* Affix = Cast<UYIAffixAsset>(Asset))
	{
		const FSoftObjectPath AffixPath(Affix);
		for (const TSharedPtr<FCraftAffixEntry>& Entry : AllAffixEntries)
		{
			if (Entry.IsValid() && Entry->Affix.ToSoftObjectPath() == AffixPath)
			{
				Entry->bSelected = true;
				break;
			}
		}
		if (AffixListView.IsValid())
		{
			AffixListView->RequestListRefresh();
		}
	}
}

void SYICraftingDashboard::SetTargetBag(UYIInventoryBag* InBag)
{
	TargetBag = InBag;
}

UYIInventoryBag* SYICraftingDashboard::GetTargetBag() const
{
	return TargetBag.Get();
}

void SYICraftingDashboard::AddCraftedItemFromToolbar()
{
	AddCraftedItemToBag();
}

void SYICraftingDashboard::SaveTargetBagFromToolbar()
{
	SaveCurrentBag();
}

void SYICraftingDashboard::RefreshAffixEntries()
{
	AllAffixEntries.Reset();

	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AffixAssets;
	AssetRegistry.Get().GetAssetsByClass(UYIAffixAsset::StaticClass()->GetClassPathName(), AffixAssets, true);

	for (const FAssetData& AssetData : AffixAssets)
	{
		UYIAffixAsset* Affix = Cast<UYIAffixAsset>(AssetData.GetAsset());
		if (!Affix)
		{
			continue;
		}

		TSharedPtr<FCraftAffixEntry> Entry = MakeShared<FCraftAffixEntry>();
		Entry->Affix = Affix;
		Entry->Name = Affix->DisplayName.IsEmpty() ? Affix->GetName() : Affix->DisplayName.ToString();
		switch (Affix->Kind)
		{
		case EYIAffixKind::Prefix: Entry->Kind = TEXT("Prefix"); break;
		case EYIAffixKind::Suffix: Entry->Kind = TEXT("Suffix"); break;
		case EYIAffixKind::Implicit: Entry->Kind = TEXT("Implicit"); break;
		default: Entry->Kind = TEXT("Unknown"); break;
		}
		Entry->Tier = Affix->Tier;
		Entry->Power = Affix->PowerLevel;
		AllAffixEntries.Add(Entry);
	}

	AllAffixEntries.Sort([](const TSharedPtr<FCraftAffixEntry>& A, const TSharedPtr<FCraftAffixEntry>& B)
	{
		if (!A.IsValid() || !B.IsValid())
		{
			return A.IsValid();
		}
		if (A->Kind == B->Kind)
		{
			return A->Name < B->Name;
		}
		return A->Kind < B->Kind;
	});

	ApplySearchFilter();
}

void SYICraftingDashboard::ApplySearchFilter()
{
	FilteredAffixEntries.Reset();
	const FString Search = SearchText.ToString();
	for (const TSharedPtr<FCraftAffixEntry>& Entry : AllAffixEntries)
	{
		if (!Entry.IsValid())
		{
			continue;
		}

		const bool bPass = Search.IsEmpty()
			|| Entry->Name.Contains(Search)
			|| Entry->Kind.Contains(Search)
			|| Entry->Affix.ToSoftObjectPath().ToString().Contains(Search);

		if (bPass)
		{
			FilteredAffixEntries.Add(Entry);
		}
	}

	if (AffixListView.IsValid())
	{
		AffixListView->RequestListRefresh();
	}
}

FReply SYICraftingDashboard::AddCraftedItemToBag()
{
	UYIInventoryBag* Bag = TargetBag.Get();
	UYIItemDefinition* Def = TargetItemDefinition.Get();
	if (!Bag || !Def)
	{
		StatusText = NSLOCTEXT("YOLOInventory", "Craft_Status_Missing", "Missing target bag or item definition.");
		return FReply::Handled();
	}

	FYIBagItem NewItem;
	NewItem.Item.Definition = Def;
	NewItem.Item.Count = FMath::Max(1, ItemCount);
	NewItem.Size = Def->DefaultSize;
	NewItem.Pos = FIntPoint::ZeroValue;

	if (bIncludeTemplateAffixes)
	{
		UYIInventoryBlueprintLibrary::ApplyTemplateAffixesToInstance(Def, NewItem.Item);
	}

	int32 SelectedCount = 0;
	int32 AppliedCount = 0;
	for (const TSharedPtr<FCraftAffixEntry>& Entry : AllAffixEntries)
	{
		if (!Entry.IsValid() || !Entry->bSelected)
		{
			continue;
		}
		++SelectedCount;
		UYIAffixAsset* Affix = Entry->Affix.IsValid() ? Entry->Affix.Get() : Entry->Affix.LoadSynchronous();
		if (!Affix)
		{
			continue;
		}

		float RolledValue = 0.f;
		const bool bApplied = UYIInventoryBlueprintLibrary::AddRolledAffix(NewItem, Affix, RollLevel, RollSeed + AppliedCount, RolledValue);
		if (bApplied)
		{
			++AppliedCount;
		}
	}

	const int32 AddedIndex = Bag->AddBagItem(NewItem);
	if (AddedIndex == INDEX_NONE)
	{
		StatusText = NSLOCTEXT("YOLOInventory", "Craft_Status_AddFailed", "Could not add crafted item to bag (space/rules constraint).");
		return FReply::Handled();
	}

	StatusText = FText::Format(
		NSLOCTEXT("YOLOInventory", "Craft_Status_Added", "Crafted item added. Selected affixes: {0}, applied: {1}."),
		FText::AsNumber(SelectedCount),
		FText::AsNumber(AppliedCount));
	return FReply::Handled();
}

FReply SYICraftingDashboard::SaveCurrentBag()
{
	UYIInventoryBag* Bag = TargetBag.Get();
	if (!Bag)
	{
		StatusText = NSLOCTEXT("YOLOInventory", "Craft_Status_NoBag", "No target bag selected.");
		return FReply::Handled();
	}

	if (YICrafting_SaveObjectPackage(Bag))
	{
		StatusText = NSLOCTEXT("YOLOInventory", "Craft_Status_Saved", "Bag saved.");
	}
	else
	{
		StatusText = NSLOCTEXT("YOLOInventory", "Craft_Status_SaveFail", "Save canceled or failed.");
	}
	return FReply::Handled();
}

TSharedRef<ITableRow> SYICraftingDashboard::MakeAffixRow(TSharedPtr<FCraftAffixEntry> Entry, const TSharedRef<STableViewBase>& Owner)
{
	auto ToggleSelection = [this, Entry]()
	{
		if (!Entry.IsValid())
		{
			return;
		}
		Entry->bSelected = !Entry->bSelected;
		if (AffixListView.IsValid())
		{
			AffixListView->RequestListRefresh();
		}
	};

	return SNew(STableRow<TSharedPtr<FCraftAffixEntry>>, Owner)
	[
		SNew(SBorder)
		.Padding(0)
		.BorderImage(FAppStyle::Get().GetBrush("NoBorder"))
		.OnMouseButtonDown_Lambda([ToggleSelection](const FGeometry&, const FPointerEvent& MouseEvent)
		{
			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				ToggleSelection();
				return FReply::Handled();
			}
			return FReply::Unhandled();
		})
		[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)
		[
			SNew(SCheckBox)
			.IsChecked_Lambda([Entry]() { return (Entry.IsValid() && Entry->bSelected) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
			.OnCheckStateChanged_Lambda([Entry](ECheckBoxState NewState)
			{
				if (Entry.IsValid())
				{
					Entry->bSelected = (NewState == ECheckBoxState::Checked);
				}
			})
		]
		+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(2)
		[
			SNew(STextBlock).Text(Entry.IsValid() ? FText::FromString(Entry->Name) : FText::GetEmpty())
		]
		+ SHorizontalBox::Slot().FillWidth(0.15f).Padding(2)
		[
			SNew(STextBlock).Text(Entry.IsValid() ? FText::FromString(Entry->Kind) : FText::GetEmpty())
		]
		+ SHorizontalBox::Slot().FillWidth(0.10f).Padding(2)
		[
			SNew(STextBlock).Text(Entry.IsValid() ? FText::AsNumber(Entry->Tier) : FText::GetEmpty())
		]
		+ SHorizontalBox::Slot().FillWidth(0.10f).Padding(2)
		[
			SNew(STextBlock).Text(Entry.IsValid() ? FText::AsNumber(Entry->Power) : FText::GetEmpty())
		]
		+ SHorizontalBox::Slot().FillWidth(0.20f).Padding(2)
		[
			SNew(STextBlock).Text(Entry.IsValid() ? FText::FromString(Entry->Affix.ToSoftObjectPath().GetAssetName()) : FText::GetEmpty())
		]
		]
	];
}
