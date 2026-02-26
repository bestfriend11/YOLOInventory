#pragma once

#include "CoreMinimal.h"

struct FYIBagItem;
class UYIInventoryBag;
class UYIInventoryComponent;

struct FYIInventoryContainerRuntimeService
{
	static UYIInventoryBag* EnsureContainedBagForItem(UYIInventoryComponent& Inventory, FYIBagItem& InOutItem, const UYIInventoryBag* ParentBag);
	static bool TryOpenContainedBagInternal(UYIInventoryComponent& Inventory, UYIInventoryBag* ParentBag, int32 ItemIndex);
	static UYIInventoryBag* CloneBagTemplate(UYIInventoryComponent& Inventory, const UYIInventoryBag* TemplateBag);
};

