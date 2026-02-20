#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IYOLOInventoryEditorCoreModule.h"

class FYOLOInventoryEditorModule : public IYOLOInventoryEditorCoreModule
{
public:
	static FYOLOInventoryEditorModule& Get();

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void OpenDashboardForAsset(UObject* Asset);
	void OpenDashboard();
	void OpenDashboardHelp();
	void RegisterHelpWidget(const TSharedPtr<class SYIUnifiedHelpPanel>& Widget);
	void UpdateHelpTabIndex(int32 Index);

	virtual void RegisterBagDashboardFactory(FYICreateBagDashboardBridge InFactory) override;
	virtual void ClearBagDashboardFactory() override;
	virtual bool HasBagDashboardFactory() const override;
	virtual TSharedRef<IYIBagDashboardBridge> CreateBagDashboardBridge() override;

	virtual void RegisterGeneratorDashboardFactory(FYICreateGeneratorDashboardBridge InFactory) override;
	virtual void ClearGeneratorDashboardFactory() override;
	virtual bool HasGeneratorDashboardFactory() const override;
	virtual TSharedRef<IYIGeneratorDashboardBridge> CreateGeneratorDashboardBridge() override;

private:
	TArray<TWeakPtr<class SYIUnifiedHelpPanel>> HelpWidgets;
	int32 LastHelpTabIndex = 0;
	TWeakPtr<class FYIUnifiedDashboardEditor> DashboardEditor;
	FYICreateBagDashboardBridge BagDashboardFactory;
	FYICreateGeneratorDashboardBridge GeneratorDashboardFactory;
};
