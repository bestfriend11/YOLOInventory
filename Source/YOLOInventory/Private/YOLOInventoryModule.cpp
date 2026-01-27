#include "YOLOInventoryModule.h"

#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"
#include "YOLOInventorySettings.h"
#include "YIInventoryComponent.h"
#include "YIInventoryBlueprintLibrary.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

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

	AddItemConsoleCommand = MakeUnique<FAutoConsoleCommandWithWorldAndArgs>(
		TEXT("yi.additem"),
		TEXT("Give item to first player's active bag. Usage: yi.additem <code> [count]"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateRaw(this, &FYOLOInventoryModule::HandleAddItemConsoleCommand));
}

void FYOLOInventoryModule::ShutdownModule()
{
	DebugConsoleCommand.Reset();
	AddItemConsoleCommand.Reset();
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

void FYOLOInventoryModule::HandleAddItemConsoleCommand(const TArray<FString>& Args, UWorld* World)
{
	if (!World || World->GetNetMode() == NM_Client)
	{
		UE_LOG(LogYOLOInventory, Warning, TEXT("yi.additem: must run on server/authority"));
		return;
	}

	if (Args.Num() < 1)
	{
		UE_LOG(LogYOLOInventory, Warning, TEXT("yi.additem: Usage yi.additem <code> [count]"));
		return;
	}

	int64 Code = FCString::Atoi64(*Args[0]);
	int32 Count = (Args.Num() > 1) ? FCString::Atoi(*Args[1]) : 1;
	Count = FMath::Max(1, Count);

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		UE_LOG(LogYOLOInventory, Warning, TEXT("yi.additem: no player controller found"));
		return;
	}

	UYIInventoryComponent* InvComp = PC->FindComponentByClass<UYIInventoryComponent>();
	if (!InvComp || !InvComp->EquippedBag)
	{
		UE_LOG(LogYOLOInventory, Warning, TEXT("yi.additem: player has no inventory component or bag"));
		return;
	}

	if (UYIInventoryBlueprintLibrary::AddItemToBagByCode(InvComp->EquippedBag, Code, Count))
	{
		UE_LOG(LogYOLOInventory, Display, TEXT("yi.additem: added code %lld x%d"), (long long)Code, Count);
	}
	else
	{
		UE_LOG(LogYOLOInventory, Warning, TEXT("yi.additem: failed to add code %lld"), (long long)Code);
	}
}
