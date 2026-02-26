#include "YIInventoryComponent.h"
#include "YIInventoryBagContextService.h"
#include "YIInventoryContainerRuntimeService.h"
#include "YIInventoryMirrorService.h"
#include "YIInventoryMutationService.h"
#include "YIItemSchemaResolver.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "YIItemInstanceFragmentAccess.h"
#include "Net/UnrealNetwork.h"
#include "UObject/Package.h"
#include "YIItemNetTypes.h"
#include "YIDebugLibrary.h"

UYIInventoryComponent::UYIInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

UYIInventoryBag* UYIInventoryComponent::CreateBag(FName BagName, FIntPoint GridSize)
{
	return FYIInventoryBagContextService::CreateBag(*this, BagName, GridSize);
}

void UYIInventoryComponent::OpenBag(UYIInventoryBag* Bag)
{
	FYIInventoryBagContextService::OpenBag(*this, Bag);
}

void UYIInventoryComponent::CloseBag(UYIInventoryBag* Bag)
{
	FYIInventoryBagContextService::CloseBag(*this, Bag);
}

UYIInventoryBag* UYIInventoryComponent::GetBag() const
{
	return FYIInventoryBagContextService::GetBag(*this);
}

FGuid UYIInventoryComponent::GetActiveContextBagId(FGameplayTag ContextTag) const
{
	return FYIInventoryBagContextService::GetActiveContextBagId(*this, ContextTag);
}

UYIInventoryBag* UYIInventoryComponent::GetActiveContextBag(FGameplayTag ContextTag) const
{
	return FYIInventoryBagContextService::GetActiveContextBag(*this, ContextTag);
}

UYIInventoryBag* UYIInventoryComponent::GetBagById(const FGuid& BagId) const
{
	return FYIInventoryBagContextService::GetBagById(*this, BagId);
}

UYIInventoryBag* UYIInventoryComponent::FindClientContextPreviewBagById(const FGuid& BagId) const
{
	return FYIInventoryMirrorService::FindClientContextPreviewBagById(*this, BagId);
}

UYIInventoryBag* UYIInventoryComponent::FindOrCreateClientContextPreviewBagById(const FGuid& BagId)
{
	return FYIInventoryMirrorService::FindOrCreateClientContextPreviewBagById(*this, BagId);
}

void UYIInventoryComponent::RebuildClientPreviewBagFromNet(UYIInventoryBag* TargetBag, const TArray<FYINetBagItem>& InItems, const FIntPoint& InGridSize, const FGuid& InBagId)
{
	FYIInventoryMirrorService::RebuildClientPreviewBagFromNet(*this, TargetBag, InItems, InGridSize, InBagId);
}

int32 UYIInventoryComponent::FindActiveContextIndex(FGameplayTag ContextTag) const
{
	return FYIInventoryBagContextService::FindActiveContextIndex(*this, ContextTag);
}

bool UYIInventoryComponent::SetActiveBagById(const FGuid& InBagId)
{
	return FYIInventoryBagContextService::SetActiveBagById(*this, InBagId);
}

bool UYIInventoryComponent::SetActiveBagByRoleTag(FGameplayTag InBagRoleTag)
{
	return FYIInventoryBagContextService::SetActiveBagByRoleTag(*this, InBagRoleTag);
}

bool UYIInventoryComponent::OpenContainedBagAtIndex(int32 ItemIndex)
{
	return FYIInventoryBagContextService::OpenContainedBagAtIndex(*this, ItemIndex);
}

UYIInventoryBag* UYIInventoryComponent::EnsureContainedBagAtIndex(int32 ItemIndex)
{
	return FYIInventoryBagContextService::EnsureContainedBagAtIndex(*this, ItemIndex);
}

bool UYIInventoryComponent::OpenParentBag()
{
	return FYIInventoryBagContextService::OpenParentBag(*this);
}

bool UYIInventoryComponent::SetActiveContextBagById(FGameplayTag ContextTag, const FGuid& InBagId)
{
	return FYIInventoryBagContextService::SetActiveContextBagById(*this, ContextTag, InBagId);
}

bool UYIInventoryComponent::SetActiveContextBagByRoleTag(FGameplayTag ContextTag, FGameplayTag InBagRoleTag)
{
	return FYIInventoryBagContextService::SetActiveContextBagByRoleTag(*this, ContextTag, InBagRoleTag);
}

bool UYIInventoryComponent::ClearActiveContextBag(FGameplayTag ContextTag)
{
	return FYIInventoryBagContextService::ClearActiveContextBag(*this, ContextTag);
}

int32 UYIInventoryComponent::ClearActiveContextsForBagId(const FGuid& InBagId)
{
	return FYIInventoryBagContextService::ClearActiveContextsForBagId(*this, InBagId);
}

void UYIInventoryComponent::ServerSetActiveBagById_Implementation(const FGuid& InBagId)
{
	SetActiveBagById(InBagId);
}

void UYIInventoryComponent::ServerSetActiveContextBagById_Implementation(FGameplayTag ContextTag, const FGuid& InBagId)
{
	SetActiveContextBagById(ContextTag, InBagId);
}

void UYIInventoryComponent::ServerClearActiveContextBag_Implementation(FGameplayTag ContextTag)
{
	ClearActiveContextBag(ContextTag);
}

void UYIInventoryComponent::ServerOpenContainedBagByInstance_Implementation(const FGuid& ParentBagId, const FGuid& ParentItemInstanceId)
{
	if (!ParentBagId.IsValid() || !ParentItemInstanceId.IsValid())
	{
		return;
	}

	UYIInventoryBag* ParentBag = GetBagById(ParentBagId);
	if (!ParentBag)
	{
		return;
	}

	int32 ItemIndex = INDEX_NONE;
	if (!FindItemIndexByInstanceId(ParentBag, ParentItemInstanceId, ItemIndex))
	{
		return;
	}

	TryOpenContainedBagInternal(ParentBag, ItemIndex);
}

void UYIInventoryComponent::ServerOpenParentBag_Implementation(const FGuid& ChildBagId)
{
	FGuid ParentBagId;
	FGuid ParentItemId;
	if (!FindContainerParentForBag(ChildBagId, ParentBagId, ParentItemId))
	{
		return;
	}
	SetActiveBagById(ParentBagId);
}

UYIInventoryBag* UYIInventoryComponent::GetBagByRoleTag(FGameplayTag BagRoleTag) const
{
	return FYIInventoryBagContextService::GetBagByRoleTag(*this, BagRoleTag);
}

UYIInventoryBag* UYIInventoryComponent::GetBagByDisplayName(FName BagName) const
{
	return FYIInventoryBagContextService::GetBagByDisplayName(*this, BagName);
}

bool UYIInventoryComponent::GetBagItemIdentity(const UYIInventoryBag* Bag, int32 ItemIndex, FYIInventoryItemRef& OutIdentity) const
{
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

bool UYIInventoryComponent::IsBagItemLockedByIdentity(const FYIInventoryItemRef& Identity) const
{
	if (!Identity.Bag.BagId.IsValid())
	{
		return false;
	}

	return LockedBagItems.ContainsByPredicate([&Identity](const FYIInventoryLockRef& Entry)
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

bool UYIInventoryComponent::SetBagItemLocked(UYIInventoryBag* Bag, int32 ItemIndex, bool bLocked)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Bag || !Bag->Items.IsValidIndex(ItemIndex))
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
	if (!GetBagItemIdentity(Bag, ItemIndex, Identity))
	{
		return false;
	}

	return SetBagItemLockedInternal(Identity, bLocked);
}

bool UYIInventoryComponent::SetBagItemLockedByCoreRef(const FYIInventoryItemRef& ItemRef, bool bLocked)
{
	return SetBagItemLockedInternal(ItemRef, bLocked);
}

bool UYIInventoryComponent::GetBagItemCoreRef(UYIInventoryBag* Bag, int32 ItemIndex, FYIInventoryItemRef& OutItemRef) const
{
	OutItemRef = FYIInventoryItemRef();

	if (!GetBagItemIdentity(Bag, ItemIndex, OutItemRef))
	{
		return false;
	}
	return true;
}

bool UYIInventoryComponent::SetBagItemLockedInternal(const FYIInventoryItemRef& ItemRef, bool bLocked)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !ItemRef.Bag.BagId.IsValid())
	{
		return false;
	}
	if (!ItemRef.Item.ItemInstanceId.IsValid() && ItemRef.Item.LegacyStackKey == 0)
	{
		return false;
	}

	const int32 ExistingIndex = LockedBagItems.IndexOfByPredicate([&](const FYIInventoryLockRef& Entry)
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
		LockedBagItems.Add(NewEntry);
		SyncNetState();
		return true;
	}

	if (ExistingIndex != INDEX_NONE)
	{
		LockedBagItems.RemoveAt(ExistingIndex);
		SyncNetState();
	}
	return true;
}

bool UYIInventoryComponent::IsBagItemLocked(UYIInventoryBag* Bag, int32 ItemIndex) const
{
	if (!Bag || !Bag->Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	FYIInventoryItemRef Identity;
	if (!GetBagItemIdentity(Bag, ItemIndex, Identity))
	{
		return false;
	}

	return IsBagItemLockedByIdentity(Identity);
}

bool UYIInventoryComponent::IsBagItemLockedByCoreRef(const FYIInventoryItemRef& ItemRef) const
{
	return IsBagItemLockedByIdentity(ItemRef);
}

bool UYIInventoryComponent::FindItemIndexByInstanceId(const UYIInventoryBag* Bag, const FGuid& InstanceId, int32& OutIndex) const
{
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

bool UYIInventoryComponent::FindContainerParentForBag(const FGuid& ChildBagId, FGuid& OutParentBagId, FGuid& OutParentItemInstanceId) const
{
	OutParentBagId.Invalidate();
	OutParentItemInstanceId.Invalidate();
	if (!ChildBagId.IsValid())
	{
		return false;
	}

	for (UYIInventoryBag* Bag : Bags)
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

bool UYIInventoryComponent::IsBagDescendantOf(const FGuid& CandidateBagId, const FGuid& PotentialAncestorBagId) const
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
		if (!FindContainerParentForBag(Current, ParentBagId, ParentItemId) || !ParentBagId.IsValid())
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

UYIInventoryBag* UYIInventoryComponent::EnsureContainedBagForItem(FYIBagItem& InOutItem, const UYIInventoryBag* ParentBag)
{
	return FYIInventoryContainerRuntimeService::EnsureContainedBagForItem(*this, InOutItem, ParentBag);
}

bool UYIInventoryComponent::TryOpenContainedBagInternal(UYIInventoryBag* ParentBag, int32 ItemIndex)
{
	return FYIInventoryContainerRuntimeService::TryOpenContainedBagInternal(*this, ParentBag, ItemIndex);
}

void UYIInventoryComponent::GetReplicatedBagDescriptors(TArray<FYINetBagDescriptor>& OutDescriptors) const
{
	FYIInventoryBagContextService::GetReplicatedBagDescriptors(*this, OutDescriptors);
}

bool UYIInventoryComponent::RemoveBag(UYIInventoryBag* Bag)
{
	return FYIInventoryBagContextService::RemoveBag(*this, Bag);
}

bool UYIInventoryComponent::AddItemToBag(UYIInventoryBag* Bag, TSoftObjectPtr<UYIItemDefinition> ItemDef, int32 Count)
{
	if (!Bag)
	{
		return false;
	}
	// Auto-clone template to avoid corrupting assets
	if (GetOwner() && GetOwner()->HasAuthority() && IsTemplateBag(Bag))
	{
		Bag = CloneBagTemplate(Bag);
		// Reopen to hook delegates
		OpenBag(Bag);
	}
	if (!ItemDef.IsValid())
	{
		ItemDef.LoadSynchronous();
		if (!ItemDef.IsValid())
		{
			return false;
		}
	}
	UYIItemDefinition* Def = ItemDef.Get();
	if (!Def) { Def = ItemDef.LoadSynchronous(); if (!Def) return false; }

	FYIBagItem New;
	New.Item.Definition = ItemDef;
	New.Item.Count = FMath::Max(1, Count);
	New.Size = YIItemSchema::GetDefaultSize(Def);
	if (YIItemSchema::IsContainerItem(Def))
	{
		New.Item.Count = 1;
	}
	int32 Idx = Bag->AddBagItem(New);
	if (Idx != INDEX_NONE && Bag->Items.IsValidIndex(Idx))
	{
		EnsureContainedBagForItem(Bag->Items[Idx], Bag);
	}
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		SyncNetState();
	}
	return Idx != INDEX_NONE;
}

void UYIInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// Normalize bag setup on authority so EquippedBag/Bags/OpenBag stay connected.
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		TMap<const UYIInventoryBag*, UYIInventoryBag*> RuntimeCloneMap;
		for (int32 Index = 0; Index < Bags.Num(); ++Index)
		{
			UYIInventoryBag* Bag = Bags[Index];
			if (!Bag)
			{
				continue;
			}
			if (IsTemplateBag(Bag))
			{
				if (UYIInventoryBag* RuntimeBag = CloneBagTemplate(Bag))
				{
					RuntimeCloneMap.Add(Bag, RuntimeBag);
					Bags[Index] = RuntimeBag;
				}
			}
		}

		Bags.RemoveAllSwap([](UYIInventoryBag* Bag) { return Bag == nullptr; }, EAllowShrinking::No);

		if (EquippedBag)
		{
			if (UYIInventoryBag** RuntimeFromList = RuntimeCloneMap.Find(EquippedBag))
			{
				EquippedBag = *RuntimeFromList;
			}
			else if (IsTemplateBag(EquippedBag))
			{
				EquippedBag = CloneBagTemplate(EquippedBag);
			}
		}

		if (!EquippedBag)
		{
			EquippedBag = GetBag();
		}

		if (EquippedBag)
		{
			EquippedBag->EnsureBagId();
			ActiveBagId = EquippedBag->BagId;
			if (!Bags.Contains(EquippedBag))
			{
				Bags.Insert(EquippedBag, 0);
			}

			// Materialize nested runtime bags for any pre-authored container items copied from bag templates
			// so equipped/net payloads immediately carry valid ContainedBagId values.
			for (int32 BagIndex = 0; BagIndex < Bags.Num(); ++BagIndex)
			{
				UYIInventoryBag* RuntimeBag = Bags[BagIndex];
				if (!RuntimeBag)
				{
					continue;
				}
				for (int32 ItemIndex = 0; ItemIndex < RuntimeBag->Items.Num(); ++ItemIndex)
				{
					EnsureContainedBagForItem(RuntimeBag->Items[ItemIndex], RuntimeBag);
				}
			}

			OpenBag(EquippedBag); // binds events + net sync consistently
		}
	}
}

UYIInventoryBag* UYIInventoryComponent::CloneBagTemplate(const UYIInventoryBag* TemplateBag)
{
	return FYIInventoryContainerRuntimeService::CloneBagTemplate(*this, TemplateBag);
}

bool UYIInventoryComponent::IsTemplateBag(const UYIInventoryBag* Bag) const
{
	if (!Bag) return false;
	const UPackage* Package = Bag->GetOutermost();
	const bool bIsAssetPkg = Package && !Package->HasAnyPackageFlags(PKG_PlayInEditor | PKG_ContainsMapData);
	const bool bPublic = Bag->HasAnyFlags(RF_Public);
	return bIsAssetPkg || bPublic;
}

void UYIInventoryComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (EquippedBag && BagChangedHandle.IsValid())
	{
		EquippedBag->OnChanged.Remove(BagChangedHandle);
		BagChangedHandle.Reset();
	}
	if (BagEventSource)
	{
		BagEventSource->OnItemAdded.RemoveDynamic(this, &UYIInventoryComponent::HandleBagItemAdded);
		BagEventSource->OnItemRemoved.RemoveDynamic(this, &UYIInventoryComponent::HandleBagItemRemoved);
		BagEventSource->OnItemMoved.RemoveDynamic(this, &UYIInventoryComponent::HandleBagItemMoved);
		BagEventSource->OnItemRotated.RemoveDynamic(this, &UYIInventoryComponent::HandleBagItemRotated);
		BagEventSource->OnItemTransferred.RemoveDynamic(this, &UYIInventoryComponent::HandleBagItemTransferred);
		BagEventSource = nullptr;
	}
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UYIInventoryComponent::SyncNetState()
{
	FYIInventoryMirrorService::SyncNetState(*this);
}

void UYIInventoryComponent::OnRep_NetBag()
{
	FYIInventoryMirrorService::OnRep_NetBag(*this);
}

void UYIInventoryComponent::OnRep_NetBagDescriptors()
{
	FYIInventoryMirrorService::OnRep_NetBagDescriptors(*this);
}

void UYIInventoryComponent::OnRep_ActiveBagContexts()
{
	FYIInventoryMirrorService::OnRep_ActiveBagContexts(*this);
}

void UYIInventoryComponent::OnRep_NetContextBagMirrors()
{
	FYIInventoryMirrorService::OnRep_NetContextBagMirrors(*this);
}

void UYIInventoryComponent::OnRep_LockedBagItems()
{
	FYIInventoryMirrorService::OnRep_LockedBagItems(*this);
}

void UYIInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, NetBagItems, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, NetBagGridSize, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, NetBagDescriptors, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, ActiveBagId, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, ActiveBagContexts, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, NetContextBagMirrors, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, LockedBagItems, COND_OwnerOnly);
}

// -------- Net-safe bag mutations --------

bool UYIInventoryComponent::MoveItem(int32 Index, FIntPoint NewPos)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (EquippedBag && IsBagItemLocked(EquippedBag, Index))
		{
			return false;
		}
		if (EquippedBag && EquippedBag->MoveItem(Index, NewPos))
		{
			SyncNetState();
			return true;
		}
		return false;
	}
	ServerMoveItem(Index, NewPos);
	return true; // optimistic; OnRep_NetBag will update preview
}

void UYIInventoryComponent::ServerMoveItem_Implementation(int32 Index, FIntPoint NewPos)
{
	MoveItem(Index, NewPos); // will take authority branch
}

bool UYIInventoryComponent::MoveItemInBag(const FGuid& BagId, const FGuid& ItemInstanceId, FIntPoint NewPos)
{
	return FYIInventoryMutationService::MoveItemInBag(*this, BagId, ItemInstanceId, NewPos);
}

void UYIInventoryComponent::ServerMoveItemInBag_Implementation(const FGuid& BagId, const FGuid& ItemInstanceId, FIntPoint NewPos)
{
	MoveItemInBag(BagId, ItemInstanceId, NewPos);
}

bool UYIInventoryComponent::MoveItemInBagAtCell(const FGuid& BagId, const FGuid& ItemInstanceId, FIntPoint DestCell, bool bAllowSingleOverlapSwap)
{
	return FYIInventoryMutationService::MoveItemInBagAtCell(*this, BagId, ItemInstanceId, DestCell, bAllowSingleOverlapSwap);
}

void UYIInventoryComponent::ServerMoveItemInBagAtCell_Implementation(const FGuid& BagId, const FGuid& ItemInstanceId, FIntPoint DestCell, bool bAllowSingleOverlapSwap)
{
	MoveItemInBagAtCell(BagId, ItemInstanceId, DestCell, bAllowSingleOverlapSwap);
}

bool UYIInventoryComponent::RotateItem(int32 Index)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (EquippedBag && IsBagItemLocked(EquippedBag, Index))
		{
			return false;
		}
		if (EquippedBag && EquippedBag->RotateItem(Index))
		{
			SyncNetState();
			return true;
		}
		return false;
	}
	ServerRotateItem(Index);
	return true;
}

void UYIInventoryComponent::ServerRotateItem_Implementation(int32 Index)
{
	RotateItem(Index); // authority branch
}

bool UYIInventoryComponent::RotateItemInBag(const FGuid& BagId, const FGuid& ItemInstanceId)
{
	return FYIInventoryMutationService::RotateItemInBag(*this, BagId, ItemInstanceId);
}

void UYIInventoryComponent::ServerRotateItemInBag_Implementation(const FGuid& BagId, const FGuid& ItemInstanceId)
{
	RotateItemInBag(BagId, ItemInstanceId);
}

int32 UYIInventoryComponent::AddBagItem(const FYIBagItem& Item)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (EquippedBag)
		{
			FYIBagItem MutableItem = Item;
			if (UYIItemDefinition* Def = MutableItem.Item.Definition.IsValid()
				? MutableItem.Item.Definition.Get()
				: MutableItem.Item.Definition.LoadSynchronous())
			{
				if (YIItemSchema::IsContainerItem(Def))
				{
					MutableItem.Item.Count = 1;
				}
			}

			if (MutableItem.Item.ContainedBagId.IsValid() &&
				EquippedBag->BagId.IsValid() &&
				IsBagDescendantOf(EquippedBag->BagId, MutableItem.Item.ContainedBagId))
			{
				return INDEX_NONE;
			}

			int32 Idx = EquippedBag->AddBagItem(MutableItem);
			if (Idx != INDEX_NONE && EquippedBag->Items.IsValidIndex(Idx))
			{
				EnsureContainedBagForItem(EquippedBag->Items[Idx], EquippedBag);
				SyncNetState();
			}
			return Idx;
		}
		return INDEX_NONE;
	}
	// Client: send net-safe version
	FYIItemInstance RuntimeItem = Item.Item;
	FYIItemInstanceNet Net;
	Net.Definition = RuntimeItem.Definition;
	Net.Count = RuntimeItem.Count;
	Net.InstanceId = RuntimeItem.InstanceId;
	Net.StackId = RuntimeItem.StackId;
	Net.CustomStackKey = RuntimeItem.CustomStackKey;
	Net.ContainedBagId = RuntimeItem.ContainedBagId;
	Net.bRotated = RuntimeItem.bRotated;
	YIItemInstanceFragments::ExportNetFragmentPayload(RuntimeItem, Net.Fragments);
	ServerAddBagItem(Net, Item.Pos, Item.Size);
	return 0; // optimistic dummy index; OnRep will refresh actual layout
}

static FYIItemInstance NetToFull(const FYIItemInstanceNet& Net)
{
	FYIItemInstance Out;
	Out.Definition = Net.Definition;
	Out.Count = Net.Count;
	Out.InstanceId = Net.InstanceId.IsValid() ? Net.InstanceId : FGuid::NewGuid();
	Out.StackId = Net.StackId.IsValid() ? Net.StackId : FGuid::NewGuid();
	Out.CustomStackKey = Net.CustomStackKey;
	Out.ContainedBagId = Net.ContainedBagId;
	Out.bRotated = Net.bRotated;
	YIItemInstanceFragments::ImportNetFragmentPayload(Out, Net.Fragments);
	return Out;
}

void UYIInventoryComponent::ServerAddBagItem_Implementation(const FYIItemInstanceNet& NetItem, FIntPoint Pos, FIntPoint Size)
{
	FYIBagItem Item;
	Item.Item = NetToFull(NetItem);
	Item.Pos = Pos;
	Item.Size = Size;
	AddBagItem(Item);
}

bool UYIInventoryComponent::RemoveItem(int32 Index)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (EquippedBag && IsBagItemLocked(EquippedBag, Index))
		{
			return false;
		}
		if (EquippedBag && EquippedBag->RemoveItem(Index))
		{
			SyncNetState();
			return true;
		}
		return false;
	}
	ServerRemoveItem(Index);
	return true;
}

bool UYIInventoryComponent::RemoveItemFromBag(const FGuid& BagId, const FGuid& ItemInstanceId)
{
	return FYIInventoryMutationService::RemoveItemFromBag(*this, BagId, ItemInstanceId);
}

void UYIInventoryComponent::ServerRemoveItem_Implementation(int32 Index)
{
	RemoveItem(Index);
}

void UYIInventoryComponent::ServerRemoveItemFromBag_Implementation(const FGuid& BagId, const FGuid& ItemInstanceId)
{
	RemoveItemFromBag(BagId, ItemInstanceId);
}

bool UYIInventoryComponent::TransferItemBetweenBagsById(const FGuid& SourceBagId, const FGuid& ItemInstanceId, const FGuid& DestBagId, int32 Count, int32& OutDestIndex)
{
	return FYIInventoryMutationService::TransferItemBetweenBagsById(*this, SourceBagId, ItemInstanceId, DestBagId, Count, OutDestIndex);
}

void UYIInventoryComponent::ServerTransferItemBetweenBagsById_Implementation(
	const FGuid& SourceBagId,
	const FGuid& ItemInstanceId,
	const FGuid& DestBagId,
	int32 Count)
{
	int32 IgnoredDestIndex = INDEX_NONE;
	TransferItemBetweenBagsById(SourceBagId, ItemInstanceId, DestBagId, Count, IgnoredDestIndex);
}

bool UYIInventoryComponent::TransferItemBetweenBagsAtCellById(
	const FGuid& SourceBagId,
	const FGuid& ItemInstanceId,
	const FGuid& DestBagId,
	FIntPoint DestCell,
	int32 Count,
	bool bAllowSingleOverlapSwap)
{
	return FYIInventoryMutationService::TransferItemBetweenBagsAtCellById(*this, SourceBagId, ItemInstanceId, DestBagId, DestCell, Count, bAllowSingleOverlapSwap);
}

void UYIInventoryComponent::ServerTransferItemBetweenBagsAtCellById_Implementation(
	const FGuid& SourceBagId,
	const FGuid& ItemInstanceId,
	const FGuid& DestBagId,
	FIntPoint DestCell,
	int32 Count,
	bool bAllowSingleOverlapSwap)
{
	TransferItemBetweenBagsAtCellById(SourceBagId, ItemInstanceId, DestBagId, DestCell, Count, bAllowSingleOverlapSwap);
}

bool UYIInventoryComponent::SwapItemIntoBagCellById(const FGuid& SourceBagId, const FGuid& ItemInstanceId, const FGuid& DestBagId, FIntPoint DestCell)
{
	return TransferItemBetweenBagsAtCellById(SourceBagId, ItemInstanceId, DestBagId, DestCell, 0, true);
}

void UYIInventoryComponent::ServerSwapItemIntoBagCellById_Implementation(
	const FGuid& SourceBagId,
	const FGuid& ItemInstanceId,
	const FGuid& DestBagId,
	FIntPoint DestCell)
{
	SwapItemIntoBagCellById(SourceBagId, ItemInstanceId, DestBagId, DestCell);
}

bool UYIInventoryComponent::CombineItemInBag(const FGuid& BagId, const FGuid& ItemInstanceId)
{
	return FYIInventoryMutationService::CombineItemInBag(*this, BagId, ItemInstanceId);
}

void UYIInventoryComponent::ServerCombineItemInBag_Implementation(const FGuid& BagId, const FGuid& ItemInstanceId)
{
	CombineItemInBag(BagId, ItemInstanceId);
}

bool UYIInventoryComponent::SplitStackInBag(const FGuid& BagId, const FGuid& ItemInstanceId, int32 Amount, FIntPoint DesiredPos)
{
	return FYIInventoryMutationService::SplitStackInBag(*this, BagId, ItemInstanceId, Amount, DesiredPos);
}

void UYIInventoryComponent::ServerSplitStackInBag_Implementation(
	const FGuid& BagId,
	const FGuid& ItemInstanceId,
	int32 Amount,
	FIntPoint DesiredPos)
{
	SplitStackInBag(BagId, ItemInstanceId, Amount, DesiredPos);
}

void UYIInventoryComponent::HandleBagItemAdded(int32 Index, FYIBagItem Item)
{
	OnInventoryItemAdded.Broadcast(EquippedBag, Index, Item);
	UYIDebugLibrary::EmitDebugMessage(
		this,
		EYIDebugChannel::Inventory,
		FString::Printf(TEXT("Added idx=%d count=%d"), Index, Item.Item.Count),
		FLinearColor(FColor::Green),
		bDebugInventoryActions,
		bDebugInventoryActions,
		2.0f,
		false,
		false,
		TEXT("InventoryComponent"));
}

void UYIInventoryComponent::HandleBagItemRemoved(int32 Index, FYIBagItem Item)
{
	if (EquippedBag && EquippedBag->BagId.IsValid())
	{
		const int32 Removed = LockedBagItems.RemoveAllSwap([this, &Item](const FYIInventoryLockRef& Entry)
		{
			if (!EquippedBag || Entry.ItemRef.Bag.BagId != EquippedBag->BagId)
			{
				return false;
			}
			if (Entry.ItemRef.Item.ItemInstanceId.IsValid() && Item.Item.InstanceId.IsValid())
			{
				return Entry.ItemRef.Item.ItemInstanceId == Item.Item.InstanceId;
			}
			return Entry.ItemRef.Item.LegacyStackKey != 0 && Entry.ItemRef.Item.LegacyStackKey == Item.Item.CustomStackKey;
		}, EAllowShrinking::No);
		if (Removed > 0 && GetOwner() && GetOwner()->HasAuthority())
		{
			SyncNetState();
		}
	}

	OnInventoryItemRemoved.Broadcast(EquippedBag, Index, Item);
	UYIDebugLibrary::EmitDebugMessage(
		this,
		EYIDebugChannel::Inventory,
		FString::Printf(TEXT("Removed idx=%d"), Index),
		FLinearColor(FColor::Orange),
		bDebugInventoryActions,
		bDebugInventoryActions,
		2.0f,
		false,
		false,
		TEXT("InventoryComponent"));
}

void UYIInventoryComponent::HandleBagItemMoved(int32 Index, FIntPoint NewPos)
{
	OnInventoryItemMoved.Broadcast(EquippedBag, Index, NewPos);
	UYIDebugLibrary::EmitDebugMessage(
		this,
		EYIDebugChannel::Inventory,
		FString::Printf(TEXT("Moved idx=%d to (%d,%d)"), Index, NewPos.X, NewPos.Y),
		FLinearColor(FColor::Cyan),
		bDebugInventoryActions,
		bDebugInventoryActions,
		2.0f,
		false,
		false,
		TEXT("InventoryComponent"));
}

void UYIInventoryComponent::HandleBagItemRotated(int32 Index)
{
	OnInventoryItemRotated.Broadcast(EquippedBag, Index);
	UYIDebugLibrary::EmitDebugMessage(
		this,
		EYIDebugChannel::Inventory,
		FString::Printf(TEXT("Rotated idx=%d"), Index),
		FLinearColor(FColor::Yellow),
		bDebugInventoryActions,
		bDebugInventoryActions,
		2.0f,
		false,
		false,
		TEXT("InventoryComponent"));
}

void UYIInventoryComponent::HandleBagItemTransferred(UYIInventoryBag* Src, UYIInventoryBag* Dest, int32 SrcIdx, int32 DestIdx)
{
	OnInventoryItemTransferred.Broadcast(Src, Dest, SrcIdx, DestIdx);
	UYIDebugLibrary::EmitDebugMessage(
		this,
		EYIDebugChannel::Inventory,
		FString::Printf(TEXT("Transfer %p:%d -> %p:%d"), Src, SrcIdx, Dest, DestIdx),
		FLinearColor(FColor::White),
		bDebugInventoryActions,
		bDebugInventoryActions,
		2.0f,
		false,
		false,
		TEXT("InventoryComponent"));
}

// -------- UI helpers --------

/* UI screen helpers moved to YOLOInventoryUI (UYIInventoryUIScreenLibrary). */





