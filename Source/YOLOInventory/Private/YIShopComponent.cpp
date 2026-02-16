#include "YIShopComponent.h"

#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "YIInventoryBag.h"
#include "YIInventoryComponent.h"
#include "YIPlayerInventoryStateComponent.h"
#include "YIItemDefinition.h"
#include "YIItemBlueprintLibrary.h"
#include "YITradeInteractionComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"

static void NotifyShopActionResult(APlayerState* PlayerState, UYIShopComponent* Shop, bool bSuccess, const FText& Reason)
{
    if (!PlayerState) return;
    if (APlayerController* PC = Cast<APlayerController>(PlayerState->GetOwner()))
    {
        if (UYITradeInteractionComponent* TradeComp = PC->FindComponentByClass<UYITradeInteractionComponent>())
        {
            TradeComp->Client_ShopActionResult(Shop, bSuccess, Reason);
        }
    }
}

UYIShopComponent::UYIShopComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UYIShopComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        BuildRuntimeStock();
        if (StockMode == EYIShopStockMode::SharedStock)
        {
            RefreshMirror();
        }
        if (RestockInterval > 0.f)
        {
            GetWorld()->GetTimerManager().SetTimer(RestockTimer, this, &UYIShopComponent::BuildRuntimeStock, RestockInterval, true);
        }
    }
}

void UYIShopComponent::BuildRuntimeStock()
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;

    RuntimeStock = CreateStockInstance();

    if (StockMode == EYIShopStockMode::PerPlayerStock)
    {
        for (auto& Pair : PlayerStock)
        {
            Pair.Value = CreateStockInstance();
        }
    }

    if (StockMode == EYIShopStockMode::SharedStock)
    {
        RefreshMirror();
    }
}

const FYIShopListing* UYIShopComponent::FindListing(int64 ItemCode) const
{
    return Listings.FindByPredicate([&](const FYIShopListing& L){ return L.ItemCode == ItemCode; });
}

bool UYIShopComponent::ConsumePrice(UYIPlayerInventoryStateComponent* BuyerState, int64 ItemCode, int32 Count)
{
    if (!BuyerState) return false;
    const FYIShopListing* Listing = FindListing(ItemCode);
    if (!Listing) return true; // free

    // Check first
    for (const FYIShopPrice& Price : Listing->Prices)
    {
        if (BuyerState->GetResourceAmount(Price.Resource) < Price.Amount * Count)
        {
            return false;
        }
    }
    // Consume
    for (const FYIShopPrice& Price : Listing->Prices)
    {
        BuyerState->ConsumeResource(Price.Resource, Price.Amount * Count);
    }
    return true;
}

void UYIShopComponent::ServerBuyItem_Implementation(int32 StockIndex, int32 Count, UYIInventoryComponent* BuyerInv, FIntPoint DestPos)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !BuyerInv || Count <= 0) return;

    APawn* BuyerPawn = Cast<APawn>(BuyerInv->GetOwner());
    APlayerState* BuyerPS = BuyerPawn ? BuyerPawn->GetPlayerState() : nullptr;
    if (!BuyerPS && BuyerPawn && BuyerPawn->GetController())
    {
        BuyerPS = BuyerPawn->GetController()->PlayerState;
    }
    UYIPlayerInventoryStateComponent* BuyerState = BuyerPS ? BuyerPS->FindComponentByClass<UYIPlayerInventoryStateComponent>() : nullptr;
    UYIInventoryBag* StockBag = GetStockForPlayer(BuyerPS);
    if (!StockBag)
    {
        OnShopPurchase.Broadcast(BuyerPS, 0, Count, false);
        NotifyShopActionResult(BuyerPS, this, false, NSLOCTEXT("YOLOInventory", "Shop_Buy_NoStock", "Shop has no stock"));
        return;
    }

    if (!StockBag->Items.IsValidIndex(StockIndex))
    {
        OnShopPurchase.Broadcast(BuyerPS, 0, Count, false);
        NotifyShopActionResult(BuyerPS, this, false, NSLOCTEXT("YOLOInventory", "Shop_Buy_InvalidIndex", "Invalid stock index"));
        return;
    }
    FYIBagItem& StockItem = StockBag->Items[StockIndex];
    if (StockItem.Item.Count < Count)
    {
        OnShopPurchase.Broadcast(BuyerPS, 0, Count, false);
        NotifyShopActionResult(BuyerPS, this, false, NSLOCTEXT("YOLOInventory", "Shop_Buy_NotEnough", "Not enough stock"));
        return;
    }

    const int64 Code = StockItem.Item.Definition.IsValid() ? StockItem.Item.Definition.Get()->UniqueCode : 0;

    // If a destination is specified, enforce placement before charging.
    if (DestPos.X >= 0 && DestPos.Y >= 0)
    {
        UYIInventoryBag* BuyerBag = BuyerInv->GetBag();
        if (!BuyerBag)
        {
            OnShopPurchase.Broadcast(BuyerPS, Code, Count, false);
            NotifyShopActionResult(BuyerPS, this, false, NSLOCTEXT("YOLOInventory", "Shop_Buy_NoBag", "No inventory bag"));
            return;
        }
        if (!BuyerBag->CanPlaceAt(DestPos, StockItem.Size))
        {
            OnShopPurchase.Broadcast(BuyerPS, Code, Count, false);
            NotifyShopActionResult(BuyerPS, this, false, NSLOCTEXT("YOLOInventory", "Shop_Buy_InvalidCell", "Invalid placement"));
            return;
        }
    }

    if (!ConsumePrice(BuyerState, Code, Count))
    {
        OnShopPurchase.Broadcast(BuyerPS, Code, Count, false);
        NotifyShopActionResult(BuyerPS, this, false, NSLOCTEXT("YOLOInventory", "Shop_Buy_NoFunds", "Not enough resources"));
        return;
    }

    // Slice and add
    FYIBagItem Slice = StockItem;
    Slice.Item.Count = Count;
    if (DestPos.X >= 0 && DestPos.Y >= 0)
    {
        Slice.Pos = DestPos;
    }
    int32 Added = INDEX_NONE;
    if (UYIInventoryBag* BuyerBag = BuyerInv->GetBag())
    {
        if (DestPos.X >= 0 && DestPos.Y >= 0)
        {
            const bool bSavedAutoMerge = BuyerBag->bAutoMergeOnAdd;
            BuyerBag->bAutoMergeOnAdd = false;
            Added = BuyerBag->AddBagItem(Slice);
            BuyerBag->bAutoMergeOnAdd = bSavedAutoMerge;
        }
        else
        {
            Added = BuyerBag->AddBagItem(Slice);
        }
    }
    if (Added == INDEX_NONE)
    {
        // refund resources
        if (BuyerState)
        {
            const FYIShopListing* Listing = FindListing(Code);
            if (Listing)
            {
                for (const FYIShopPrice& Price : Listing->Prices)
                {
                    BuyerState->AddResource(Price.Resource, Price.Amount * Count);
                }
            }
        }
        OnShopPurchase.Broadcast(BuyerPS, Code, Count, false);
        NotifyShopActionResult(BuyerPS, this, false, NSLOCTEXT("YOLOInventory", "Shop_Buy_NoSpace", "No space in inventory"));
        return;
    }

    StockItem.Item.Count -= Count;
    if (StockItem.Item.Count <= 0)
    {
        StockBag->RemoveItem(StockIndex);
    }
    BuyerInv->SyncNetState();
    if (bAutoSortStock && StockBag)
    {
        StockBag->AutoPack();
    }

    if (StockMode == EYIShopStockMode::SharedStock)
    {
        RefreshMirror();
    }
    else
    {
        NotifyStockForPlayer(BuyerPS);
    }

    OnShopPurchase.Broadcast(BuyerPS, Code, Count, true);
    NotifyShopActionResult(BuyerPS, this, true, NSLOCTEXT("YOLOInventory", "Shop_Buy_Success", "Purchased"));
    if (bDebugShopActions && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Green,
            FString::Printf(TEXT("[Shop] Buy code=%lld x%d"), Code, Count));
    }
}

bool UYIShopComponent::ServerBuyItem_Validate(int32 StockIndex, int32 Count, UYIInventoryComponent* BuyerInv, FIntPoint DestPos)
{
    return true; // lightweight; implementation does full checks
}

void UYIShopComponent::ServerSellItem_Implementation(int32 SourceIndex, int32 Count, UYIInventoryComponent* SellerInv)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !SellerInv || Count <= 0 || !bAllowSelling) return;

    APawn* SellerPawn = Cast<APawn>(SellerInv->GetOwner());
    APlayerState* SellerPS = SellerPawn ? SellerPawn->GetPlayerState() : nullptr;
    if (!SellerPS && SellerPawn && SellerPawn->GetController())
    {
        SellerPS = SellerPawn->GetController()->PlayerState;
    }
    UYIPlayerInventoryStateComponent* SellerState = SellerPS ? SellerPS->FindComponentByClass<UYIPlayerInventoryStateComponent>() : nullptr;

    UYIInventoryBag* SellerBag = SellerInv->GetBag();
    if (!SellerBag || !SellerBag->Items.IsValidIndex(SourceIndex))
    {
        OnShopSale.Broadcast(SellerPS, 0, Count, false);
        NotifyShopActionResult(SellerPS, this, false, NSLOCTEXT("YOLOInventory", "Shop_Sell_InvalidIndex", "Invalid inventory index"));
        return;
    }

    FYIBagItem OriginalItem = SellerBag->Items[SourceIndex];
    if (OriginalItem.Item.Count < Count)
    {
        OnShopSale.Broadcast(SellerPS, 0, Count, false);
        NotifyShopActionResult(SellerPS, this, false, NSLOCTEXT("YOLOInventory", "Shop_Sell_NotEnough", "Not enough items to sell"));
        return;
    }

    const int64 Code = OriginalItem.Item.Definition.IsValid() ? OriginalItem.Item.Definition.Get()->UniqueCode : 0;
    const FYIShopListing* Listing = FindListing(Code);
    if (!Listing && !bAllowSellingUnlisted)
    {
        OnShopSale.Broadcast(SellerPS, Code, Count, false);
        NotifyShopActionResult(SellerPS, this, false, NSLOCTEXT("YOLOInventory", "Shop_Sell_Unlisted", "Item cannot be sold here"));
        return;
    }

    // Remove or reduce from seller bag first (authority).
    if (OriginalItem.Item.Count == Count)
    {
        if (!SellerBag->RemoveItem(SourceIndex)) return;
    }
    else
    {
        SellerBag->Items[SourceIndex].Item.Count -= Count;
        SellerBag->OnChanged.Broadcast();
    }

    // Add to shop stock (shared or per-player).
    UYIInventoryBag* StockBag = GetStockForPlayer(SellerPS);
    if (!StockBag)
    {
        // Restore seller item on failure.
        if (OriginalItem.Item.Count == Count)
        {
            SellerBag->AddBagItem(OriginalItem);
        }
        else
        {
            SellerBag->Items[SourceIndex].Item.Count += Count;
            SellerBag->OnChanged.Broadcast();
        }
        OnShopSale.Broadcast(SellerPS, Code, Count, false);
        NotifyShopActionResult(SellerPS, this, false, NSLOCTEXT("YOLOInventory", "Shop_Sell_NoStock", "Shop cannot accept items"));
        return;
    }

    FYIBagItem Slice = OriginalItem;
    Slice.Item.Count = Count;
    const int32 Added = StockBag->AddBagItem(Slice);
    if (Added == INDEX_NONE)
    {
        // Restore seller item on failure.
        if (OriginalItem.Item.Count == Count)
        {
            SellerBag->AddBagItem(OriginalItem);
        }
        else
        {
            SellerBag->Items[SourceIndex].Item.Count += Count;
            SellerBag->OnChanged.Broadcast();
        }
        OnShopSale.Broadcast(SellerPS, Code, Count, false);
        NotifyShopActionResult(SellerPS, this, false, NSLOCTEXT("YOLOInventory", "Shop_Sell_NoSpace", "Shop stock is full"));
        return;
    }

    // Pay seller if listing exists.
    if (SellerState && Listing)
    {
        for (const FYIShopPrice& Price : Listing->Prices)
        {
            const int64 Pay = static_cast<int64>(Price.Amount * Count * SellPriceMultiplier);
            if (Pay > 0)
            {
                SellerState->AddResource(Price.Resource, Pay);
            }
        }
    }

    SellerInv->SyncNetState();
    if (bAutoSortStock && StockBag)
    {
        StockBag->AutoPack();
    }

    if (StockMode == EYIShopStockMode::SharedStock)
    {
        RefreshMirror();
    }
    else
    {
        NotifyStockForPlayer(SellerPS);
    }

    OnShopSale.Broadcast(SellerPS, Code, Count, true);
    NotifyShopActionResult(SellerPS, this, true, NSLOCTEXT("YOLOInventory", "Shop_Sell_Success", "Sold"));
    if (bDebugShopActions && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Yellow,
            FString::Printf(TEXT("[Shop] Sell code=%lld x%d"), Code, Count));
    }
}

bool UYIShopComponent::ServerSellItem_Validate(int32 SourceIndex, int32 Count, UYIInventoryComponent* SellerInv)
{
    return true;
}

void UYIShopComponent::RefreshMirror()
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    StockMirror.Reset();
    StockMirrorSize = FIntPoint(8,6);
    if (RuntimeStock)
    {
        GetStockMirrorForBag(RuntimeStock, StockMirror, StockMirrorSize);
    }
    // Push
    if (AActor* OwnerActor = GetOwner())
    {
        OwnerActor->ForceNetUpdate();
    }
    // Listen-server UI needs a local notify (OnRep won't fire on server)
    if (GetWorld() && GetWorld()->GetNetMode() != NM_DedicatedServer)
    {
        OnStockMirrorUpdated.Broadcast();
    }
}

void UYIShopComponent::OnRep_StockMirror()
{
    OnStockMirrorUpdated.Broadcast();
}

void UYIShopComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UYIShopComponent, StockMirror);
    DOREPLIFETIME(UYIShopComponent, StockMirrorSize);
}

UYIInventoryBag* UYIShopComponent::CreateStockInstance() const
{
    if (StockTemplate.IsValid())
    {
        return DuplicateObject<UYIInventoryBag>(StockTemplate.Get(), const_cast<UYIShopComponent*>(this));
    }
    if (StockTemplate.ToSoftObjectPath().IsValid())
    {
        if (UYIInventoryBag* Loaded = StockTemplate.LoadSynchronous())
        {
            return DuplicateObject<UYIInventoryBag>(Loaded, const_cast<UYIShopComponent*>(this));
        }
    }
    UYIInventoryBag* NewBag = NewObject<UYIInventoryBag>(const_cast<UYIShopComponent*>(this));
    NewBag->GridSize = FIntPoint(8,6);
    return NewBag;
}

UYIInventoryBag* UYIShopComponent::GetStockForPlayer(APlayerState* PlayerState)
{
    if (StockMode == EYIShopStockMode::SharedStock)
    {
        return RuntimeStock;
    }
    if (StockMode == EYIShopStockMode::LocalOnly)
    {
        return RuntimeStock;
    }
    if (!PlayerState)
    {
        return RuntimeStock;
    }
    if (TObjectPtr<UYIInventoryBag>* Found = PlayerStock.Find(PlayerState))
    {
        return Found->Get();
    }
    UYIInventoryBag* NewBag = CreateStockInstance();
    PlayerStock.Add(PlayerState, NewBag);
    return NewBag;
}

void UYIShopComponent::GetStockMirrorForBag(const UYIInventoryBag* Bag, TArray<FYINetBagItem>& OutItems, FIntPoint& OutSize) const
{
    OutItems.Reset();
    OutSize = FIntPoint(8,6);
    if (!Bag) return;

    OutSize = Bag->GridSize;
    for (const FYIBagItem& It : Bag->Items)
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
        OutItems.Add(Net);
    }
}

void UYIShopComponent::GetStockMirrorForPlayer(APlayerState* PlayerState, TArray<FYINetBagItem>& OutItems, FIntPoint& OutSize)
{
	const UYIInventoryBag* StockBag = GetStockForPlayer(PlayerState);
	GetStockMirrorForBag(StockBag, OutItems, OutSize);
}

void UYIShopComponent::NotifyStockForPlayer(APlayerState* PlayerState)
{
    if (!PlayerState) return;
    APlayerController* PC = Cast<APlayerController>(PlayerState->GetOwner());
    if (!PC) return;

    UYITradeInteractionComponent* TradeComp = PC->FindComponentByClass<UYITradeInteractionComponent>();
    if (!TradeComp) return;

    TArray<FYINetBagItem> OutItems;
    FIntPoint OutSize = FIntPoint(0,0);
    UYIInventoryBag* StockBag = GetStockForPlayer(PlayerState);
    GetStockMirrorForBag(StockBag, OutItems, OutSize);
    TradeComp->Client_ShopStockReady(this, OutItems, OutSize);
}
