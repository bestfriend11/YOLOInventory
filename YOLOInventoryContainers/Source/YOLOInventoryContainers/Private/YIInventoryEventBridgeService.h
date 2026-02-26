#pragma once

#include "CoreMinimal.h"

struct FYIBagItem;
class UYIInventoryBag;
class UYIInventoryComponent;

struct FYIInventoryEventBridgeService
{
	static void HandleBagItemAdded(UYIInventoryComponent& Inventory, int32 Index, FYIBagItem Item);
	static void HandleBagItemRemoved(UYIInventoryComponent& Inventory, int32 Index, FYIBagItem Item);
	static void HandleBagItemMoved(UYIInventoryComponent& Inventory, int32 Index, FIntPoint NewPos);
	static void HandleBagItemRotated(UYIInventoryComponent& Inventory, int32 Index);
	static void HandleBagItemTransferred(UYIInventoryComponent& Inventory, UYIInventoryBag* Src, UYIInventoryBag* Dest, int32 SrcIdx, int32 DestIdx);
};

