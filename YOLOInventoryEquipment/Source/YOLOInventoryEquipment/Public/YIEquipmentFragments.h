#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YIItemFragments.h"
#include "YIEquipmentFragments.generated.h"

/**
 * Static equip validation metadata.
 * Feature systems may enforce these requirements at equip-time.
 */
USTRUCT(BlueprintType, meta=(DisplayName="Equip Requirements"))
struct YOLOINVENTORYEQUIPMENT_API FYIItemEquipRequirementsFragment : public FYIItemDefinitionFragmentBase
{
	GENERATED_BODY()

	/** Minimum character level to equip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Requirements", meta=(ClampMin="1"))
	int32 MinLevel = 1;

	/** Required tags that must exist on the equipping actor/state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Requirements")
	FGameplayTagContainer RequiredTags;

	/** Tags that block equip if present on the equipping actor/state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Requirements")
	FGameplayTagContainer BlockedTags;
};

/**
 * Runtime durability payload (feature-owned variant).
 * Uses integer durability for deterministic replication and economy-safe math.
 */
USTRUCT(BlueprintType, meta=(DisplayName="Durability Runtime"))
struct YOLOINVENTORYEQUIPMENT_API FYIItemDurabilityRuntimeFragment : public FYIItemFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Durability")
	bool bEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Durability", meta=(ClampMin="0"))
	int32 Current = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Durability", meta=(ClampMin="0"))
	int32 Max = 0;
};

/** Runtime charges payload for consumable/active items. */
USTRUCT(BlueprintType, meta=(DisplayName="Charges Runtime"))
struct YOLOINVENTORYEQUIPMENT_API FYIItemChargesRuntimeFragment : public FYIItemFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Charges", meta=(ClampMin="0"))
	int32 Current = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Charges", meta=(ClampMin="0"))
	int32 Max = 0;
};

/** Runtime cooldown payload persisted per-item instance. */
USTRUCT(BlueprintType, meta=(DisplayName="Cooldown Runtime"))
struct YOLOINVENTORYEQUIPMENT_API FYIItemCooldownRuntimeFragment : public FYIItemFragmentBase
{
	GENERATED_BODY()

	/** Absolute server world time (seconds) of last successful activation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Cooldown")
	double LastActivatedServerTime = 0.0;

	/** Cooldown duration (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Cooldown", meta=(ClampMin="0.0"))
	float CooldownDurationSeconds = 0.0f;
};
