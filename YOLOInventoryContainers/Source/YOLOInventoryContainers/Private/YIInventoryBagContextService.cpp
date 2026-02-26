#include "YIInventoryBagContextService.h"

#include "YIInventoryBag.h"
#include "YIInventoryComponent.h"

UYIInventoryBag* FYIInventoryBagContextService::CreateBag(UYIInventoryComponent& Inventory, FName BagName, FIntPoint GridSize)
{
	UYIInventoryBag* NewBag = NewObject<UYIInventoryBag>(&Inventory);
	if (!NewBag)
	{
		return nullptr;
	}

	NewBag->EnsureBagId();
	NewBag->GridSize = GridSize;
	NewBag->DisplayName = FText::FromName(BagName);
	Inventory.Bags.Add(NewBag);

	if (!Inventory.EquippedBag)
	{
		OpenBag(Inventory, NewBag);
	}
	else if (!Inventory.ActiveBagId.IsValid())
	{
		Inventory.ActiveBagId = NewBag->BagId;
	}
	return NewBag;
}

void FYIInventoryBagContextService::OpenBag(UYIInventoryComponent& Inventory, UYIInventoryBag* Bag)
{
	if (!Bag)
	{
		return;
	}

	if (Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority() && Inventory.IsTemplateBag(Bag))
	{
		UYIInventoryBag* RuntimeBag = Inventory.CloneBagTemplate(Bag);
		if (!RuntimeBag)
		{
			return;
		}
		Bag = RuntimeBag;
	}

	if (!Inventory.Bags.Contains(Bag))
	{
		Inventory.Bags.Add(Bag);
	}

	if (Inventory.EquippedBag && Inventory.BagChangedHandle.IsValid())
	{
		Inventory.EquippedBag->OnChanged.Remove(Inventory.BagChangedHandle);
		Inventory.BagChangedHandle.Reset();
	}
	if (Inventory.BagEventSource)
	{
		Inventory.BagEventSource->OnItemAdded.RemoveDynamic(&Inventory, &UYIInventoryComponent::HandleBagItemAdded);
		Inventory.BagEventSource->OnItemRemoved.RemoveDynamic(&Inventory, &UYIInventoryComponent::HandleBagItemRemoved);
		Inventory.BagEventSource->OnItemMoved.RemoveDynamic(&Inventory, &UYIInventoryComponent::HandleBagItemMoved);
		Inventory.BagEventSource->OnItemRotated.RemoveDynamic(&Inventory, &UYIInventoryComponent::HandleBagItemRotated);
		Inventory.BagEventSource->OnItemTransferred.RemoveDynamic(&Inventory, &UYIInventoryComponent::HandleBagItemTransferred);
		Inventory.BagEventSource = nullptr;
	}

	Inventory.EquippedBag = Bag;
	if (Inventory.EquippedBag)
	{
		Inventory.EquippedBag->EnsureBagId();
		Inventory.ActiveBagId = Inventory.EquippedBag->BagId;
		Inventory.BagChangedHandle = Inventory.EquippedBag->OnChanged.AddUObject(&Inventory, &UYIInventoryComponent::SyncNetState);
		Inventory.BagEventSource = Inventory.EquippedBag;
		Inventory.BagEventSource->OnItemAdded.AddDynamic(&Inventory, &UYIInventoryComponent::HandleBagItemAdded);
		Inventory.BagEventSource->OnItemRemoved.AddDynamic(&Inventory, &UYIInventoryComponent::HandleBagItemRemoved);
		Inventory.BagEventSource->OnItemMoved.AddDynamic(&Inventory, &UYIInventoryComponent::HandleBagItemMoved);
		Inventory.BagEventSource->OnItemRotated.AddDynamic(&Inventory, &UYIInventoryComponent::HandleBagItemRotated);
		Inventory.BagEventSource->OnItemTransferred.AddDynamic(&Inventory, &UYIInventoryComponent::HandleBagItemTransferred);
	}

	if (Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority())
	{
		Inventory.SyncNetState();
	}
	Inventory.OnBagOpened.Broadcast(Bag);
}

void FYIInventoryBagContextService::CloseBag(UYIInventoryComponent& Inventory, UYIInventoryBag* Bag)
{
	if (!Bag)
	{
		return;
	}

	Bag->EnsureBagId();
	if (Inventory.EquippedBag == Bag)
	{
		if (Inventory.BagChangedHandle.IsValid())
		{
			Inventory.EquippedBag->OnChanged.Remove(Inventory.BagChangedHandle);
			Inventory.BagChangedHandle.Reset();
		}
		if (Inventory.BagEventSource)
		{
			Inventory.BagEventSource->OnItemAdded.RemoveDynamic(&Inventory, &UYIInventoryComponent::HandleBagItemAdded);
			Inventory.BagEventSource->OnItemRemoved.RemoveDynamic(&Inventory, &UYIInventoryComponent::HandleBagItemRemoved);
			Inventory.BagEventSource->OnItemMoved.RemoveDynamic(&Inventory, &UYIInventoryComponent::HandleBagItemMoved);
			Inventory.BagEventSource->OnItemRotated.RemoveDynamic(&Inventory, &UYIInventoryComponent::HandleBagItemRotated);
			Inventory.BagEventSource->OnItemTransferred.RemoveDynamic(&Inventory, &UYIInventoryComponent::HandleBagItemTransferred);
			Inventory.BagEventSource = nullptr;
		}
		Inventory.EquippedBag = nullptr;
		Inventory.ActiveBagId.Invalidate();
		if (Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority())
		{
			Inventory.SyncNetState();
		}
	}

	const int32 ClearedContextCount = ClearActiveContextsForBagId(Inventory, Bag->BagId);
	if (ClearedContextCount == 0)
	{
		Inventory.OnBagClosed.Broadcast(Bag);
	}
}

UYIInventoryBag* FYIInventoryBagContextService::GetBag(const UYIInventoryComponent& Inventory)
{
	auto ResolvePrimaryBag = [&Inventory]() -> UYIInventoryBag*
	{
		if (Inventory.ActiveBagId.IsValid())
		{
			if (UYIInventoryBag* ActiveBag = GetBagById(Inventory, Inventory.ActiveBagId))
			{
				return ActiveBag;
			}
		}
		for (UYIInventoryBag* Bag : Inventory.Bags)
		{
			if (Bag)
			{
				return Bag;
			}
		}
		return nullptr;
	};

	if (Inventory.GetOwner() && Inventory.GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		if (Inventory.EquippedBag)
		{
			return Inventory.EquippedBag.Get();
		}
		return ResolvePrimaryBag();
	}

	if (Inventory.ClientPreviewBag)
	{
		return Inventory.ClientPreviewBag;
	}
	if (Inventory.EquippedBag)
	{
		return Inventory.EquippedBag.Get();
	}
	return ResolvePrimaryBag();
}

FGuid FYIInventoryBagContextService::GetActiveContextBagId(const UYIInventoryComponent& Inventory, FGameplayTag ContextTag)
{
	if (!ContextTag.IsValid())
	{
		return FGuid();
	}

	for (const FYIActiveBagContextEntry& Entry : Inventory.ActiveBagContexts)
	{
		if (Entry.ContextTag == ContextTag)
		{
			return Entry.BagId;
		}
	}
	return FGuid();
}

UYIInventoryBag* FYIInventoryBagContextService::GetActiveContextBag(const UYIInventoryComponent& Inventory, FGameplayTag ContextTag)
{
	const FGuid ContextBagId = GetActiveContextBagId(Inventory, ContextTag);
	return ContextBagId.IsValid() ? GetBagById(Inventory, ContextBagId) : nullptr;
}

UYIInventoryBag* FYIInventoryBagContextService::GetBagById(const UYIInventoryComponent& Inventory, const FGuid& BagId)
{
	if (!BagId.IsValid())
	{
		return nullptr;
	}

	for (UYIInventoryBag* Bag : Inventory.Bags)
	{
		if (!Bag)
		{
			continue;
		}
		if (!Bag->BagId.IsValid())
		{
			Bag->EnsureBagId();
		}
		if (Bag->BagId == BagId)
		{
			return Bag;
		}
	}

	if (Inventory.ClientPreviewBag && Inventory.ClientPreviewBag->BagId == BagId)
	{
		return Inventory.ClientPreviewBag;
	}
	if (UYIInventoryBag* ContextPreview = Inventory.FindClientContextPreviewBagById(BagId))
	{
		return ContextPreview;
	}
	return nullptr;
}

UYIInventoryBag* FYIInventoryBagContextService::GetBagByRoleTag(const UYIInventoryComponent& Inventory, FGameplayTag BagRoleTag)
{
	if (!BagRoleTag.IsValid())
	{
		return nullptr;
	}

	for (UYIInventoryBag* Bag : Inventory.Bags)
	{
		if (Bag && Bag->BagRoleTag.IsValid() && Bag->BagRoleTag.MatchesTag(BagRoleTag))
		{
			return Bag;
		}
	}
	return nullptr;
}

UYIInventoryBag* FYIInventoryBagContextService::GetBagByDisplayName(const UYIInventoryComponent& Inventory, FName BagName)
{
	if (BagName.IsNone())
	{
		return nullptr;
	}

	for (UYIInventoryBag* Bag : Inventory.Bags)
	{
		if (Bag && Bag->DisplayName.EqualTo(FText::FromName(BagName)))
		{
			return Bag;
		}
	}
	return nullptr;
}

int32 FYIInventoryBagContextService::FindActiveContextIndex(const UYIInventoryComponent& Inventory, FGameplayTag ContextTag)
{
	if (!ContextTag.IsValid())
	{
		return INDEX_NONE;
	}

	for (int32 Index = 0; Index < Inventory.ActiveBagContexts.Num(); ++Index)
	{
		if (Inventory.ActiveBagContexts[Index].ContextTag == ContextTag)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

bool FYIInventoryBagContextService::SetActiveBagById(UYIInventoryComponent& Inventory, const FGuid& InBagId)
{
	if (!InBagId.IsValid())
	{
		return false;
	}

	if (Inventory.GetOwner() && !Inventory.GetOwner()->HasAuthority())
	{
		Inventory.ServerSetActiveBagById(InBagId);
		return true;
	}

	if (UYIInventoryBag* Bag = GetBagById(Inventory, InBagId))
	{
		OpenBag(Inventory, Bag);
		return true;
	}
	return false;
}

bool FYIInventoryBagContextService::SetActiveBagByRoleTag(UYIInventoryComponent& Inventory, FGameplayTag InBagRoleTag)
{
	if (!InBagRoleTag.IsValid())
	{
		return false;
	}

	if (UYIInventoryBag* Bag = GetBagByRoleTag(Inventory, InBagRoleTag))
	{
		OpenBag(Inventory, Bag);
		return true;
	}
	return false;
}

bool FYIInventoryBagContextService::OpenContainedBagAtIndex(UYIInventoryComponent& Inventory, int32 ItemIndex)
{
	UYIInventoryBag* ActiveBag = GetBag(Inventory);
	if (!ActiveBag || !ActiveBag->Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	if (Inventory.GetOwner() && !Inventory.GetOwner()->HasAuthority())
	{
		const FYIBagItem& Item = ActiveBag->Items[ItemIndex];
		if (!Item.Item.InstanceId.IsValid())
		{
			return false;
		}
		const FGuid ParentBagId = ActiveBag->BagId.IsValid() ? ActiveBag->BagId : Inventory.ActiveBagId;
		Inventory.ServerOpenContainedBagByInstance(ParentBagId, Item.Item.InstanceId);
		return true;
	}

	return Inventory.TryOpenContainedBagInternal(ActiveBag, ItemIndex);
}

UYIInventoryBag* FYIInventoryBagContextService::EnsureContainedBagAtIndex(UYIInventoryComponent& Inventory, int32 ItemIndex)
{
	UYIInventoryBag* ActiveBag = GetBag(Inventory);
	if (!ActiveBag || !ActiveBag->Items.IsValidIndex(ItemIndex))
	{
		return nullptr;
	}

	UYIInventoryBag* Contained = Inventory.EnsureContainedBagForItem(ActiveBag->Items[ItemIndex], ActiveBag);
	if (Contained && Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority())
	{
		Inventory.SyncNetState();
	}
	return Contained;
}

bool FYIInventoryBagContextService::OpenParentBag(UYIInventoryComponent& Inventory)
{
	UYIInventoryBag* ActiveBag = GetBag(Inventory);
	if (!ActiveBag)
	{
		return false;
	}

	const FGuid ChildBagId = ActiveBag->BagId;
	if (!ChildBagId.IsValid())
	{
		return false;
	}

	if (Inventory.GetOwner() && !Inventory.GetOwner()->HasAuthority())
	{
		Inventory.ServerOpenParentBag(ChildBagId);
		return true;
	}

	FGuid ParentBagId;
	FGuid ParentItemId;
	if (!Inventory.FindContainerParentForBag(ChildBagId, ParentBagId, ParentItemId))
	{
		return false;
	}
	return SetActiveBagById(Inventory, ParentBagId);
}

bool FYIInventoryBagContextService::SetActiveContextBagById(UYIInventoryComponent& Inventory, FGameplayTag ContextTag, const FGuid& InBagId)
{
	if (!ContextTag.IsValid() || !InBagId.IsValid())
	{
		return false;
	}

	if (Inventory.GetOwner() && !Inventory.GetOwner()->HasAuthority())
	{
		Inventory.ServerSetActiveContextBagById(ContextTag, InBagId);
		return true;
	}

	if (UYIInventoryBag* Bag = GetBagById(Inventory, InBagId))
	{
		Bag->EnsureBagId();
		UYIInventoryBag* PreviousBag = nullptr;
		const int32 ExistingIndex = FindActiveContextIndex(Inventory, ContextTag);
		if (ExistingIndex != INDEX_NONE)
		{
			if (Inventory.ActiveBagContexts[ExistingIndex].BagId != Bag->BagId)
			{
				PreviousBag = GetBagById(Inventory, Inventory.ActiveBagContexts[ExistingIndex].BagId);
			}
			Inventory.ActiveBagContexts[ExistingIndex].BagId = Bag->BagId;
		}
		else
		{
			FYIActiveBagContextEntry& NewEntry = Inventory.ActiveBagContexts.AddDefaulted_GetRef();
			NewEntry.ContextTag = ContextTag;
			NewEntry.BagId = Bag->BagId;
		}
		if (Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority())
		{
			Inventory.SyncNetState();
		}
		if (PreviousBag && PreviousBag != Bag)
		{
			Inventory.OnBagClosed.Broadcast(PreviousBag);
		}
		Inventory.OnBagOpened.Broadcast(Bag);
		return true;
	}
	return false;
}

bool FYIInventoryBagContextService::SetActiveContextBagByRoleTag(UYIInventoryComponent& Inventory, FGameplayTag ContextTag, FGameplayTag InBagRoleTag)
{
	if (!ContextTag.IsValid() || !InBagRoleTag.IsValid())
	{
		return false;
	}

	if (UYIInventoryBag* Bag = GetBagByRoleTag(Inventory, InBagRoleTag))
	{
		Bag->EnsureBagId();
		return SetActiveContextBagById(Inventory, ContextTag, Bag->BagId);
	}
	return false;
}

bool FYIInventoryBagContextService::ClearActiveContextBag(UYIInventoryComponent& Inventory, FGameplayTag ContextTag)
{
	if (!ContextTag.IsValid())
	{
		return false;
	}

	if (Inventory.GetOwner() && !Inventory.GetOwner()->HasAuthority())
	{
		Inventory.ServerClearActiveContextBag(ContextTag);
		return true;
	}

	const int32 ExistingIndex = FindActiveContextIndex(Inventory, ContextTag);
	if (ExistingIndex == INDEX_NONE)
	{
		return false;
	}

	UYIInventoryBag* ClosedBag = GetBagById(Inventory, Inventory.ActiveBagContexts[ExistingIndex].BagId);
	Inventory.ActiveBagContexts.RemoveAt(ExistingIndex);

	if (Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority())
	{
		Inventory.SyncNetState();
	}
	if (ClosedBag)
	{
		Inventory.OnBagClosed.Broadcast(ClosedBag);
	}
	return true;
}

int32 FYIInventoryBagContextService::ClearActiveContextsForBagId(UYIInventoryComponent& Inventory, const FGuid& InBagId)
{
	if (!InBagId.IsValid())
	{
		return 0;
	}

	int32 RemovedCount = 0;
	for (int32 Index = Inventory.ActiveBagContexts.Num() - 1; Index >= 0; --Index)
	{
		if (Inventory.ActiveBagContexts[Index].BagId == InBagId)
		{
			Inventory.ActiveBagContexts.RemoveAtSwap(Index, 1, EAllowShrinking::No);
			++RemovedCount;
		}
	}

	if (RemovedCount > 0)
	{
		if (Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority())
		{
			Inventory.SyncNetState();
		}
		if (UYIInventoryBag* ClosedBag = GetBagById(Inventory, InBagId))
		{
			Inventory.OnBagClosed.Broadcast(ClosedBag);
		}
	}

	return RemovedCount;
}

void FYIInventoryBagContextService::GetReplicatedBagDescriptors(const UYIInventoryComponent& Inventory, TArray<FYINetBagDescriptor>& OutDescriptors)
{
	OutDescriptors = Inventory.NetBagDescriptors;
}

bool FYIInventoryBagContextService::RemoveBag(UYIInventoryComponent& Inventory, UYIInventoryBag* Bag)
{
	if (!Bag)
	{
		return false;
	}

	Bag->EnsureBagId();
	Inventory.LockedBagItems.RemoveAllSwap([Bag](const FYIInventoryLockRef& Entry)
	{
		return Entry.ItemRef.Bag.BagId == Bag->BagId;
	}, EAllowShrinking::No);

	if (Inventory.ActiveBagId.IsValid() && Inventory.ActiveBagId == Bag->BagId)
	{
		Inventory.ActiveBagId.Invalidate();
	}
	ClearActiveContextsForBagId(Inventory, Bag->BagId);

	const bool bWasEquipped = (Inventory.EquippedBag == Bag);
	if (bWasEquipped)
	{
		CloseBag(Inventory, Bag);
	}

	const int32 Index = Inventory.Bags.IndexOfByKey(Bag);
	if (Index == INDEX_NONE)
	{
		return false;
	}

	Inventory.Bags.RemoveAt(Index);
	if (bWasEquipped)
	{
		if (UYIInventoryBag* NextBag = GetBag(Inventory))
		{
			OpenBag(Inventory, NextBag);
		}
	}
	return true;
}

