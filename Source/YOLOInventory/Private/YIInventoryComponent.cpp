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

	NetBagItems.Reset();
	if (EquippedBag)
	{
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
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, NetBagGridSize, COND_OwnerOnly);
}

// -------- Net-safe bag mutations --------

bool UYIInventoryComponent::MoveItem(int32 Index, FIntPoint NewPos)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
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
		return true;
	}
	ServerDropItemToWorld(NetItem, SpawnTransform);
	return true;
}

void UYIInventoryComponent::ServerDropItemToWorld_Implementation(const FYIItemInstanceNet& NetItem, const FTransform& SpawnTransform)
{
	DropItemToWorld(NetItem, SpawnTransform);
}

// -------- UI helpers --------

UInventoryScreenWidget* UYIInventoryComponent::OpenInventoryScreen()
{
	if (!GetOwner()) return nullptr;
	if (GetOwner()->GetNetMode() == NM_DedicatedServer) return nullptr; // no UI on dedicated server
	if (ActiveInventoryScreen.IsValid())
	{
		if (UInventoryGridWidget* Grid = ActiveInventoryScreen->GetGrid())
		{
			Grid->SetBag(GetBag());
		}
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

	UInventoryScreenWidget* Screen = CreateWidget<UInventoryScreenWidget>(PC, InventoryScreenClass.Get());
	if (!Screen) return nullptr;

	if (UInventoryGridWidget* Grid = Screen->GetGrid())
	{
		Grid->SetBag(GetBag());
	}
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
