#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "YIGASBridgeApiTypes.h"
#include "YIGASBridgeSubsystem.generated.h"

/**
 * First-pass GAS bridge service surface.
 * Default implementation is intentionally thin and conservative.
 * Feature teams can subclass/extend this subsystem in project plugins.
 */
UCLASS(BlueprintType)
class YOLOINVENTORYGASBRIDGE_API UYIGASBridgeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|GASBridge")
	FYIGASBridgeOpResult RequestApplyItemGrants(const FYIGASBridgeGrantRequest& Request);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|GASBridge")
	FYIGASBridgeOpResult RequestRemoveItemGrants(const FYIGASBridgeGrantRequest& Request);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|GASBridge")
	FYIGASBridgeOpResult RequestActivateItem(const FYIGASBridgeActivateRequest& Request);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|GASBridge")
	FYIGASBridgeOpResult BuildDescriptionTokens(const FYIGASBridgeDescriptionRequest& Request, TArray<FYIGASBridgeDescriptionToken>& OutTokens) const;

protected:
	virtual FYIGASBridgeOpResult ApplyItemGrants_Internal(const FYIGASBridgeGrantRequest& Request);
	virtual FYIGASBridgeOpResult RemoveItemGrants_Internal(const FYIGASBridgeGrantRequest& Request);
	virtual FYIGASBridgeOpResult ActivateItem_Internal(const FYIGASBridgeActivateRequest& Request);
	virtual FYIGASBridgeOpResult BuildDescriptionTokens_Internal(const FYIGASBridgeDescriptionRequest& Request, TArray<FYIGASBridgeDescriptionToken>& OutTokens) const;

	static FYIGASBridgeOpResult MakeRejectedResult(
		EYIGASBridgeOpKind OpKind,
		EYIGASBridgeOpError Error,
		const FGuid& RequestId,
		const FText& Message);
};

