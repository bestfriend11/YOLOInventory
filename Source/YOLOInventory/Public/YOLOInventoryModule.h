#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogYOLOInventory, Log, All);

class FAutoConsoleCommandWithWorldAndArgs;
class UWorld;

class FYOLOInventoryModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void HandleDebugConsoleCommand(const TArray<FString>& Args, UWorld* World);

	TUniquePtr<FAutoConsoleCommandWithWorldAndArgs> DebugConsoleCommand;
};
