#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YIItemFeatureResolverRegistry.h"
#include "YIItemInstance.h"
#include "YIEquipmentPolicyResolver.generated.h"

UENUM(BlueprintType)
enum class EYIEquipPolicyDenyReason : uint8
{
	None = 0,
	MissingDefinition,
	LevelTooLow,
	MissingRequiredTags,
	BlockedByTags
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYEQUIPMENT_API FYIEquipPolicyContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	int32 ActorLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	FGameplayTagContainer ActorTags;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYEQUIPMENT_API FYIEquipPolicyResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	bool bAllowed = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	EYIEquipPolicyDenyReason DenyReason = EYIEquipPolicyDenyReason::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	FText Message;
};

class YOLOINVENTORYEQUIPMENT_API IYIEquipPolicyResolver : public IYIItemFeatureResolver
{
public:
	virtual bool EvaluateEquipPolicy(const FYIItemInstance& Item, const FYIEquipPolicyContext& Context, FYIEquipPolicyResult& OutResult) const = 0;
};

class YOLOINVENTORYEQUIPMENT_API FYIDefaultEquipPolicyResolver : public IYIEquipPolicyResolver
{
public:
	virtual FName GetResolverKey() const override { return YIItemFeatureKeys::EquipPolicy; }
	virtual bool EvaluateEquipPolicy(const FYIItemInstance& Item, const FYIEquipPolicyContext& Context, FYIEquipPolicyResult& OutResult) const override;
};

