#include "YIInventoryComponent.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "YIItemBlueprintLibrary.h"
#include "UObject/Package.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "InventoryScreenWidget.h"
#include "TradingScreenWidget.h"
#include "ShopScreenWidget.h"
#include "YITradeSessionActor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"
#include "YIItemPickup.h" // FYIItemInstanceNet / attribute pairs
#include "YIShopComponent.h"

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

bool UYIInventoryComponent::SetActiveSpellbookBagById(const FGuid& InBagId)
{
	if (!InBagId.IsValid())
	{
		return false;
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

bool UYIInventoryComponent::GetBagItemIdentity(const UYIInventoryBag* Bag, int32 ItemIndex, FYILockedBagItemRef& OutIdentity) const
{
	OutIdentity = FYILockedBagItemRef();
	if (!Bag || !Bag->Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	const FYIBagItem& BagItem = Bag->Items[ItemIndex];
	OutIdentity.BagId = Bag->BagId;
	OutIdentity.ItemInstanceId = BagItem.Item.InstanceId;
	OutIdentity.CustomStackKey = BagItem.Item.CustomStackKey;
	OutIdentity.Code = 0;
	if (UYIItemDefinition* Def = BagItem.Item.Definition.IsValid()
		? BagItem.Item.Definition.Get()
		: BagItem.Item.Definition.LoadSynchronous())
	{
		OutIdentity.Code = Def->UniqueCode;
	}

	return OutIdentity.BagId.IsValid() && (OutIdentity.ItemInstanceId.IsValid() || OutIdentity.CustomStackKey != 0);
}

bool UYIInventoryComponent::IsBagItemLockedByIdentity(const FYILockedBagItemRef& Identity) const
{
	if (!Identity.BagId.IsValid())
	{
		return false;
	}

	return LockedBagItems.ContainsByPredicate([&Identity](const FYILockedBagItemRef& Entry)
	{
		if (Entry.BagId != Identity.BagId)
		{
			return false;
		}
		if (Identity.ItemInstanceId.IsValid() && Entry.ItemInstanceId.IsValid())
		{
			return Entry.ItemInstanceId == Identity.ItemInstanceId;
		}
		return Identity.CustomStackKey != 0 && Entry.CustomStackKey == Identity.CustomStackKey;
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

	FYILockedBagItemRef Identity;
	if (!GetBagItemIdentity(Bag, ItemIndex, Identity))
	{
		return false;
	}

	return SetBagItemLockedByInstanceRef(Identity.BagId, Identity.ItemInstanceId, Identity.CustomStackKey, Identity.Code, bLocked);
}

bool UYIInventoryComponent::SetBagItemLockedByRef(const FGuid& BagId, int64 CustomStackKey, int64 Code, bool bLocked)
{
	return SetBagItemLockedByInstanceRef(BagId, FGuid(), CustomStackKey, Code, bLocked);
}

bool UYIInventoryComponent::SetBagItemLockedByInstanceRef(const FGuid& BagId, const FGuid& ItemInstanceId, int64 CustomStackKey, int64 Code, bool bLocked)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !BagId.IsValid())
	{
		return false;
	}
	if (!ItemInstanceId.IsValid() && CustomStackKey == 0)
	{
		return false;
	}

	const int32 ExistingIndex = LockedBagItems.IndexOfByPredicate([&](const FYILockedBagItemRef& Entry)
	{
		if (Entry.BagId != BagId)
		{
			return false;
		}
		if (ItemInstanceId.IsValid() && Entry.ItemInstanceId.IsValid())
		{
			return Entry.ItemInstanceId == ItemInstanceId;
		}
		return CustomStackKey != 0 && Entry.CustomStackKey == CustomStackKey;
	});

	if (bLocked)
	{
		if (ExistingIndex != INDEX_NONE)
		{
			return true;
		}

		FYILockedBagItemRef NewEntry;
		NewEntry.BagId = BagId;
		NewEntry.ItemInstanceId = ItemInstanceId;
		NewEntry.CustomStackKey = CustomStackKey;
		NewEntry.Code = Code;
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

	FYILockedBagItemRef Identity;
	if (!GetBagItemIdentity(Bag, ItemIndex, Identity))
	{
		return false;
	}

	return IsBagItemLockedByIdentity(Identity);
}

void UYIInventoryComponent::GetReplicatedBagDescriptors(TArray<FYINetBagDescriptor>& OutDescriptors) const
{
	OutDescriptors = NetBagDescriptors;
}

bool UYIInventoryComponent::RemoveBag(UYIInventoryBag* Bag)
{
	if (!Bag) return false;
	Bag->EnsureBagId();
	LockedBagItems.RemoveAllSwap([Bag](const FYILockedBagItemRef& Entry)
	{
		return Entry.BagId == Bag->BagId;
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
	New.Size = Def->DefaultSize;
	int32 Idx = Bag->AddBagItem(New);
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
	CloseTradeScreen();
	CloseShopScreen();
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

	ClientPreviewBag->Items.Reset();

	for (const FYINetBagItem& Net : NetBagItems)
	{
		if (Net.Code == 0 || Net.Count <= 0) continue;
		FYIBagItem Item;
		Item.Item = UYIItemBlueprintLibrary::MakeItemInstanceByCode(Net.Code, Net.Count);
		if (Net.InstanceId.IsValid())
		{
			Item.Item.InstanceId = Net.InstanceId;
		}
		if (Net.StackId.IsValid())
		{
			Item.Item.StackId = Net.StackId;
		}
		Item.Item.CustomStackKey = Net.CustomStackKey;
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
			int32 Idx = EquippedBag->AddBagItem(Item);
			if (Idx != INDEX_NONE) { SyncNetState(); }
			return Idx;
		}
		return INDEX_NONE;
	}
	// Client: send net-safe version
	FYIItemInstanceNet Net;
	Net.Definition = Item.Item.Definition;
	Net.Count = Item.Item.Count;
	Net.InstanceId = Item.Item.InstanceId;
	Net.StackId = Item.Item.StackId;
	Net.CustomStackKey = Item.Item.CustomStackKey;
	Net.bRotated = Item.Item.bRotated;
	Net.Affixes = Item.Item.Affixes;
	Net.Attributes.Reset();
	for (const TPair<FName, float>& KV : Item.Item.Attributes)
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
	Out.bRotated = Net.bRotated;
	Out.Affixes = Net.Affixes;
	Out.Attributes.Reset();
	for (const FYIAttributeKV& KV : Net.Attributes)
	{
		Out.Attributes.Add(KV.Name, KV.Value);
	}
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

bool UYIInventoryComponent::DropItemToWorld(const FYIItemInstanceNet& NetItem, const FTransform& SpawnTransform)
{
	if (!GetOwner())
	{
		return false;
	}
	if (GetOwner()->HasAuthority())
	{
		FYIItemInstance Full = NetToFull(NetItem);
		UYIInventoryBlueprintLibrary::SpawnItemPickupFromInstance(GetOwner(), Full, SpawnTransform);
		OnInventoryItemDroppedToWorld.Broadcast(NetItem, SpawnTransform);
		if (bDebugInventoryActions && GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Cyan,
				FString::Printf(TEXT("[Inventory] Dropped to world (%d)"), NetItem.Count));
		}
		return true;
	}
	ServerDropItemToWorld(NetItem, SpawnTransform);
	return true;
}

void UYIInventoryComponent::ServerDropItemToWorld_Implementation(const FYIItemInstanceNet& NetItem, const FTransform& SpawnTransform)
{
	DropItemToWorld(NetItem, SpawnTransform);
}

void UYIInventoryComponent::HandleBagItemAdded(int32 Index, FYIBagItem Item)
{
	OnInventoryItemAdded.Broadcast(EquippedBag, Index, Item);
	if (bDebugInventoryActions && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Green,
			FString::Printf(TEXT("[Inventory] Added idx %d x%d"), Index, Item.Item.Count));
	}
}

void UYIInventoryComponent::HandleBagItemRemoved(int32 Index, FYIBagItem Item)
{
	if (EquippedBag && EquippedBag->BagId.IsValid())
	{
		const int32 Removed = LockedBagItems.RemoveAllSwap([this, &Item](const FYILockedBagItemRef& Entry)
		{
			if (!EquippedBag || Entry.BagId != EquippedBag->BagId)
			{
				return false;
			}
			if (Entry.ItemInstanceId.IsValid() && Item.Item.InstanceId.IsValid())
			{
				return Entry.ItemInstanceId == Item.Item.InstanceId;
			}
			return Entry.CustomStackKey != 0 && Entry.CustomStackKey == Item.Item.CustomStackKey;
		}, EAllowShrinking::No);
		if (Removed > 0 && GetOwner() && GetOwner()->HasAuthority())
		{
			SyncNetState();
		}
	}

	OnInventoryItemRemoved.Broadcast(EquippedBag, Index, Item);
	if (bDebugInventoryActions && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Orange,
			FString::Printf(TEXT("[Inventory] Removed idx %d"), Index));
	}
}

void UYIInventoryComponent::HandleBagItemMoved(int32 Index, FIntPoint NewPos)
{
	OnInventoryItemMoved.Broadcast(EquippedBag, Index, NewPos);
	if (bDebugInventoryActions && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Cyan,
			FString::Printf(TEXT("[Inventory] Moved idx %d -> (%d,%d)"), Index, NewPos.X, NewPos.Y));
	}
}

void UYIInventoryComponent::HandleBagItemRotated(int32 Index)
{
	OnInventoryItemRotated.Broadcast(EquippedBag, Index);
	if (bDebugInventoryActions && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Yellow,
			FString::Printf(TEXT("[Inventory] Rotated idx %d"), Index));
	}
}

void UYIInventoryComponent::HandleBagItemTransferred(UYIInventoryBag* Src, UYIInventoryBag* Dest, int32 SrcIdx, int32 DestIdx)
{
	OnInventoryItemTransferred.Broadcast(Src, Dest, SrcIdx, DestIdx);
	if (bDebugInventoryActions && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::White,
			FString::Printf(TEXT("[Inventory] Transfer %p:%d -> %p:%d"), Src, SrcIdx, Dest, DestIdx));
	}
}

// -------- UI helpers --------

UInventoryScreenWidget* UYIInventoryComponent::OpenInventoryScreen()
{
	if (!GetOwner()) return nullptr;
	if (GetOwner()->GetNetMode() == NM_DedicatedServer) return nullptr; // no UI on dedicated server
	if (ActiveInventoryScreen.IsValid())
	{
		ActiveInventoryScreen->BindInventoryBagContexts(this);
		return ActiveInventoryScreen.Get();
	}

	if (!InventoryScreenClass.IsNull())
	{
		InventoryScreenClass.LoadSynchronous();
	}
	if (!InventoryScreenClass.IsValid())
	{
		InventoryScreenClass = TSoftClassPtr<UInventoryScreenWidget>(UInventoryScreenWidget::StaticClass());
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

	UInventoryScreenWidget* Screen = CreateWidget<UInventoryScreenWidget>(PC, InventoryScreenClass.Get());
	if (!Screen) return nullptr;

	Screen->BindInventoryBagContexts(this);
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

UTradingScreenWidget* UYIInventoryComponent::OpenTradeScreen(AYITradeSessionActor* Session, UYIInventoryBag* LocalBag)
{
	if (!Session) return nullptr;
	if (!GetOwner() || GetOwner()->GetNetMode() == NM_DedicatedServer) return nullptr;
	if (ActiveTradeScreen.IsValid())
	{
		ActiveTradeScreen->SetSession(Session, LocalBag ? LocalBag : GetBag(), nullptr);
		return ActiveTradeScreen.Get();
	}

	if (!TradingScreenClass.IsNull())
	{
		TradingScreenClass.LoadSynchronous();
	}
	if (!TradingScreenClass.IsValid()) return nullptr;

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

	UTradingScreenWidget* Screen = CreateWidget<UTradingScreenWidget>(PC, TradingScreenClass.Get());
	if (!Screen) return nullptr;

	Screen->SetSession(Session, LocalBag ? LocalBag : GetBag(), nullptr);
	Screen->AddToViewport();
	ActiveTradeScreen = Screen;
	return Screen;
}

void UYIInventoryComponent::CloseTradeScreen()
{
	if (ActiveTradeScreen.IsValid())
	{
		ActiveTradeScreen->RemoveFromParent();
		ActiveTradeScreen.Reset();
	}
}

UShopScreenWidget* UYIInventoryComponent::OpenShopScreen(UYIShopComponent* Shop, UYIInventoryBag* LocalBag, const TArray<FYINetBagItem>& Stock, FIntPoint StockSize)
{
	if (!Shop) return nullptr;
	if (!GetOwner() || GetOwner()->GetNetMode() == NM_DedicatedServer) return nullptr;
	if (ActiveShopScreen.IsValid())
	{
		ActiveShopScreen->SetShop(Shop, LocalBag ? LocalBag : GetBag(), Stock, StockSize);
		return ActiveShopScreen.Get();
	}

	if (!ShopScreenClass.IsNull())
	{
		ShopScreenClass.LoadSynchronous();
	}
	if (!ShopScreenClass.IsValid()) return nullptr;

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

	UShopScreenWidget* Screen = CreateWidget<UShopScreenWidget>(PC, ShopScreenClass.Get());
	if (!Screen) return nullptr;

	Screen->SetShop(Shop, LocalBag ? LocalBag : GetBag(), Stock, StockSize);
	Screen->AddToViewport();
	ActiveShopScreen = Screen;
	return Screen;
}

void UYIInventoryComponent::UpdateShopScreen(UYIShopComponent* Shop, UYIInventoryBag* LocalBag, const TArray<FYINetBagItem>& Stock, FIntPoint StockSize)
{
	if (!Shop) return;
	if (ActiveShopScreen.IsValid())
	{
		ActiveShopScreen->SetShop(Shop, LocalBag ? LocalBag : GetBag(), Stock, StockSize);
	}
}

void UYIInventoryComponent::CloseShopScreen()
{
	if (ActiveShopScreen.IsValid())
	{
		ActiveShopScreen->RemoveFromParent();
		ActiveShopScreen.Reset();
	}
}

void UYIInventoryComponent::CloseAllScreens()
{
	CloseInventoryScreen();
	CloseTradeScreen();
	CloseShopScreen();
}
