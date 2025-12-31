#include "SYIPalette.h"
#include "YIItemDefinitionEditor.h"
#include "YINode_Item.h"
#include "YIStackEntry_Examples.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Styling/AppStyle.h"
#include "YIPaletteDragDropOp.h"
#include "YIStackEntry_Examples.h"
#include "Widgets/Views/SListView.h"
#include "SYIPaletteRow.h"

void SYIPalette::Construct(const FArguments& InArgs)
{
	Owner = InArgs._OwnerEditor;

	// Build the data model
	AllEntries.Reset();
	{
		auto AddHeader=[this](const TCHAR* Cat){ TSharedPtr<FYIPaletteEntry> H=MakeShared<FYIPaletteEntry>(); H->Label=FText::FromString(Cat); H->Category=FName(Cat); H->bIsHeader=true; AllEntries.Add(H); };
		AddHeader(TEXT("UI"));
		TSharedPtr<FYIPaletteEntry> E;
		E = MakeShared<FYIPaletteEntry>(); E->Label=NSLOCTEXT("YOLOInventory","UI_NameDesc","UI: Name & Description"); E->Tooltip=NSLOCTEXT("YOLOInventory","UI_NameDesc_TT","Adds UI name/description/icon/mesh/effects"); E->StackName=TEXT("UI"); E->EntryClass=UYIUI_NameDesc::StaticClass(); E->Category=TEXT("UI"); AllEntries.Add(E);
		AddHeader(TEXT("Ability"));
		E = MakeShared<FYIPaletteEntry>(); E->Label=NSLOCTEXT("YOLOInventory","Ability_Grant","Ability: Grant Ability"); E->Tooltip=NSLOCTEXT("YOLOInventory","Ability_Grant_TT","Adds ability grant entry"); E->StackName=TEXT("Ability"); E->EntryClass=UYIAbility_GrantAbility::StaticClass(); E->Category=TEXT("Ability"); AllEntries.Add(E);
		AddHeader(TEXT("Upgrade"));
		E = MakeShared<FYIPaletteEntry>(); E->Label=NSLOCTEXT("YOLOInventory","Upgrade_Path","Upgrade: Path"); E->Tooltip=NSLOCTEXT("YOLOInventory","Upgrade_Path_TT","Adds upgrade path entry"); E->StackName=TEXT("Upgrade"); E->EntryClass=UYIUpgrade_Path::StaticClass(); E->Category=TEXT("Upgrade"); AllEntries.Add(E);
		AddHeader(TEXT("Economy"));
		E = MakeShared<FYIPaletteEntry>(); E->Label=NSLOCTEXT("YOLOInventory","Economy_Market","Economy: Market"); E->Tooltip=NSLOCTEXT("YOLOInventory","Economy_Market_TT","Adds market/selling entry"); E->StackName=TEXT("Economy"); E->EntryClass=UYIEconomy_Market::StaticClass(); E->Category=TEXT("Economy"); AllEntries.Add(E);
	}
	Filtered = AllEntries;
	// Add rarity filter dropdown UI

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "PaletteTitle", "YOLO Inventory Palette")) ]
		+ SVerticalBox::Slot().AutoHeight()[
			SAssignNew(SearchBox, SSearchBox)
			.OnTextChanged_Lambda([this](const FText& InText){ FilterText = InText; RefreshList(); })
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SAssignNew(ListView, SListView<FEntryPtr>)
			.ListItemsSource(&Filtered)
			.OnGenerateRow_Lambda([this](FEntryPtr InItem, const TSharedRef<STableViewBase>& Owner) -> TSharedRef<ITableRow>
			{
				if (InItem->bIsHeader)
				{
					return SNew(STableRow<FEntryPtr>, Owner)
					[
						SNew(STextBlock).Text(InItem->Label)
						.Font(FAppStyle::Get().GetFontStyle("DetailsView.CategoryFontStyle"))
					];
				}
				return SNew(STableRow<FEntryPtr>, Owner)
				[
					SNew(SYIPaletteRow).Entry(InItem)
				];
			})
		]
	];
}

void SYIPalette::RefreshList()
{
	Filtered.Reset();
	const FString Filter = FilterText.ToString();
	for (const FEntryPtr& E : AllEntries)
	{
		if (Filter.IsEmpty() || E->Label.ToString().Contains(Filter) || E->Category.ToString().Contains(Filter))
		{
			Filtered.Add(E);
		}
	}
	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

TSharedRef<SWidget> SYIPalette::MakePaletteEntry(const FText& Label, const FText& Tooltip, UClass* EntryClass, const FName& StackName)
{
	return SNew(SButton)
	.ToolTipText(Tooltip)
	.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
	.ContentPadding(FMargin(4))
	.OnClicked_Lambda([this, EntryClass, StackName]() -> FReply
	{
		if (TSharedPtr<FYIItemDefinitionEditor> Pinned = Owner.Pin())
		{
			// Selection-based insertion removed; ItemDefinition editor no longer exposes selected nodes.
			// Intentionally no-op here to keep palette compiling without legacy selection APIs.
		}
		return FReply::Handled();
	})
	
	[
		SNew(STextBlock).Text(Label)
	];
}
