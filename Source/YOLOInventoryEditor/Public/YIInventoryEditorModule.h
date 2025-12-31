#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

// Global asset category id for YOLO Inventory assets, assigned during module startup
extern uint32 GYOLOInventoryAssetCategory;

// Helper for factories to fetch YOLO Inventory asset category
static inline uint32 GetYoLoAssetCategoryBit() { return GYOLOInventoryAssetCategory; }

class FYOLOInventoryEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterAssetTypeActions();
	void UnregisterAssetTypeActions();

	TArray<TSharedPtr<class FAssetTypeActions_Base>> RegisteredAssetTypeActions;
};
