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
#include "Styling/AppStyle.h"

static const FName Tab_Dashboard_Items(TEXT("YOLOInventory_Dashboard_Items"));
static const FName Tab_Dashboard_Affixes(TEXT("YOLOInventory_Dashboard_Affixes"));
static const FName Tab_Dashboard_Generators(TEXT("YOLOInventory_Dashboard_Generators"));
static const FName Tab_Dashboard_Help(TEXT("YOLOInventory_Dashboard_Help"));

void FYIUnifiedDashboardEditor::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* AssetToFocus)
{
	CreateWidgetsIfNeeded();

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("YOLOInventory_Dashboard_Layout_v1")
		->AddArea(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.75f)
				->AddTab(Tab_Dashboard_Items, ETabState::OpenedTab)
				->AddTab(Tab_Dashboard_Affixes, ETabState::ClosedTab)
				->AddTab(Tab_Dashboard_Generators, ETabState::ClosedTab)
				->SetForegroundTab(Tab_Dashboard_Items)
			)
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.25f)
				->AddTab(Tab_Dashboard_Help, ETabState::OpenedTab)
			)
		);

	const bool bCreateMenu = true;
	const bool bCreateToolbar = true;
	if (!EditorContext.IsValid())
	{
		EditorContext = TStrongObjectPtr<UYIUnifiedDashboardContext>(NewObject<UYIUnifiedDashboardContext>(GetTransientPackage(), NAME_None, RF_Transactional));
	}
	InitAssetEditor(Mode, InitToolkitHost, GetToolkitFName(), Layout, bCreateMenu, bCreateToolbar, { EditorContext.Get() });

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

	TabManager->TryInvokeTab(Tab_Dashboard_Items);
	TabManager->TryInvokeTab(Tab_Dashboard_Help);

	if (AssetToFocus)
	{
		OpenAsset(AssetToFocus);
	}
}

void FYIUnifiedDashboardEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FWorkflowCentricApplication::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Items);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Affixes);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Generators);
	InTabManager->UnregisterTabSpawner(Tab_Dashboard_Help);
}

void FYIUnifiedDashboardEditor::CreateWidgetsIfNeeded()
{
	if (!ItemDashboard.IsValid())
	{
		ItemDashboard = SNew(SYIItemDashboard);
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
	return SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabItemsLabel", "Items"))
		[
			ItemDashboard.ToSharedRef()
		];
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnAffixesTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	return SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabAffixesLabel", "Affixes"))
		[
			AffixDashboard.ToSharedRef()
		];
}

TSharedRef<SDockTab> FYIUnifiedDashboardEditor::SpawnGeneratorsTab(const FSpawnTabArgs& Args)
{
	CreateWidgetsIfNeeded();
	return SNew(SDockTab)
		.Label(NSLOCTEXT("YOLOInventory", "DashboardTabGeneratorsLabel", "Generators"))
		[
			GeneratorDashboard.ToSharedRef()
		];
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

void FYIUnifiedDashboardEditor::SetActiveTab(EYIUnifiedDashboardTab NewTab)
{
	ActiveTab = NewTab;
	switch (ActiveTab)
	{
	case EYIUnifiedDashboardTab::Items:
		if (TabManager.IsValid()) { TabManager->TryInvokeTab(Tab_Dashboard_Items); }
		FYOLOInventoryEditorModule::Get().UpdateHelpTabIndex(0);
		break;
	case EYIUnifiedDashboardTab::Affixes:
		if (TabManager.IsValid()) { TabManager->TryInvokeTab(Tab_Dashboard_Affixes); }
		FYOLOInventoryEditorModule::Get().UpdateHelpTabIndex(1);
		break;
	case EYIUnifiedDashboardTab::Generators:
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
