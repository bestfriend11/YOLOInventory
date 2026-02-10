#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

// Global asset category id for YOLO Inventory assets, assigned during module startup
extern uint32 GYOLOInventoryAssetCategory;

// Helper for factories to fetch YOLO Inventory asset category
static inline uint32 GetYoLoAssetCategoryBit() { return GYOLOInventoryAssetCategory; }

class FYOLOInventoryEditorModule : public IModuleInterface
{
public:
	static FYOLOInventoryEditorModule& Get();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void OpenDashboardForAsset(UObject* Asset);
	void RegisterDashboardWidget(const TSharedPtr<class SYIUnifiedDashboard>& Widget);
	void RegisterHelpWidget(const TSharedPtr<class SYIUnifiedHelpPanel>& Widget);
	void UpdateHelpTabIndex(int32 Index);

private:
	void RegisterAssetTypeActions();
	void UnregisterAssetTypeActions();

	TArray<TSharedPtr<class FAssetTypeActions_Base>> RegisteredAssetTypeActions;
	TWeakPtr<class SYIUnifiedDashboard> DashboardWidget;
	TArray<TWeakPtr<class SYIUnifiedHelpPanel>> HelpWidgets;
	int32 LastHelpTabIndex = 0;
	TWeakObjectPtr<UObject> PendingAsset;
};
