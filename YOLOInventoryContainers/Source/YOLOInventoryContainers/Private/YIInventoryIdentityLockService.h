#pragma once

#include "CoreMinimal.h"

struct FYIInventoryItemRef;
class UYIInventoryBag;
class UYIInventoryComponent;

struct FYIInventoryIdentityLockService
{
	static bool GetBagItemIdentity(const UYIInventoryComponent& Inventory, const UYIInventoryBag* Bag, int32 ItemIndex, FYIInventoryItemRef& OutIdentity);
	static bool IsBagItemLockedByIdentity(const UYIInventoryComponent& Inventory, const FYIInventoryItemRef& Identity);

	static bool SetBagItemLocked(UYIInventoryComponent& Inventory, UYIInventoryBag* Bag, int32 ItemIndex, bool bLocked);
	static bool SetBagItemLockedByCoreRef(UYIInventoryComponent& Inventory, const FYIInventoryItemRef& ItemRef, bool bLocked);
	static bool GetBagItemCoreRef(const UYIInventoryComponent& Inventory, UYIInventoryBag* Bag, int32 ItemIndex, FYIInventoryItemRef& OutItemRef);
	static bool SetBagItemLockedInternal(UYIInventoryComponent& Inventory, const FYIInventoryItemRef& ItemRef, bool bLocked);
	static bool IsBagItemLocked(const UYIInventoryComponent& Inventory, UYIInventoryBag* Bag, int32 ItemIndex);
	static bool IsBagItemLockedByCoreRef(const UYIInventoryComponent& Inventory, const FYIInventoryItemRef& ItemRef);

	static bool FindItemIndexByInstanceId(const UYIInventoryComponent& Inventory, const UYIInventoryBag* Bag, const FGuid& InstanceId, int32& OutIndex);
	static bool FindContainerParentForBag(const UYIInventoryComponent& Inventory, const FGuid& ChildBagId, FGuid& OutParentBagId, FGuid& OutParentItemInstanceId);
	static bool IsBagDescendantOf(const UYIInventoryComponent& Inventory, const FGuid& CandidateBagId, const FGuid& PotentialAncestorBagId);
};

