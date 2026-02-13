#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "YILootDropProfile.generated.h"

class UYIItemGenerator;
class UYILootTable;
class UYIItemDefinition;
class UYIInventoryBag;
class UYIInventoryComponent;
class AYIItemPickup;
class AActor;

UENUM(BlueprintType)
enum class EYILootDropSpawnMode : uint8
{
	WorldPickup UMETA(DisplayName = "World Pickup"),
	DirectToInventory UMETA(DisplayName = "Direct To Inventory")
};

UENUM(BlueprintType)
enum class EYILootDropLevelSource : uint8
{
	FixedLevel UMETA(DisplayName = "Fixed Level"),
	ContextLevel UMETA(DisplayName = "Context Level"),
	OwnerLevel UMETA(DisplayName = "Owner Level"),
	InstigatorLevel UMETA(DisplayName = "Instigator Level")
};

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYILootGuaranteedDropEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guaranteed")
	TSoftObjectPtr<UYIItemDefinition> Definition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guaranteed", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Chance = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guaranteed", meta = (ClampMin = "1"))
	int32 MinCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guaranteed", meta = (ClampMin = "1"))
	int32 MaxCount = 1;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYILootDropContext
{
	GENERATED_BODY()

	/** Usually the killer actor for mob drops, or opener actor for chests. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context")
	TObjectPtr<AActor> InstigatorActor = nullptr;

	/** Optional explicit destination inventory (used by DirectToInventory mode). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context")
	TObjectPtr<UYIInventoryComponent> TargetInventory = nullptr;

	/** Optional explicit destination bag (used by DirectToInventory mode). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context")
	TObjectPtr<UYIInventoryBag> TargetBag = nullptr;

	/** Used when LevelSource == ContextLevel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context", meta = (ClampMin = "1"))
	int32 ContextLevel = 1;

	/** 0 means auto-generate seed on server. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context")
	int32 Seed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context")
	bool bOverrideSpawnTransform = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Context", meta = (EditCondition = "bOverrideSpawnTransform", EditConditionHides))
	FTransform SpawnTransform = FTransform::Identity;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYILootDropResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
	bool bSuccess = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
	int32 EffectiveLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
	int32 EffectiveSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
	int32 NumGeneratedRolls = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
	int32 NumGuaranteedDrops = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
	int32 NumSpawnedPickups = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
	int32 NumAddedToInventory = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Result")
	FText Message;
};

UCLASS(BlueprintType)
class YOLOINVENTORY_API UYILootDropProfile : public UDataAsset
{
	GENERATED_BODY()
public:
	/** Preferred generation path (full affix roll path). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TSoftObjectPtr<UYIItemGenerator> Generator;

	/** Fallback generation path when Generator is not set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	TSoftObjectPtr<UYILootTable> LootTable;

	/** Number of generated rolls to perform. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0"))
	int32 MinRolls = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation", meta = (ClampMin = "0"))
	int32 MaxRolls = 1;

	/** Additional deterministic drops that are evaluated before generated rolls. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Guaranteed")
	TArray<FYILootGuaranteedDropEntry> GuaranteedDrops;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	EYILootDropLevelSource LevelSource = EYILootDropLevelSource::ContextLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level", meta = (ClampMin = "1"))
	int32 FixedLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	int32 LevelOffset = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level")
	bool bClampLevel = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level", meta = (ClampMin = "1", EditCondition = "bClampLevel", EditConditionHides))
	int32 MinLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level", meta = (ClampMin = "1", EditCondition = "bClampLevel", EditConditionHides))
	int32 MaxLevel = 9999;

	/** 0 means auto-generate from runtime context. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Random")
	int32 DefaultSeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	EYILootDropSpawnMode SpawnMode = EYILootDropSpawnMode::WorldPickup;

	/** For DirectToInventory mode, prefer killer/opener inventory when available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	bool bPreferInstigatorInventory = true;

	/** For DirectToInventory mode, fallback to owner inventory when instigator inventory is missing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	bool bFallbackToOwnerInventory = true;

	/** If DirectToInventory fails, spawn in world instead of losing the drop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	bool bFallbackToWorldPickup = true;

	/** Optional pickup class override for world spawn delivery. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	TSubclassOf<AYIItemPickup> PickupClassOverride;

	/** Scatter radius around spawn transform for each dropped pickup. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery", meta = (ClampMin = "0.0"))
	float PickupScatterRadius = 60.0f;

	/** One-shot profiles ignore repeated triggers after first successful drop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lifecycle")
	bool bOneShot = false;

	/** If true, owner destroy event can trigger this profile automatically from component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lifecycle")
	bool bDropOnOwnerDestroyed = false;
};
