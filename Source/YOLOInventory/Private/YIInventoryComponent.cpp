#include "YIInventoryComponent.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "YIItemBlueprintLibrary.h"
#include "UObject/Package.h"

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
		NewBag->GridSize = GridSize;
		NewBag->DisplayName = FText::FromName(BagName);
		Bags.Add(NewBag);
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
		Bag = CloneBagTemplate(Bag);
	}
	if (EquippedBag == Bag)
	{
		OnBagOpened.Broadcast(Bag);
		return;
	}

	if (EquippedBag && BagChangedHandle.IsValid())
	{
		EquippedBag->OnChanged.Remove(BagChangedHandle);
		BagChangedHandle.Reset();
	}

	EquippedBag = Bag;
	if (EquippedBag)
	{
		BagChangedHandle = EquippedBag->OnChanged.AddUObject(this, &UYIInventoryComponent::SyncNetState);
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
	if (EquippedBag == Bag) { EquippedBag = nullptr; }
	OnBagClosed.Broadcast(Bag);
}

UYIInventoryBag* UYIInventoryComponent::GetBag() const
{
	if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_Authority)
	{
		return EquippedBag;
	}
	// Client: prefer preview mirror if present
	return ClientPreviewBag ? ClientPreviewBag : EquippedBag;
}

bool UYIInventoryComponent::RemoveBag(UYIInventoryBag* Bag)
{
	if (!Bag) return false;
	int32 Index = Bags.IndexOfByKey(Bag);
	if (Index != INDEX_NONE)
	{
		Bags.RemoveAt(Index);
		if (EquippedBag == Bag) { EquippedBag = nullptr; }
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

	// If EquippedBag is an asset/template, clone to a runtime instance (server only)
	if (GetOwner() && GetOwner()->HasAuthority() && EquippedBag)
	{
		if (IsTemplateBag(EquippedBag))
		{
			UYIInventoryBag* RuntimeBag = CloneBagTemplate(EquippedBag);
			if (RuntimeBag)
			{
				EquippedBag = RuntimeBag;
				OpenBag(EquippedBag); // rebind delegate and sync
			}
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

	// Copy layout/settings
	NewBag->DisplayName = TemplateBag->DisplayName;
	NewBag->GridSize = TemplateBag->GridSize;
	NewBag->CellPixelSize = TemplateBag->CellPixelSize;
	NewBag->bAllowRotation = TemplateBag->bAllowRotation;
	NewBag->MinifyScale = TemplateBag->MinifyScale;
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
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UYIInventoryComponent::SyncNetState()
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	NetBagItems.Reset();
	if (EquippedBag)
	{
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
		// Default grid; if an equipped bag exists locally, copy layout
		if (EquippedBag)
		{
			ClientPreviewBag->GridSize = EquippedBag->GridSize;
			ClientPreviewBag->CellPixelSize = EquippedBag->CellPixelSize;
			ClientPreviewBag->bAllowRotation = EquippedBag->bAllowRotation;
			ClientPreviewBag->MinifyScale = EquippedBag->MinifyScale;
		}
	}

	ClientPreviewBag->Items.Reset();

	for (const FYINetBagItem& Net : NetBagItems)
	{
		if (Net.Code == 0 || Net.Count <= 0) continue;
		FYIBagItem Item;
		Item.Item = UYIItemBlueprintLibrary::MakeItemInstanceByCode(Net.Code, Net.Count);
		Item.Item.CustomStackKey = Net.CustomStackKey;
		Item.Pos = Net.Pos;
		Item.Size = Net.Size;
		ClientPreviewBag->Items.Add(Item);
	}

	// Notify UI bound to this bag
	ClientPreviewBag->OnChanged.Broadcast();
	OnBagOpened.Broadcast(ClientPreviewBag); // UI can listen to refresh
}

void UYIInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, NetBagItems, COND_OwnerOnly);
}
