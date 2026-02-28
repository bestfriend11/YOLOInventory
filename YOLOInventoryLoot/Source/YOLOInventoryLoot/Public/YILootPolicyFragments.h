#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YIItemFragments.h"
#include "YILootPolicyFragments.generated.h"

/** Static eligibility policy for loot generation contexts. */
USTRUCT(BlueprintType, meta=(DisplayName="Loot Eligibility Policy"))
struct YOLOINVENTORYLOOT_API FYIItemLootEligibilityFragment : public FYIItemDefinitionFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot|Policy")
	bool bLootEligible = true;

	/** Optional minimum generator level for this item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot|Policy")
	int32 MinLootLevel = 1;

	/** Optional maximum generator level for this item. <= 0 means unbounded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot|Policy")
	int32 MaxLootLevel = 0;

	/** Required context tags (e.g. Loot.Context.Boss). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot|Policy")
	FGameplayTagContainer RequiredLootTags;

	/** Blocked context tags. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot|Policy")
	FGameplayTagContainer BlockedLootTags;
};

/** Static roll tuning policy to derive weighted roll behavior. */
USTRUCT(BlueprintType, meta=(DisplayName="Loot Roll Policy"))
struct YOLOINVENTORYLOOT_API FYIItemRollPolicyFragment : public FYIItemDefinitionFragmentBase
{
	GENERATED_BODY()

	/** Weight multiplier in basis points (10000 = x1.0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot|Roll", meta=(ClampMin="0"))
	int32 WeightScaleBps = 10000;

	/** Optional minimum stack/count for generated entries. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot|Roll", meta=(ClampMin="1"))
	int32 MinGeneratedCount = 1;

	/** Optional maximum stack/count for generated entries (0 means leave generator default). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot|Roll", meta=(ClampMin="0"))
	int32 MaxGeneratedCount = 0;
};

