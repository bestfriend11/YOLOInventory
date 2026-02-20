#pragma once

#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "YIBagDashboardBridge.h"
#include "YIGeneratorDashboardBridge.h"

class YOLOINVENTORYEDITORCORE_API IYOLOInventoryEditorCoreModule : public IModuleInterface
{
public:
	static inline IYOLOInventoryEditorCoreModule& Get()
	{
		return FModuleManager::LoadModuleChecked<IYOLOInventoryEditorCoreModule>("YOLOInventoryEditorCore");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("YOLOInventoryEditorCore");
	}

	virtual void RegisterBagDashboardFactory(FYICreateBagDashboardBridge InFactory) = 0;
	virtual void ClearBagDashboardFactory() = 0;
	virtual bool HasBagDashboardFactory() const = 0;
	virtual TSharedRef<IYIBagDashboardBridge> CreateBagDashboardBridge() = 0;

	virtual void RegisterGeneratorDashboardFactory(FYICreateGeneratorDashboardBridge InFactory) = 0;
	virtual void ClearGeneratorDashboardFactory() = 0;
	virtual bool HasGeneratorDashboardFactory() const = 0;
	virtual TSharedRef<IYIGeneratorDashboardBridge> CreateGeneratorDashboardBridge() = 0;
};
