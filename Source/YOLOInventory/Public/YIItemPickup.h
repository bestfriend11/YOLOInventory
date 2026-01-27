#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YIItemPickup.generated.h"

class UStaticMeshComponent;
class UYIItemDefinition;

/**
 * Simple replicated pickup that represents an item definition + count in the world.
 * Designed to be authoritative on the server; clients resolve the definition via the registry.
 */
UCLASS(Blueprintable)
class YOLOINVENTORY_API AYIItemPickup : public AActor
{
	GENERATED_BODY()
public:
	AYIItemPickup();

	// AActor
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Set the item this pickup represents (server-only). */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Pickup")
	void SetItemByCode(int64 InCode, int32 InCount = 1);

	/** Get the item definition (may be null on clients until resolved). */
	UFUNCTION(BlueprintPure, Category="YOLOInventory|Pickup")
	UYIItemDefinition* GetItemDefinition() const;

	/** Quantity represented by this pickup (replicated). */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_ItemData, Category="YOLOInventory|Pickup")
	int32 Count;

	/** Unique code for the item definition (replicated). */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_ItemData, Category="YOLOInventory|Pickup")
	int64 ItemCode;

	/** Designer list of selectable definitions (editor convenience). Server uses SelectedDefinition if ItemCode is unset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="YOLOInventory|Pickup")
	TArray<TSoftObjectPtr<UYIItemDefinition>> SelectableDefinitions;

	/** Index into SelectableDefinitions to use by default (server authority). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="YOLOInventory|Pickup", meta=(ClampMin="0"))
	int32 SelectedDefinitionIndex = 0;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="YOLOInventory|Pickup")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UFUNCTION()
	void OnRep_ItemData();

	/** Refresh visuals from the loaded definition (safe on client/server). */
	void RefreshVisuals();

	/** Soft ref cached to avoid repeated registry lookups. */
	UPROPERTY()
	TSoftObjectPtr<UYIItemDefinition> Definition;
};
