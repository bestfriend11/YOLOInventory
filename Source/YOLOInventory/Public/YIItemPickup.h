#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YIItemPickup.generated.h"

class UStaticMeshComponent;
class UYIItemDefinition;

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIAttributeKV
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") FName Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") float Value = 0.f;
};

/** Net-safe version of item instance for pickups (avoids TMap replication). */
USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIItemInstanceNet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") TSoftObjectPtr<UYIItemDefinition> Definition;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") int32 Count = 1;
	/** Stable runtime identity for this specific item instance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") FGuid InstanceId;
	/** Stack/group identity. Copies that belong to the same stack lineage share StackId. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") FGuid StackId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") int64 CustomStackKey = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") FGuid ContainedBagId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") bool bRotated = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") TArray<struct FYIAffixInstance> Affixes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item") TArray<FYIAttributeKV> Attributes;
};

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

	/** Full item instance data (affixes, durability, attributes). Net-safe variant. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_ItemData, Category="YOLOInventory|Pickup")
	FYIItemInstanceNet ItemInstance;

	/** Designer list of selectable definitions (editor convenience). Server uses SelectedDefinition if ItemCode is unset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="YOLOInventory|Pickup")
	TArray<TSoftObjectPtr<UYIItemDefinition>> SelectableDefinitions;

	/** Index into SelectableDefinitions to use by default (server authority). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="YOLOInventory|Pickup", meta=(ClampMin="0"))
	int32 SelectedDefinitionIndex = 0;

	/** Refresh visuals from the loaded definition (safe on client/server). */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Pickup")
	void RefreshVisuals();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="YOLOInventory|Pickup")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UFUNCTION()
	void OnRep_ItemData();

	/** Soft ref cached to avoid repeated registry lookups. */
	UPROPERTY()
	TSoftObjectPtr<UYIItemDefinition> Definition;
};
