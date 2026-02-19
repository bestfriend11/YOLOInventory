#include "YOLOInventoryModule.h"

#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "YOLOInventorySettings.h"
#include "Engine/World.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
#include "YIDebugLibrary.h"

DEFINE_LOG_CATEGORY(LogYOLOInventory);

IMPLEMENT_MODULE(FYOLOInventoryModule, YOLOInventory)

namespace YOLOInventoryModulePrivate
{
	static bool ParseToggleArg(const FString& Arg, bool CurrentValue, bool& OutValue)
	{
		if (Arg.Equals(TEXT("toggle"), ESearchCase::IgnoreCase))
		{
			OutValue = !CurrentValue;
			return true;
		}
		if (Arg.Equals(TEXT("1")) || Arg.Equals(TEXT("true"), ESearchCase::IgnoreCase) || Arg.Equals(TEXT("on"), ESearchCase::IgnoreCase))
		{
			OutValue = true;
			return true;
		}
		if (Arg.Equals(TEXT("0")) || Arg.Equals(TEXT("false"), ESearchCase::IgnoreCase) || Arg.Equals(TEXT("off"), ESearchCase::IgnoreCase))
		{
			OutValue = false;
			return true;
		}
		return false;
	}

	static bool ResolveChannelByName(const FString& Name, bool*& OutChannelFlag, UYOLOInventorySettings& Settings)
	{
		const FString N = Name.ToLower();
		if (N == TEXT("all"))
		{
			OutChannelFlag = nullptr;
			return true;
		}
		if (N == TEXT("general")) { OutChannelFlag = &Settings.bDebugChannelGeneral; return true; }
		if (N == TEXT("persistence")) { OutChannelFlag = &Settings.bDebugChannelPersistence; return true; }
		if (N == TEXT("inventory")) { OutChannelFlag = &Settings.bDebugChannelInventory; return true; }
		if (N == TEXT("equipment")) { OutChannelFlag = &Settings.bDebugChannelEquipment; return true; }
		if (N == TEXT("actionbar") || N == TEXT("action_bar")) { OutChannelFlag = &Settings.bDebugChannelActionBar; return true; }
		if (N == TEXT("trade")) { OutChannelFlag = &Settings.bDebugChannelTrade; return true; }
		if (N == TEXT("shop")) { OutChannelFlag = &Settings.bDebugChannelShop; return true; }
		if (N == TEXT("grid")) { OutChannelFlag = &Settings.bDebugChannelGrid; return true; }
		if (N == TEXT("phase2")) { OutChannelFlag = &Settings.bDebugChannelPhase2; return true; }
		return false;
	}
}

void FYOLOInventoryModule::StartupModule()
{
	if (const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("YOLOInventory")))
	{
		const FString ShaderDirectory = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders"));
		AddShaderSourceDirectoryMapping(TEXT("/Plugin/YOLOInventory"), ShaderDirectory);
		bShaderDirectoryMapped = true;
	}

#if WITH_AUTOMATION_TESTS
	// Ensure the automation tests module is loaded so headless runs discover YOLOInventory.* specs
	FModuleManager::Get().LoadModulePtr<IModuleInterface>("YOLOInventoryTests");
#endif

	DebugConsoleCommand = MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("YOLOInventory.Debug"),
		TEXT("Enable or disable legacy YOLOInventory grid debug overlay output. Usage: YOLOInventory.Debug [0|1|toggle]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateRaw(this, &FYOLOInventoryModule::HandleDebugConsoleCommand));

	DebugPipelineConsoleCommand = MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("YOLOInventory.Debug.Pipeline"),
		TEXT("Enable/disable global YOLOInventory debug pipeline. Usage: YOLOInventory.Debug.Pipeline [0|1|toggle]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateRaw(this, &FYOLOInventoryModule::HandleDebugPipelineConsoleCommand));

	DebugScreenConsoleCommand = MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("YOLOInventory.Debug.Screen"),
		TEXT("Enable/disable on-screen debug output. Usage: YOLOInventory.Debug.Screen [0|1|toggle]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateRaw(this, &FYOLOInventoryModule::HandleDebugScreenConsoleCommand));

	DebugLogConsoleCommand = MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("YOLOInventory.Debug.Log"),
		TEXT("Enable/disable log debug output routing. Usage: YOLOInventory.Debug.Log [0|1|toggle]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateRaw(this, &FYOLOInventoryModule::HandleDebugLogConsoleCommand));

	DebugForceConsoleCommand = MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("YOLOInventory.Debug.Force"),
		TEXT("Enable/disable forced debug messages (critical/pinned). Usage: YOLOInventory.Debug.Force [0|1|toggle]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateRaw(this, &FYOLOInventoryModule::HandleDebugForceConsoleCommand));

	DebugChannelConsoleCommand = MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("YOLOInventory.Debug.Channel"),
		TEXT("Enable/disable a debug channel. Usage: YOLOInventory.Debug.Channel <all|general|persistence|inventory|equipment|actionbar|trade|shop|grid|phase2> [0|1|toggle]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateRaw(this, &FYOLOInventoryModule::HandleDebugChannelConsoleCommand));

	DebugHistoryConsoleCommand = MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("YOLOInventory.Debug.History"),
		TEXT("Manage debug history. Usage: YOLOInventory.Debug.History [clear]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateRaw(this, &FYOLOInventoryModule::HandleDebugHistoryConsoleCommand));

	DebugStatusConsoleCommand = MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("YOLOInventory.Debug.Status"),
		TEXT("Print current YOLOInventory debug pipeline state."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateRaw(this, &FYOLOInventoryModule::HandleDebugStatusConsoleCommand));

	DebugProfileConsoleCommand = MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("YOLOInventory.Debug.Profile"),
		TEXT("Apply debug profile. Usage: YOLOInventory.Debug.Profile <off|minimal|normal|verbose>"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateRaw(this, &FYOLOInventoryModule::HandleDebugProfileConsoleCommand));

}

void FYOLOInventoryModule::ShutdownModule()
{
	bShaderDirectoryMapped = false;

	DebugConsoleCommand.Reset();
	DebugPipelineConsoleCommand.Reset();
	DebugScreenConsoleCommand.Reset();
	DebugLogConsoleCommand.Reset();
	DebugForceConsoleCommand.Reset();
	DebugChannelConsoleCommand.Reset();
	DebugHistoryConsoleCommand.Reset();
	DebugStatusConsoleCommand.Reset();
	DebugProfileConsoleCommand.Reset();
}

void FYOLOInventoryModule::HandleDebugConsoleCommand(const TArray<FString>& Args, UWorld* /*World*/)
{
	bool bEnable = true;
	if (Args.Num() > 0)
	{
		YOLOInventoryModulePrivate::ParseToggleArg(Args[0], UYOLOInventorySettings::Get().bShowDebug, bEnable);
	}

	UYOLOInventorySettings& Settings = UYOLOInventorySettings::GetMutable();
	Settings.bShowDebug = bEnable;
	Settings.SaveConfig();

	UE_LOG(LogYOLOInventory, Display, TEXT("YOLOInventory debug %s"), bEnable ? TEXT("ENABLED") : TEXT("DISABLED"));
}

void FYOLOInventoryModule::HandleDebugPipelineConsoleCommand(const TArray<FString>& Args, UWorld* /*World*/)
{
	UYOLOInventorySettings& Settings = UYOLOInventorySettings::GetMutable();
	bool bEnable = Settings.bEnableDebugPipeline;
	if (Args.Num() > 0)
	{
		YOLOInventoryModulePrivate::ParseToggleArg(Args[0], Settings.bEnableDebugPipeline, bEnable);
	}
	Settings.bEnableDebugPipeline = bEnable;
	Settings.SaveConfig();
	UE_LOG(LogYOLOInventory, Display, TEXT("YOLOInventory debug pipeline %s"), bEnable ? TEXT("ENABLED") : TEXT("DISABLED"));
}

void FYOLOInventoryModule::HandleDebugScreenConsoleCommand(const TArray<FString>& Args, UWorld* /*World*/)
{
	UYOLOInventorySettings& Settings = UYOLOInventorySettings::GetMutable();
	bool bEnable = Settings.bDebugOutputToScreen;
	if (Args.Num() > 0)
	{
		YOLOInventoryModulePrivate::ParseToggleArg(Args[0], Settings.bDebugOutputToScreen, bEnable);
	}
	Settings.bDebugOutputToScreen = bEnable;
	Settings.SaveConfig();
	UE_LOG(LogYOLOInventory, Display, TEXT("YOLOInventory debug screen output %s"), bEnable ? TEXT("ENABLED") : TEXT("DISABLED"));
}

void FYOLOInventoryModule::HandleDebugLogConsoleCommand(const TArray<FString>& Args, UWorld* /*World*/)
{
	UYOLOInventorySettings& Settings = UYOLOInventorySettings::GetMutable();
	bool bEnable = Settings.bDebugOutputToLog;
	if (Args.Num() > 0)
	{
		YOLOInventoryModulePrivate::ParseToggleArg(Args[0], Settings.bDebugOutputToLog, bEnable);
	}
	Settings.bDebugOutputToLog = bEnable;
	Settings.SaveConfig();
	UE_LOG(LogYOLOInventory, Display, TEXT("YOLOInventory debug log output %s"), bEnable ? TEXT("ENABLED") : TEXT("DISABLED"));
}

void FYOLOInventoryModule::HandleDebugForceConsoleCommand(const TArray<FString>& Args, UWorld* /*World*/)
{
	UYOLOInventorySettings& Settings = UYOLOInventorySettings::GetMutable();
	bool bEnable = Settings.bAllowForcedDebugMessages;
	if (Args.Num() > 0)
	{
		YOLOInventoryModulePrivate::ParseToggleArg(Args[0], Settings.bAllowForcedDebugMessages, bEnable);
	}
	Settings.bAllowForcedDebugMessages = bEnable;
	Settings.SaveConfig();
	UE_LOG(LogYOLOInventory, Display, TEXT("YOLOInventory forced debug messages %s"), bEnable ? TEXT("ENABLED") : TEXT("DISABLED"));
}

void FYOLOInventoryModule::HandleDebugChannelConsoleCommand(const TArray<FString>& Args, UWorld* /*World*/)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogYOLOInventory, Display, TEXT("Usage: YOLOInventory.Debug.Channel <all|general|persistence|inventory|equipment|actionbar|trade|shop|grid|phase2> [0|1|toggle]"));
		return;
	}

	UYOLOInventorySettings& Settings = UYOLOInventorySettings::GetMutable();
	bool* ChannelFlag = nullptr;
	if (!YOLOInventoryModulePrivate::ResolveChannelByName(Args[0], ChannelFlag, Settings))
	{
		UE_LOG(LogYOLOInventory, Warning, TEXT("Unknown debug channel '%s'."), *Args[0]);
		return;
	}

	const bool bAffectsAll = ChannelFlag == nullptr;
	bool bNewValue = true;
	if (Args.Num() > 1)
	{
		const bool bCurrent = bAffectsAll ? Settings.bDebugChannelGeneral : *ChannelFlag;
		if (!YOLOInventoryModulePrivate::ParseToggleArg(Args[1], bCurrent, bNewValue))
		{
			UE_LOG(LogYOLOInventory, Warning, TEXT("Invalid channel toggle value '%s'."), *Args[1]);
			return;
		}
	}
	else
	{
		const bool bCurrent = bAffectsAll ? Settings.bDebugChannelGeneral : *ChannelFlag;
		bNewValue = !bCurrent;
	}

	if (bAffectsAll)
	{
		Settings.bDebugChannelGeneral = bNewValue;
		Settings.bDebugChannelPersistence = bNewValue;
		Settings.bDebugChannelInventory = bNewValue;
		Settings.bDebugChannelEquipment = bNewValue;
		Settings.bDebugChannelActionBar = bNewValue;
		Settings.bDebugChannelTrade = bNewValue;
		Settings.bDebugChannelShop = bNewValue;
		Settings.bDebugChannelGrid = bNewValue;
		Settings.bDebugChannelPhase2 = bNewValue;
	}
	else
	{
		*ChannelFlag = bNewValue;
	}

	Settings.SaveConfig();
	UE_LOG(LogYOLOInventory, Display, TEXT("YOLOInventory debug channel '%s' %s"), *Args[0], bNewValue ? TEXT("ENABLED") : TEXT("DISABLED"));
}

void FYOLOInventoryModule::HandleDebugHistoryConsoleCommand(const TArray<FString>& Args, UWorld* /*World*/)
{
	if (Args.Num() > 0 && Args[0].Equals(TEXT("clear"), ESearchCase::IgnoreCase))
	{
		UYIDebugLibrary::ClearDebugMessageHistory();
		UE_LOG(LogYOLOInventory, Display, TEXT("YOLOInventory debug history cleared."));
		return;
	}

	TArray<FYIDebugMessageRecord> History;
	UYIDebugLibrary::GetDebugMessageHistory(History);
	UE_LOG(LogYOLOInventory, Display, TEXT("YOLOInventory debug history entries: %d"), History.Num());
}

void FYOLOInventoryModule::HandleDebugStatusConsoleCommand(const TArray<FString>& /*Args*/, UWorld* /*World*/)
{
	const UYOLOInventorySettings& Settings = UYOLOInventorySettings::Get();
	UE_LOG(LogYOLOInventory, Display, TEXT("Pipeline=%s Screen=%s Log=%s Force=%s History=%s Dedupe=%s StableKeys=%s"),
		Settings.bEnableDebugPipeline ? TEXT("ON") : TEXT("OFF"),
		Settings.bDebugOutputToScreen ? TEXT("ON") : TEXT("OFF"),
		Settings.bDebugOutputToLog ? TEXT("ON") : TEXT("OFF"),
		Settings.bAllowForcedDebugMessages ? TEXT("ON") : TEXT("OFF"),
		Settings.bDebugRouteToHistory ? TEXT("ON") : TEXT("OFF"),
		Settings.bDebugDeduplicateMessages ? TEXT("ON") : TEXT("OFF"),
		Settings.bDebugUseStableScreenKeys ? TEXT("ON") : TEXT("OFF"));
	UE_LOG(LogYOLOInventory, Display, TEXT("Durations: screen=%.2fs pinned=%.2fs duplicate_interval=%.2fs history_max=%d"),
		Settings.DebugScreenSeconds,
		Settings.DebugPinnedScreenSeconds,
		Settings.DebugDuplicateIntervalSeconds,
		Settings.DebugHistoryMaxEntries);

	UE_LOG(LogYOLOInventory, Display, TEXT("Channels: General=%d Persistence=%d Inventory=%d Equipment=%d ActionBar=%d Trade=%d Shop=%d Grid=%d Phase2=%d"),
		Settings.bDebugChannelGeneral ? 1 : 0,
		Settings.bDebugChannelPersistence ? 1 : 0,
		Settings.bDebugChannelInventory ? 1 : 0,
		Settings.bDebugChannelEquipment ? 1 : 0,
		Settings.bDebugChannelActionBar ? 1 : 0,
		Settings.bDebugChannelTrade ? 1 : 0,
		Settings.bDebugChannelShop ? 1 : 0,
		Settings.bDebugChannelGrid ? 1 : 0,
		Settings.bDebugChannelPhase2 ? 1 : 0);
}

void FYOLOInventoryModule::HandleDebugProfileConsoleCommand(const TArray<FString>& Args, UWorld* /*World*/)
{
	if (Args.Num() < 1)
	{
		UE_LOG(LogYOLOInventory, Display, TEXT("Usage: YOLOInventory.Debug.Profile <off|minimal|normal|verbose>"));
		return;
	}

	UYOLOInventorySettings& Settings = UYOLOInventorySettings::GetMutable();
	const FString Profile = Args[0].ToLower();

	if (Profile == TEXT("off"))
	{
		Settings.bEnableDebugPipeline = false;
		Settings.bDebugOutputToLog = false;
		Settings.bDebugOutputToScreen = false;
	}
	else if (Profile == TEXT("minimal"))
	{
		Settings.bEnableDebugPipeline = true;
		Settings.bDebugOutputToLog = true;
		Settings.bDebugOutputToScreen = false;
		Settings.bAllowForcedDebugMessages = false;
		Settings.bDebugChannelGeneral = true;
		Settings.bDebugChannelPersistence = false;
		Settings.bDebugChannelInventory = false;
		Settings.bDebugChannelEquipment = false;
		Settings.bDebugChannelActionBar = false;
		Settings.bDebugChannelTrade = false;
		Settings.bDebugChannelShop = false;
		Settings.bDebugChannelGrid = false;
		Settings.bDebugChannelPhase2 = false;
	}
	else if (Profile == TEXT("normal"))
	{
		Settings.bEnableDebugPipeline = true;
		Settings.bDebugOutputToLog = true;
		Settings.bDebugOutputToScreen = false;
		Settings.bAllowForcedDebugMessages = false;
		Settings.bDebugChannelGeneral = true;
		Settings.bDebugChannelPersistence = true;
		Settings.bDebugChannelInventory = true;
		Settings.bDebugChannelEquipment = true;
		Settings.bDebugChannelActionBar = true;
		Settings.bDebugChannelTrade = true;
		Settings.bDebugChannelShop = true;
		Settings.bDebugChannelGrid = false;
		Settings.bDebugChannelPhase2 = false;
	}
	else if (Profile == TEXT("verbose"))
	{
		Settings.bEnableDebugPipeline = true;
		Settings.bDebugOutputToLog = true;
		Settings.bDebugOutputToScreen = true;
		Settings.bAllowForcedDebugMessages = true;
		Settings.bDebugChannelGeneral = true;
		Settings.bDebugChannelPersistence = true;
		Settings.bDebugChannelInventory = true;
		Settings.bDebugChannelEquipment = true;
		Settings.bDebugChannelActionBar = true;
		Settings.bDebugChannelTrade = true;
		Settings.bDebugChannelShop = true;
		Settings.bDebugChannelGrid = true;
		Settings.bDebugChannelPhase2 = true;
	}
	else
	{
		UE_LOG(LogYOLOInventory, Warning, TEXT("Unknown debug profile '%s'."), *Args[0]);
		return;
	}

	Settings.SaveConfig();
	UE_LOG(LogYOLOInventory, Display, TEXT("YOLOInventory debug profile set to '%s'."), *Profile);
}
