#include "YITradeSessionActor.h"

#include "Net/UnrealNetwork.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "YIInventoryComponent.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "Engine/Engine.h"

AYITradeSessionActor::AYITradeSessionActor()
{
    bReplicates = true;
    SetReplicateMovement(false);
    SetActorEnableCollision(false);
    bAlwaysRelevant = true;
    bNetUseOwnerRelevancy = false;
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
}

// Helper to produce a net-safe view of a bag (no maps)
static void CopyBagToNetView(const UYIInventoryComponent* Inv, TArray<FYINetBagItem>& Out, FIntPoint& OutGridSize)
{
    Out.Reset();
    OutGridSize = FIntPoint(10,6);
    if (!Inv) return;
    if (const UYIInventoryBag* Bag = Inv->GetBag())
    {
        OutGridSize = Bag->GridSize;
        for (const FYIBagItem& It : Bag->Items)
        {
            if (It.Item.Count <= 0) continue;
            FYINetBagItem Net;
            Net.Code = It.Item.Definition.IsValid() ? It.Item.Definition.Get()->UniqueCode : 0;
            if (Net.Code == 0 && It.Item.Definition.ToSoftObjectPath().IsValid())
            {
                if (const UYIItemDefinition* Def = Cast<UYIItemDefinition>(It.Item.Definition.LoadSynchronous()))
                {
                    Net.Code = Def->UniqueCode;
                }
            }
            Net.Count = It.Item.Count;
            Net.Pos = It.Pos;
            Net.Size = It.Size;
            Net.CustomStackKey = It.Item.CustomStackKey;
            Out.Add(Net);
        }
    }
}

FYITradeOffer& AYITradeSessionActor::GetOffer(ETradeSide Side)
{
    return (Side == ETradeSide::SideA) ? OfferA : OfferB;
}

const FYITradeOffer& AYITradeSessionActor::GetOffer(ETradeSide Side) const
{
    return (Side == ETradeSide::SideA) ? OfferA : OfferB;
}

ETradeSide AYITradeSessionActor::GetSideForPlayer(APlayerState* Player) const
{
    if (!Player) return ETradeSide::SideA;
    if (Player == PlayerA) return ETradeSide::SideA;
    if (Player == PlayerB) return ETradeSide::SideB;
    return ETradeSide::SideA;
}

TArray<FYITradeOfferSource>& AYITradeSessionActor::GetOfferSources(ETradeSide Side)
{
    return (Side == ETradeSide::SideA) ? OfferSourcesA : OfferSourcesB;
}

FYITradeOfferSource& AYITradeSessionActor::GetOfferSource(ETradeSide Side, int32 Index)
{
    return GetOfferSources(Side)[Index];
}

bool AYITradeSessionActor::IsSideOwner(ETradeSide Side, APlayerController* PC) const
{
	if (!PC) return false;
	const APlayerState* PS = PC->PlayerState;
	return (Side == ETradeSide::SideA) ? (PS == PlayerA) : (PS == PlayerB);
}

void AYITradeSessionActor::RefreshInventoryViews()
{
	if (!HasAuthority()) return;
	CopyBagToNetView(GetInventoryForSide(ETradeSide::SideA), InventoryA, InventorySizeA);
	CopyBagToNetView(GetInventoryForSide(ETradeSide::SideB), InventoryB, InventorySizeB);
	ForceNetUpdate();
}

void AYITradeSessionActor::ServerAddItem_Implementation(ETradeSide Side, UYIInventoryComponent* SourceInv, int32 SlotIndex, int32 Count)
{
	if (!HasAuthority() || !SourceInv || SlotIndex == INDEX_NONE || Count <= 0)
	{
		return;
    }

    UYIInventoryBag* Bag = SourceInv->GetBag();
    if (!Bag || !Bag->Items.IsValidIndex(SlotIndex))
    {
        return;
    }
    const FYIBagItem& SrcItem = Bag->Items[SlotIndex];
    if (SrcItem.Item.Count < Count)
    {
        return;
    }

    FYIBagItem Slice = SrcItem;
    Slice.Item.Count = Count;

    FYINetBagItem Net;
    Net.Code = SrcItem.Item.Definition.IsValid() ? SrcItem.Item.Definition.Get()->UniqueCode : 0;
    if (Net.Code == 0 && SrcItem.Item.Definition.ToSoftObjectPath().IsValid())
    {
        if (UYIItemDefinition* Def = Cast<UYIItemDefinition>(SrcItem.Item.Definition.LoadSynchronous()))
        {
            Net.Code = Def->UniqueCode;
        }
    }
    Net.Count = Count;
    Net.Pos = SrcItem.Pos;
    Net.Size = SrcItem.Size;
    Net.CustomStackKey = SrcItem.Item.CustomStackKey;

    FYITradeOffer& Offer = GetOffer(Side);
    TArray<FYITradeOfferSource>& Sources = GetOfferSources(Side);
    Offer.Items.Add(Net);

    FYITradeOfferSource Src;
    Src.SourceInv = SourceInv;
    Src.SlotIndex = SlotIndex;
    Src.Count = Count;
    Src.ItemCopy = Slice;
    Sources.Add(Src);

    OnRep_Offers();
    if (Side == ETradeSide::SideA) bAReady = false; else bBReady = false;
    RefreshInventoryViews();
}

void AYITradeSessionActor::ServerRemoveItem_Implementation(ETradeSide Side, int32 Index)
{
    FYITradeOffer& Offer = GetOffer(Side);
    if (Offer.Items.IsValidIndex(Index))
    {
        Offer.Items.RemoveAt(Index);
        TArray<FYITradeOfferSource>& Sources = GetOfferSources(Side);
        if (Sources.IsValidIndex(Index)) { Sources.RemoveAt(Index); }
        OnRep_Offers();
        if (Side == ETradeSide::SideA) bAReady = false; else bBReady = false;
        RefreshInventoryViews();
    }
}

void AYITradeSessionActor::ServerSetResource_Implementation(ETradeSide Side, FName Resource, int64 Amount)
{
    FYITradeOffer& Offer = GetOffer(Side);
    Offer.Resources.Add(Resource, Amount - Offer.Resources.Get(Resource));
    OnRep_Offers();
    RefreshInventoryViews();
}

void AYITradeSessionActor::ServerSetReady_Implementation(ETradeSide Side, bool bReady)
{
    if (Side == ETradeSide::SideA) bAReady = bReady; else bBReady = bReady;
    TryCommit();
}

void AYITradeSessionActor::ServerCancel_Implementation()
{
    if (OnTradeCancelled.IsBound()) { OnTradeCancelled.Broadcast(); }
    Destroy();
}

UYIInventoryComponent* AYITradeSessionActor::GetInventoryForSide(ETradeSide Side) const
{
    if (Side == ETradeSide::SideA && PawnA)
    {
        return PawnA->FindComponentByClass<UYIInventoryComponent>();
    }
    if (Side == ETradeSide::SideB)
    {
        APawn* UsePawn = NPCPawn ? NPCPawn.Get() : PawnB.Get();
        if (UsePawn)
        {
            return UsePawn->FindComponentByClass<UYIInventoryComponent>();
        }
    }
    return nullptr;
}

bool AYITradeSessionActor::ApplyOffersToSide(ETradeSide From, ETradeSide To, FText& OutError)
{
    UYIInventoryComponent* DestInv = GetInventoryForSide(To);
    if (!DestInv || !DestInv->GetOwner() || DestInv->GetOwner()->GetLocalRole() != ROLE_Authority)
    {
        OutError = FText::FromString(TEXT("Destination inventory missing"));
        return false;
    }

    TArray<FYITradeOfferSource>& Sources = GetOfferSources(From);

    for (int32 i = 0; i < Sources.Num(); ++i)
    {
        const FYITradeOfferSource& Src = Sources[i];
        UYIInventoryComponent* SrcInv = Src.SourceInv.Get();
        if (!SrcInv || !SrcInv->GetBag() || !SrcInv->GetBag()->Items.IsValidIndex(Src.SlotIndex))
        {
            OutError = FText::FromString(TEXT("Source item missing"));
            return false;
        }

        FYIBagItem& SrcSlot = SrcInv->GetBag()->Items[Src.SlotIndex];
        if (SrcSlot.Item.Count < Src.Count)
        {
            OutError = FText::FromString(TEXT("Insufficient count"));
            return false;
        }

        // Remove from source
        SrcSlot.Item.Count -= Src.Count;
        if (SrcSlot.Item.Count <= 0)
        {
            SrcInv->GetBag()->RemoveItem(Src.SlotIndex);
        }
        SrcInv->SyncNetState();

        // Add to dest
        FYIBagItem ToAdd = Src.ItemCopy;
        int32 AddedIdx = DestInv->GetBag()->AddBagItem(ToAdd);
        if (AddedIdx == INDEX_NONE)
        {
            OutError = FText::FromString(TEXT("Destination bag full"));
            return false;
        }
    }

    DestInv->SyncNetState();
    return true;
}

void AYITradeSessionActor::TryCommit()
{
    if (!bAReady || !bBReady)
    {
        return;
    }
    FText Err;
    if (!ApplyOffersToSide(ETradeSide::SideA, ETradeSide::SideB, Err) ||
        !ApplyOffersToSide(ETradeSide::SideB, ETradeSide::SideA, Err))
    {
        bAReady = bBReady = false;
        OnRep_Offers();
        RefreshInventoryViews();
        if (OnTradeFailed.IsBound()) { OnTradeFailed.Broadcast(); }
        return;
    }

    if (OnTradeCommitted.IsBound()) { OnTradeCommitted.Broadcast(); }
    Destroy();
}

void AYITradeSessionActor::OnRep_Offers()
{
    if (OnOffersUpdated.IsBound()) { OnOffersUpdated.Broadcast(); }
}

void AYITradeSessionActor::OnRep_Inventories()
{
    if (OnInventoriesUpdated.IsBound()) { OnInventoriesUpdated.Broadcast(); }
    // Also fire offers updated so UI can refresh both mirrors in one pass
    if (OnOffersUpdated.IsBound()) { OnOffersUpdated.Broadcast(); }
}

void AYITradeSessionActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AYITradeSessionActor, PlayerA);
    DOREPLIFETIME(AYITradeSessionActor, PlayerB);
    DOREPLIFETIME(AYITradeSessionActor, bSideBIsNPC);
    DOREPLIFETIME(AYITradeSessionActor, PawnA);
    DOREPLIFETIME(AYITradeSessionActor, PawnB);
    DOREPLIFETIME(AYITradeSessionActor, NPCPawn);
    DOREPLIFETIME(AYITradeSessionActor, OfferA);
    DOREPLIFETIME(AYITradeSessionActor, OfferB);
    DOREPLIFETIME(AYITradeSessionActor, bAReady);
    DOREPLIFETIME(AYITradeSessionActor, bBReady);
    DOREPLIFETIME(AYITradeSessionActor, InventoryA);
    DOREPLIFETIME(AYITradeSessionActor, InventoryB);
    DOREPLIFETIME(AYITradeSessionActor, InventorySizeA);
    DOREPLIFETIME(AYITradeSessionActor, InventorySizeB);
}
