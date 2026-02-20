#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

class FYOLOInventoryEditorModule : public IModuleInterface
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

private:
	TArray<TWeakPtr<class SYIUnifiedHelpPanel>> HelpWidgets;
	int32 LastHelpTabIndex = 0;
	TWeakPtr<class FYIUnifiedDashboardEditor> DashboardEditor;
};
