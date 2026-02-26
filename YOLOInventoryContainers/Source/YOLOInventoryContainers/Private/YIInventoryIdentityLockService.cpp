#include "YIInventoryIdentityLockService.h"

#include "YIInventoryBag.h"
#include "YIInventoryComponent.h"
#include "YIItemDefinition.h"

bool FYIInventoryIdentityLockService::GetBagItemIdentity(const UYIInventoryComponent& Inventory, const UYIInventoryBag* Bag, int32 ItemIndex, FYIInventoryItemRef& OutIdentity)
{
	(void)Inventory;
	OutIdentity = FYIInventoryItemRef();
	if (!Bag || !Bag->Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	const FYIBagItem& BagItem = Bag->Items[ItemIndex];
	OutIdentity.Bag.BagId = Bag->BagId;
	OutIdentity.Item.ItemInstanceId = BagItem.Item.InstanceId;
	OutIdentity.Item.LegacyStackKey = BagItem.Item.CustomStackKey;
	OutIdentity.Item.ItemCode = 0;
	if (UYIItemDefinition* Def = BagItem.Item.Definition.IsValid()
		? BagItem.Item.Definition.Get()
		: BagItem.Item.Definition.LoadSynchronous())
	{
		OutIdentity.Item.ItemCode = Def->UniqueCode;
	}

	return OutIdentity.Bag.BagId.IsValid() && (OutIdentity.Item.ItemInstanceId.IsValid() || OutIdentity.Item.LegacyStackKey != 0);
}

bool FYIInventoryIdentityLockService::IsBagItemLockedByIdentity(const UYIInventoryComponent& Inventory, const FYIInventoryItemRef& Identity)
{
	if (!Identity.Bag.BagId.IsValid())
	{
		return false;
	}

	return Inventory.LockedBagItems.ContainsByPredicate([&Identity](const FYIInventoryLockRef& Entry)
	{
		if (Entry.ItemRef.Bag.BagId != Identity.Bag.BagId)
		{
			return false;
		}
		if (Identity.Item.ItemInstanceId.IsValid() && Entry.ItemRef.Item.ItemInstanceId.IsValid())
		{
			return Entry.ItemRef.Item.ItemInstanceId == Identity.Item.ItemInstanceId;
		}
		return Identity.Item.LegacyStackKey != 0 && Entry.ItemRef.Item.LegacyStackKey == Identity.Item.LegacyStackKey;
	});
}

bool FYIInventoryIdentityLockService::SetBagItemLocked(UYIInventoryComponent& Inventory, UYIInventoryBag* Bag, int32 ItemIndex, bool bLocked)
{
	if (!Inventory.GetOwner() || !Inventory.GetOwner()->HasAuthority() || !Bag || !Bag->Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	Bag->EnsureBagId();

	FYIBagItem& MutableItem = Bag->Items[ItemIndex];
	if (!MutableItem.Item.InstanceId.IsValid())
	{
		MutableItem.Item.InstanceId = FGuid::NewGuid();
	}
	if (!MutableItem.Item.StackId.IsValid())
	{
		MutableItem.Item.StackId = FGuid::NewGuid();
	}
	if (MutableItem.Item.CustomStackKey == 0)
	{
		const int64 NewKey = (static_cast<int64>(FDateTime::UtcNow().GetTicks()) ^ static_cast<int64>(FMath::Rand())) & MAX_int64;
		MutableItem.Item.CustomStackKey = NewKey == 0 ? 1 : NewKey;
	}

	FYIInventoryItemRef Identity;
	if (!GetBagItemIdentity(Inventory, Bag, ItemIndex, Identity))
	{
		return false;
	}

	return SetBagItemLockedInternal(Inventory, Identity, bLocked);
}

bool FYIInventoryIdentityLockService::SetBagItemLockedByCoreRef(UYIInventoryComponent& Inventory, const FYIInventoryItemRef& ItemRef, bool bLocked)
{
	return SetBagItemLockedInternal(Inventory, ItemRef, bLocked);
}

bool FYIInventoryIdentityLockService::GetBagItemCoreRef(const UYIInventoryComponent& Inventory, UYIInventoryBag* Bag, int32 ItemIndex, FYIInventoryItemRef& OutItemRef)
{
	OutItemRef = FYIInventoryItemRef();
	return GetBagItemIdentity(Inventory, Bag, ItemIndex, OutItemRef);
}

bool FYIInventoryIdentityLockService::SetBagItemLockedInternal(UYIInventoryComponent& Inventory, const FYIInventoryItemRef& ItemRef, bool bLocked)
{
	if (!Inventory.GetOwner() || !Inventory.GetOwner()->HasAuthority() || !ItemRef.Bag.BagId.IsValid())
	{
		return false;
	}
	if (!ItemRef.Item.ItemInstanceId.IsValid() && ItemRef.Item.LegacyStackKey == 0)
	{
		return false;
	}

	const int32 ExistingIndex = Inventory.LockedBagItems.IndexOfByPredicate([&](const FYIInventoryLockRef& Entry)
	{
		if (Entry.ItemRef.Bag.BagId != ItemRef.Bag.BagId)
		{
			return false;
		}
		if (ItemRef.Item.ItemInstanceId.IsValid() && Entry.ItemRef.Item.ItemInstanceId.IsValid())
		{
			return Entry.ItemRef.Item.ItemInstanceId == ItemRef.Item.ItemInstanceId;
		}
		return ItemRef.Item.LegacyStackKey != 0 && Entry.ItemRef.Item.LegacyStackKey == ItemRef.Item.LegacyStackKey;
	});

	if (bLocked)
	{
		if (ExistingIndex != INDEX_NONE)
		{
			return true;
		}

		FYIInventoryLockRef NewEntry;
		NewEntry.ItemRef = ItemRef;
		Inventory.LockedBagItems.Add(NewEntry);
		Inventory.SyncNetState();
		return true;
	}

	if (ExistingIndex != INDEX_NONE)
	{
		Inventory.LockedBagItems.RemoveAt(ExistingIndex);
		Inventory.SyncNetState();
	}
	return true;
}

bool FYIInventoryIdentityLockService::IsBagItemLocked(const UYIInventoryComponent& Inventory, UYIInventoryBag* Bag, int32 ItemIndex)
{
	if (!Bag || !Bag->Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	FYIInventoryItemRef Identity;
	if (!GetBagItemIdentity(Inventory, Bag, ItemIndex, Identity))
	{
		return false;
	}

	return IsBagItemLockedByIdentity(Inventory, Identity);
}

bool FYIInventoryIdentityLockService::IsBagItemLockedByCoreRef(const UYIInventoryComponent& Inventory, const FYIInventoryItemRef& ItemRef)
{
	return IsBagItemLockedByIdentity(Inventory, ItemRef);
}

bool FYIInventoryIdentityLockService::FindItemIndexByInstanceId(const UYIInventoryComponent& Inventory, const UYIInventoryBag* Bag, const FGuid& InstanceId, int32& OutIndex)
{
	(void)Inventory;
	OutIndex = INDEX_NONE;
	if (!Bag || !InstanceId.IsValid())
	{
		return false;
	}

	for (int32 Index = 0; Index < Bag->Items.Num(); ++Index)
	{
		if (Bag->Items[Index].Item.InstanceId == InstanceId)
		{
			OutIndex = Index;
			return true;
		}
	}
	return false;
}

bool FYIInventoryIdentityLockService::FindContainerParentForBag(const UYIInventoryComponent& Inventory, const FGuid& ChildBagId, FGuid& OutParentBagId, FGuid& OutParentItemInstanceId)
{
	OutParentBagId.Invalidate();
	OutParentItemInstanceId.Invalidate();
	if (!ChildBagId.IsValid())
	{
		return false;
	}

	for (UYIInventoryBag* Bag : Inventory.Bags)
	{
		if (!Bag)
		{
			continue;
		}
		Bag->EnsureBagId();
		for (const FYIBagItem& Item : Bag->Items)
		{
			if (Item.Item.ContainedBagId == ChildBagId)
			{
				OutParentBagId = Bag->BagId;
				OutParentItemInstanceId = Item.Item.InstanceId;
				return true;
			}
		}
	}
	return false;
}

bool FYIInventoryIdentityLockService::IsBagDescendantOf(const UYIInventoryComponent& Inventory, const FGuid& CandidateBagId, const FGuid& PotentialAncestorBagId)
{
	if (!CandidateBagId.IsValid() || !PotentialAncestorBagId.IsValid())
	{
		return false;
	}
	if (CandidateBagId == PotentialAncestorBagId)
	{
		return true;
	}

	FGuid Current = CandidateBagId;
	for (int32 Depth = 0; Depth < 32; ++Depth)
	{
		FGuid ParentBagId;
		FGuid ParentItemId;
		if (!FindContainerParentForBag(Inventory, Current, ParentBagId, ParentItemId) || !ParentBagId.IsValid())
		{
			return false;
		}
		if (ParentBagId == PotentialAncestorBagId)
		{
			return true;
		}
		Current = ParentBagId;
	}
	return false;
}

