#include "YOLOInventoryModule.h"

#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "YOLOInventorySettings.h"

DEFINE_LOG_CATEGORY(LogYOLOInventory);

IMPLEMENT_MODULE(FYOLOInventoryModule, YOLOInventory)

void FYOLOInventoryModule::StartupModule()
{
#if WITH_AUTOMATION_TESTS
	// Ensure the automation tests module is loaded so headless runs discover YOLOInventory.* specs
	FModuleManager::Get().LoadModulePtr<IModuleInterface>("YOLOInventoryTests");
#endif

	DebugConsoleCommand = MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("YOLOInventory.Debug"),
		TEXT("Enable or disable YOLOInventory debug overlay output. Usage: YOLOInventory.Debug [0|1|toggle]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateRaw(this, &FYOLOInventoryModule::HandleDebugConsoleCommand));
}

void FYOLOInventoryModule::ShutdownModule()
{
	DebugConsoleCommand.Reset();
}

void FYOLOInventoryModule::HandleDebugConsoleCommand(const TArray<FString>& Args, UWorld* /*World*/)
{
	bool bEnable = true;
	if (Args.Num() > 0)
	{
		const FString& Arg = Args[0];
		if (Arg.Equals(TEXT("0")) || Arg.Equals(TEXT("false"), ESearchCase::IgnoreCase) || Arg.Equals(TEXT("off"), ESearchCase::IgnoreCase))
		{
			bEnable = false;
		}
		else if (Arg.Equals(TEXT("toggle"), ESearchCase::IgnoreCase))
		{
			bEnable = !UYOLOInventorySettings::Get().bShowDebug;
		}
	}

	UYOLOInventorySettings& Settings = UYOLOInventorySettings::GetMutable();
	Settings.bShowDebug = bEnable;
	Settings.SaveConfig();

	UE_LOG(LogYOLOInventory, Display, TEXT("YOLOInventory debug %s"), bEnable ? TEXT("ENABLED") : TEXT("DISABLED"));
}
