#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "YIEquipmentLayoutAsset.generated.h"

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIEquipmentSlotLayoutEntry
{
	GENERATED_BODY()

	/** Gameplay tag this slot accepts (for example Equip.Slot.Head). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Layout")
	FGameplayTag SlotTag;

	/** Designer-facing label shown in slot widgets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Layout")
	FText DisplayName;

	/** Sort key used before auto placement. Lower appears first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Layout")
	int32 SortOrder = 0;

	/** Optional explicit grid row; use -1 to auto-place. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Layout", meta = (ClampMin = "-1"))
	int32 Row = -1;

	/** Optional explicit grid column; use -1 to auto-place. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Layout", meta = (ClampMin = "-1"))
	int32 Column = -1;

	/** Slot row span inside generated grid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Layout", meta = (ClampMin = "1"))
	int32 RowSpan = 1;

	/** Slot column span inside generated grid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Layout", meta = (ClampMin = "1"))
	int32 ColumnSpan = 1;

	/** Icon size used for generated UInventoryEquipmentSlotWidget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Layout", meta = (ClampMin = "8.0", ClampMax = "256.0"))
	FVector2D IconSize = FVector2D(56.f, 56.f);
};

UCLASS(BlueprintType)
class YOLOINVENTORY_API UYIEquipmentLayoutAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	/** Used for entries that do not specify explicit Row/Column. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Layout", meta = (ClampMin = "1", ClampMax = "16"))
	int32 AutoColumnCount = 4;

	/** Uniform grid slot padding for generated slot widgets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Layout", meta = (ClampMin = "0.0", ClampMax = "32.0"))
	float SlotPadding = 4.f;

	/** Slot definitions used to build equipment UI layouts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Layout")
	TArray<FYIEquipmentSlotLayoutEntry> Slots;

	UFUNCTION(BlueprintCallable, Category = "YOLOInventory|Equipment|Layout")
	void SortSlotsByOrder();

	UFUNCTION(BlueprintPure, Category = "YOLOInventory|Equipment|Layout")
	void GetSortedSlots(TArray<FYIEquipmentSlotLayoutEntry>& OutSlots) const;
};

