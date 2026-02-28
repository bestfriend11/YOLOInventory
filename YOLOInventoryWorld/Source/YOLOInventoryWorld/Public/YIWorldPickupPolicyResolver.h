#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YIItemFeatureResolverRegistry.h"
#include "YIItemInstance.h"
#include "YIWorldPickupPolicyResolver.generated.h"

UENUM(BlueprintType)
enum class EYIWorldPickupDenyReason : uint8
{
	None = 0,
	PickupDisabled,
	OwnerOnly,
	MissingRequiredTags,
	BlockedByTags
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYWORLD_API FYIWorldPickupPolicyContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World|Pickup")
	bool bIsOwner = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World|Pickup")
	FGameplayTagContainer PickerTags;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYWORLD_API FYIWorldPickupPolicyResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World|Pickup")
	bool bAllowed = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World|Pickup")
	bool bAutoPickup = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World|Pickup")
	EYIWorldPickupDenyReason DenyReason = EYIWorldPickupDenyReason::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World|Pickup")
	FText Message;
};

class YOLOINVENTORYWORLD_API IYIWorldPickupPolicyResolver : public IYIItemFeatureResolver
{
public:
	virtual bool EvaluatePickupPolicy(const FYIItemInstance& Item, const FYIWorldPickupPolicyContext& Context, FYIWorldPickupPolicyResult& OutResult) const = 0;
};

class YOLOINVENTORYWORLD_API FYIDefaultWorldPickupPolicyResolver : public IYIWorldPickupPolicyResolver
{
public:
	virtual FName GetResolverKey() const override { return YIItemFeatureKeys::PickupPolicy; }
	virtual bool EvaluatePickupPolicy(const FYIItemInstance& Item, const FYIWorldPickupPolicyContext& Context, FYIWorldPickupPolicyResult& OutResult) const override;
};

