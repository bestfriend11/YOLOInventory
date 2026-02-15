#include "SYICatalogDashboard.h"

#include "YIItemDefinition.h"
#include "Data/YIDataTableItemSource.h"
#include "Data/YIDataTableAffixSource.h"
#include "YIAffixAsset.h"
#include "YIAffixPoolAsset.h"
#include "YILootTable.h"
#include "YIRarityProfile.h"
#include "YIItemGenerator.h"
#include "YIInventoryBag.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"

#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Styling/AppStyle.h"

namespace
{
static FString YICatalogFilterToLabel(EYICatalogFilter Filter)
{
	switch (Filter)
	{
	case EYICatalogFilter::Sources: return TEXT("Sources");
	case EYICatalogFilter::Items: return TEXT("Items");
	case EYICatalogFilter::Affixes: return TEXT("Affixes");
	case EYICatalogFilter::Generators: return TEXT("Generators");
	case EYICatalogFilter::Bags: return TEXT("Bags");
	default: return TEXT("All");
	}
}
}

void SYICatalogDashboard::Construct(const FArguments& InArgs)
{
	OnOpenAsset = InArgs._OnOpenAsset;

	RebuildFilterOptions();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(6)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Padding(8)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory", "Catalog_Refresh", "Refresh"))
					.OnClicked_Lambda([this]()
					{
						Refresh();
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory", "Catalog_OpenSelected", "Open Selected"))
					.OnClicked_Lambda([this]()
					{
						OpenSelected();
						return FReply::Handled();
					})
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(8, 0)
				[
					SNew(SSearchBox)
					.HintText(NSLOCTEXT("YOLOInventory", "Catalog_Search", "Search name/path/type..."))
					.OnTextChanged(this, &SYICatalogDashboard::OnSearchTextChanged)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
				[
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&FilterOptions)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
					{
						return SNew(STextBlock).Text(InItem.IsValid() ? FText::FromString(*InItem) : FText::GetEmpty());
					})
					.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewItem, ESelectInfo::Type)
					{
						if (!NewItem.IsValid())
						{
							return;
						}

						if (*NewItem == TEXT("Sources")) ActiveFilter = EYICatalogFilter::Sources;
						else if (*NewItem == TEXT("Items")) ActiveFilter = EYICatalogFilter::Items;
						else if (*NewItem == TEXT("Affixes")) ActiveFilter = EYICatalogFilter::Affixes;
						else if (*NewItem == TEXT("Generators")) ActiveFilter = EYICatalogFilter::Generators;
						else if (*NewItem == TEXT("Bags")) ActiveFilter = EYICatalogFilter::Bags;
						else ActiveFilter = EYICatalogFilter::All;

						ApplyFilters();
					})
					.Content()
					[
						SNew(STextBlock)
						.Text_Lambda([this]()
						{
							return FText::FromString(YICatalogFilterToLabel(ActiveFilter));
						})
					]
				]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(6, 0, 6, 6)
		[
			SAssignNew(ListView, SListView<TSharedPtr<FYICatalogEntry>>)
			.ListItemsSource(&FilteredEntries)
			.SelectionMode(ESelectionMode::Single)
			.OnGenerateRow(this, &SYICatalogDashboard::MakeRow)
			.OnMouseButtonDoubleClick_Lambda([this](TSharedPtr<FYICatalogEntry>)
			{
				OpenSelected();
			})
			.HeaderRow
			(
				SNew(SHeaderRow)
				+ SHeaderRow::Column("Group").DefaultLabel(NSLOCTEXT("YOLOInventory", "Catalog_ColGroup", "Group")).FillWidth(0.12f)
				+ SHeaderRow::Column("Type").DefaultLabel(NSLOCTEXT("YOLOInventory", "Catalog_ColType", "Type")).FillWidth(0.18f)
				+ SHeaderRow::Column("Name").DefaultLabel(NSLOCTEXT("YOLOInventory", "Catalog_ColName", "Name")).FillWidth(0.22f)
				+ SHeaderRow::Column("Status").DefaultLabel(NSLOCTEXT("YOLOInventory", "Catalog_ColStatus", "Status")).FillWidth(0.12f)
				+ SHeaderRow::Column("Linked").DefaultLabel(NSLOCTEXT("YOLOInventory", "Catalog_ColLinked", "Linked")).FillWidth(0.08f)
				+ SHeaderRow::Column("Path").DefaultLabel(NSLOCTEXT("YOLOInventory", "Catalog_ColPath", "Path")).FillWidth(0.28f)
			)
		]
	];

	Refresh();
}

void SYICatalogDashboard::Refresh()
{
	AllEntries.Reset();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& Registry = AssetRegistryModule.Get();

	TMap<FString, int32> ItemSourceLinkCounts;
	TMap<FString, int32> AffixSourceLinkCounts;

	{
		TArray<FAssetData> ItemDefs;
		Registry.GetAssetsByClass(UYIItemDefinition::StaticClass()->GetClassPathName(), ItemDefs, true);
		for (const FAssetData& AssetData : ItemDefs)
		{
			if (const UYIItemDefinition* ItemDef = Cast<UYIItemDefinition>(AssetData.GetAsset()))
			{
				const FSoftObjectPath SourcePath = ItemDef->SourceDataSource.ToSoftObjectPath();
				if (SourcePath.IsValid())
				{
					ItemSourceLinkCounts.FindOrAdd(SourcePath.ToString())++;
				}
			}
		}
	}

	{
		TArray<FAssetData> AffixAssets;
		Registry.GetAssetsByClass(UYIAffixAsset::StaticClass()->GetClassPathName(), AffixAssets, true);
		for (const FAssetData& AssetData : AffixAssets)
		{
			if (const UYIAffixAsset* Affix = Cast<UYIAffixAsset>(AssetData.GetAsset()))
			{
				const FSoftObjectPath SourcePath = Affix->SourceDataSource.ToSoftObjectPath();
				if (SourcePath.IsValid())
				{
					AffixSourceLinkCounts.FindOrAdd(SourcePath.ToString())++;
				}
			}
		}
	}

	auto AddEntriesForClass = [&Registry, this](UClass* Class, const TCHAR* Group, const TCHAR* Type, TFunction<void(UObject*, FYICatalogEntry&)> Populate)
	{
		TArray<FAssetData> Assets;
		Registry.GetAssetsByClass(Class->GetClassPathName(), Assets, true);
		for (const FAssetData& AssetData : Assets)
		{
			UObject* Object = AssetData.GetAsset();
			if (!Object)
			{
				continue;
			}

			TSharedPtr<FYICatalogEntry> Entry = MakeShared<FYICatalogEntry>();
			Entry->Group = Group;
			Entry->Type = Type;
			Entry->Name = AssetData.AssetName.ToString();
			Entry->Path = AssetData.GetSoftObjectPath().ToString();
			Entry->Status = TEXT("OK");
			Entry->Object = TSoftObjectPtr<UObject>(AssetData.GetSoftObjectPath());

			Populate(Object, *Entry);
			AllEntries.Add(Entry);
		}
	};

	AddEntriesForClass(UYIDataTableItemSource::StaticClass(), TEXT("Sources"), TEXT("Item Source"),
		[this, &ItemSourceLinkCounts](UObject* Object, FYICatalogEntry& Entry)
		{
			const FString Key = Object->GetPathName();
			Entry.LinkedCount = ItemSourceLinkCounts.FindRef(Key);
			Entry.Status = Entry.LinkedCount > 0 ? TEXT("Linked") : TEXT("Unused");
		});

	AddEntriesForClass(UYIDataTableAffixSource::StaticClass(), TEXT("Sources"), TEXT("Affix Source"),
		[this, &AffixSourceLinkCounts](UObject* Object, FYICatalogEntry& Entry)
		{
			const FString Key = Object->GetPathName();
			Entry.LinkedCount = AffixSourceLinkCounts.FindRef(Key);
			Entry.Status = Entry.LinkedCount > 0 ? TEXT("Linked") : TEXT("Unused");
		});

	AddEntriesForClass(UYIItemDefinition::StaticClass(), TEXT("Items"), TEXT("Item Definition"),
		[](UObject* Object, FYICatalogEntry& Entry)
		{
			if (const UYIItemDefinition* Def = Cast<UYIItemDefinition>(Object))
			{
				Entry.Status = Def->SourceDataSource.ToSoftObjectPath().IsValid() ? TEXT("Linked") : TEXT("Manual");
			}
		});

	AddEntriesForClass(UYIAffixAsset::StaticClass(), TEXT("Affixes"), TEXT("Affix Asset"),
		[](UObject* Object, FYICatalogEntry& Entry)
		{
			if (const UYIAffixAsset* Affix = Cast<UYIAffixAsset>(Object))
			{
				Entry.Status = Affix->SourceDataSource.ToSoftObjectPath().IsValid() ? TEXT("Linked") : TEXT("Manual");
			}
		});

	AddEntriesForClass(UYIAffixPoolAsset::StaticClass(), TEXT("Affixes"), TEXT("Affix Pool"),
		[](UObject*, FYICatalogEntry&) {});

	AddEntriesForClass(UYILootTable::StaticClass(), TEXT("Generators"), TEXT("Loot Table"),
		[](UObject* Object, FYICatalogEntry& Entry)
		{
			if (const UYILootTable* LootTable = Cast<UYILootTable>(Object))
			{
				Entry.Status = LootTable->Entries.Num() > 0 ? TEXT("Configured") : TEXT("Empty");
				Entry.LinkedCount = LootTable->Entries.Num();
			}
		});

	AddEntriesForClass(UYIRarityProfile::StaticClass(), TEXT("Generators"), TEXT("Rarity Profile"),
		[](UObject*, FYICatalogEntry&) {});

	AddEntriesForClass(UYIItemGenerator::StaticClass(), TEXT("Generators"), TEXT("Item Generator"),
		[](UObject* Object, FYICatalogEntry& Entry)
		{
			if (const UYIItemGenerator* Generator = Cast<UYIItemGenerator>(Object))
			{
				Entry.Status = Generator->LootTable.ToSoftObjectPath().IsValid() ? TEXT("Configured") : TEXT("Missing Loot");
			}
		});

	AddEntriesForClass(UYIInventoryBag::StaticClass(), TEXT("Bags"), TEXT("Bag"),
		[](UObject*, FYICatalogEntry&) {});

	AllEntries.Sort([](const TSharedPtr<FYICatalogEntry>& A, const TSharedPtr<FYICatalogEntry>& B)
	{
		if (!A.IsValid() || !B.IsValid())
		{
			return A.IsValid();
		}
		int32 GroupCmp = A->Group.Compare(B->Group);
		if (GroupCmp != 0)
		{
			return GroupCmp < 0;
		}
		int32 TypeCmp = A->Type.Compare(B->Type);
		if (TypeCmp != 0)
		{
			return TypeCmp < 0;
		}
		return A->Name < B->Name;
	});

	ApplyFilters();
}

UObject* SYICatalogDashboard::GetSelectedAsset() const
{
	if (!ListView.IsValid())
	{
		return nullptr;
	}
	const TArray<TSharedPtr<FYICatalogEntry>> Selected = ListView->GetSelectedItems();
	if (Selected.Num() <= 0 || !Selected[0].IsValid())
	{
		return nullptr;
	}
	return Selected[0]->Object.LoadSynchronous();
}

TSharedRef<ITableRow> SYICatalogDashboard::MakeRow(TSharedPtr<FYICatalogEntry> Entry, const TSharedRef<STableViewBase>& Owner)
{
	return SNew(STableRow<TSharedPtr<FYICatalogEntry>>, Owner)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.12f).Padding(2, 0)
		[
			SNew(STextBlock).Text(Entry.IsValid() ? FText::FromString(Entry->Group) : FText::GetEmpty())
		]
		+ SHorizontalBox::Slot().FillWidth(0.18f).Padding(2, 0)
		[
			SNew(STextBlock).Text(Entry.IsValid() ? FText::FromString(Entry->Type) : FText::GetEmpty())
		]
		+ SHorizontalBox::Slot().FillWidth(0.22f).Padding(2, 0)
		[
			SNew(STextBlock).Text(Entry.IsValid() ? FText::FromString(Entry->Name) : FText::GetEmpty())
		]
		+ SHorizontalBox::Slot().FillWidth(0.12f).Padding(2, 0)
		[
			SNew(STextBlock).Text(Entry.IsValid() ? FText::FromString(Entry->Status) : FText::GetEmpty())
		]
		+ SHorizontalBox::Slot().FillWidth(0.08f).Padding(2, 0)
		[
			SNew(STextBlock).Text(Entry.IsValid() ? FText::AsNumber(Entry->LinkedCount) : FText::GetEmpty())
		]
		+ SHorizontalBox::Slot().FillWidth(0.28f).Padding(2, 0)
		[
			SNew(STextBlock).Text(Entry.IsValid() ? FText::FromString(Entry->Path) : FText::GetEmpty())
		]
	];
}

void SYICatalogDashboard::ApplyFilters()
{
	FilteredEntries.Reset();
	const FString Search = SearchText.ToString();

	auto MatchesFilter = [this](const FYICatalogEntry& Entry) -> bool
	{
		switch (ActiveFilter)
		{
		case EYICatalogFilter::Sources: return Entry.Group == TEXT("Sources");
		case EYICatalogFilter::Items: return Entry.Group == TEXT("Items");
		case EYICatalogFilter::Affixes: return Entry.Group == TEXT("Affixes");
		case EYICatalogFilter::Generators: return Entry.Group == TEXT("Generators");
		case EYICatalogFilter::Bags: return Entry.Group == TEXT("Bags");
		default: return true;
		}
	};

	for (const TSharedPtr<FYICatalogEntry>& Entry : AllEntries)
	{
		if (!Entry.IsValid() || !MatchesFilter(*Entry))
		{
			continue;
		}

		const bool bSearchPass = Search.IsEmpty()
			|| Entry->Name.Contains(Search)
			|| Entry->Type.Contains(Search)
			|| Entry->Path.Contains(Search)
			|| Entry->Status.Contains(Search);
		if (bSearchPass)
		{
			FilteredEntries.Add(Entry);
		}
	}

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

void SYICatalogDashboard::OpenSelected()
{
	UObject* Selected = GetSelectedAsset();
	if (Selected && OnOpenAsset.IsBound())
	{
		OnOpenAsset.Execute(Selected);
	}
}

void SYICatalogDashboard::OnSearchTextChanged(const FText& InText)
{
	SearchText = InText;
	ApplyFilters();
}

void SYICatalogDashboard::RebuildFilterOptions()
{
	FilterOptions.Reset();
	FilterOptions.Add(MakeShared<FString>(TEXT("All")));
	FilterOptions.Add(MakeShared<FString>(TEXT("Sources")));
	FilterOptions.Add(MakeShared<FString>(TEXT("Items")));
	FilterOptions.Add(MakeShared<FString>(TEXT("Affixes")));
	FilterOptions.Add(MakeShared<FString>(TEXT("Generators")));
	FilterOptions.Add(MakeShared<FString>(TEXT("Bags")));
}
