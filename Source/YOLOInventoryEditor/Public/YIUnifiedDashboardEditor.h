#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"
#include "SYIUnifiedDashboard.h"

class SYIItemDashboard;
class SYIAffixDashboard;
class SYIGeneratorDashboard;
class SYIUnifiedHelpPanel;
class FWorkspaceItem;
class FToolBarBuilder;
class UYIUnifiedDashboardContext;

class YOLOINVENTORYEDITOR_API FYIUnifiedDashboardEditor : public FWorkflowCentricApplication
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

protected:
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;

private:
	void CreateWidgetsIfNeeded();
	void ExtendToolbar();
	void FillDashboardToolbar(FToolBarBuilder& ToolbarBuilder);

	TSharedRef<SDockTab> SpawnItemsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnAffixesTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnGeneratorsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnHelpTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnItemDetailsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnItemMappingsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnItemPreviewTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnItemPreflightTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnItemDiffTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnItemBatchTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnItemLogsTab(const FSpawnTabArgs& Args);

	TSharedPtr<SYIItemDashboard> ItemDashboard;
	TSharedPtr<SYIAffixDashboard> AffixDashboard;
	TSharedPtr<SYIGeneratorDashboard> GeneratorDashboard;
	TSharedPtr<SYIUnifiedHelpPanel> HelpPanel;
	TStrongObjectPtr<UYIUnifiedDashboardContext> EditorContext;
	TSharedPtr<FExtender> ToolbarExtender;

	EYIUnifiedDashboardTab ActiveTab = EYIUnifiedDashboardTab::Items;
	TSharedPtr<FWorkspaceItem> WorkspaceMenuCategory;
};
