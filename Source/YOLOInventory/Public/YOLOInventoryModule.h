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
	void HandleDebugScreenConsoleCommand(const TArray<FString>& Args, UWorld* World);
	void HandleDebugLogConsoleCommand(const TArray<FString>& Args, UWorld* World);
	void HandleDebugForceConsoleCommand(const TArray<FString>& Args, UWorld* World);
	void HandleDebugPipelineConsoleCommand(const TArray<FString>& Args, UWorld* World);
	void HandleDebugChannelConsoleCommand(const TArray<FString>& Args, UWorld* World);
	void HandleDebugHistoryConsoleCommand(const TArray<FString>& Args, UWorld* World);
	void HandleDebugStatusConsoleCommand(const TArray<FString>& Args, UWorld* World);
	void HandleDebugProfileConsoleCommand(const TArray<FString>& Args, UWorld* World);

	TUniquePtr<FAutoConsoleCommandWithWorldAndArgs> DebugConsoleCommand;
	TUniquePtr<FAutoConsoleCommandWithWorldAndArgs> DebugScreenConsoleCommand;
	TUniquePtr<FAutoConsoleCommandWithWorldAndArgs> DebugLogConsoleCommand;
	TUniquePtr<FAutoConsoleCommandWithWorldAndArgs> DebugForceConsoleCommand;
	TUniquePtr<FAutoConsoleCommandWithWorldAndArgs> DebugPipelineConsoleCommand;
	TUniquePtr<FAutoConsoleCommandWithWorldAndArgs> DebugChannelConsoleCommand;
	TUniquePtr<FAutoConsoleCommandWithWorldAndArgs> DebugHistoryConsoleCommand;
	TUniquePtr<FAutoConsoleCommandWithWorldAndArgs> DebugStatusConsoleCommand;
	TUniquePtr<FAutoConsoleCommandWithWorldAndArgs> DebugProfileConsoleCommand;
};
