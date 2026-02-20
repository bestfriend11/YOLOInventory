#include "YIInventoryComponent.h"
#include "YIItemSchemaResolver.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "YIItemRegistrySubsystem.h"
#include "UObject/Package.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "YIItemNetTypes.h"
#include "YIDebugLibrary.h"

namespace YIInventoryComponentUIBindings
{
	struct FYIInventoryScreenBindParams
	{
		UYIInventoryComponent* InInventoryComponent = nullptr;
	};

	static void YIInventoryComp_BindInventoryScreenWidget(UUserWidget* Widget, UYIInventoryComponent* InventoryComponent)
	{
		if (!Widget || !InventoryComponent)
		{
			return;
		}

		if (UFunction* Fn = Widget->FindFunction(TEXT("BindInventoryBagContexts")))
		{
			YIInventoryComponentUIBindings::FYIInventoryScreenBindParams Params;
			Params.InInventoryComponent = InventoryComponent;
			Widget->ProcessEvent(Fn, &Params);
		}
	}
}

using namespace YIInventoryComponentUIBindings;

namespace
{
	static FYIItemInstance YIInventoryComp_MakeItemInstanceByCode(int64 Code, int32 Count)
	{
		FYIItemInstance Out;
		Out.Count = Count;
		if (Code == 0 || !GEngine)
		{
			return Out;
		}

		if (UYIItemRegistrySubsystem* Registry = GEngine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
		{
			Out.Definition = Registry->GetByCode(Code);
		}
		return Out;
	}
}

UYIInventoryComponent::UYIInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

UYIInventoryBag* UYIInventoryComponent::CreateBag(FName BagName, FIntPoint GridSize)
{
	UYIInventoryBag* NewBag = NewObject<UYIInventoryBag>(this);
	if (NewBag)
	{
		NewBag->EnsureBagId();
		NewBag->GridSize = GridSize;
		NewBag->DisplayName = FText::FromName(BagName);
		Bags.Add(NewBag);
		// Keep setup simple: first created bag becomes active automatically.
		if (!EquippedBag)
		{
			OpenBag(NewBag);
		}
		else if (!ActiveBagId.IsValid())
		{
			ActiveBagId = NewBag->BagId;
		}
		return NewBag;
	}
	return nullptr;
}

void UYIInventoryComponent::OpenBag(UYIInventoryBag* Bag)
{
	if (!Bag) return;
	// Prevent templates from being used directly at runtime
	if (GetOwner() && GetOwner()->HasAuthority() && IsTemplateBag(Bag))
	{
		UYIInventoryBag* RuntimeBag = CloneBagTemplate(Bag);
		if (!RuntimeBag)
		{
			return;
		}
		Bag = RuntimeBag;
	}

	// Ensure the active bag is tracked in Bags so designers do not need to keep two fields in sync.
	if (!Bags.Contains(Bag))
	{
		Bags.Add(Bag);
	}

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

	EquippedBag = Bag;
	if (EquippedBag)
	{
		EquippedBag->EnsureBagId();
		ActiveBagId = EquippedBag->BagId;
		BagChangedHandle = EquippedBag->OnChanged.AddUObject(this, &UYIInventoryComponent::SyncNetState);
		BagEventSource = EquippedBag;
		BagEventSource->OnItemAdded.AddDynamic(this, &UYIInventoryComponent::HandleBagItemAdded);
		BagEventSource->OnItemRemoved.AddDynamic(this, &UYIInventoryComponent::HandleBagItemRemoved);
		BagEventSource->OnItemMoved.AddDynamic(this, &UYIInventoryComponent::HandleBagItemMoved);
		BagEventSource->OnItemRotated.AddDynamic(this, &UYIInventoryComponent::HandleBagItemRotated);
		BagEventSource->OnItemTransferred.AddDynamic(this, &UYIInventoryComponent::HandleBagItemTransferred);
	}

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		SyncNetState();
	}
	OnBagOpened.Broadcast(Bag);
}

void UYIInventoryComponent::CloseBag(UYIInventoryBag* Bag)
{
	if (!Bag) return;
	if (EquippedBag == Bag)
	{
		if (BagChangedHandle.IsValid())
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
		EquippedBag = nullptr;
		ActiveBagId.Invalidate();
		if (GetOwner() && GetOwner()->HasAuthority())
		{
			SyncNetState();
		}
	}
	OnBagClosed.Broadcast(Bag);
}

UYIInventoryBag* UYIInventoryComponent::GetBag() const
{
	auto ResolvePrimaryBag = [this]() -> UYIInventoryBag*
	{
		if (ActiveBagId.IsValid())
		{
			if (UYIInventoryBag* ActiveBag = GetBagById(ActiveBagId))
			{
				return ActiveBag;
			}
		}
		for (UYIInventoryBag* Bag : Bags)
		{
			if (Bag)
			{
				return Bag;
			}
		}
		return nullptr;
	};

	if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		if (EquippedBag)
		{
			return EquippedBag.Get();
		}
		return ResolvePrimaryBag();
	}
	// Client: prefer preview mirror if present
	if (ClientPreviewBag)
	{
		return ClientPreviewBag;
	}
	if (EquippedBag)
	{
		return EquippedBag.Get();
	}
	return ResolvePrimaryBag();
}

UYIInventoryBag* UYIInventoryComponent::GetActiveSpellbookBag() const
{
	if (!ActiveSpellbookBagId.IsValid())
	{
		return nullptr;
	}
	return GetBagById(ActiveSpellbookBagId);
}

UYIInventoryBag* UYIInventoryComponent::GetBagById(const FGuid& BagId) const
{
	if (!BagId.IsValid())
	{
		return nullptr;
	}

	for (UYIInventoryBag* Bag : Bags)
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
	return nullptr;
}

bool UYIInventoryComponent::SetActiveBagById(const FGuid& InBagId)
{
	if (!InBagId.IsValid())
	{
		return false;
	}

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerSetActiveBagById(InBagId);
		return true;
	}

	if (UYIInventoryBag* Bag = GetBagById(InBagId))
	{
		OpenBag(Bag);
		return true;
	}
	return false;
}

bool UYIInventoryComponent::SetActiveBagByRoleTag(FGameplayTag InBagRoleTag)
{
	if (!InBagRoleTag.IsValid())
	{
		return false;
	}
	if (UYIInventoryBag* Bag = GetBagByRoleTag(InBagRoleTag))
	{
		OpenBag(Bag);
		return true;
	}
	return false;
}

bool UYIInventoryComponent::OpenContainedBagAtIndex(int32 ItemIndex)
{
	UYIInventoryBag* ActiveBag = GetBag();
	if (!ActiveBag || !ActiveBag->Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		const FYIBagItem& Item = ActiveBag->Items[ItemIndex];
		if (!Item.Item.InstanceId.IsValid())
		{
			return false;
		}
		const FGuid ParentBagId = ActiveBag->BagId.IsValid() ? ActiveBag->BagId : ActiveBagId;
		ServerOpenContainedBagByInstance(ParentBagId, Item.Item.InstanceId);
		return true;
	}

	return TryOpenContainedBagInternal(ActiveBag, ItemIndex);
}

bool UYIInventoryComponent::OpenParentBag()
{
	UYIInventoryBag* ActiveBag = GetBag();
	if (!ActiveBag)
	{
		return false;
	}

	const FGuid ChildBagId = ActiveBag->BagId;
	if (!ChildBagId.IsValid())
	{
		return false;
	}

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerOpenParentBag(ChildBagId);
		return true;
	}

	FGuid ParentBagId;
	FGuid ParentItemId;
	if (!FindContainerParentForBag(ChildBagId, ParentBagId, ParentItemId))
	{
		return false;
	}
	return SetActiveBagById(ParentBagId);
}

bool UYIInventoryComponent::SetActiveSpellbookBagById(const FGuid& InBagId)
{
	if (!InBagId.IsValid())
	{
		return false;
	}

	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		ServerSetActiveSpellbookBagById(InBagId);
		return true;
	}

	if (UYIInventoryBag* Bag = GetBagById(InBagId))
	{
		Bag->EnsureBagId();
		ActiveSpellbookBagId = Bag->BagId;
		if (GetOwner() && GetOwner()->HasAuthority())
		{
			SyncNetState();
		}
		return true;
	}
	return false;
}

bool UYIInventoryComponent::SetActiveSpellbookBagByRoleTag(FGameplayTag InBagRoleTag)
{
	if (!InBagRoleTag.IsValid())
	{
		return false;
	}

	if (UYIInventoryBag* Bag = GetBagByRoleTag(InBagRoleTag))
	{
		Bag->EnsureBagId();
		ActiveSpellbookBagId = Bag->BagId;
		if (GetOwner() && GetOwner()->HasAuthority())
		{
			SyncNetState();
		}
		return true;
	}
	return false;
}

void UYIInventoryComponent::ServerSetActiveBagById_Implementation(const FGuid& InBagId)
{
	SetActiveBagById(InBagId);
}

void UYIInventoryComponent::ServerSetActiveSpellbookBagById_Implementation(const FGuid& InBagId)
{
	SetActiveSpellbookBagById(InBagId);
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
	if (!BagRoleTag.IsValid())
	{
		return nullptr;
	}

	for (UYIInventoryBag* Bag : Bags)
	{
		if (Bag && Bag->BagRoleTag.IsValid() && Bag->BagRoleTag.MatchesTag(BagRoleTag))
		{
			return Bag;
		}
	}
	return nullptr;
}

UYIInventoryBag* UYIInventoryComponent::GetBagByDisplayName(FName BagName) const
{
	if (BagName.IsNone())
	{
		return nullptr;
	}

	for (UYIInventoryBag* Bag : Bags)
	{
		if (Bag && Bag->DisplayName.EqualTo(FText::FromName(BagName)))
		{
			return Bag;
		}
	}
	return nullptr;
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
	(void)ParentBag;
	UYIItemDefinition* Definition = InOutItem.Item.Definition.IsValid()
		? InOutItem.Item.Definition.Get()
		: InOutItem.Item.Definition.LoadSynchronous();
	if (!Definition || !YIItemSchema::IsContainerItem(Definition))
	{
		return nullptr;
	}

	if (InOutItem.Item.ContainedBagId.IsValid())
	{
		if (UYIInventoryBag* Existing = GetBagById(InOutItem.Item.ContainedBagId))
		{
			return Existing;
		}
	}

	UYIInventoryBag* ChildBag = nullptr;
	if (const UYIInventoryBag* TemplateBag = Cast<UYIInventoryBag>(YIItemSchema::GetContainerTemplateBag(Definition).LoadSynchronous()))
	{
		ChildBag = CloneBagTemplate(TemplateBag);
	}
	else
	{
		ChildBag = NewObject<UYIInventoryBag>(this);
		if (ChildBag)
		{
			ChildBag->EnsureBagId();
			const FText EffectiveName = YIItemSchema::GetDisplayName(Definition);
			ChildBag->DisplayName = EffectiveName.IsEmpty()
				? FText::FromString(TEXT("Container"))
				: EffectiveName;
			const FIntPoint DefaultGrid = YIItemSchema::GetContainerDefaultGridSize(Definition);
			ChildBag->GridSize = FIntPoint(
				FMath::Max(1, DefaultGrid.X),
				FMath::Max(1, DefaultGrid.Y));
			ChildBag->bAllowRotation = true;
		}
	}

	if (!ChildBag)
	{
		return nullptr;
	}

	ChildBag->EnsureBagId();
	if (!Bags.Contains(ChildBag))
	{
		Bags.Add(ChildBag);
	}
	InOutItem.Item.ContainedBagId = ChildBag->BagId;
	InOutItem.Item.Count = 1; // container items are always non-stackable runtime instances
	return ChildBag;
}

bool UYIInventoryComponent::TryOpenContainedBagInternal(UYIInventoryBag* ParentBag, int32 ItemIndex)
{
	if (!ParentBag || !ParentBag->Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	FYIBagItem& Item = ParentBag->Items[ItemIndex];
	UYIInventoryBag* ChildBag = EnsureContainedBagForItem(Item, ParentBag);
	if (!ChildBag)
	{
		return false;
	}

	if (!ChildBag->BagId.IsValid())
	{
		ChildBag->EnsureBagId();
	}

	// Prevent cycles: destination child bag can never be parent/ancestor of its current parent.
	if (ParentBag->BagId.IsValid() && IsBagDescendantOf(ParentBag->BagId, ChildBag->BagId))
	{
		return false;
	}

	OpenBag(ChildBag);
	return true;
}

void UYIInventoryComponent::GetReplicatedBagDescriptors(TArray<FYINetBagDescriptor>& OutDescriptors) const
{
	OutDescriptors = NetBagDescriptors;
}

bool UYIInventoryComponent::RemoveBag(UYIInventoryBag* Bag)
{
	if (!Bag) return false;
	Bag->EnsureBagId();
	LockedBagItems.RemoveAllSwap([Bag](const FYIInventoryLockRef& Entry)
	{
		return Entry.ItemRef.Bag.BagId == Bag->BagId;
	}, EAllowShrinking::No);
	if (ActiveBagId.IsValid() && ActiveBagId == Bag->BagId)
	{
		ActiveBagId.Invalidate();
	}
	if (ActiveSpellbookBagId.IsValid() && ActiveSpellbookBagId == Bag->BagId)
	{
		ActiveSpellbookBagId.Invalidate();
	}
	const bool bWasEquipped = (EquippedBag == Bag);
	if (bWasEquipped)
	{
		CloseBag(Bag);
	}

	int32 Index = Bags.IndexOfByKey(Bag);
	if (Index != INDEX_NONE)
	{
		Bags.RemoveAt(Index);
		// Keep workflow predictable: if the current bag was removed, auto-open another one if available.
		if (bWasEquipped)
		{
			if (UYIInventoryBag* NextBag = GetBag())
			{
				OpenBag(NextBag);
			}
		}
		return true;
	}
	return false;
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
			OpenBag(EquippedBag); // binds events + net sync consistently
		}
	}
}

UYIInventoryBag* UYIInventoryComponent::CloneBagTemplate(const UYIInventoryBag* TemplateBag)
{
	if (!TemplateBag)
	{
		return nullptr;
	}

	UYIInventoryBag* NewBag = NewObject<UYIInventoryBag>(this);
	if (!NewBag)
	{
		return nullptr;
	}
	NewBag->EnsureBagId();

	// Copy layout/settings
	NewBag->DisplayName = TemplateBag->DisplayName;
	NewBag->BagRoleTag = TemplateBag->BagRoleTag;
	NewBag->GridSize = TemplateBag->GridSize;
	NewBag->CellPixelSize = TemplateBag->CellPixelSize;
	NewBag->bAllowRotation = TemplateBag->bAllowRotation;
	NewBag->MinifyScale = TemplateBag->MinifyScale;
	NewBag->GridStyleAsset = TemplateBag->GridStyleAsset;
	NewBag->GridLineColor = TemplateBag->GridLineColor;
	NewBag->OuterLineColor = TemplateBag->OuterLineColor;
	NewBag->CellBgColor = TemplateBag->CellBgColor;
	NewBag->GridThickness = TemplateBag->GridThickness;
	NewBag->bShowCellTooltips = TemplateBag->bShowCellTooltips;
	NewBag->bShowSortingHeaders = TemplateBag->bShowSortingHeaders;
	NewBag->bEnableThumbnails = TemplateBag->bEnableThumbnails;
	NewBag->bEnableHoverHighlight = TemplateBag->bEnableHoverHighlight;
	NewBag->bUseTagFilter = TemplateBag->bUseTagFilter;
	NewBag->TagFilters = TemplateBag->TagFilters;
	NewBag->bUseFolderFilter = TemplateBag->bUseFolderFilter;
	NewBag->FolderFilters = TemplateBag->FolderFilters;
	NewBag->bAutoMergeOnAdd = TemplateBag->bAutoMergeOnAdd;

	// Copy items
	NewBag->Items = TemplateBag->Items;
	return NewBag;
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
	CloseInventoryScreen();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UYIInventoryComponent::SyncNetState()
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (!ActiveBagId.IsValid() && EquippedBag)
	{
		EquippedBag->EnsureBagId();
		ActiveBagId = EquippedBag->BagId;
	}
	if (!ActiveBagId.IsValid())
	{
		for (UYIInventoryBag* Bag : Bags)
		{
			if (Bag)
			{
				Bag->EnsureBagId();
				ActiveBagId = Bag->BagId;
				break;
			}
		}
	}

	NetBagItems.Reset();
	NetBagDescriptors.Reset();
	NetBagDescriptors.Reserve(Bags.Num());

	for (UYIInventoryBag* Bag : Bags)
	{
		if (!Bag)
		{
			continue;
		}
		Bag->EnsureBagId();
		FYINetBagDescriptor Desc;
		Desc.BagId = Bag->BagId;
		Desc.DisplayName = Bag->DisplayName;
		Desc.BagRoleTag = Bag->BagRoleTag;
		Desc.GridSize = Bag->GridSize;
		Desc.ItemCount = Bag->Items.Num();
		Desc.ParentBagId.Invalidate();
		Desc.ParentItemInstanceId.Invalidate();
		Desc.bIsNestedContainer = FindContainerParentForBag(Bag->BagId, Desc.ParentBagId, Desc.ParentItemInstanceId);
		Desc.bIsActive = (Bag->BagId == ActiveBagId);
		NetBagDescriptors.Add(Desc);
	}

	if (EquippedBag)
	{
		EquippedBag->EnsureBagId();
		NetBagGridSize = EquippedBag->GridSize;
		for (const FYIBagItem& It : EquippedBag->Items)
		{
			if (It.Item.Count <= 0) continue;
			FYINetBagItem Net;
			Net.Code = It.Item.Definition.IsValid() ? It.Item.Definition.Get()->UniqueCode : 0;
			if (Net.Code == 0 && It.Item.Definition.ToSoftObjectPath().IsValid())
			{
				if (UYIItemDefinition* Def = Cast<UYIItemDefinition>(It.Item.Definition.LoadSynchronous()))
				{
					Net.Code = Def->UniqueCode;
				}
			}
			Net.Count = It.Item.Count;
			Net.InstanceId = It.Item.InstanceId;
			Net.StackId = It.Item.StackId;
			Net.Pos = It.Pos;
			Net.Size = It.Size;
			Net.CustomStackKey = It.Item.CustomStackKey;
			Net.ContainedBagId = It.Item.ContainedBagId;
			NetBagItems.Add(Net);
		}
	}

	// Force Net update
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->ForceNetUpdate();
	}
}

void UYIInventoryComponent::OnRep_NetBag()
{
	// Build or refresh a lightweight client preview bag for UI; not authoritative.
	if (!ClientPreviewBag)
	{
		ClientPreviewBag = NewObject<UYIInventoryBag>(this);
		if (!ClientPreviewBag) return;
	}

	ClientPreviewBag->GridSize = NetBagGridSize;
	ClientPreviewBag->BagId = ActiveBagId;

	ClientPreviewBag->Items.Reset();

	for (const FYINetBagItem& Net : NetBagItems)
	{
		if (Net.Code == 0 || Net.Count <= 0) continue;
		FYIBagItem Item;
		Item.Item = YIInventoryComp_MakeItemInstanceByCode(Net.Code, Net.Count);
		if (Net.InstanceId.IsValid())
		{
			Item.Item.InstanceId = Net.InstanceId;
		}
		if (Net.StackId.IsValid())
		{
			Item.Item.StackId = Net.StackId;
		}
		Item.Item.CustomStackKey = Net.CustomStackKey;
		Item.Item.ContainedBagId = Net.ContainedBagId;
		Item.Pos = Net.Pos;
		Item.Size = Net.Size;
		ClientPreviewBag->Items.Add(Item);
	}

	// Notify UI bound to this bag
	ClientPreviewBag->OnChanged.Broadcast();
	OnBagOpened.Broadcast(ClientPreviewBag); // UI can listen to refresh
}

void UYIInventoryComponent::OnRep_NetBagDescriptors()
{
	// No-op for now: UI can poll NetBagDescriptors via component reference.
}

void UYIInventoryComponent::OnRep_ActiveBagContexts()
{
	// Notify UI bindings that resolve from active bag contexts.
	if (UYIInventoryBag* ActiveBag = GetBagById(ActiveBagId))
	{
		OnBagOpened.Broadcast(ActiveBag);
	}
	else if (ClientPreviewBag && ActiveBagId.IsValid())
	{
		OnBagOpened.Broadcast(ClientPreviewBag);
	}

	if (UYIInventoryBag* SpellbookBag = GetBagById(ActiveSpellbookBagId))
	{
		OnBagOpened.Broadcast(SpellbookBag);
	}
}

void UYIInventoryComponent::OnRep_LockedBagItems()
{
	if (ClientPreviewBag)
	{
		ClientPreviewBag->OnChanged.Broadcast();
	}
	if (EquippedBag)
	{
		EquippedBag->OnChanged.Broadcast();
	}
}

void UYIInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, NetBagItems, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, NetBagGridSize, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, NetBagDescriptors, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, ActiveBagId, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, ActiveSpellbookBagId, COND_OwnerOnly);
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
	RuntimeItem.SyncCoreFragmentsToLegacy();
	FYIItemInstanceNet Net;
	Net.Definition = RuntimeItem.Definition;
	Net.Count = RuntimeItem.Count;
	Net.InstanceId = RuntimeItem.InstanceId;
	Net.StackId = RuntimeItem.StackId;
	Net.CustomStackKey = RuntimeItem.CustomStackKey;
	Net.ContainedBagId = RuntimeItem.ContainedBagId;
	Net.bRotated = RuntimeItem.bRotated;
	Net.Affixes = RuntimeItem.Affixes;
	Net.Attributes.Reset();
	for (const TPair<FName, float>& KV : RuntimeItem.Attributes)
	{
		FYIAttributeKV OutKV; OutKV.Name = KV.Key; OutKV.Value = KV.Value; Net.Attributes.Add(OutKV);
	}
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
	Out.Affixes = Net.Affixes;
	Out.Attributes.Reset();
	for (const FYIAttributeKV& KV : Net.Attributes)
	{
		Out.Attributes.Add(KV.Name, KV.Value);
	}
	Out.SyncLegacyToCoreFragments();
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

void UYIInventoryComponent::ServerRemoveItem_Implementation(int32 Index)
{
	RemoveItem(Index);
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

UUserWidget* UYIInventoryComponent::OpenInventoryScreen()
{
	if (!GetOwner()) return nullptr;
	if (GetOwner()->GetNetMode() == NM_DedicatedServer) return nullptr; // no UI on dedicated server
	if (ActiveInventoryScreen.IsValid())
	{
		YIInventoryComp_BindInventoryScreenWidget(ActiveInventoryScreen.Get(), this);
		return ActiveInventoryScreen.Get();
	}

	if (!InventoryScreenClass.IsNull())
	{
		InventoryScreenClass.LoadSynchronous();
	}
	if (!InventoryScreenClass.IsValid()) return nullptr;

	APlayerController* PC = nullptr;
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		PC = Cast<APlayerController>(Pawn->GetController());
	}
	else
	{
		PC = Cast<APlayerController>(GetOwner());
	}
	if (!PC || !PC->IsLocalController()) return nullptr;

	UUserWidget* Screen = CreateWidget<UUserWidget>(PC, InventoryScreenClass.Get());
	if (!Screen) return nullptr;

	YIInventoryComp_BindInventoryScreenWidget(Screen, this);
	Screen->AddToViewport();
	ActiveInventoryScreen = Screen;
	return Screen;
}

void UYIInventoryComponent::CloseInventoryScreen()
{
	if (ActiveInventoryScreen.IsValid())
	{
		ActiveInventoryScreen->RemoveFromParent();
		ActiveInventoryScreen.Reset();
	}
}

void UYIInventoryComponent::CloseAllScreens()
{
	CloseInventoryScreen();
}
