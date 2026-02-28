#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ScalableFloat.h"
#include "YIItemFragments.h"
#include "YIGASBridgeFragments.generated.h"

class UGameplayAbility;
class UGameplayEffect;

/**
 * Static fragment describing equip-time GAS grants owned by an item definition.
 * Keep this in GAS bridge plugin to avoid hard GAS dependency in schema/core.
 */
USTRUCT(BlueprintType, meta=(DisplayName="GAS Grant Definition"))
struct YOLOINVENTORYGASBRIDGE_API FYIItemGASGrantDefinitionFragment : public FYIItemDefinitionFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|Grant")
	bool bGrantOnEquip = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|Grant")
	bool bRemoveOnUnequip = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|Grant")
	TArray<TSoftClassPtr<UGameplayAbility>> GrantedAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|Grant")
	TArray<TSoftClassPtr<UGameplayEffect>> GrantedEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|Grant")
	FGameplayTagContainer GrantedTags;
};

/**
 * Static fragment describing item-use gameplay effects.
 */
USTRUCT(BlueprintType, meta=(DisplayName="GAS Use Effect Definition"))
struct YOLOINVENTORYGASBRIDGE_API FYIItemGASUseEffectDefinitionFragment : public FYIItemDefinitionFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|Use")
	TArray<TSoftClassPtr<UGameplayEffect>> UseEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|Use")
	FGameplayTag ActivationTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|Use")
	bool bConsumeChargesOnSuccess = true;
};

/**
 * Static scaling fragment for level/difficulty-dependent GAS magnitude evaluation.
 * ScalableFloat is evaluated by GAS bridge at runtime context (server-authoritative).
 */
USTRUCT(BlueprintType, meta=(DisplayName="GAS Scaling Definition"))
struct YOLOINVENTORYGASBRIDGE_API FYIItemGASScalingDefinitionFragment : public FYIItemDefinitionFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|Scaling")
	FScalableFloat PowerScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|Scaling")
	FScalableFloat CooldownScale;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|Scaling")
	FScalableFloat CostScale;
};

/**
 * Runtime GAS state persisted per item instance.
 */
USTRUCT(BlueprintType, meta=(DisplayName="GAS Runtime State"))
struct YOLOINVENTORYGASBRIDGE_API FYIItemGASRuntimeStateFragment : public FYIItemFragmentBase
{
	GENERATED_BODY()

	/** Last successful activation server timestamp. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|Runtime")
	double LastActivatedServerTime = 0.0;

	/** Optional runtime stack-like state for bridge policies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|Runtime")
	int32 RuntimeStateCount = 0;

	/** Optional runtime tags for bridge-local policies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS|Runtime")
	FGameplayTagContainer RuntimeTags;
};

