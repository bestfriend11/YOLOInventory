#include "YIDebugLibrary.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "HAL/CriticalSection.h"
#include "Misc/Crc.h"
#include "Misc/ScopeLock.h"
#include "YIDebugRouterSubsystem.h"

namespace YIDebugLibraryPrivate
{
	static FCriticalSection HistoryCS;
	static TArray<FYIDebugMessageRecord> MessageHistory;
	static TMap<uint32, double> LastEmitTimeByMessageHash;

	static uint32 BuildMessageHash(EYIDebugChannel Channel, const FString& Source, const FString& Message)
	{
		const FString Full = FString::Printf(TEXT("%d|%s|%s"), static_cast<int32>(Channel), *Source, *Message);
		return FCrc::StrCrc32(*Full);
	}

	static uint64 BuildStableScreenKey(uint32 Hash)
	{
		return 0x5949444200000000ULL | static_cast<uint64>(Hash); // "YIDB"
	}

	static void PushHistory(const FYIDebugMessageRecord& Record)
	{
		const int32 MaxEntries = FMath::Max(16, UYOLOInventorySettings::Get().DebugHistoryMaxEntries);
		FScopeLock Lock(&HistoryCS);
		MessageHistory.Add(Record);
		if (MessageHistory.Num() > MaxEntries)
		{
			MessageHistory.RemoveAt(0, MessageHistory.Num() - MaxEntries);
		}
	}

	static bool ShouldSuppressDuplicate(uint32 MessageHash, const UYOLOInventorySettings& Settings)
	{
		if (!Settings.bDebugDeduplicateMessages)
		{
			return false;
		}

		const double Now = FPlatformTime::Seconds();
		const double Interval = FMath::Max(0.0, static_cast<double>(Settings.DebugDuplicateIntervalSeconds));
		FScopeLock Lock(&HistoryCS);
		if (const double* LastTime = LastEmitTimeByMessageHash.Find(MessageHash))
		{
			if ((Now - *LastTime) < Interval)
			{
				return true;
			}
		}
		LastEmitTimeByMessageHash.Add(MessageHash, Now);
		return false;
	}
}

void UYIDebugLibrary::EmitDebugMessage(
	UObject* WorldContextObject,
	EYIDebugChannel Channel,
	const FString& Message,
	FLinearColor Color,
	bool bAllowOnScreen,
	bool bAllowLog,
	float ScreenSeconds,
	bool bPinned,
	bool bForce,
	const FString& Source)
{
	const UYOLOInventorySettings& Settings = UYOLOInventorySettings::Get();
	if (!Settings.bEnableDebugPipeline)
	{
		return;
	}

	const bool bEffectiveForce = bForce && Settings.bAllowForcedDebugMessages;
	if (!bEffectiveForce && !Settings.IsDebugChannelEnabled(Channel))
	{
		return;
	}

	const FString Prefix = Source.IsEmpty()
		? FString::Printf(TEXT("[YI][%d] "), static_cast<int32>(Channel))
		: FString::Printf(TEXT("[YI][%d][%s] "), static_cast<int32>(Channel), *Source);
	const FString FullMessage = Prefix + Message;
	const uint32 MessageHash = YIDebugLibraryPrivate::BuildMessageHash(Channel, Source, Message);

	if (!bEffectiveForce && YIDebugLibraryPrivate::ShouldSuppressDuplicate(MessageHash, Settings))
	{
		return;
	}

	if (bAllowLog && Settings.bDebugOutputToLog)
	{
		UE_LOG(LogTemp, Log, TEXT("%s"), *FullMessage);
	}

	const bool bCanScreen = GEngine && (bAllowOnScreen && Settings.bDebugOutputToScreen);
	if (bCanScreen)
	{
		const float Duration = bPinned
			? Settings.DebugPinnedScreenSeconds
			: (ScreenSeconds > 0.0f ? ScreenSeconds : Settings.DebugScreenSeconds);
		const uint64 Key = (bPinned || Settings.bDebugUseStableScreenKeys)
			? YIDebugLibraryPrivate::BuildStableScreenKey(MessageHash)
			: static_cast<uint64>(-1);
		GEngine->AddOnScreenDebugMessage(Key, Duration, Color.ToFColor(true), FullMessage);
	}

	FYIDebugMessageRecord Record;
	Record.UtcTime = FDateTime::UtcNow();
	Record.Channel = Channel;
	Record.Source = Source;
	Record.Message = Message;
	Record.Color = Color;
	if (Settings.bDebugRouteToHistory)
	{
		YIDebugLibraryPrivate::PushHistory(Record);
	}

	if (WorldContextObject)
	{
		if (UWorld* World = WorldContextObject->GetWorld())
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				if (UYIDebugRouterSubsystem* Router = GameInstance->GetSubsystem<UYIDebugRouterSubsystem>())
				{
					Router->BroadcastDebugMessage(Record);
				}
			}
		}
	}
}

void UYIDebugLibrary::GetDebugMessageHistory(TArray<FYIDebugMessageRecord>& OutMessages)
{
	FScopeLock Lock(&YIDebugLibraryPrivate::HistoryCS);
	OutMessages = YIDebugLibraryPrivate::MessageHistory;
}

void UYIDebugLibrary::ClearDebugMessageHistory()
{
	FScopeLock Lock(&YIDebugLibraryPrivate::HistoryCS);
	YIDebugLibraryPrivate::MessageHistory.Reset();
	YIDebugLibraryPrivate::LastEmitTimeByMessageHash.Reset();
}

void UYIDebugLibrary::SetDebugPipelineEnabled(bool bEnabled)
{
	UYOLOInventorySettings& Settings = UYOLOInventorySettings::GetMutable();
	Settings.bEnableDebugPipeline = bEnabled;
	Settings.SaveConfig();
}

bool UYIDebugLibrary::IsDebugChannelEnabled(EYIDebugChannel Channel)
{
	return UYOLOInventorySettings::Get().IsDebugChannelEnabled(Channel);
}

void UYIDebugLibrary::SetDebugChannelEnabled(EYIDebugChannel Channel, bool bEnabled)
{
	UYOLOInventorySettings& Settings = UYOLOInventorySettings::GetMutable();
	switch (Channel)
	{
	case EYIDebugChannel::General: Settings.bDebugChannelGeneral = bEnabled; break;
	case EYIDebugChannel::Persistence: Settings.bDebugChannelPersistence = bEnabled; break;
	case EYIDebugChannel::Inventory: Settings.bDebugChannelInventory = bEnabled; break;
	case EYIDebugChannel::Equipment: Settings.bDebugChannelEquipment = bEnabled; break;
	case EYIDebugChannel::ActionBar: Settings.bDebugChannelActionBar = bEnabled; break;
	case EYIDebugChannel::Trade: Settings.bDebugChannelTrade = bEnabled; break;
	case EYIDebugChannel::Shop: Settings.bDebugChannelShop = bEnabled; break;
	case EYIDebugChannel::Grid: Settings.bDebugChannelGrid = bEnabled; break;
	case EYIDebugChannel::Phase2: Settings.bDebugChannelPhase2 = bEnabled; break;
	default: break;
	}
	Settings.SaveConfig();
}

void UYIDebugLibrary::SetDebugScreenOutputEnabled(bool bEnabled)
{
	UYOLOInventorySettings& Settings = UYOLOInventorySettings::GetMutable();
	Settings.bDebugOutputToScreen = bEnabled;
	Settings.SaveConfig();
}

void UYIDebugLibrary::SetDebugLogOutputEnabled(bool bEnabled)
{
	UYOLOInventorySettings& Settings = UYOLOInventorySettings::GetMutable();
	Settings.bDebugOutputToLog = bEnabled;
	Settings.SaveConfig();
}
