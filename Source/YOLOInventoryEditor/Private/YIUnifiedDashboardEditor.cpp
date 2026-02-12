#include "YIUnifiedDashboardEditor.h"
#include "SYIItemDashboard.h"
#include "SYIAffixDashboard.h"
#include "SYIGeneratorDashboard.h"
#include "SYIUnifiedDashboard.h"
#include "YIInventoryEditorModule.h"
#include "YIUnifiedDashboardContext.h"
#include "YIItemDefinition.h"
#include "Data/YIDataTableItemSource.h"
#include "Engine/DataTable.h"
#include "YIAffixAsset.h"
#include "YIAffixPoolAsset.h"
#include "YILootTable.h"
#include "YIRarityProfile.h"
#include "YIItemGenerator.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Styling/AppStyle.h"

static const FName Tab_Dashboard_Items(TEXT("YOLOInventory_Dashboard_Items"));
static const FName Tab_Dashboard_Affixes(TEXT("YOLOInventory_Dashboard_Affixes"));
static const FName Tab_Dashboard_Generators(TEXT("YOLOInventory_Dashboard_Generators"));
static const FName Tab_Dashboard_Help(TEXT("YOLOInventory_Dashboard_Help"));
static const FName Tab_Dashboard_ItemDetails(TEXT("YOLOInventory_Dashboard_ItemDetails"));
static const FName Tab_Dashboard_ItemMappings(TEXT("YOLOInventory_Dashboard_ItemMappings"));
static const FName Tab_Dashboard_ItemPreview(TEXT("YOLOInventory_Dashboard_ItemPreview"));
static const FName Tab_Dashboard_ItemPreflight(TEXT("YOLOInventory_Dashboard_ItemPreflight"));
static const FName Tab_Dashboard_ItemDiff(TEXT("YOLOInventory_Dashboard_ItemDiff"));
static const FName Tab_Dashboard_ItemBatch(TEXT("YOLOInventory_Dashboard_ItemBatch"));
static const FName Tab_Dashboard_ItemLogs(TEXT("YOLOInventory_Dashboard_ItemLogs"));

void FYIUnifiedDashboardEditor::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* AssetToFocus)
{
	CreateWidgetsIfNeeded();

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("YOLOInventory_Dashboard_Layout_v2")
		->AddArea(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.40f)
				->AddTab(Tab_Dashboard_Items, ETabState::OpenedTab)
				->AddTab(Tab_Dashboard_Affixes, ETabState::ClosedTab)
				->AddTab(Tab_Dashboard_Generators, ETabState::ClosedTab)
				->SetForegroundTab(Tab_Dashboard_Items)
			)
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.35f)
				->AddTab(Tab_Dashboard_ItemDetails, ETabState::OpenedTab)
				->AddTab(Tab_Dashboard_ItemMappings, ETabState::ClosedTab)
				->AddTab(Tab_Dashboard_ItemPreview, ETabState::ClosedTab)
				->SetForegroundTab(Tab_Dashboard_ItemDetails)
			)
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.25f)
				->AddTab(Tab_Dashboard_ItemPreflight, ETabState::ClosedTab)
				->AddTab(Tab_Dashboard_ItemDiff, ETabState::ClosedTab)
				->AddTab(Tab_Dashboard_ItemBatch, ETabState::ClosedTab)
				->AddTab(Tab_Dashboard_ItemLogs, ETabState::ClosedTab)
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

	TabManager->RegisterTabSpawner(Tab_Dashboard_Items, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnItemsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabItems", "Items"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
	TabManager->RegisterTabSpawner(Tab_Dashboard_Affixes, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnAffixesTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabAffixes", "Affixes"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
	TabManager->RegisterTabSpawner(Tab_Dashboard_Generators, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnGeneratorsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabGenerators", "Generators"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
	TabManager->RegisterTabSpawner(Tab_Dashboard_Help, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnHelpTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabHelp", "Help"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
	TabManager->RegisterTabSpawner(Tab_Dashboard_ItemDetails, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnItemDetailsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabItemDetails", "Item Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
	TabManager->RegisterTabSpawner(Tab_Dashboard_ItemMappings, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnItemMappingsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabItemMappings", "Item Mappings"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
	TabManager->RegisterTabSpawner(Tab_Dashboard_ItemPreview, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnItemPreviewTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabItemPreview", "Item Preview"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
	TabManager->RegisterTabSpawner(Tab_Dashboard_ItemPreflight, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnItemPreflightTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabItemPreflight", "Item Preflight"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
	TabManager->RegisterTabSpawner(Tab_Dashboard_ItemDiff, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnItemDiffTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabItemDiff", "Item Diff"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
	TabManager->RegisterTabSpawner(Tab_Dashboard_ItemBatch, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnItemBatchTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabItemBatch", "Item Batch"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
	TabManager->RegisterTabSpawner(Tab_Dashboard_ItemLogs, FOnSpawnTab::CreateSP(this, &FYIUnifiedDashboardEditor::SpawnItemLogsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory", "DashboardTabItemLogs", "Item Logs"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	TabManager->TryInvokeTab(Tab_Dashboard_Items);
	TabManager->TryInvokeTab(Tab_Dashboard_Help);
	TabManager->TryInvokeTab(Tab_Dashboard_ItemDetails);

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

	ToolbarBuilder.BeginSection("YOLOInventoryPanels");
	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemDetails); }),
			FCanExecuteAction::CreateLambda([this]() { return ActiveTab == EYIUnifiedDashboardTab::Items; })),
		NAME_None,
		NSLOCTEXT("YOLOInventory", "Dash_TB_Details", "Details"),
		NSLOCTEXT("YOLOInventory", "Dash_TB_Details_Tip", "Open Item Details panel"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.Details"));
	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemMappings); }),
			FCanExecuteAction::CreateLambda([this]() { return ActiveTab == EYIUnifiedDashboardTab::Items; })),
		NAME_None,
		NSLOCTEXT("YOLOInventory", "Dash_TB_Mappings", "Mappings"),
		NSLOCTEXT("YOLOInventory", "Dash_TB_Mappings_Tip", "Open Item Mappings panel"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Link"));
	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemPreview); }),
			FCanExecuteAction::CreateLambda([this]() { return ActiveTab == EYIUnifiedDashboardTab::Items; })),
		NAME_None,
		NSLOCTEXT("YOLOInventory", "Dash_TB_Preview", "Preview"),
		NSLOCTEXT("YOLOInventory", "Dash_TB_Preview_Tip", "Open Item Preview panel"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Visibility"));
	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemPreflight); }),
			FCanExecuteAction::CreateLambda([this]() { return ActiveTab == EYIUnifiedDashboardTab::Items; })),
		NAME_None,
		NSLOCTEXT("YOLOInventory", "Dash_TB_Preflight", "Preflight"),
		NSLOCTEXT("YOLOInventory", "Dash_TB_Preflight_Tip", "Open Item Preflight panel"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Error"));
	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemDiff); }),
			FCanExecuteAction::CreateLambda([this]() { return ActiveTab == EYIUnifiedDashboardTab::Items; })),
		NAME_None,
		NSLOCTEXT("YOLOInventory", "Dash_TB_Diff", "Diff"),
		NSLOCTEXT("YOLOInventory", "Dash_TB_Diff_Tip", "Open Item Diff panel"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Diff"));
	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemBatch); }),
			FCanExecuteAction::CreateLambda([this]() { return ActiveTab == EYIUnifiedDashboardTab::Items; })),
		NAME_None,
		NSLOCTEXT("YOLOInventory", "Dash_TB_Batch", "Batch"),
		NSLOCTEXT("YOLOInventory", "Dash_TB_Batch_Tip", "Open Batch Queue panel"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.ListView"));
	ToolbarBuilder.AddToolBarButton(
		FUIAction(
			FExecuteAction::CreateLambda([OpenTab]() { OpenTab(Tab_Dashboard_ItemLogs); }),
			FCanExecuteAction::CreateLambda([this]() { return ActiveTab == EYIUnifiedDashboardTab::Items; })),
		NAME_None,
		NSLOCTEXT("YOLOInventory", "Dash_TB_Logs", "Logs"),
		NSLOCTEXT("YOLOInventory", "Dash_TB_Logs_Tip", "Open Errors and Notifications panel"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Warning"));
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

void FYIUnifiedDashboardEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FWorkflowCentricApplication::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Items);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Affixes);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Generators);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Help);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemDetails);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemMappings);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemPreview);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemPreflight);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemDiff);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemBatch);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_ItemLogs);
}

void FYIUnifiedDashboardEditor::CreateWidgetsIfNeeded()
{
	if (!ItemDashboard.IsValid())
	{
		ItemDashboard = SNew(SYIItemDashboard).LayoutMode(EYIItemDashboardLayout::ItemListOnly);
	}
	if (!AffixDashboard.IsValid())
	{
		AffixDashboard = SNew(SYIAffixDashboard);
	}
	if (!GeneratorDashboard.IsValid())
	{
		GeneratorDashboard = SNew(SYIGeneratorDashboard);
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
			SetActiveTab(EYIUnifiedDashboardTab::Items);
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
			SetActiveTab(EYIUnifiedDashboardTab::Items);
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
			SetActiveTab(EYIUnifiedDashboardTab::Items);
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
			SetActiveTab(EYIUnifiedDashboardTab::Items);
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
			SetActiveTab(EYIUnifiedDashboardTab::Items);
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
			SetActiveTab(EYIUnifiedDashboardTab::Items);
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
			SetActiveTab(EYIUnifiedDashboardTab::Items);
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
			SetActiveTab(EYIUnifiedDashboardTab::Items);
		}));
	return Tab;
}

void FYIUnifiedDashboardEditor::SetActiveTab(EYIUnifiedDashboardTab NewTab)
{
	ActiveTab = NewTab;
	switch (ActiveTab)
	{
	case EYIUnifiedDashboardTab::Items:
		if (TabManager.IsValid()) { TabManager->TryInvokeTab(Tab_Dashboard_Items); }
		if (TabManager.IsValid()) { TabManager->TryInvokeTab(Tab_Dashboard_ItemDetails); }
		FYOLOInventoryEditorModule::Get().UpdateHelpTabIndex(0);
		break;
	case EYIUnifiedDashboardTab::Affixes:
		CloseItemPanelTabs();
		if (TabManager.IsValid()) { TabManager->TryInvokeTab(Tab_Dashboard_Affixes); }
		FYOLOInventoryEditorModule::Get().UpdateHelpTabIndex(1);
		break;
	case EYIUnifiedDashboardTab::Generators:
		CloseItemPanelTabs();
		if (TabManager.IsValid()) { TabManager->TryInvokeTab(Tab_Dashboard_Generators); }
		FYOLOInventoryEditorModule::Get().UpdateHelpTabIndex(2);
		break;
	default:
		break;
	}
}

void FYIUnifiedDashboardEditor::OpenHelpTab()
{
	if (TabManager.IsValid())
	{
		TabManager->TryInvokeTab(Tab_Dashboard_Help);
	}
}

void FYIUnifiedDashboardEditor::OpenAsset(UObject* Asset)
{
	if (!Asset)
	{
		SetActiveTab(EYIUnifiedDashboardTab::Items);
		return;
	}

	CreateWidgetsIfNeeded();

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
