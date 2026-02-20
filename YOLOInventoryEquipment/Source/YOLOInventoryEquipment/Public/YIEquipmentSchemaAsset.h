#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "YIEquipmentComponent.h"
#include "YIEquipmentSchemaAsset.generated.h"

/**
 * Reusable equipment schema shared by multiple pawns/classes.
 * Holds authoritative slot definitions and slot-domain rules; UI layout is intentionally external (UMG).
 */
UCLASS(BlueprintType)
class YOLOINVENTORYEQUIPMENT_API UYIEquipmentSchemaAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	/** Optional allow-list of equip slot tags (for example Equip.Slot.Head). Empty means prefix-based/definition-based checks. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment Schema|Rules")
	FGameplayTagContainer AllowedEquipSlots;

	/** Auto-resolve item equip slot from item tags using this prefix. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment Schema|Rules")
	FString EquipSlotTagPrefix = TEXT("Equip.Slot.");

	/** Canonical slot definitions for this schema. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment Schema")
	TArray<FYIEquipmentSlotDefinition> SlotDefinitions;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Equipment|Schema")
	void SortSlotDefinitions();
};

