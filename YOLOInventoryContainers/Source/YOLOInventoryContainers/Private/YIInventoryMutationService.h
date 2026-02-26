#pragma once

#include "CoreMinimal.h"

class UYIInventoryComponent;

struct FYIInventoryMutationService
{
	static bool MoveItemInActiveBagByIndex(UYIInventoryComponent& Inventory, int32 Index, FIntPoint NewPos);
	static bool MoveItemInBag(UYIInventoryComponent& Inventory, const FGuid& BagId, const FGuid& ItemInstanceId, FIntPoint NewPos);
	static bool MoveItemInBagAtCell(UYIInventoryComponent& Inventory, const FGuid& BagId, const FGuid& ItemInstanceId, FIntPoint DestCell, bool bAllowSingleOverlapSwap);
	static bool RotateItemInActiveBagByIndex(UYIInventoryComponent& Inventory, int32 Index);
	static bool RotateItemInBag(UYIInventoryComponent& Inventory, const FGuid& BagId, const FGuid& ItemInstanceId);
	static bool RemoveItemFromActiveBagByIndex(UYIInventoryComponent& Inventory, int32 Index);
	static bool RemoveItemFromBag(UYIInventoryComponent& Inventory, const FGuid& BagId, const FGuid& ItemInstanceId);
	static bool TransferItemBetweenBagsById(UYIInventoryComponent& Inventory, const FGuid& SourceBagId, const FGuid& ItemInstanceId, const FGuid& DestBagId, int32 Count, int32& OutDestIndex);
	static bool TransferItemBetweenBagsAtCellById(UYIInventoryComponent& Inventory, const FGuid& SourceBagId, const FGuid& ItemInstanceId, const FGuid& DestBagId, FIntPoint DestCell, int32 Count, bool bAllowSingleOverlapSwap);
	static bool CombineItemInBag(UYIInventoryComponent& Inventory, const FGuid& BagId, const FGuid& ItemInstanceId);
	static bool SplitStackInBag(UYIInventoryComponent& Inventory, const FGuid& BagId, const FGuid& ItemInstanceId, int32 Amount, FIntPoint DesiredPos);
};
