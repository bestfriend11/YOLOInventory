#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "YIWorldLootBlueprintLibrary.generated.h"

class AYIItemPickup;
class UYIInventoryBag;
class UYIItemDefinition;
struct FYIItemInstance;

UCLASS()
class YOLOINVENTORYWORLD_API UYIWorldLootBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Spawn a replicated pickup actor for an item definition code. Server-only; returns nullptr on clients. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|WorldLoot", meta=(WorldContext="WorldContextObject", DisplayName="Spawn Item Pickup by Code", BlueprintAuthorityOnly="true"))
	static AYIItemPickup* SpawnItemPickupByCode(UObject* WorldContextObject, int64 Code, const FTransform& Transform, int32 Count = 1, TSubclassOf<AYIItemPickup> PickupClass = nullptr);

	/** Spawn a replicated pickup using a definition asset. Server-only; falls back to code lookup if asset null. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|WorldLoot", meta=(WorldContext="WorldContextObject", DisplayName="Spawn Item Pickup from Definition", BlueprintAuthorityOnly="true"))
	static AYIItemPickup* SpawnItemPickup(UObject* WorldContextObject, UYIItemDefinition* Definition, const FTransform& Transform, int32 Count = 1, TSubclassOf<AYIItemPickup> PickupClass = nullptr);

	/** Spawn a replicated pickup from an item instance (preserves affixes/durability). Server-only. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|WorldLoot", meta=(WorldContext="WorldContextObject", DisplayName="Spawn Item Pickup from Instance", BlueprintAuthorityOnly="true"))
	static AYIItemPickup* SpawnItemPickupFromInstance(UObject* WorldContextObject, const FYIItemInstance& Instance, const FTransform& Transform, TSubclassOf<AYIItemPickup> PickupClass = nullptr);

	/** Drop part or all of a bag stack into the world as a pickup. Server-only. Count<=0 drops full stack. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|WorldLoot", meta=(WorldContext="WorldContextObject", DisplayName="Drop Bag Item to World", BlueprintAuthorityOnly="true"))
	static bool DropBagItemToWorld(UObject* WorldContextObject, UYIInventoryBag* Bag, int32 Index, const FTransform& SpawnTransform, int32 Count = 0, TSubclassOf<AYIItemPickup> PickupClass = nullptr);

	/** Destroy part or all of a bag stack (no pickup). Server-only. Count<=0 destroys full stack. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|WorldLoot", meta=(WorldContext="WorldContextObject", DisplayName="Destroy Bag Item", BlueprintAuthorityOnly="true"))
	static bool DestroyBagItem(UObject* WorldContextObject, UYIInventoryBag* Bag, int32 Index, int32 Count = 0);

	/** Consume a pickup actor into a bag (preserves instance) and destroy the pickup. Server-only. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|WorldLoot", meta=(WorldContext="WorldContextObject", DisplayName="Pickup Item Actor Into Bag", BlueprintAuthorityOnly="true"))
	static bool PickupItemActorIntoBag(UObject* WorldContextObject, UYIInventoryBag* Bag, AYIItemPickup* Pickup);
};
