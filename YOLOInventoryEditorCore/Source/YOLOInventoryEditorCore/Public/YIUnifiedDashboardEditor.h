#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"
#include "SYIUnifiedDashboard.h"
#include "YIUnifiedDashboardContext.h"
#include "YIGeneratorDashboardBridge.h"

class SYIItemDashboard;
class SYIFragmentDashboard;
class SYICraftingDashboard;
class SYIUnifiedHelpPanel;
class IYIBagDashboardBridge;
class FWorkspaceItem;
class FToolBarBuilder;

class YOLOINVENTORYEDITORCORE_API FYIUnifiedDashboardEditor : public FWorkflowCentricApplication
{
public:
	void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UObject* AssetToFocus = nullptr);

	void OpenAsset(UObject* Asset);
	void OpenHelpTab();
	void SetActiveTab(EYIUnifiedDashboardTab NewTab);

	// FAssetEditorToolkit
	virtual FName GetToolkitFName() const override { return FName("YOLOInventoryDashboard"); }
	virtual FText GetBaseToolkitName() const override { return NSLOCTEXT("YOLOInventory", "DashboardToolkit", "YOLO Inventory Dashboard"); }
	virtual FString GetWorldCentricTabPrefix() const override { return TEXT("YOLOInventory"); }
	virtual FLinearColor GetWorldCentricTabColorScale() const override { return FLinearColor(0.15f, 0.55f, 0.95f, 1.f); }
	virtual void SaveAsset_Execute() override;

protected:
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

private:
	void CreateWidgetsIfNeeded();
	void ExtendToolbar();
	void FillDashboardToolbar(FToolBarBuilder& ToolbarBuilder);
	void CloseItemPanelTabs();
	void CloseAffixPanelTabs();
	void CloseGeneratorPanelTabs();
	void CloseCraftingPanelTabs();
	void CloseBagPanelTabs();
	void CloseEquipmentModeTabs();

	TSharedRef<SDockTab> SpawnItemsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnAffixesTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnGeneratorsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnCraftingTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnBagsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnEquipmentTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnHelpTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnItemDetailsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnItemMappingsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnItemPreviewTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnItemPreflightTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnItemDiffTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnItemLogsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnAffixDetailsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnAffixMappingsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnAffixPreviewTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnGeneratorDetailsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnGeneratorTestTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnCraftingDetailsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnBagsDetailsTab(const FSpawnTabArgs& Args);

	TSharedPtr<SYIItemDashboard> ItemDashboard;
	TSharedPtr<SYIFragmentDashboard> AffixDashboard;
	TSharedPtr<IYIGeneratorDashboardBridge> GeneratorDashboard;
	TSharedPtr<SYICraftingDashboard> CraftingDashboard;
	TSharedPtr<IYIBagDashboardBridge> BagDashboard;
	TSharedPtr<SYIUnifiedHelpPanel> HelpPanel;
	TStrongObjectPtr<UYIUnifiedDashboardContext> EditorContext;
	TSharedPtr<FExtender> ToolbarExtender;

	EYIUnifiedDashboardTab ActiveTab = EYIUnifiedDashboardTab::Items;
	TSharedPtr<FWorkspaceItem> WorkspaceMenuCategory;
};
