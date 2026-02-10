#include "SYIUnifiedDashboard.h"
#include "SYIItemDashboard.h"
#include "SYIAffixDashboard.h"
#include "SYIGeneratorDashboard.h"
#include "YIItemDefinition.h"
#include "Data/YIDataTableItemSource.h"
#include "YIAffixAsset.h"
#include "YIAffixPoolAsset.h"
#include "YIAffixFactory.h"
#include "YIAffixPoolFactory.h"
#include "YILootTable.h"
#include "YIRarityProfile.h"
#include "YIItemGenerator.h"
#include "YILootTableFactory.h"
#include "YIRarityProfileFactory.h"
#include "YIItemGeneratorFactory.h"
#include "Factories/DataAssetFactory.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Misc/PackageName.h"
#include "Engine/DataTable.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Input/SButton.h"
#include "Styling/AppStyle.h"

void SYIUnifiedDashboard::Construct(const FArguments& InArgs)
{
	ItemDashboard = SNew(SYIItemDashboard);
	AffixDashboard = SNew(SYIAffixDashboard);
	GeneratorDashboard = SNew(SYIGeneratorDashboard);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(8, 6)
			[
				SNew(SSegmentedControl<EYIUnifiedDashboardTab>)
				.Value(this, &SYIUnifiedDashboard::GetActiveTab)
				.OnValueChanged(this, &SYIUnifiedDashboard::HandleTabChanged)
				+ SSegmentedControl<EYIUnifiedDashboardTab>::Slot(EYIUnifiedDashboardTab::Items)
				.Text(NSLOCTEXT("YOLOInventory", "UnifiedDash_Items", "Items"))
				+ SSegmentedControl<EYIUnifiedDashboardTab>::Slot(EYIUnifiedDashboardTab::Affixes)
				.Text(NSLOCTEXT("YOLOInventory", "UnifiedDash_Affixes", "Affixes"))
				+ SSegmentedControl<EYIUnifiedDashboardTab>::Slot(EYIUnifiedDashboardTab::Generators)
				.Text(NSLOCTEXT("YOLOInventory", "UnifiedDash_Generators", "Generators"))
			]
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(6, 0, 6, 6)
			[
				SNew(SSplitter)
				+ SSplitter::Slot().Value(0.70f)
				[
					SAssignNew(TabSwitcher, SWidgetSwitcher)
					+ SWidgetSwitcher::Slot()
					[
						ItemDashboard.ToSharedRef()
					]
					+ SWidgetSwitcher::Slot()
					[
						AffixDashboard.ToSharedRef()
					]
					+ SWidgetSwitcher::Slot()
					[
						GeneratorDashboard.ToSharedRef()
					]
				]
				+ SSplitter::Slot().Value(0.30f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
					.Padding(8)
					[
						SAssignNew(HelpSwitcher, SWidgetSwitcher)
						+ SWidgetSwitcher::Slot()
						[
							BuildHelpForItems()
						]
						+ SWidgetSwitcher::Slot()
						[
							BuildHelpForAffixes()
						]
						+ SWidgetSwitcher::Slot()
						[
							BuildHelpForGenerators()
						]
					]
				]
			]
		]
	];

	SetActiveTab(EYIUnifiedDashboardTab::Items);
}

void SYIUnifiedDashboard::HandleTabChanged(EYIUnifiedDashboardTab NewTab)
{
	SetActiveTab(NewTab);
}

void SYIUnifiedDashboard::SetActiveTab(EYIUnifiedDashboardTab NewTab)
{
	ActiveTab = NewTab;
	if (!TabSwitcher.IsValid())
	{
		return;
	}

	int32 Index = 0;
	switch (ActiveTab)
	{
	case EYIUnifiedDashboardTab::Items:
		Index = 0;
		break;
	case EYIUnifiedDashboardTab::Affixes:
		Index = 1;
		break;
	case EYIUnifiedDashboardTab::Generators:
		Index = 2;
		break;
	default:
		break;
	}
	TabSwitcher->SetActiveWidgetIndex(Index);
	if (HelpSwitcher.IsValid())
	{
		HelpSwitcher->SetActiveWidgetIndex(Index);
	}
}

void SYIUnifiedDashboard::OpenAsset(UObject* Asset)
{
	if (!Asset)
	{
		return;
	}

	if (Asset->IsA<UYIItemDefinition>() || Asset->IsA<UYIDataTableItemSource>() || Asset->IsA<UDataTable>())
	{
		SetActiveTab(EYIUnifiedDashboardTab::Items);
		if (ItemDashboard.IsValid())
		{
			ItemDashboard->OpenAsset(Asset);
		}
		return;
	}

	if (Asset->IsA<UYIAffixAsset>() || Asset->IsA<UYIAffixPoolAsset>())
	{
		SetActiveTab(EYIUnifiedDashboardTab::Affixes);
		if (AffixDashboard.IsValid())
		{
			AffixDashboard->OpenAsset(Asset);
		}
		return;
	}

	if (Asset->IsA<UYILootTable>() || Asset->IsA<UYIRarityProfile>() || Asset->IsA<UYIItemGenerator>())
	{
		SetActiveTab(EYIUnifiedDashboardTab::Generators);
		if (GeneratorDashboard.IsValid())
		{
			GeneratorDashboard->OpenAsset(Asset);
		}
		return;
	}
}

TSharedRef<SWidget> SYIUnifiedDashboard::MakeHelpCard(const FText& Title, const FText& Body, const FLinearColor& Accent, bool bExpanded)
{
	return SNew(SExpandableArea)
		.InitiallyCollapsed(!bExpanded)
		.AreaTitle(Title)
		.BodyContent()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			.Padding(8)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
				.Padding(8)
				.BorderBackgroundColor(Accent)
				[
					SNew(STextBlock)
					.Text(Body)
					.AutoWrapText(true)
				]
			]
		];
}

TSharedRef<SWidget> SYIUnifiedDashboard::MakeWizardStep(int32 StepNumber, const FText& Title, const FText& Body, const FText& ButtonText, TFunction<void()> OnClick)
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::Format(NSLOCTEXT("YOLOInventory","Help_Wizard_Step","Step {0}: {1}"), FText::AsNumber(StepNumber), Title))
				.Font(FAppStyle::Get().GetFontStyle("BoldFont"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0,4,0,4)
			[
				SNew(STextBlock)
				.Text(Body)
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SButton)
				.Text(ButtonText)
				.OnClicked_Lambda([OnClick]()
				{
					if (OnClick)
					{
						OnClick();
					}
					return FReply::Handled();
				})
			]
		];
}

TSharedRef<SWidget> SYIUnifiedDashboard::BuildHelpForItems()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			MakeHelpCard(
				NSLOCTEXT("YOLOInventory","Help_Items_Overview","What this tab is"),
				NSLOCTEXT("YOLOInventory","Help_Items_Overview_Text",
					"This is the core item catalog. It shows both item assets (UYIItemDefinition) and data-table rows from your item sources. "
					"Use it to validate unique codes, create assets from rows, and inspect item data without leaving the dashboard."),
				FLinearColor(0.10f, 0.25f, 0.45f, 0.25f))
		]
		+ SScrollBox::Slot()
		[
			MakeHelpCard(
				NSLOCTEXT("YOLOInventory","Help_Items_DataSources","Data sources & transformers"),
				NSLOCTEXT("YOLOInventory","Help_Items_DataSources_Text",
					"Item sources are UYIDataTableItemSource assets that point to a DataTable. Each row is transformed into a UYIItemDefinition. "
					"Set the TransformerClass on the source to control how rows map to item definitions. "
					"Use 'Create Item Asset from Row' to generate or update the corresponding item asset."),
				FLinearColor(0.20f, 0.32f, 0.18f, 0.22f))
		]
		+ SScrollBox::Slot()
		[
			MakeHelpCard(
				NSLOCTEXT("YOLOInventory","Help_Items_Fields","Key fields (UYIItemDefinition)"),
				NSLOCTEXT("YOLOInventory","Help_Items_Fields_Text",
					"• UniqueCode: stable ID for saves/replication.\n"
					"• DisplayName/Description/Icon: UI presentation.\n"
					"• Size: grid footprint in bags.\n"
					"• MaxStack/Count: stacking rules.\n"
					"• Tags/Capabilities: enables features (equip, consume, etc.).\n"
					"• SFX Profile (optional): per-item UI sounds."),
				FLinearColor(0.32f, 0.22f, 0.10f, 0.22f))
		]
		+ SScrollBox::Slot()
		[
			MakeHelpCard(
				NSLOCTEXT("YOLOInventory","Help_Items_Blueprint","Runtime usage (Blueprint)"),
				NSLOCTEXT("YOLOInventory","Help_Items_Blueprint_Text",
					"At runtime, item instances reference UYIItemDefinition via soft pointers. "
					"To spawn or grant items, use inventory component or pickup helpers that create FYIItemInstance from a definition. "
					"Keep UniqueCode stable to support replication and save/load."),
				FLinearColor(0.18f, 0.18f, 0.32f, 0.22f))
		]
		+ SScrollBox::Slot()
		[
			MakeHelpCard(
				NSLOCTEXT("YOLOInventory","Help_Items_Workflow","Recommended workflow"),
				NSLOCTEXT("YOLOInventory","Help_Items_Workflow_Text",
					"1) Define item rows in DataTables.\n"
					"2) Create a DataTable Item Source and assign TransformerClass.\n"
					"3) Use this tab to create or update item assets.\n"
					"4) Use item assets in bags, generators, and pickups."),
				FLinearColor(0.10f, 0.10f, 0.10f, 0.12f))
		]
		+ SScrollBox::Slot()
		[
			MakeWizardStep(
				1,
				NSLOCTEXT("YOLOInventory","Help_Items_Wizard_Title","Wizard: Create an Item Source"),
				NSLOCTEXT("YOLOInventory","Help_Items_Wizard_Text",
					"Creates a UYIDataTableItemSource asset under /Game/YOLOInventory/ItemSources. "
					"Assign your DataTable and TransformerClass after creation."),
				NSLOCTEXT("YOLOInventory","Help_Items_Wizard_Button","Create Item Source"),
				[this]() { CreateItemSource(); })
		];
}

TSharedRef<SWidget> SYIUnifiedDashboard::BuildHelpForAffixes()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			MakeHelpCard(
				NSLOCTEXT("YOLOInventory","Help_Affix_Overview","What this tab is"),
				NSLOCTEXT("YOLOInventory","Help_Affix_Overview_Text",
					"Affixes are modular stat modifiers you can roll onto items (e.g., +Damage, +Vitality). "
					"Affix Pools group affixes and define weights and conditions for rolling."),
				FLinearColor(0.45f, 0.22f, 0.12f, 0.22f))
		]
		+ SScrollBox::Slot()
		[
			MakeHelpCard(
				NSLOCTEXT("YOLOInventory","Help_Affix_Usage","How to use affixes"),
				NSLOCTEXT("YOLOInventory","Help_Affix_Usage_Text",
					"Affixes are referenced by generators or applied directly to item instances. "
					"Use pools in generators to roll multiple affixes with weighted chances. "
					"Affix assets define name, tooltip format, and value range."),
				FLinearColor(0.20f, 0.30f, 0.15f, 0.20f))
		]
		+ SScrollBox::Slot()
		[
			MakeHelpCard(
				NSLOCTEXT("YOLOInventory","Help_Affix_Fields","Key fields (Affix/Affix Pool)"),
				NSLOCTEXT("YOLOInventory","Help_Affix_Fields_Text",
					"Affix:\n"
					"• DisplayName, TooltipFormat, Min/Max values.\n"
					"• Category/Tags for filtering.\n"
					"Affix Pool:\n"
					"• Entries with weights.\n"
					"• Optional filters for item class or rarity."),
				FLinearColor(0.28f, 0.18f, 0.05f, 0.20f))
		]
		+ SScrollBox::Slot()
		[
			MakeHelpCard(
				NSLOCTEXT("YOLOInventory","Help_Affix_Blueprint","Runtime usage (Blueprint)"),
				NSLOCTEXT("YOLOInventory","Help_Affix_Blueprint_Text",
					"Generated items store rolled affixes on the item instance. "
					"UI can read these to format tooltips and calculate stats. "
					"Affix pools are used by generators, not by the runtime inventory directly."),
				FLinearColor(0.14f, 0.14f, 0.24f, 0.20f))
		]
		+ SScrollBox::Slot()
		[
			MakeWizardStep(
				1,
				NSLOCTEXT("YOLOInventory","Help_Affix_Wizard_Title","Wizard: Create Affix + Pool"),
				NSLOCTEXT("YOLOInventory","Help_Affix_Wizard_Text",
					"Creates a new Affix and Affix Pool under /Game/YOLOInventory/Affixes. "
					"Add the affix to the pool after creation."),
				NSLOCTEXT("YOLOInventory","Help_Affix_Wizard_Button","Create Affix & Pool"),
				[this]() { CreateAffix(); CreateAffixPool(); })
		];
}

TSharedRef<SWidget> SYIUnifiedDashboard::BuildHelpForGenerators()
{
	return SNew(SScrollBox)
		+ SScrollBox::Slot()
		[
			MakeHelpCard(
				NSLOCTEXT("YOLOInventory","Help_Gen_Overview","What this tab is"),
				NSLOCTEXT("YOLOInventory","Help_Gen_Overview_Text",
					"Generators create item instances from loot tables and rarity profiles. "
					"This tab lets you author Loot Tables, Rarity Profiles, and Item Generators, and run test rolls."),
				FLinearColor(0.12f, 0.30f, 0.35f, 0.22f))
		]
		+ SScrollBox::Slot()
		[
			MakeHelpCard(
				NSLOCTEXT("YOLOInventory","Help_Gen_Workflow","Generator workflow"),
				NSLOCTEXT("YOLOInventory","Help_Gen_Workflow_Text",
					"1) Create a Loot Table with weighted entries.\n"
					"2) Create a Rarity Profile that defines rarity weights and affix rules.\n"
					"3) Create an Item Generator that references both.\n"
					"4) Use the generator in gameplay systems to roll items."),
				FLinearColor(0.20f, 0.20f, 0.20f, 0.18f))
		]
		+ SScrollBox::Slot()
		[
			MakeHelpCard(
				NSLOCTEXT("YOLOInventory","Help_Gen_Fields","Key fields (Loot/Rarity/Generator)"),
				NSLOCTEXT("YOLOInventory","Help_Gen_Fields_Text",
					"Loot Table:\n"
					"• Weighted entries (item definition or tag-based groups).\n"
					"Rarity Profile:\n"
					"• Rarity weights, affix counts per rarity.\n"
					"Generator:\n"
					"• Loot Table + Rarity Profile + optional seed/level inputs."),
				FLinearColor(0.25f, 0.18f, 0.05f, 0.18f))
		]
		+ SScrollBox::Slot()
		[
			MakeHelpCard(
				NSLOCTEXT("YOLOInventory","Help_Gen_Blueprint","Runtime usage (Blueprint)"),
				NSLOCTEXT("YOLOInventory","Help_Gen_Blueprint_Text",
					"Generators are used by gameplay or spawn logic (loot drops, chests, vendors). "
					"Call generator methods to produce FYIItemInstance arrays, then add them to a bag or spawn pickups."),
				FLinearColor(0.18f, 0.12f, 0.25f, 0.18f))
		]
		+ SScrollBox::Slot()
		[
			MakeHelpCard(
				NSLOCTEXT("YOLOInventory","Help_Gen_DS1","Dungeon Siege‑style guidance"),
				NSLOCTEXT("YOLOInventory","Help_Gen_DS1_Text",
					"To approximate DS1:\n"
					"• Use base item definitions as loot entries.\n"
					"• Use affix pools for prefixes/suffixes.\n"
					"• Use rarity profile to control affix counts.\n"
					"• Use generators in chests and enemy drops.\n"
					"Known gaps: item level scaling, affix tier curves, economy tuning (planned)."),
				FLinearColor(0.30f, 0.22f, 0.12f, 0.18f))
		]
		+ SScrollBox::Slot()
		[
			MakeWizardStep(
				1,
				NSLOCTEXT("YOLOInventory","Help_Gen_Wizard_Title","Wizard: Create Generator Stack"),
				NSLOCTEXT("YOLOInventory","Help_Gen_Wizard_Text",
					"Creates Loot Table, Rarity Profile, and Item Generator under /Game/YOLOInventory. "
					"Hook them up and run test rolls from this tab."),
				NSLOCTEXT("YOLOInventory","Help_Gen_Wizard_Button","Create Loot/Rarity/Generator"),
				[this]() { CreateLootTable(); CreateRarityProfile(); CreateItemGenerator(); })
		];
}

static void CreateAssetWithFactory(UFactory* Factory, const FString& TargetPath, const FString& BaseName)
{
	if (!Factory)
	{
		return;
	}

	IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	FString PackageName, AssetName;
	Tools.CreateUniqueAssetName(TargetPath / BaseName, TEXT(""), PackageName, AssetName);
	Tools.CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), Factory->SupportedClass, Factory);
}

void SYIUnifiedDashboard::CreateItemSource()
{
	UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
	Factory->DataAssetClass = UYIDataTableItemSource::StaticClass();
	CreateAssetWithFactory(Factory, TEXT("/Game/YOLOInventory/ItemSources"), TEXT("ItemSource"));
}

void SYIUnifiedDashboard::CreateAffix()
{
	CreateAssetWithFactory(NewObject<UYIAffixFactory>(), TEXT("/Game/YOLOInventory/Affixes"), TEXT("Affix"));
}

void SYIUnifiedDashboard::CreateAffixPool()
{
	CreateAssetWithFactory(NewObject<UYIAffixPoolFactory>(), TEXT("/Game/YOLOInventory/Affixes"), TEXT("AffixPool"));
}

void SYIUnifiedDashboard::CreateLootTable()
{
	CreateAssetWithFactory(NewObject<UYILootTableFactory>(), TEXT("/Game/YOLOInventory/Loot"), TEXT("LootTable"));
}

void SYIUnifiedDashboard::CreateRarityProfile()
{
	CreateAssetWithFactory(NewObject<UYIRarityProfileFactory>(), TEXT("/Game/YOLOInventory/Rarity"), TEXT("RarityProfile"));
}

void SYIUnifiedDashboard::CreateItemGenerator()
{
	CreateAssetWithFactory(NewObject<UYIItemGeneratorFactory>(), TEXT("/Game/YOLOInventory/Generators"), TEXT("ItemGenerator"));
}
