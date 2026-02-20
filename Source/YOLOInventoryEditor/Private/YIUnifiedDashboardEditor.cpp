#include "YIUnifiedDashboardEditor.h"
#include "SYIItemDashboard.h"
#include "SYIFragmentDashboard.h"
#include "SYICraftingDashboard.h"
#include "SYIUnifiedDashboard.h"
#include "YIInventoryEditorModule.h"
#include "IYOLOInventoryEditorCoreModule.h"
#include "YIUnifiedDashboardContext.h"
#include "YIItemDefinition.h"
#include "Data/YIDataTableItemSource.h"
#include "Data/YIDataTableAffixSource.h"
#include "Engine/DataTable.h"
#include "YIAffixAsset.h"
#include "YIAffixPoolAsset.h"
#include "YIInventoryBag.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"
#include "FileHelpers.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectGlobals.h"

static const FName Tab_Dashboard_Items(TEXT("YOLOInventory_Dashboard_Items"));
static const FName Tab_Dashboard_Affixes(TEXT("YOLOInventory_Dashboard_Affixes"));
static const FName Tab_Dashboard_Generators(TEXT("YOLOInventory_Dashboard_Generators"));
static const FName Tab_Dashboard_Crafting(TEXT("YOLOInventory_Dashboard_Crafting"));
static const FName Tab_Dashboard_Bags(TEXT("YOLOInventory_Dashboard_Bags"));
static const FName Tab_Dashboard_Equipment(TEXT("YOLOInventory_Dashboard_Equipment"));
static const FName Tab_Dashboard_Help(TEXT("YOLOInventory_Dashboard_Help"));
static const FName Tab_Dashboard_ItemDetails(TEXT("YOLOInventory_Dashboard_ItemDetails"));
static const FName Tab_Dashboard_ItemMappings(TEXT("YOLOInventory_Dashboard_ItemMappings"));
static const FName Tab_Dashboard_ItemPreview(TEXT("YOLOInventory_Dashboard_ItemPreview"));
static const FName Tab_Dashboard_ItemPreflight(TEXT("YOLOInventory_Dashboard_ItemPreflight"));
static const FName Tab_Dashboard_ItemDiff(TEXT("YOLOInventory_Dashboard_ItemDiff"));
static const FName Tab_Dashboard_ItemLogs(TEXT("YOLOInventory_Dashboard_ItemLogs"));
static const FName Tab_Dashboard_AffixDetails(TEXT("YOLOInventory_Dashboard_AffixDetails"));
static const FName Tab_Dashboard_AffixMappings(TEXT("YOLOInventory_Dashboard_AffixMappings"));
static const FName Tab_Dashboard_AffixPreview(TEXT("YOLOInventory_Dashboard_AffixPreview"));
static const FName Tab_Dashboard_GeneratorDetails(TEXT("YOLOInventory_Dashboard_GeneratorDetails"));
static const FName Tab_Dashboard_GeneratorTest(TEXT("YOLOInventory_Dashboard_GeneratorTest"));
static const FName Tab_Dashboard_CraftingDetails(TEXT("YOLOInventory_Dashboard_CraftingDetails"));
static const FName Tab_Dashboard_BagDetails(TEXT("YOLOInventory_Dashboard_BagDetails"));

static bool YIUnifiedDashboard_ShouldPanelStealMode(EYIUnifiedDashboardTab ActiveTab)
{
	(void)ActiveTab;
	// Mode changes are explicit via mode controls, not by activating helper panels.
	return false;
}

namespace
{
	static void YIUnifiedDashboard_TryLoadEditorModule(const FName ModuleName)
	{
		if (!FModuleManager::Get().IsModuleLoaded(ModuleName))
		{
			FModuleManager::Get().LoadModule(ModuleName);
		}
	}

	static UClass* YIUnifiedDashboard_LoadOptionalClass(const TCHAR* ClassPath)
	{
		if (!ClassPath || !*ClassPath)
		{
			return nullptr;
		}
		return LoadObject<UClass>(nullptr, ClassPath);
	}

	static bool YIUnifiedDashboard_IsAssetOfOptionalClass(UObject* Asset, const TCHAR* ClassPath)
	{
		if (!Asset)
		{
			return false;
		}
		if (UClass* OptionalClass = YIUnifiedDashboard_LoadOptionalClass(ClassPath))
		{
			return Asset->IsA(OptionalClass);
		}
		return false;
	}

	static bool YIUnifiedDashboard_IsLootAsset(UObject* Asset)
	{
		return YIUnifiedDashboard_IsAssetOfOptionalClass(Asset, TEXT("/Script/YOLOInventoryLoot.YILootTable"))
			|| YIUnifiedDashboard_IsAssetOfOptionalClass(Asset, TEXT("/Script/YOLOInventoryLoot.YIRarityProfile"))
			|| YIUnifiedDashboard_IsAssetOfOptionalClass(Asset, TEXT("/Script/YOLOInventoryLoot.YIItemGenerator"));
	}

	static bool YIUnifiedDashboard_IsEquipmentSchemaAsset(UObject* Asset)
	{
		return YIUnifiedDashboard_IsAssetOfOptionalClass(Asset, TEXT("/Script/YOLOInventoryEquipment.YIEquipmentSchemaAsset"));
	}
}

void FYIUnifiedDashboardEditor::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* AssetToFocus)
{
	CreateWidgetsIfNeeded();

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("YOLOInventory_Dashboard_Layout_v6")
		->AddArea(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.75f)
				->AddTab(Tab_Dashboard_Items, ETabState::OpenedTab)
				->AddTab(Tab_Dashboard_Affixes, ETabState::ClosedTab)
				->AddTab(Tab_Dashboard_Generators, ETabState::ClosedTab)
				->AddTab(Tab_Dashboard_Crafting, ETabState::ClosedTab)
				->AddTab(Tab_Dashboard_Bags, ETabState::ClosedTab)
				->AddTab(Tab_Dashboard_Equipment, ETabState::ClosedTab)
				->SetForegroundTab(Tab_Dashboard_Items)
			)
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.25f)
				->AddTab(Tab_Dashboard_Help, ETabState::OpenedTab)
				->SetForegroundTab(Tab_Dashboard_Help)
			)
		);

	const bool bCreateMenu = true;
	const bool bCreateToolbar = true;
	if (!EditorContext.IsValid())
	{
		EditorContext = TStrongObjectPtr<UYIUnifiedDashboardContext>(NewObject<UYIUnifiedDashboardContext>(GetTransientPackage(), NAME_None, RF_Transactional));
	}
	InitAssetEditor(Mode, InitToolkitHost, GetToolkitFName(), Layout, bCreateMenu, bCreateToolbar, { EditorContext.Get() });
	ExtendToolbar();

	WorkspaceMenuCategory = TabManager->AddLocalWorkspaceMenuCategory(NSLOCTEXT("YOLOInventory", "DashboardWorkspace", "YOLO Inventory Dashboard"));
	const TSharedRef<FWorkspaceItem> ModesWorkspace = WorkspaceMenuCategory->AddGroup(
		FName("YOLOInventoryDashboardModes"),
		NSLOCTEXT("YOLOInventory", "DashboardWorkspaceModes", "Modes"),
		FSlateIcon(),
		true);
	const TSharedRef<FWorkspaceItem> PanelsWorkspace = WorkspaceMenuCategory->AddGroup(
		FName("YOLOInventoryDashboardPanels"),
		NSLOCTEXT("YOLOInventory", "DashboardWorkspacePanels", "Panels"),
		FSlateIcon(),
		true);
	const TSharedRef<FWorkspaceItem> ItemPanelsWorkspace = PanelsWorkspace->AddGroup(
		FName("YOLOInventoryDashboardItemPanels"),
		NSLOCTEXT("YOLOInventory", "DashboardWorkspaceItemPanels", "Items"),
		FSlateIcon(),
		true);
	const TSharedRef<FWorkspaceItem> AffixPanelsWorkspace = PanelsWorkspace->AddGroup(
		FName("YOLOInventoryDashboardAffixPanels"),
		NSLOCTEXT("YOLOInventory", "DashboardWorkspaceAffixPanels", "Fragments"),
		FSlateIcon(),
		true);
	const TSharedRef<FWorkspaceItem> GeneratorPanelsWorkspace = PanelsWorkspace->AddGroup(
		FName("YOLOInventoryDashboardGeneratorPanels"),
		NSLOCTEXT("YOLOInventory", "DashboardWorkspaceGeneratorPanels", "Generators"),
		FSlateIcon(),
		true);
	const TSharedRef<FWorkspaceItem> BagPanelsWorkspace = PanelsWorkspace->AddGroup(
		FName("YOLOInventoryDashboardBagPanels"),
		NSLOCTEXT("YOLOInventory", "DashboardWorkspaceBagPanels", "Bags"),
		FSlateIcon(),
		true);
	const TSharedRef<FWorkspaceItem> HelpWorkspace = WorkspaceMenuCategory->AddGroup(
		FName("YOLOInventoryDashboardHelpPanels"),
		NSLOCTEXT("YOLOInventory", "DashboardWorkspaceHelp", "Help"),
		FSlateIcon(),
		true);

	TabManager->RegisterTabSpawner(Tab_Dashboard_Items, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnItemsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabItems", "Items"))
		.SetGroup(ModesWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_Affixes, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnAffixesTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabAffixes", "Fragments"))
		.SetGroup(ModesWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_Generators, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnGeneratorsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabGenerators", "Generators"))
		.SetGroup(ModesWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_Crafting, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnCraftingTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabCrafting", "Crafting"))
		.SetGroup(ModesWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_Bags, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnBagsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabBags", "Bags"))
		.SetGroup(ModesWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_Equipment, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnEquipmentTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabEquipment", "Equipment"))
		.SetGroup(ModesWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_Help, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnHelpTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabHelp", "Help"))
		.SetGroup(HelpWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_ItemDetails, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnItemDetailsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabItemDetails", "Item Details"))
		.SetGroup(ItemPanelsWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_ItemMappings, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnItemMappingsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabItemMappings", "Item Mappings"))
		.SetGroup(ItemPanelsWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_ItemPreview, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnItemPreviewTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabItemPreview", "Item Preview"))
		.SetGroup(ItemPanelsWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_ItemPreflight, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnItemPreflightTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabItemPreflight", "Item Preflight"))
		.SetGroup(ItemPanelsWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_ItemDiff, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnItemDiffTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabItemDiff", "Item Diff"))
		.SetGroup(ItemPanelsWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_ItemLogs, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnItemLogsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabItemLogs", "Item Logs"))
		.SetGroup(ItemPanelsWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_AffixDetails, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnAffixDetailsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabAffixDetails", "Fragment Details"))
		.SetGroup(AffixPanelsWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_AffixMappings, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnAffixMappingsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabAffixMappings", "Fragment Notes"))
		.SetGroup(AffixPanelsWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_AffixPreview, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnAffixPreviewTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabAffixPreview", "Fragment Preview"))
		.SetGroup(AffixPanelsWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_GeneratorDetails, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnGeneratorDetailsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabGeneratorDetails", "Generator Details"))
		.SetGroup(GeneratorPanelsWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_GeneratorTest, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnGeneratorTestTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabGeneratorTest", "Generator Test"))
		.SetGroup(GeneratorPanelsWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_BagDetails, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnBagsDetailsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabBagDetails", "Bag Details"))
		.SetGroup(BagPanelsWorkspace);
	TabManager->TryInvokeTab(Tab_Dashboard_Items);
	TabManager->TryInvokeTab(Tab_Dashboard_Help);

	if (AssetToFocus)
	{
		OpenAsset(AssetToFocus);
	}
}

void FYIUnifiedDashboardEditor::ExtendToolbar()
{
	if (ToolbarExtender.IsValid())
	{
		return;
	}

	ToolbarExtender = MakeShared<FExtender>();
	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateSP(this, &FYIUnifiedDashboardEditor::FillDashboardToolbar));
	AddToolbarExtender(ToolbarExtender);
	RegenerateMenusAndToolbars();
}

void FYIUnifiedDashboardEditor::FillDashboardToolbar(FToolBarBuilder& ToolbarBuilder)
{
	auto OpenTab = [this](const FName TabId)
	{
		if (TabManager.IsValid())
		{
			TabManager->TryInvokeTab(TabId);
		}
	};

	auto AddMenuAction = [](FMenuBuilder& MenuBuilder, const FText& Label, const FText& Tooltip, const FSlateIcon& Icon, const FExecuteAction& Execute)
	{
		MenuBuilder.AddMenuEntry(Label, Tooltip, Icon, FUIAction(Execute));
	};

	ToolbarBuilder.BeginSection("YOLOInventoryCompactModes");
	auto AddModeButton = [this, &ToolbarBuilder](EYIUnifiedDashboardTab ModeTab, const FText& Label, const FText& Tooltip, const FSlateIcon& Icon)
	{
		ToolbarBuilder.AddToolBarButton(
			FUIAction(
				FExecuteAction::CreateLambda([this, ModeTab]() { SetActiveTab(ModeTab); }),
				FCanExecuteAction(),
				FIsActionChecked::CreateLambda([this, ModeTab]() { return ActiveTab == ModeTab; })),
			NAME_None,
			Label,
			Tooltip,
			Icon,
			EUserInterfaceActionType::ToggleButton);
	};

	AddModeButton(EYIUnifiedDashboardTab::Items,
		NSLOCTEXT("YOLOInventory", "Dash_Mode_Items", "Items"),
		NSLOCTEXT("YOLOInventory", "Dash_Mode_Items_TT", "Switch to Items mode"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
	AddModeButton(EYIUnifiedDashboardTab::Affixes,
		NSLOCTEXT("YOLOInventory", "Dash_Mode_Affixes", "Fragments"),
		NSLOCTEXT("YOLOInventory", "Dash_Mode_Affixes_TT", "Switch to Fragment mode"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Star"));
	AddModeButton(EYIUnifiedDashboardTab::Generators,
		NSLOCTEXT("YOLOInventory", "Dash_Mode_Generators", "Generators"),
		NSLOCTEXT("YOLOInventory", "Dash_Mode_Generators_TT", "Switch to Generators mode"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Adjust"));
	AddModeButton(EYIUnifiedDashboardTab::Crafting,
		NSLOCTEXT("YOLOInventory", "Dash_Mode_Crafting", "Crafting"),
		NSLOCTEXT("YOLOInventory", "Dash_Mode_Crafting_TT", "Switch to Crafting mode"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"));
	AddModeButton(EYIUnifiedDashboardTab::Bags,
		NSLOCTEXT("YOLOInventory", "Dash_Mode_Bags", "Bags"),
		NSLOCTEXT("YOLOInventory", "Dash_Mode_Bags_TT", "Switch to Bags mode"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.FolderOpen"));
	AddModeButton(EYIUnifiedDashboardTab::Equipment,
		NSLOCTEXT("YOLOInventory", "Dash_Mode_Equipment", "Equipment"),
		NSLOCTEXT("YOLOInventory", "Dash_Mode_Equipment_TT", "Switch to Equipment mode"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Layout"));

	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateSP(this, &FYIUnifiedDashboardEditor::OpenHelpTab)),
		NAME_None,
		NSLOCTEXT("YOLOInventory", "Dash_Mode_Help", "Help"),
		NSLOCTEXT("YOLOInventory", "Dash_Mode_Help_TT", "Open Help panel"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Help"));
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateSP(this, &FYIUnifiedDashboardEditor::SaveAsset_Execute)),
		NAME_None,
		NSLOCTEXT("YOLOInventory", "Dash_Mode_Save", "Save"),
		NSLOCTEXT("YOLOInventory", "Dash_Mode_Save_TT", "Save current mode selection"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Save"));
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("YOLOInventoryCompactPanels");
	ToolbarBuilder.AddComboButton(
		FUIAction(),
		FOnGetContent::CreateLambda([this, OpenTab, AddMenuAction]()
		{
			FMenuBuilder MenuBuilder(true, nullptr);
			switch (ActiveTab)
			{
			case EYIUnifiedDashboardTab::Items:
				AddMenuAction(MenuBuilder, NSLOCTEXT("YOLOInventory", "Dash_Panel_ItemDetails", "Item Details"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"), FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemDetails); }));
				AddMenuAction(MenuBuilder, NSLOCTEXT("YOLOInventory", "Dash_Panel_ItemMappings", "Item Mappings"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Link"), FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemMappings); }));
				AddMenuAction(MenuBuilder, NSLOCTEXT("YOLOInventory", "Dash_Panel_ItemPreview", "Item Preview"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Visibility"), FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemPreview); }));
				AddMenuAction(MenuBuilder, NSLOCTEXT("YOLOInventory", "Dash_Panel_ItemPreflight", "Item Preflight"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Error"), FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemPreflight); }));
				AddMenuAction(MenuBuilder, NSLOCTEXT("YOLOInventory", "Dash_Panel_ItemDiff", "Item Diff"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Diff"), FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemDiff); }));
				AddMenuAction(MenuBuilder, NSLOCTEXT("YOLOInventory", "Dash_Panel_ItemLogs", "Item Logs"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Warning"), FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemLogs); }));
				break;
			case EYIUnifiedDashboardTab::Affixes:
				AddMenuAction(MenuBuilder, NSLOCTEXT("YOLOInventory", "Dash_Panel_AffixDetails", "Fragment Details"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"), FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_AffixDetails); }));
				AddMenuAction(MenuBuilder, NSLOCTEXT("YOLOInventory", "Dash_Panel_AffixMappings", "Fragment Notes"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Link"), FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_AffixMappings); }));
				AddMenuAction(MenuBuilder, NSLOCTEXT("YOLOInventory", "Dash_Panel_AffixPreview", "Fragment Preview"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Visibility"), FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_AffixPreview); }));
				break;
			case EYIUnifiedDashboardTab::Generators:
				AddMenuAction(MenuBuilder, NSLOCTEXT("YOLOInventory", "Dash_Panel_GeneratorDetails", "Generator Details"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"), FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_GeneratorDetails); }));
				AddMenuAction(MenuBuilder, NSLOCTEXT("YOLOInventory", "Dash_Panel_GeneratorTest", "Generator Test"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Play"), FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_GeneratorTest); }));
				break;
			case EYIUnifiedDashboardTab::Bags:
				AddMenuAction(MenuBuilder, NSLOCTEXT("YOLOInventory", "Dash_Panel_BagDetails", "Bag Details"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"), FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_BagDetails); }));
				break;
			case EYIUnifiedDashboardTab::Equipment:
				AddMenuAction(MenuBuilder, NSLOCTEXT("YOLOInventory", "Dash_Panel_EquipmentLayout", "Equipment Schema"), FText::GetEmpty(), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Layout"), FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_Equipment); }));
				break;
			default:
				MenuBuilder.AddMenuEntry(NSLOCTEXT("YOLOInventory", "Dash_Panel_None", "No mode panels"), FText::GetEmpty(), FSlateIcon(), FUIAction());
				break;
			}
			return MenuBuilder.MakeWidget();
		}),
		NSLOCTEXT("YOLOInventory", "Dash_Panel_Menu", "Panels"),
		NSLOCTEXT("YOLOInventory", "Dash_Panel_Menu_TT", "Open mode panel tabs"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Toolbar.More"));
	ToolbarBuilder.EndSection();

	// Keep toolbar compact: actions live in panel UI and mode-specific context menus.
	return;
}

void FYIUnifiedDashboardEditor::CloseItemPanelTabs()
{
	if (!TabManager.IsValid())
	{
		return;
	}

	auto CloseTab = [this](const FName TabId)
	{
		if (TSharedPtr<SDockTab> Tab = TabManager->FindExistingLiveTab(TabId))
		{
			Tab->RequestCloseTab();
		}
	};

	CloseTab(Tab_Dashboard_ItemDetails);
	CloseTab(Tab_Dashboard_ItemMappings);
	CloseTab(Tab_Dashboard_ItemPreview);
	CloseTab(Tab_Dashboard_ItemPreflight);
	CloseTab(Tab_Dashboard_ItemDiff);
	CloseTab(Tab_Dashboard_ItemLogs);
}

void FYIUnifiedDashboardEditor::CloseAffixPanelTabs()
{
	if (!TabManager.IsValid())
	{
		return;
	}

	auto CloseTab = [this](const FName TabId)
	{
		if (TSharedPtr<SDockTab> Tab = TabManager->FindExistingLiveTab(TabId))
		{
			Tab->RequestCloseTab();
		}
	};

	CloseTab(Tab_Dashboard_AffixDetails);
	CloseTab(Tab_Dashboard_AffixMappings);
	CloseTab(Tab_Dashboard_AffixPreview);
}

void FYIUnifiedDashboardEditor::CloseGeneratorPanelTabs()
{
	if (!TabManager.IsValid())
	{
		return;
	}

	auto CloseTab = [this](const FName TabId)
	{
		if (TSharedPtr<SDockTab> Tab = TabManager->FindExistingLiveTab(TabId))
		{
			Tab->RequestCloseTab();
		}
	};

	CloseTab(Tab_Dashboard_GeneratorDetails);
	CloseTab(Tab_Dashboard_GeneratorTest);
}

void FYIUnifiedDashboardEditor::CloseCraftingPanelTabs()
{
	if (!TabManager.IsValid())
	{
		return;
	}

	if (TSharedPtr<SDockTab> Tab = TabManager->FindExistingLiveTab(Tab_Dashboard_Crafting))
	{
		Tab->RequestCloseTab();
	}
	if (TSharedPtr<SDockTab> Tab = TabManager->FindExistingLiveTab(Tab_Dashboard_CraftingDetails))
	{
		Tab->RequestCloseTab();
	}
}

void FYIUnifiedDashboardEditor::CloseBagPanelTabs()
{
	if (!TabManager.IsValid())
	{
		return;
	}

	if (TSharedPtr<SDockTab> Tab = TabManager->FindExistingLiveTab(Tab_Dashboard_Bags))
	{
		Tab->RequestCloseTab();
	}
	if (TSharedPtr<SDockTab> Tab = TabManager->FindExistingLiveTab(Tab_Dashboard_BagDetails))
	{
		Tab->RequestCloseTab();
	}
}

void FYIUnifiedDashboardEditor::CloseEquipmentModeTabs()
{
	if (!TabManager.IsValid())
	{
		return;
	}

	if (TSharedPtr<SDockTab> Tab = TabManager->FindExistingLiveTab(Tab_Dashboard_Equipment))
	{
		Tab->RequestCloseTab();
	}
}

void FYIUnifiedDashboardEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FWorkflowCentricApplication::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Items);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Affixes);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Generators);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Crafting);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Bags);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Equipment);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Help);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemDetails);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemMappings);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemPreview);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemPreflight);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemDiff);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemLogs);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_AffixDetails);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_AffixMappings);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_AffixPreview);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_GeneratorDetails);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_GeneratorTest);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_CraftingDetails);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_BagDetails);
}

void FYIUnifiedDashboardEditor::CreateWidgetsIfNeeded()
{
	if (!ItemDashboard.IsValid())
	{
		ItemDashboard = SNew(SYIItemDashboard).LayoutMode(EYIItemDashboardLayout::ItemListOnly);
	}
	if (!AffixDashboard.IsValid())
	{
		AffixDashboard = SNew(SYIFragmentDashboard).LayoutMode(EYIFragmentDashboardLayout::AssetListOnly);
	}
	if (!GeneratorDashboard.IsValid())
	{
		YIUnifiedDashboard_TryLoadEditorModule(TEXT("YOLOInventoryEditorCore"));
		if (IYOLOInventoryEditorCoreModule::IsAvailable())
		{
			IYOLOInventoryEditorCoreModule& EditorCoreModule = IYOLOInventoryEditorCoreModule::Get();
			if (!EditorCoreModule.HasGeneratorDashboardFactory())
			{
				YIUnifiedDashboard_TryLoadEditorModule(TEXT("YOLOInventoryEditorLoot"));
			}
			if (EditorCoreModule.HasGeneratorDashboardFactory())
			{
				GeneratorDashboard = EditorCoreModule.CreateGeneratorDashboardBridge();
			}
		}
	}
	if (!CraftingDashboard.IsValid())
	{
		CraftingDashboard = SNew(SYICraftingDashboard);
	}
	if (!BagDashboard.IsValid())
	{
		YIUnifiedDashboard_TryLoadEditorModule(TEXT("YOLOInventoryEditorCore"));
		if (IYOLOInventoryEditorCoreModule::IsAvailable())
		{
			IYOLOInventoryEditorCoreModule& EditorCoreModule = IYOLOInventoryEditorCoreModule::Get();
			if (!EditorCoreModule.HasBagDashboardFactory())
			{
				YIUnifiedDashboard_TryLoadEditorModule(TEXT("YOLOInventoryEditorGrid"));
			}
			if (EditorCoreModule.HasBagDashboardFactory())
			{
				BagDashboard = EditorCoreModule.CreateBagDashboardBridge();
			}
		}
	}
	if (!HelpPanel.IsValid())
	{
		HelpPanel = SNew(SYIUnifiedHelpPanel);
		FYOLOInventoryEditorModule::Get().RegisterHelpWidget(HelpPanel);
	}
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnItemsTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabItemsLabel", "Items"))
		[
			ItemDashboard.ToSharedRef()
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			if (YIUnifiedDashboard_ShouldPanelStealMode(ActiveTab))
			{
				SetActiveTab(EYIUnifiedDashboardTab::Items);
			}
		}));
	return Tab;
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnAffixesTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabAffixesLabel", "Fragments"))
		[
			AffixDashboard.ToSharedRef()
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			SetActiveTab(EYIUnifiedDashboardTab::Affixes);
		}));
	return Tab;
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnGeneratorsTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabGeneratorsLabel", "Generators"))
		[
			GeneratorDashboard.IsValid()
				? GeneratorDashboard->GetRootWidget()
				: SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "GeneratorDashboardMissing", "YOLOInventoryEditorLoot plugin is not enabled."))
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			SetActiveTab(EYIUnifiedDashboardTab::Generators);
		}));
	return Tab;
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnCraftingTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabCraftingLabel", "Crafting"))
		[
			CraftingDashboard.ToSharedRef()
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			SetActiveTab(EYIUnifiedDashboardTab::Crafting);
		}));
	return Tab;
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnBagsTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabBagsLabel", "Bags"))
		[
			BagDashboard.IsValid()
				? BagDashboard->GetRootWidget()
				: SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "BagDashboardMissing", "YOLOInventoryEditorGrid plugin is not enabled."))
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			SetActiveTab(EYIUnifiedDashboardTab::Bags);
		}));
	return Tab;
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnEquipmentTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabEquipmentLabel", "Equipment"))
		[
			BagDashboard.IsValid()
				? BagDashboard->GetEquipmentLayoutPanelWidget()
				: SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "EquipmentDashboardMissing", "YOLOInventoryEditorGrid plugin is not enabled."))
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			SetActiveTab(EYIUnifiedDashboardTab::Equipment);
		}));
	return Tab;
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnHelpTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	return SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabHelpLabel", "Help"))
		[
			HelpPanel.ToSharedRef()
		];
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnItemDetailsTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabItemDetailsLabel", "Item Details"))
		[
			ItemDashboard->GetDetailsPanelWidget()
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			if (YIUnifiedDashboard_ShouldPanelStealMode(ActiveTab))
			{
				SetActiveTab(EYIUnifiedDashboardTab::Items);
			}
		}));
	return Tab;
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnItemMappingsTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabItemMappingsLabel", "Item Mappings"))
		[
			ItemDashboard->GetMappingPanelWidget()
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			if (YIUnifiedDashboard_ShouldPanelStealMode(ActiveTab))
			{
				SetActiveTab(EYIUnifiedDashboardTab::Items);
			}
		}));
	return Tab;
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnItemPreviewTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabItemPreviewLabel", "Item Preview"))
		[
			ItemDashboard->GetPreviewPanelWidget()
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			if (YIUnifiedDashboard_ShouldPanelStealMode(ActiveTab))
			{
				SetActiveTab(EYIUnifiedDashboardTab::Items);
			}
		}));
	return Tab;
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnItemPreflightTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabItemPreflightLabel", "Item Preflight"))
		[
			ItemDashboard->GetPreflightPanelWidget()
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			if (YIUnifiedDashboard_ShouldPanelStealMode(ActiveTab))
			{
				SetActiveTab(EYIUnifiedDashboardTab::Items);
			}
		}));
	return Tab;
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnItemDiffTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabItemDiffLabel", "Item Diff"))
		[
			ItemDashboard->GetDiffPanelWidget()
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			if (YIUnifiedDashboard_ShouldPanelStealMode(ActiveTab))
			{
				SetActiveTab(EYIUnifiedDashboardTab::Items);
			}
		}));
	return Tab;
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnItemLogsTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabItemLogsLabel", "Item Logs"))
		[
			ItemDashboard->GetLogsPanelWidget()
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			if (YIUnifiedDashboard_ShouldPanelStealMode(ActiveTab))
			{
				SetActiveTab(EYIUnifiedDashboardTab::Items);
			}
		}));
	return Tab;
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnAffixDetailsTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabAffixDetailsLabel", "Fragment Details"))
		[
			AffixDashboard->GetDetailsPanelWidget()
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			if (YIUnifiedDashboard_ShouldPanelStealMode(ActiveTab))
			{
				SetActiveTab(EYIUnifiedDashboardTab::Affixes);
			}
		}));
	return Tab;
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnAffixMappingsTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabAffixMappingsLabel", "Fragment Notes"))
		[
			AffixDashboard->GetMappingPanelWidget()
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			if (YIUnifiedDashboard_ShouldPanelStealMode(ActiveTab))
			{
				SetActiveTab(EYIUnifiedDashboardTab::Affixes);
			}
		}));
	return Tab;
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnAffixPreviewTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabAffixPreviewLabel", "Fragment Preview"))
		[
			AffixDashboard->GetPreviewPanelWidget()
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			if (YIUnifiedDashboard_ShouldPanelStealMode(ActiveTab))
			{
				SetActiveTab(EYIUnifiedDashboardTab::Affixes);
			}
		}));
	return Tab;
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnGeneratorDetailsTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabGeneratorDetailsLabel", "Generator Details"))
		[
			GeneratorDashboard.IsValid()
				? GeneratorDashboard->GetDetailsPanelWidget()
				: SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "GeneratorDashboardDetailsMissing", "Generator dashboard unavailable."))
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			if (YIUnifiedDashboard_ShouldPanelStealMode(ActiveTab))
			{
				SetActiveTab(EYIUnifiedDashboardTab::Generators);
			}
		}));
	return Tab;
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnGeneratorTestTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabGeneratorTestLabel", "Generator Test"))
		[
			GeneratorDashboard.IsValid()
				? GeneratorDashboard->GetTestPanelWidget()
				: SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "GeneratorDashboardTestMissing", "Generator dashboard unavailable."))
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			if (YIUnifiedDashboard_ShouldPanelStealMode(ActiveTab))
			{
				SetActiveTab(EYIUnifiedDashboardTab::Generators);
			}
		}));
	return Tab;
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnCraftingDetailsTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	return SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabCraftingDetailsLabel", "Crafting Details"))
		[
			CraftingDashboard.ToSharedRef()
		];
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnBagsDetailsTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	return SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabBagsDetailsLabel", "Bag Details"))
		[
			BagDashboard.IsValid()
				? BagDashboard->GetDetailsPanelWidget()
				: SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "BagDashboardDetailsMissing", "Bag dashboard unavailable."))
		];
}

void FYIUnifiedDashboardEditor::SetActiveTab(EYIUnifiedDashboardTab NewTab)
{
	ActiveTab = NewTab;
	auto EnsureTabOpen = [this](const FName TabId)
	{
		if (TabManager.IsValid())
		{
			TabManager->TryInvokeTab(TabId);
		}
	};

	switch (ActiveTab)
	{
	case EYIUnifiedDashboardTab::Items:
		CloseAffixPanelTabs();
		CloseGeneratorPanelTabs();
		CloseCraftingPanelTabs();
		CloseBagPanelTabs();
		CloseEquipmentModeTabs();
		EnsureTabOpen(Tab_Dashboard_Items);
		EnsureTabOpen(Tab_Dashboard_ItemDetails);
		EnsureTabOpen(Tab_Dashboard_ItemMappings);
		EnsureTabOpen(Tab_Dashboard_ItemPreview);
		EnsureTabOpen(Tab_Dashboard_ItemPreflight);
		EnsureTabOpen(Tab_Dashboard_ItemDiff);
		EnsureTabOpen(Tab_Dashboard_ItemLogs);
		FYOLOInventoryEditorModule::Get().UpdateHelpTabIndex(0);
		break;
	case EYIUnifiedDashboardTab::Affixes:
		CloseItemPanelTabs();
		CloseGeneratorPanelTabs();
		CloseCraftingPanelTabs();
		CloseBagPanelTabs();
		CloseEquipmentModeTabs();
		EnsureTabOpen(Tab_Dashboard_Affixes);
		EnsureTabOpen(Tab_Dashboard_AffixDetails);
		EnsureTabOpen(Tab_Dashboard_AffixMappings);
		EnsureTabOpen(Tab_Dashboard_AffixPreview);
		FYOLOInventoryEditorModule::Get().UpdateHelpTabIndex(1);
		break;
	case EYIUnifiedDashboardTab::Generators:
		CloseItemPanelTabs();
		CloseAffixPanelTabs();
		CloseCraftingPanelTabs();
		CloseBagPanelTabs();
		CloseEquipmentModeTabs();
		EnsureTabOpen(Tab_Dashboard_Generators);
		EnsureTabOpen(Tab_Dashboard_GeneratorDetails);
		EnsureTabOpen(Tab_Dashboard_GeneratorTest);
		FYOLOInventoryEditorModule::Get().UpdateHelpTabIndex(2);
		break;
	case EYIUnifiedDashboardTab::Crafting:
		CloseItemPanelTabs();
		CloseAffixPanelTabs();
		CloseGeneratorPanelTabs();
		CloseBagPanelTabs();
		CloseEquipmentModeTabs();
		if (TabManager.IsValid())
		{
			if (TSharedPtr<SDockTab> DetailTab = TabManager->FindExistingLiveTab(Tab_Dashboard_CraftingDetails))
			{
				DetailTab->RequestCloseTab();
			}
		}
		if (CraftingDashboard.IsValid() && BagDashboard.IsValid() && !CraftingDashboard->GetTargetBag())
		{
			CraftingDashboard->SetTargetBag(BagDashboard->GetSelectedBag());
		}
		EnsureTabOpen(Tab_Dashboard_Crafting);
		FYOLOInventoryEditorModule::Get().UpdateHelpTabIndex(0);
		break;
	case EYIUnifiedDashboardTab::Bags:
		CloseItemPanelTabs();
		CloseAffixPanelTabs();
		CloseGeneratorPanelTabs();
		CloseCraftingPanelTabs();
		CloseEquipmentModeTabs();
		if (BagDashboard.IsValid() && CraftingDashboard.IsValid() && !BagDashboard->GetSelectedBag())
		{
			BagDashboard->SetSelectedBag(CraftingDashboard->GetTargetBag());
		}
		EnsureTabOpen(Tab_Dashboard_Bags);
		EnsureTabOpen(Tab_Dashboard_BagDetails);
		FYOLOInventoryEditorModule::Get().UpdateHelpTabIndex(0);
		break;
	case EYIUnifiedDashboardTab::Equipment:
		CloseItemPanelTabs();
		CloseAffixPanelTabs();
		CloseGeneratorPanelTabs();
		CloseCraftingPanelTabs();
		CloseBagPanelTabs();
		EnsureTabOpen(Tab_Dashboard_Equipment);
		FYOLOInventoryEditorModule::Get().UpdateHelpTabIndex(0);
		break;
	default:
		break;
	}
	RegenerateMenusAndToolbars();
}

void FYIUnifiedDashboardEditor::OpenHelpTab()
{
	if (TabManager.IsValid())
	{
		TabManager->TryInvokeTab(Tab_Dashboard_Help);
	}
}

void FYIUnifiedDashboardEditor::SaveAsset_Execute()
{
	switch (ActiveTab)
	{
	case EYIUnifiedDashboardTab::Items:
		if (ItemDashboard.IsValid())
		{
			ItemDashboard->SaveCurrentAssetFromToolbar();
			return;
		}
		break;
	case EYIUnifiedDashboardTab::Affixes:
		if (AffixDashboard.IsValid())
		{
			AffixDashboard->SaveCurrentAssetFromToolbar();
			return;
		}
		break;
	case EYIUnifiedDashboardTab::Generators:
		if (GeneratorDashboard.IsValid())
		{
			GeneratorDashboard->SaveCurrentAssetFromToolbar();
			return;
		}
		break;
	case EYIUnifiedDashboardTab::Crafting:
		if (CraftingDashboard.IsValid())
		{
			CraftingDashboard->SaveTargetBagFromToolbar();
			return;
		}
		break;
	case EYIUnifiedDashboardTab::Bags:
		if (BagDashboard.IsValid())
		{
			BagDashboard->SaveCurrentBagFromToolbar();
			return;
		}
		break;
	case EYIUnifiedDashboardTab::Equipment:
		if (BagDashboard.IsValid())
		{
			BagDashboard->SaveCurrentEquipmentLayoutFromToolbar();
			return;
		}
		break;
	default:
		break;
	}

	FWorkflowCentricApplication::SaveAsset_Execute();
}

void FYIUnifiedDashboardEditor::OpenAsset(UObject* Asset)
{
	if (!Asset)
	{
		SetActiveTab(EYIUnifiedDashboardTab::Items);
		return;
	}

	CreateWidgetsIfNeeded();

	const bool bIsItemFamilyAsset = Asset->IsA<UYIItemDefinition>() || Asset->IsA<UYIDataTableItemSource>() || Asset->IsA<UDataTable>();
	if (bIsItemFamilyAsset && ActiveTab != EYIUnifiedDashboardTab::Items)
	{
		// Keep current workflow mode when users pick items from auxiliary item panels.
		switch (ActiveTab)
		{
		case EYIUnifiedDashboardTab::Bags:
			if (BagDashboard.IsValid())
			{
				BagDashboard->OpenAsset(Asset);
			}
			return;
		case EYIUnifiedDashboardTab::Crafting:
			if (CraftingDashboard.IsValid())
			{
				CraftingDashboard->OpenAsset(Asset);
			}
			if (BagDashboard.IsValid())
			{
				BagDashboard->OpenAsset(Asset);
			}
			return;
		case EYIUnifiedDashboardTab::Affixes:
			if (AffixDashboard.IsValid())
			{
				AffixDashboard->OpenAsset(Asset);
			}
			return;
		case EYIUnifiedDashboardTab::Generators:
			if (GeneratorDashboard.IsValid())
			{
				GeneratorDashboard->OpenAsset(Asset);
			}
			return;
		case EYIUnifiedDashboardTab::Equipment:
			if (BagDashboard.IsValid())
			{
				BagDashboard->OpenAsset(Asset);
			}
			return;
		default:
			break;
		}
	}

	// In Bags/Crafting modes, keep focus there for item selection workflows.
	if (ActiveTab == EYIUnifiedDashboardTab::Bags && (Asset->IsA<UYIInventoryBag>() || Asset->IsA<UYIItemDefinition>() || YIUnifiedDashboard_IsEquipmentSchemaAsset(Asset)))
	{
		if (YIUnifiedDashboard_IsEquipmentSchemaAsset(Asset))
		{
			SetActiveTab(EYIUnifiedDashboardTab::Equipment);
			if (BagDashboard.IsValid())
			{
				BagDashboard->OpenAsset(Asset);
			}
			return;
		}
		if (BagDashboard.IsValid())
		{
			BagDashboard->OpenAsset(Asset);
		}
		return;
	}
	if (ActiveTab == EYIUnifiedDashboardTab::Equipment && (Asset->IsA<UYIInventoryBag>() || Asset->IsA<UYIItemDefinition>() || YIUnifiedDashboard_IsEquipmentSchemaAsset(Asset)))
	{
		if (BagDashboard.IsValid())
		{
			BagDashboard->OpenAsset(Asset);
		}
		return;
	}
	if (ActiveTab == EYIUnifiedDashboardTab::Crafting && (Asset->IsA<UYIInventoryBag>() || Asset->IsA<UYIItemDefinition>() || Asset->IsA<UYIAffixAsset>()))
	{
		if (CraftingDashboard.IsValid())
		{
			CraftingDashboard->OpenAsset(Asset);
		}
		if (Asset->IsA<UYIInventoryBag>() && BagDashboard.IsValid())
		{
			BagDashboard->OpenAsset(Asset);
		}
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

	if (Asset->IsA<UYIAffixAsset>() || Asset->IsA<UYIAffixPoolAsset>() || Asset->IsA<UYIDataTableAffixSource>())
	{
		SetActiveTab(EYIUnifiedDashboardTab::Affixes);
		if (AffixDashboard.IsValid())
		{
			AffixDashboard->OpenAsset(Asset);
		}
		return;
	}

	if (YIUnifiedDashboard_IsLootAsset(Asset))
	{
		SetActiveTab(EYIUnifiedDashboardTab::Generators);
		if (GeneratorDashboard.IsValid())
		{
			GeneratorDashboard->OpenAsset(Asset);
		}
		return;
	}

	if (Asset->IsA<UYIInventoryBag>())
	{
		SetActiveTab(EYIUnifiedDashboardTab::Bags);
		if (BagDashboard.IsValid())
		{
			BagDashboard->OpenAsset(Asset);
		}
		if (CraftingDashboard.IsValid())
		{
			CraftingDashboard->OpenAsset(Asset);
		}
		return;
	}

	if (YIUnifiedDashboard_IsEquipmentSchemaAsset(Asset))
	{
		SetActiveTab(EYIUnifiedDashboardTab::Equipment);
		if (BagDashboard.IsValid())
		{
			BagDashboard->OpenAsset(Asset);
		}
		return;
	}

}
