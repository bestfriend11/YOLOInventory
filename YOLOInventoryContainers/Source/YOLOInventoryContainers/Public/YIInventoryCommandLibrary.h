#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "YIInventoryCoreTypes.h"
#include "YIInventoryCommandLibrary.generated.h"

class UYIInventoryBag;
class UYIInventoryComponent;

/**
 * UI-agnostic inventory command helpers.
 * Prefer these wrappers (explicit bag/item identity) over index-based convenience methods on UYIInventoryComponent.
 */
UCLASS()
class YOLOINVENTORYCONTAINERS_API UYIInventoryCommandLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category="YOLO Inventory|Commands", meta=(ToolTip="Build canonical item ref from bag + index."))
	static bool BuildItemRef(UYIInventoryComponent* Inventory, UYIInventoryBag* Bag, int32 ItemIndex, FYIInventoryItemRef& OutItemRef);

	UFUNCTION(BlueprintPure, Category="YOLO Inventory|Commands", meta=(ToolTip="Build canonical item ref from the component's current active bag + item index."))
	static bool BuildActiveBagItemRef(UYIInventoryComponent* Inventory, int32 ItemIndex, FYIInventoryItemRef& OutItemRef);

	UFUNCTION(BlueprintPure, Category="YOLO Inventory|Commands", meta=(ToolTip="Build canonical item ref from an active context bag (by context tag) + item index."))
	static bool BuildContextBagItemRef(UYIInventoryComponent* Inventory, FGameplayTag ContextTag, int32 ItemIndex, FYIInventoryItemRef& OutItemRef);

	UFUNCTION(BlueprintCallable, Category="YOLO Inventory|Commands", meta=(ToolTip="Move item using canonical item reference."))
	static bool MoveItemByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef, FIntPoint NewPos);

	UFUNCTION(BlueprintCallable, Category="YOLO Inventory|Commands", meta=(ToolTip="Move/swap item to exact cell using canonical item reference."))
	static bool MoveItemAtCellByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef, FIntPoint DestCell, bool bAllowSingleOverlapSwap = true);

	UFUNCTION(BlueprintCallable, Category="YOLO Inventory|Commands", meta=(ToolTip="Rotate item using canonical item reference."))
	static bool RotateItemByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef);

	UFUNCTION(BlueprintCallable, Category="YOLO Inventory|Commands", meta=(ToolTip="Remove item using canonical item reference."))
	static bool RemoveItemByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef);

	UFUNCTION(BlueprintCallable, Category="YOLO Inventory|Commands", meta=(ToolTip="Combine stack using canonical item reference."))
	static bool CombineItemByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef);

	UFUNCTION(BlueprintCallable, Category="YOLO Inventory|Commands", meta=(ToolTip="Split stack using canonical item reference."))
	static bool SplitStackByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef, int32 Amount, FIntPoint DesiredPos);

	UFUNCTION(BlueprintCallable, Category="YOLO Inventory|Commands", meta=(ToolTip="Transfer item to another bag using canonical item reference."))
	static bool TransferItemToBagByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef, const FGuid& DestBagId, int32 Count, int32& OutDestIndex);

	UFUNCTION(BlueprintCallable, Category="YOLO Inventory|Commands", meta=(ToolTip="Transfer item to an exact destination cell in another bag using canonical item reference."))
	static bool TransferItemToBagCellByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef, const FGuid& DestBagId, FIntPoint DestCell, int32 Count, bool bAllowSingleOverlapSwap = false);

	UFUNCTION(BlueprintCallable, Category="YOLO Inventory|Commands", meta=(ToolTip="Swap-capable transfer into exact destination cell using canonical item reference."))
	static bool SwapItemToBagCellByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef, const FGuid& DestBagId, FIntPoint DestCell);

	UFUNCTION(BlueprintCallable, Category="YOLO Inventory|Commands", meta=(ToolTip="Lock or unlock item using canonical item reference."))
	static bool SetItemLockedByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef, bool bLocked);
};
