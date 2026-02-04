#include "YIShopComponent.h"

#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "YIInventoryBag.h"
#include "YIInventoryComponent.h"
#include "YIPlayerInventoryStateComponent.h"
#include "YIItemDefinition.h"
#include "YIItemBlueprintLibrary.h"

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
        RefreshMirror();
        if (RestockInterval > 0.f)
        {
            GetWorld()->GetTimerManager().SetTimer(RestockTimer, this, &UYIShopComponent::BuildRuntimeStock, RestockInterval, true);
        }
    }
}

void UYIShopComponent::BuildRuntimeStock()
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;

    RuntimeStock = nullptr;
    if (StockTemplate.IsValid())
    {
        RuntimeStock = DuplicateObject<UYIInventoryBag>(StockTemplate.Get(), this);
    }
    else if (StockTemplate.ToSoftObjectPath().IsValid())
    {
        RuntimeStock = DuplicateObject<UYIInventoryBag>(StockTemplate.LoadSynchronous(), this);
    }
    if (!RuntimeStock)
    {
        // create empty
        RuntimeStock = NewObject<UYIInventoryBag>(this);
        RuntimeStock->GridSize = FIntPoint(8,6);
    }
    RefreshMirror();
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

void UYIShopComponent::ServerBuyItem_Implementation(int32 StockIndex, int32 Count, UYIInventoryComponent* BuyerInv)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !RuntimeStock || !BuyerInv || Count <= 0) return;

    if (!RuntimeStock->Items.IsValidIndex(StockIndex)) return;
    FYIBagItem& StockItem = RuntimeStock->Items[StockIndex];
    if (StockItem.Item.Count < Count) return;

    const int64 Code = StockItem.Item.Definition.IsValid() ? StockItem.Item.Definition.Get()->UniqueCode : 0;

    UYIPlayerInventoryStateComponent* BuyerState = BuyerInv->GetOwner() ? BuyerInv->GetOwner()->FindComponentByClass<UYIPlayerInventoryStateComponent>() : nullptr;
    if (!ConsumePrice(BuyerState, Code, Count)) return;

    // Slice and add
    FYIBagItem Slice = StockItem;
    Slice.Item.Count = Count;
    int32 Added = BuyerInv->GetBag() ? BuyerInv->GetBag()->AddBagItem(Slice) : INDEX_NONE;
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
        return;
    }

    StockItem.Item.Count -= Count;
    if (StockItem.Item.Count <= 0)
    {
        RuntimeStock->RemoveItem(StockIndex);
    }
    BuyerInv->SyncNetState();
    RefreshMirror();
}

bool UYIShopComponent::ServerBuyItem_Validate(int32 StockIndex, int32 Count, UYIInventoryComponent* BuyerInv)
{
    return true; // lightweight; implementation does full checks
}

void UYIShopComponent::RefreshMirror()
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    StockMirror.Reset();
    StockMirrorSize = FIntPoint(8,6);
    if (RuntimeStock)
    {
        StockMirrorSize = RuntimeStock->GridSize;
        for (const FYIBagItem& It : RuntimeStock->Items)
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
            StockMirror.Add(Net);
        }
    }
    // Push
    if (AActor* OwnerActor = GetOwner())
    {
        OwnerActor->ForceNetUpdate();
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
