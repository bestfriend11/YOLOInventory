#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "YIEquipmentLayoutAsset.generated.h"

UENUM(BlueprintType)
enum class EYIEquipmentLayoutMode : uint8
{
	Grid UMETA(DisplayName = "Grid"),
	Canvas UMETA(DisplayName = "Canvas")
};

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

	/** Top-left position used when LayoutMode is Canvas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Layout|Canvas")
	bool bUseCanvasPosition = false;

	/** Top-left position used when LayoutMode is Canvas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Layout|Canvas")
	FVector2D CanvasPosition = FVector2D::ZeroVector;

	/** Slot widget size used when LayoutMode is Canvas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Layout|Canvas", meta = (ClampMin = "8.0", ClampMax = "1024.0"))
	FVector2D CanvasSize = FVector2D(96.f, 96.f);
};

UCLASS(BlueprintType)
class YOLOINVENTORY_API UYIEquipmentLayoutAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Layout")
	EYIEquipmentLayoutMode LayoutMode = EYIEquipmentLayoutMode::Grid;

	/** Canvas size hint for editor/runtime layout previews. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment Layout|Canvas", meta = (ClampMin = "64.0", ClampMax = "4096.0"))
	FVector2D CanvasSize = FVector2D(600.f, 450.f);

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
