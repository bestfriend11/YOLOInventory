#pragma once

#include "CoreMinimal.h"
#include "YIInventoryBag.h"
#include "YIInventoryEventTypes.generated.h"

class UYIInventoryBag;

UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FYIOnInventoryBagOpened, UYIInventoryBag*, Bag);
UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FYIOnInventoryBagClosed, UYIInventoryBag*, Bag);

UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FYIOnInventoryItemAdded, UYIInventoryBag*, Bag, int32, Index, FYIBagItem, Item);
UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FYIOnInventoryItemRemoved, UYIInventoryBag*, Bag, int32, Index, FYIBagItem, Item);
UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FYIOnInventoryItemMoved, UYIInventoryBag*, Bag, int32, Index, FIntPoint, NewPos);
UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FYIOnInventoryItemRotated, UYIInventoryBag*, Bag, int32, Index);
UDELEGATE()
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FYIOnInventoryItemTransferred, UYIInventoryBag*, Source, UYIInventoryBag*, Dest, int32, SourceIndex, int32, DestIndex);
