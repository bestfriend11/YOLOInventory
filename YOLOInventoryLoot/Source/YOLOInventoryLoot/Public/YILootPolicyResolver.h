#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YIItemFeatureResolverRegistry.h"
#include "YIItemDefinition.h"
#include "YILootPolicyResolver.generated.h"

UENUM(BlueprintType)
enum class EYILootPolicyDenyReason : uint8
{
	None = 0,
	NotEligible,
	LevelOutOfRange,
	MissingRequiredTags,
	BlockedByTags
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYLOOT_API FYILootPolicyContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot")
	int32 LootLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot")
	FGameplayTagContainer LootContextTags;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYLOOT_API FYILootPolicyResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot")
	bool bEligible = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot")
	EYILootPolicyDenyReason DenyReason = EYILootPolicyDenyReason::None;

	/** Weight multiplier in basis points (10000 = x1.0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot")
	int32 WeightScaleBps = 10000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot")
	int32 MinGeneratedCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot")
	int32 MaxGeneratedCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot")
	FText Message;
};

class YOLOINVENTORYLOOT_API IYILootPolicyResolver : public IYIItemFeatureResolver
{
public:
	virtual bool EvaluateLootPolicy(const UYIItemDefinition* Definition, const FYILootPolicyContext& Context, FYILootPolicyResult& OutResult) const = 0;
};

class YOLOINVENTORYLOOT_API FYIDefaultLootPolicyResolver : public IYILootPolicyResolver
{
public:
	virtual FName GetResolverKey() const override { return YIItemFeatureKeys::LootPolicy; }
	virtual bool EvaluateLootPolicy(const UYIItemDefinition* Definition, const FYILootPolicyContext& Context, FYILootPolicyResult& OutResult) const override;
};

