#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YIAffix.h"
#include "YIItemFragments.generated.h"

class UTexture2D;
class UStaticMesh;
class UYIAffixAsset;
class UYIAffixPoolAsset;

/** Marker base for instanced item fragments. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemFragmentBase
{
	GENERATED_BODY()
};

/** Marker base for static definition fragments (shared by all instances of a definition). */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemDefinitionFragmentBase
{
	GENERATED_BODY()
};

/** Shared UI metadata for an item definition. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemUIDefinitionFragment : public FYIItemDefinitionFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment", meta=(MultiLine=true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	TSoftObjectPtr<UTexture2D> Icon;
};

/** Shared grid layout policy for an item definition. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemLayoutDefinitionFragment : public FYIItemDefinitionFragmentBase
{
	GENERATED_BODY()

	/** Width/height footprint in grid cells. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment", meta=(ClampMin="1"))
	FIntPoint DefaultSize = FIntPoint(1, 1);

	/** Whether item instances may rotate in grid bags. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	bool bAllowRotation = true;
};

/** Shared stacking policy for an item definition. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemStackingDefinitionFragment : public FYIItemDefinitionFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	bool bAllowStacking = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment", meta=(ClampMin="1"))
	int32 MaxStackCount = 99;

	/** Keep mutable-state safety checks enabled for stacking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	bool bUseRiskChecks = true;
};

/** Shared world-drop rendering/physics hints for an item definition. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemPickupDefinitionFragment : public FYIItemDefinitionFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	FVector MeshScale = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	bool bSimulatePhysicsOnDrop = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	FName CollisionProfile = TEXT("PhysicsActor");
};

/** Shared weight/encumbrance data for an item definition. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemWeightDefinitionFragment : public FYIItemDefinitionFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment", meta=(ClampMin="0.0"))
	float WeightPerUnit = 0.f;
};

/** Shared equipment slot metadata for an item definition. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemEquipmentDefinitionFragment : public FYIItemDefinitionFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	FGameplayTag PrimaryEquipSlot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	FGameplayTagContainer OccupiedSlots;
};

/** Shared affix generation/template data for an item definition. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemAffixDefinitionFragment : public FYIItemDefinitionFragmentBase
{
	GENERATED_BODY()

	/** Affixes always applied to new instances of this definition. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	TArray<TSoftObjectPtr<UYIAffixAsset>> TemplateAffixes;

	/** Minimum number of random affixes to roll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment", meta=(ClampMin="0"))
	int32 MinRandomModifiers = 0;

	/** Maximum number of random affixes to roll. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment", meta=(ClampMin="0"))
	int32 MaxRandomModifiers = 0;

	/** Prefix pool used during randomized generation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	TSoftObjectPtr<UYIAffixPoolAsset> PrefixPool;

	/** Suffix pool used during randomized generation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	TSoftObjectPtr<UYIAffixPoolAsset> SuffixPool;
};

/** Dynamic key/value attributes carried by an item instance. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemAttributesFragment : public FYIItemFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	TMap<FName, float> Values;
};

/** Rolled affix payload carried by an item instance. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemAffixesFragment : public FYIItemFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	TArray<FYIAffixInstance> Values;
};

/** Optional durability state carried by an item instance. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemDurabilityFragment : public FYIItemFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	bool bEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	float Current = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	float Max = 0.f;
};
