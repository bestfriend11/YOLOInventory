#include "YIUnifiedDashboardEditor.h"
#include "SYIItemDashboard.h"
#include "SYIAffixDashboard.h"
#include "SYIGeneratorDashboard.h"
#include "SYICraftingDashboard.h"
#include "SYIBagDashboard.h"
#include "SYIUnifiedDashboard.h"
#include "YIInventoryEditorModule.h"
#include "YIUnifiedDashboardContext.h"
#include "YIItemDefinition.h"
#include "Data/YIDataTableItemSource.h"
#include "Data/YIDataTableAffixSource.h"
#include "Engine/DataTable.h"
#include "YIAffixAsset.h"
#include "YIAffixPoolAsset.h"
#include "YILootTable.h"
#include "YIRarityProfile.h"
#include "YIItemGenerator.h"
#include "YIInventoryBag.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"

static const FName Tab_Dashboard_Items(TEXT("YOLOInventory_Dashboard_Items"));
static const FName Tab_Dashboard_Affixes(TEXT("YOLOInventory_Dashboard_Affixes"));
static const FName Tab_Dashboard_Generators(TEXT("YOLOInventory_Dashboard_Generators"));
static const FName Tab_Dashboard_Crafting(TEXT("YOLOInventory_Dashboard_Crafting"));
static const FName Tab_Dashboard_Bags(TEXT("YOLOInventory_Dashboard_Bags"));
static const FName Tab_Dashboard_Help(TEXT("YOLOInventory_Dashboard_Help"));
static const FName Tab_Dashboard_ItemDetails(TEXT("YOLOInventory_Dashboard_ItemDetails"));
static const FName Tab_Dashboard_ItemMappings(TEXT("YOLOInventory_Dashboard_ItemMappings"));
static const FName Tab_Dashboard_ItemPreview(TEXT("YOLOInventory_Dashboard_ItemPreview"));
static const FName Tab_Dashboard_ItemPreflight(TEXT("YOLOInventory_Dashboard_ItemPreflight"));
static const FName Tab_Dashboard_ItemDiff(TEXT("YOLOInventory_Dashboard_ItemDiff"));
static const FName Tab_Dashboard_ItemBatch(TEXT("YOLOInventory_Dashboard_ItemBatch"));
static const FName Tab_Dashboard_ItemLogs(TEXT("YOLOInventory_Dashboard_ItemLogs"));
static const FName Tab_Dashboard_AffixDetails(TEXT("YOLOInventory_Dashboard_AffixDetails"));
static const FName Tab_Dashboard_AffixSource(TEXT("YOLOInventory_Dashboard_AffixSource"));
static const FName Tab_Dashboard_AffixMappings(TEXT("YOLOInventory_Dashboard_AffixMappings"));
static const FName Tab_Dashboard_AffixPreview(TEXT("YOLOInventory_Dashboard_AffixPreview"));
static const FName Tab_Dashboard_GeneratorDetails(TEXT("YOLOInventory_Dashboard_GeneratorDetails"));
static const FName Tab_Dashboard_GeneratorTest(TEXT("YOLOInventory_Dashboard_GeneratorTest"));
static const FName Tab_Dashboard_CraftingDetails(TEXT("YOLOInventory_Dashboard_CraftingDetails"));
static const FName Tab_Dashboard_BagDetails(TEXT("YOLOInventory_Dashboard_BagDetails"));

static bool YIUnifiedDashboard_ShouldPanelStealMode(EYIUnifiedDashboardTab ActiveTab)
{
	// Keep Bags/Crafting workflow stable when users click auxiliary tabs.
	return ActiveTab != EYIUnifiedDashboardTab::Bags && ActiveTab != EYIUnifiedDashboardTab::Crafting;
}

void FYIUnifiedDashboardEditor::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* AssetToFocus)
{
	CreateWidgetsIfNeeded();

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("YOLOInventory_Dashboard_Layout_v4")
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
		NSLOCTEXT("YOLOInventory", "DashboardWorkspaceAffixPanels", "Affixes"),
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
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabAffixes", "Affixes"))
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
	TabManager->RegisterTabSpawner(Tab_Dashboard_ItemBatch, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnItemBatchTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabItemBatch", "Item Batch"))
		.SetGroup(ItemPanelsWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_ItemLogs, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnItemLogsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabItemLogs", "Item Logs"))
		.SetGroup(ItemPanelsWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_AffixDetails, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnAffixDetailsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabAffixDetails", "Affix Details"))
		.SetGroup(AffixPanelsWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_AffixSource, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnAffixSourceTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabAffixSource", "Affix Source"))
		.SetGroup(AffixPanelsWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_AffixMappings, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnAffixMappingsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabAffixMappings", "Affix Mappings"))
		.SetGroup(AffixPanelsWorkspace);
	TabManager->RegisterTabSpawner(Tab_Dashboard_AffixPreview, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnAffixPreviewTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabAffixPreview", "Affix Preview"))
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

	ToolbarBuilder.BeginSection("YOLOInventoryModes");
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateLambda([this]() { SetActiveTab(EYIUnifiedDashboardTab::Items); })),
		NAME_None,
		NSLOCTEXT("YOLOInventory", "Dash_TB_ModeItems", "Items"),
		NSLOCTEXT("YOLOInventory", "Dash_TB_ModeItems_Tip", "Switch to Items mode"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateLambda([this]() { SetActiveTab(EYIUnifiedDashboardTab::Affixes); })),
		NAME_None,
		NSLOCTEXT("YOLOInventory", "Dash_TB_ModeAffixes", "Affixes"),
		NSLOCTEXT("YOLOInventory", "Dash_TB_ModeAffixes_Tip", "Switch to Affixes mode"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Star"));
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateLambda([this]() { SetActiveTab(EYIUnifiedDashboardTab::Generators); })),
		NAME_None,
		NSLOCTEXT("YOLOInventory", "Dash_TB_ModeGenerators", "Generators"),
		NSLOCTEXT("YOLOInventory", "Dash_TB_ModeGenerators_Tip", "Switch to Generators mode"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Adjust"));
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateLambda([this]() { SetActiveTab(EYIUnifiedDashboardTab::Crafting); })),
		NAME_None,
		NSLOCTEXT("YOLOInventory", "Dash_TB_ModeCrafting", "Crafting"),
		NSLOCTEXT("YOLOInventory", "Dash_TB_ModeCrafting_Tip", "Craft specific item instances with selected affixes"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"));
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateLambda([this]() { SetActiveTab(EYIUnifiedDashboardTab::Bags); })),
		NAME_None,
		NSLOCTEXT("YOLOInventory", "Dash_TB_ModeBags", "Bags"),
		NSLOCTEXT("YOLOInventory", "Dash_TB_ModeBags_Tip", "Edit bag runtime layout and contents"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.FolderOpen"));
	ToolbarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateLambda([this]() { OpenHelpTab(); })),
		NAME_None,
		NSLOCTEXT("YOLOInventory", "Dash_TB_ModeHelp", "Help"),
		NSLOCTEXT("YOLOInventory", "Dash_TB_ModeHelp_Tip", "Open Help"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Help"));
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("YOLOInventoryPanels");
	switch (ActiveTab)
	{
	case EYIUnifiedDashboardTab::Items:
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemDetails); })),
			NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_Details", "Details"),
			NSLOCTEXT("YOLOInventory", "Dash_TB_Details_Tip", "Open Item Details panel"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemMappings); })),
			NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_Mappings", "Mappings"),
			NSLOCTEXT("YOLOInventory", "Dash_TB_Mappings_Tip", "Open Item Mappings panel"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Link"));
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemPreview); })),
			NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_Preview", "Preview"),
			NSLOCTEXT("YOLOInventory", "Dash_TB_Preview_Tip", "Open Item Preview panel"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Visibility"));
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemPreflight); })),
			NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_Preflight", "Preflight"),
			NSLOCTEXT("YOLOInventory", "Dash_TB_Preflight_Tip", "Open Item Preflight panel"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Error"));
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemDiff); })),
			NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_Diff", "Diff"),
			NSLOCTEXT("YOLOInventory", "Dash_TB_Diff_Tip", "Open Item Diff panel"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Diff"));
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemBatch); })),
			NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_Batch", "Batch"),
			NSLOCTEXT("YOLOInventory", "Dash_TB_Batch_Tip", "Open Item Batch panel"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.ListView"));
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemLogs); })),
			NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_Logs", "Logs"),
			NSLOCTEXT("YOLOInventory", "Dash_TB_Logs_Tip", "Open Item Logs panel"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Warning"));
		break;
	case EYIUnifiedDashboardTab::Affixes:
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_AffixDetails); })),
			NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_AffixDetails", "Details"),
			NSLOCTEXT("YOLOInventory", "Dash_TB_AffixDetails_Tip", "Open Affix Details panel"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_AffixSource); })),
			NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_AffixSource", "Source"),
			NSLOCTEXT("YOLOInventory", "Dash_TB_AffixSource_Tip", "Open Affix Source panel"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Search"));
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_AffixMappings); })),
			NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_AffixMappings", "Mappings"),
			NSLOCTEXT("YOLOInventory", "Dash_TB_AffixMappings_Tip", "Open Affix Mappings panel"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Link"));
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_AffixPreview); })),
			NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_AffixPreview", "Preview"),
			NSLOCTEXT("YOLOInventory", "Dash_TB_AffixPreview_Tip", "Open Affix Preview panel"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Visibility"));
		break;
	case EYIUnifiedDashboardTab::Generators:
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_GeneratorDetails); })),
			NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_GenDetails", "Details"),
			NSLOCTEXT("YOLOInventory", "Dash_TB_GenDetails_Tip", "Open Generator Details panel"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_GeneratorTest); })),
			NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_GenTest", "Test"),
			NSLOCTEXT("YOLOInventory", "Dash_TB_GenTest_Tip", "Open Generator Test panel"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Play"));
		break;
	case EYIUnifiedDashboardTab::Crafting:
		break;
	case EYIUnifiedDashboardTab::Bags:
		ToolbarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_BagDetails); })),
			NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_BagDetails", "Bag Details"),
			NSLOCTEXT("YOLOInventory", "Dash_TB_BagDetails_Tip", "Open bag details panel"),
			FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
		break;
	default:
		break;
	}
	ToolbarBuilder.EndSection();

	ToolbarBuilder.BeginSection("YOLOInventoryActions");
	switch (ActiveTab)
	{
	case EYIUnifiedDashboardTab::Items:
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (ItemDashboard.IsValid()) ItemDashboard->GuidedSetupFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_GuidedItems", "Guided Setup"), NSLOCTEXT("YOLOInventory", "Dash_TB_GuidedItems_Tip", "Run guided item source setup with readiness checks"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Adjust"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (ItemDashboard.IsValid()) ItemDashboard->SaveCurrentAssetFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_SaveItem", "Save"), NSLOCTEXT("YOLOInventory", "Dash_TB_SaveItem_Tip", "Save currently selected item/dashboard asset"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Save"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (ItemDashboard.IsValid()) ItemDashboard->RefreshFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_Refresh", "Refresh"), NSLOCTEXT("YOLOInventory", "Dash_TB_Refresh_Tip", "Refresh item dashboard list"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Refresh"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (ItemDashboard.IsValid()) ItemDashboard->CreateDataSourceFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_NewItemSource", "New Source"), NSLOCTEXT("YOLOInventory", "Dash_TB_NewItemSource_Tip", "Create item data source asset"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (ItemDashboard.IsValid()) ItemDashboard->CreateOrUpdateSelectedFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_CreateUpdate", "Create/Update"), NSLOCTEXT("YOLOInventory", "Dash_TB_CreateUpdate_Tip", "Create or update selected item rows"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Save"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (ItemDashboard.IsValid()) ItemDashboard->UpdateLinkedSelectedFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_UpdateLinked", "Update Linked"), NSLOCTEXT("YOLOInventory", "Dash_TB_UpdateLinked_Tip", "Update selected linked item assets"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Merge"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (ItemDashboard.IsValid()) ItemDashboard->PreflightSelectedFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_PreflightRun", "Run Preflight"), NSLOCTEXT("YOLOInventory", "Dash_TB_PreflightRun_Tip", "Run preflight checks on selection"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Error"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (ItemDashboard.IsValid()) ItemDashboard->QueueSelectedFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_Queue", "Queue"), NSLOCTEXT("YOLOInventory", "Dash_TB_Queue_Tip", "Queue selected rows"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.ListView"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (ItemDashboard.IsValid()) ItemDashboard->RunQueueFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_RunQueue", "Run Queue"), NSLOCTEXT("YOLOInventory", "Dash_TB_RunQueue_Tip", "Run queued item jobs"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Play"));
		break;
	case EYIUnifiedDashboardTab::Affixes:
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (AffixDashboard.IsValid()) AffixDashboard->GuidedSetupFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_GuidedAffix", "Guided Setup"), NSLOCTEXT("YOLOInventory", "Dash_TB_GuidedAffix_Tip", "Run guided affix source setup with readiness checks"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Adjust"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (AffixDashboard.IsValid()) AffixDashboard->SaveCurrentAssetFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_SaveAffix", "Save"), NSLOCTEXT("YOLOInventory", "Dash_TB_SaveAffix_Tip", "Save currently selected affix/dashboard asset"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Save"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (AffixDashboard.IsValid()) AffixDashboard->CreateAffixFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_NewAffix", "New Affix"), NSLOCTEXT("YOLOInventory", "Dash_TB_NewAffix_Tip", "Create affix asset"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (AffixDashboard.IsValid()) AffixDashboard->CreateAffixPoolFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_NewAffixPool", "New Pool"), NSLOCTEXT("YOLOInventory", "Dash_TB_NewAffixPool_Tip", "Create affix pool asset"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (AffixDashboard.IsValid()) AffixDashboard->CreateAffixSourceFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_NewAffixSource", "New Source"), NSLOCTEXT("YOLOInventory", "Dash_TB_NewAffixSource_Tip", "Create affix data source asset"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (AffixDashboard.IsValid()) AffixDashboard->CreateOrUpdateSelectedRowsFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_CreateUpdateAffixRows", "Create/Update"), NSLOCTEXT("YOLOInventory", "Dash_TB_CreateUpdateAffixRows_Tip", "Create/update selected affix rows"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Save"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (AffixDashboard.IsValid()) AffixDashboard->ImportFromSourceFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_ImportAffixRows", "Import"), NSLOCTEXT("YOLOInventory", "Dash_TB_ImportAffixRows_Tip", "Import all rows from current affix source"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Import"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (AffixDashboard.IsValid()) AffixDashboard->UpdateSelectedAffixFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_UpdateAffix", "Update Linked"), NSLOCTEXT("YOLOInventory", "Dash_TB_UpdateAffix_Tip", "Update selected affix from linked source"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Refresh"));
		break;
	case EYIUnifiedDashboardTab::Generators:
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (GeneratorDashboard.IsValid()) GeneratorDashboard->GuidedSetupFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_GuidedGen", "Guided Setup"), NSLOCTEXT("YOLOInventory", "Dash_TB_GuidedGen_Tip", "Populate datasource items and run generator readiness test"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Adjust"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (GeneratorDashboard.IsValid()) GeneratorDashboard->SaveCurrentAssetFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_SaveGenerator", "Save"), NSLOCTEXT("YOLOInventory", "Dash_TB_SaveGenerator_Tip", "Save currently selected generator/dashboard asset"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Save"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (GeneratorDashboard.IsValid()) GeneratorDashboard->CreateLootTableFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_NewLoot", "New Loot"), NSLOCTEXT("YOLOInventory", "Dash_TB_NewLoot_Tip", "Create loot table"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (GeneratorDashboard.IsValid()) GeneratorDashboard->CreateRarityProfileFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_NewRarity", "New Rarity"), NSLOCTEXT("YOLOInventory", "Dash_TB_NewRarity_Tip", "Create rarity profile"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (GeneratorDashboard.IsValid()) GeneratorDashboard->CreateItemGeneratorFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_NewGenerator", "New Generator"), NSLOCTEXT("YOLOInventory", "Dash_TB_NewGenerator_Tip", "Create item generator"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (GeneratorDashboard.IsValid()) GeneratorDashboard->PopulateLootTableFromDataSourcesFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_PopulateLoot", "Populate Loot"), NSLOCTEXT("YOLOInventory", "Dash_TB_PopulateLoot_Tip", "Populate selected loot table from datasource-backed item assets"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Refresh"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (GeneratorDashboard.IsValid()) GeneratorDashboard->SyncAffixPoolsFromDataSourcesFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_SyncAffixPools", "Sync Pools"), NSLOCTEXT("YOLOInventory", "Dash_TB_SyncAffixPools_Tip", "Sync affix pools from affix data sources"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Refresh"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (GeneratorDashboard.IsValid()) GeneratorDashboard->RunGeneratorTestFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_RunGenerator", "Generate"), NSLOCTEXT("YOLOInventory", "Dash_TB_RunGenerator_Tip", "Run generator test"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Play"));
		break;
	case EYIUnifiedDashboardTab::Crafting:
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (CraftingDashboard.IsValid()) CraftingDashboard->AddCraftedItemFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_CraftAdd", "Add Crafted"), NSLOCTEXT("YOLOInventory", "Dash_TB_CraftAdd_Tip", "Add crafted item instance to selected bag"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (CraftingDashboard.IsValid()) CraftingDashboard->SaveTargetBagFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_CraftSaveBag", "Save Bag"), NSLOCTEXT("YOLOInventory", "Dash_TB_CraftSaveBag_Tip", "Save selected crafting target bag"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Save"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]()
			{
				if (CraftingDashboard.IsValid() && BagDashboard.IsValid())
				{
					BagDashboard->SetSelectedBag(CraftingDashboard->GetTargetBag());
				}
				SetActiveTab(EYIUnifiedDashboardTab::Bags);
			})), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_CraftOpenBag", "Open Bag Mode"), NSLOCTEXT("YOLOInventory", "Dash_TB_CraftOpenBag_Tip", "Open selected bag in Bags mode"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.FolderOpen"));
		break;
	case EYIUnifiedDashboardTab::Bags:
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (BagDashboard.IsValid()) BagDashboard->CreateBagFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_BagNew", "New Bag"), NSLOCTEXT("YOLOInventory", "Dash_TB_BagNew_Tip", "Create a new inventory bag asset"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Plus"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]() { if (BagDashboard.IsValid()) BagDashboard->SaveCurrentBagFromToolbar(); })), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_BagSave", "Save Bag"), NSLOCTEXT("YOLOInventory", "Dash_TB_BagSave_Tip", "Save selected bag asset"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Save"));
		ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]()
			{
				if (BagDashboard.IsValid() && CraftingDashboard.IsValid())
				{
					CraftingDashboard->SetTargetBag(BagDashboard->GetSelectedBag());
				}
				SetActiveTab(EYIUnifiedDashboardTab::Crafting);
			})), NAME_None, NSLOCTEXT("YOLOInventory", "Dash_TB_BagToCraft", "Use In Crafting"), NSLOCTEXT("YOLOInventory", "Dash_TB_BagToCraft_Tip", "Use selected bag as crafting target"), FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Adjust"));
		break;
	default:
		break;
	}
	ToolbarBuilder.EndSection();
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
	CloseTab(Tab_Dashboard_ItemBatch);
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
	CloseTab(Tab_Dashboard_AffixSource);
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

void FYIUnifiedDashboardEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FWorkflowCentricApplication::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Items);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Affixes);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Generators);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Crafting);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Bags);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Help);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemDetails);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemMappings);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemPreview);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemPreflight);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemDiff);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemBatch);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemLogs);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_AffixDetails);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_AffixSource);
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
		AffixDashboard = SNew(SYIAffixDashboard).LayoutMode(EYIAffixDashboardLayout::AssetListOnly);
	}
	if (!GeneratorDashboard.IsValid())
	{
		GeneratorDashboard = SNew(SYIGeneratorDashboard).LayoutMode(EYIGeneratorDashboardLayout::AssetListOnly);
	}
	if (!CraftingDashboard.IsValid())
	{
		CraftingDashboard = SNew(SYICraftingDashboard);
	}
	if (!BagDashboard.IsValid())
	{
		BagDashboard = SNew(SYIBagDashboard);
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
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabAffixesLabel", "Affixes"))
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
			GeneratorDashboard.ToSharedRef()
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
			BagDashboard.ToSharedRef()
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			SetActiveTab(EYIUnifiedDashboardTab::Bags);
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

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnItemBatchTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabItemBatchLabel", "Item Batch"))
		[
			ItemDashboard->GetBatchPanelWidget()
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
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabAffixDetailsLabel", "Affix Details"))
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

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnAffixSourceTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabAffixSourceLabel", "Affix Source"))
		[
			AffixDashboard->GetSourcePanelWidget()
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
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabAffixMappingsLabel", "Affix Mappings"))
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
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabAffixPreviewLabel", "Affix Preview"))
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
			GeneratorDashboard->GetDetailsPanelWidget()
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
			GeneratorDashboard->GetTestPanelWidget()
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
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabCraftingDetailsLabel", "Crafting Details"))
		[
			CraftingDashboard.ToSharedRef()
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			SetActiveTab(EYIUnifiedDashboardTab::Crafting);
		}));
	return Tab;
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnBagsDetailsTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabBagsDetailsLabel", "Bag Details"))
		[
			BagDashboard->GetDetailsPanelWidget()
		];
	Tab->SetOnTabActivated(SDockTab::FOnTabActivatedCallback::CreateLambda([this](TSharedRef<SDockTab>, ETabActivationCause)
		{
			SetActiveTab(EYIUnifiedDashboardTab::Bags);
		}));
	return Tab;
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
		EnsureTabOpen(Tab_Dashboard_Items);
		EnsureTabOpen(Tab_Dashboard_ItemDetails);
		EnsureTabOpen(Tab_Dashboard_ItemMappings);
		EnsureTabOpen(Tab_Dashboard_ItemPreview);
		EnsureTabOpen(Tab_Dashboard_ItemPreflight);
		EnsureTabOpen(Tab_Dashboard_ItemDiff);
		EnsureTabOpen(Tab_Dashboard_ItemBatch);
		EnsureTabOpen(Tab_Dashboard_ItemLogs);
		FYOLOInventoryEditorModule::Get().UpdateHelpTabIndex(0);
		break;
	case EYIUnifiedDashboardTab::Affixes:
		CloseItemPanelTabs();
		CloseGeneratorPanelTabs();
		CloseCraftingPanelTabs();
		CloseBagPanelTabs();
		EnsureTabOpen(Tab_Dashboard_Affixes);
		EnsureTabOpen(Tab_Dashboard_AffixDetails);
		EnsureTabOpen(Tab_Dashboard_AffixSource);
		EnsureTabOpen(Tab_Dashboard_AffixMappings);
		EnsureTabOpen(Tab_Dashboard_AffixPreview);
		FYOLOInventoryEditorModule::Get().UpdateHelpTabIndex(1);
		break;
	case EYIUnifiedDashboardTab::Generators:
		CloseItemPanelTabs();
		CloseAffixPanelTabs();
		CloseCraftingPanelTabs();
		CloseBagPanelTabs();
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
		if (BagDashboard.IsValid() && CraftingDashboard.IsValid() && !BagDashboard->GetSelectedBag())
		{
			BagDashboard->SetSelectedBag(CraftingDashboard->GetTargetBag());
		}
		EnsureTabOpen(Tab_Dashboard_Bags);
		EnsureTabOpen(Tab_Dashboard_BagDetails);
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

	// In Bags/Crafting modes, keep focus there for item selection workflows.
	if (ActiveTab == EYIUnifiedDashboardTab::Bags && (Asset->IsA<UYIInventoryBag>() || Asset->IsA<UYIItemDefinition>()))
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

	if (Asset->IsA<UYILootTable>() || Asset->IsA<UYIRarityProfile>() || Asset->IsA<UYIItemGenerator>())
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

}
