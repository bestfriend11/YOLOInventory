#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YOLOInventorySettings.h"
#include "YIDebugLibrary.generated.h"

USTRUCT(BlueprintType)
struct YOLOINVENTORYCORE_API FYIDebugMessageRecord
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Debug")
	FDateTime UtcTime;

	UPROPERTY(BlueprintReadOnly, Category="Debug")
	EYIDebugChannel Channel = EYIDebugChannel::General;

	UPROPERTY(BlueprintReadOnly, Category="Debug")
	FString Source;

	UPROPERTY(BlueprintReadOnly, Category="Debug")
	FString Message;

	UPROPERTY(BlueprintReadOnly, Category="Debug")
	FLinearColor Color = FLinearColor::White;
};

/**
 * Centralized runtime debug routing for YOLOInventory.
 * Designers can control output entirely from plugin settings + console commands.
 */
UCLASS()
class YOLOINVENTORYCORE_API UYIDebugLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Debug", meta=(WorldContext="WorldContextObject"))
	static void EmitDebugMessage(
		UObject* WorldContextObject,
		EYIDebugChannel Channel,
		const FString& Message,
		FLinearColor Color,
		bool bAllowOnScreen,
		bool bAllowLog,
		float ScreenSeconds = 0.0f,
		bool bPinned = false,
		bool bForce = false,
		const FString& Source = TEXT(""));

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Debug")
	static void GetDebugMessageHistory(TArray<FYIDebugMessageRecord>& OutMessages);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Debug")
	static void ClearDebugMessageHistory();

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Debug")
	static void SetDebugPipelineEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Debug")
	static bool IsDebugChannelEnabled(EYIDebugChannel Channel);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Debug")
	static void SetDebugChannelEnabled(EYIDebugChannel Channel, bool bEnabled);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Debug")
	static void SetDebugScreenOutputEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Debug")
	static void SetDebugLogOutputEnabled(bool bEnabled);
};
