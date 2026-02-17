#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "YIDebugLibrary.h"
#include "YIDebugRouterSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FYIDebugMessageEvent, const FYIDebugMessageRecord&, Message);

/**
 * Runtime relay for YOLOInventory debug messages.
 * Designers can bind UI widgets to OnDebugMessage without C++ edits.
 */
UCLASS()
class YOLOINVENTORY_API UYIDebugRouterSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Debug")
	FYIDebugMessageEvent OnDebugMessage;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Debug")
	void GetBufferedMessages(TArray<FYIDebugMessageRecord>& OutMessages) const;

	void BroadcastDebugMessage(const FYIDebugMessageRecord& Message);
};
