#include "YITradeInteractionComponent.h"

#include "YIInventoryBlueprintLibrary.h"
#include "YIInventoryComponent.h"
#include "YIShopComponent.h"
#include "YIInventoryBag.h"
#include "YITradeSessionActor.h"
#include "TradingScreenWidget.h"
#include "ShopScreenWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"
#include "YIInventoryComponent.h"

UYITradeInteractionComponent::UYITradeInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetComponentTickEnabled(false);
    SetIsReplicatedByDefault(true);
}

void UYITradeInteractionComponent::BeginPlay()
{
    Super::BeginPlay();

    // Ensure this is attached to a PC for clarity/debugging.
    if (!IsOwnerValidForTrade(true))
    {
        UE_LOG(LogTemp, Warning, TEXT("YITradeInteractionComponent should be on a PlayerController. Owner: %s"), *GetNameSafe(GetOwner()));
    }

    // Auto-bind to inventory on possession changes
    if (APlayerController* PC = GetOwningPC())
    {
        PC->OnPossessedPawnChanged.AddDynamic(this, &UYITradeInteractionComponent::OnPossessedPawnChanged);
        OnPossessedPawnChanged(nullptr, PC->GetPawn());
    }
}

void UYITradeInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    APlayerController* PC = GetOwningPC();
    if (!PC || !PC->IsLocalController())
    {
        return;
    }

    // Trade distance enforcement
    if (CurrentSession)
    {
        if (AActor* Other = GetOtherTradeActor())
        {
            if (!IsWithinDistance(Other, TradeKeepAliveDistance))
            {
                CloseTradeLocal(true);
            }
        }
    }

    // Shop distance enforcement
    if (CurrentShop && CurrentShop->GetOwner())
    {
        if (!IsWithinDistance(CurrentShop->GetOwner(), ShopKeepAliveDistance))
        {
            CloseShopLocal();
        }
    }

    // Disable tick when nothing is active
    if (!CurrentSession && !CurrentShop)
    {
        SetComponentTickEnabled(false);
    }
}

void UYITradeInteractionComponent::RequestTrade(AActor* Target, bool bTargetIsNPC)
{
    // Client entry point: validate locally (Blueprints can override), then forward to server
    if (!IsOwnerValidForTrade(true))
    {
        Client_TradeSessionFailed(NSLOCTEXT("YOLOInventory", "Trade_InvalidOwner", "Trade component must be on PlayerController"));
        return;
    }
    if (!ValidateTradeRequest(Target, bTargetIsNPC))
    {
        Client_TradeSessionFailed(NSLOCTEXT("YOLOInventory", "Trade_ClientRejected", "Trade request rejected"));
        return;
    }
    if (!IsWithinDistance(Target, TradeInteractionDistance))
    {
        Client_TradeSessionFailed(NSLOCTEXT("YOLOInventory", "Trade_TooFar", "Target is too far"));
        return;
    }
    Server_RequestTrade(Target, bTargetIsNPC);
}

void UYITradeInteractionComponent::RequestTradeTransfer(ETradeSide FromSide, ETradeSide ToSide, int32 SourceIndex, FIntPoint DestPos, int32 Count)
{
    if (!IsOwnerValidForTrade(true))
    {
        return;
    }
    Server_TransferItem(FromSide, ToSide, SourceIndex, DestPos, Count);
}

void UYITradeInteractionComponent::RequestShop(UYIShopComponent* Shop)
{
    if (!IsOwnerValidForTrade(true))
    {
        return;
    }
    if (!Shop)
    {
        return;
    }
    if (!ValidateShopRequest(Shop))
    {
        return;
    }
    if (Shop->GetOwner() && !IsWithinDistance(Shop->GetOwner(), ShopInteractionDistance))
    {
        return;
    }
    Server_RequestShop(Shop);
}

void UYITradeInteractionComponent::RequestShopBuy(UYIShopComponent* Shop, int32 StockIndex, int32 Count, UYIInventoryComponent* BuyerInv)
{
    if (!IsOwnerValidForTrade(true))
    {
        return;
    }
    if (!Shop || !BuyerInv || Count <= 0)
    {
        return;
    }
    Server_RequestShopBuy(Shop, StockIndex, Count, BuyerInv);
}

void UYITradeInteractionComponent::RequestShopSell(UYIShopComponent* Shop, int32 SourceIndex, int32 Count, UYIInventoryComponent* SellerInv)
{
    if (!IsOwnerValidForTrade(true))
    {
        return;
    }
    if (!Shop || !SellerInv || Count <= 0)
    {
        return;
    }
    Server_RequestShopSell(Shop, SourceIndex, Count, SellerInv);
}

void UYITradeInteractionComponent::Server_RequestTrade_Implementation(AActor* Target, bool bTargetIsNPC)
{
    APlayerController* PC = GetOwningPC();
    if (!PC || !PC->PlayerState)
    {
        Client_TradeSessionFailed(NSLOCTEXT("YOLOInventory", "Trade_NoPC", "Invalid player controller"));
        return;
    }
    if (!Target)
    {
        Client_TradeSessionFailed(NSLOCTEXT("YOLOInventory", "Trade_NoTarget", "No trade target"));
        return;
    }

    APawn* InitiatorPawn = PC->GetPawn();
    if (!InitiatorPawn)
    {
        Client_TradeSessionFailed(NSLOCTEXT("YOLOInventory", "Trade_NoPawn", "You have no pawn"));
        return;
    }

    AYITradeSessionActor* Session = UYIInventoryBlueprintLibrary::StartTradeSession(this, InitiatorPawn, Target, bTargetIsNPC);
    if (!Session)
    {
        Client_TradeSessionFailed(NSLOCTEXT("YOLOInventory", "Trade_SpawnFail", "Could not start trade"));
        return;
    }

    Session->SetOwner(PC); // ensure relevance
    Session->ForceNetUpdate();

    CurrentSession = Session;
    if (AActor* OwnerActor = GetOwner())
    {
        OwnerActor->ForceNetUpdate(); // push replicated property to owning client
    }

    // Only the owning local controller drives UI immediately; remote clients will get OnRep when the actor resolves for them.
    if (PC->IsLocalController())
    {
        OnRep_CurrentSession();
    }

    // If target is a player, also assign the session to their trade component so their client opens the UI.
    if (!bTargetIsNPC)
    {
        APawn* TargetPawn = Cast<APawn>(Target);
        APlayerController* TargetPC = TargetPawn ? Cast<APlayerController>(TargetPawn->GetController()) : nullptr;
        if (TargetPC)
        {
            if (UYITradeInteractionComponent* TargetComp = TargetPC->FindComponentByClass<UYITradeInteractionComponent>())
            {
                TargetComp->ServerAssignSession(Session);
            }
        }
    }
}

bool UYITradeInteractionComponent::Server_RequestTrade_Validate(AActor* Target, bool bTargetIsNPC)
{
    // Keep validation permissive to avoid disconnects; do real checks in _Implementation.
    // Only sanity-check that we have a PC owner; if not, reject to prevent spoofing.
    return IsOwnerValidForTrade(false);
}

void UYITradeInteractionComponent::Server_RequestShop_Implementation(UYIShopComponent* Shop)
{
    if (!IsOwnerValidForTrade(false) || !Shop)
    {
        return;
    }

    APlayerController* PC = GetOwningPC();
    if (!PC || !PC->PlayerState)
    {
        return;
    }

    TArray<FYINetBagItem> OutItems;
    FIntPoint OutSize = FIntPoint(0,0);
    Shop->GetStockMirrorForPlayer(PC->PlayerState, OutItems, OutSize);
    Client_ShopStockReady(Shop, OutItems, OutSize);
}

bool UYITradeInteractionComponent::Server_RequestShop_Validate(UYIShopComponent* Shop)
{
    return IsOwnerValidForTrade(false) && Shop != nullptr;
}

void UYITradeInteractionComponent::Server_RequestShopBuy_Implementation(UYIShopComponent* Shop, int32 StockIndex, int32 Count, UYIInventoryComponent* BuyerInv)
{
    if (!IsOwnerValidForTrade(false) || !Shop || !BuyerInv || Count <= 0)
    {
        Client_ShopActionResult(Shop, false, NSLOCTEXT("YOLOInventory", "Shop_Buy_Invalid", "Invalid buy request"));
        return;
    }

    APlayerController* PC = GetOwningPC();
    if (!PC || !PC->PlayerState)
    {
        Client_ShopActionResult(Shop, false, NSLOCTEXT("YOLOInventory", "Shop_Buy_NoPC", "No player controller"));
        return;
    }

    // Only allow buying into the requester's inventory.
    if (APawn* Pawn = PC->GetPawn())
    {
        if (BuyerInv->GetOwner() != Pawn)
        {
            Client_ShopActionResult(Shop, false, NSLOCTEXT("YOLOInventory", "Shop_Buy_WrongOwner", "Invalid inventory owner"));
            return;
        }
    }

    Shop->ServerBuyItem(StockIndex, Count, BuyerInv);
    Client_ShopActionResult(Shop, true, NSLOCTEXT("YOLOInventory", "Shop_Buy_OK", "Purchase requested"));
}

bool UYITradeInteractionComponent::Server_RequestShopBuy_Validate(UYIShopComponent* Shop, int32 StockIndex, int32 Count, UYIInventoryComponent* BuyerInv)
{
    return IsOwnerValidForTrade(false) && Shop != nullptr && BuyerInv != nullptr && Count > 0;
}

void UYITradeInteractionComponent::Server_RequestShopSell_Implementation(UYIShopComponent* Shop, int32 SourceIndex, int32 Count, UYIInventoryComponent* SellerInv)
{
    if (!IsOwnerValidForTrade(false) || !Shop || !SellerInv || Count <= 0)
    {
        Client_ShopActionResult(Shop, false, NSLOCTEXT("YOLOInventory", "Shop_Sell_Invalid", "Invalid sell request"));
        return;
    }

    APlayerController* PC = GetOwningPC();
    if (!PC || !PC->PlayerState)
    {
        Client_ShopActionResult(Shop, false, NSLOCTEXT("YOLOInventory", "Shop_Sell_NoPC", "No player controller"));
        return;
    }

    // Only allow selling from the requester's inventory.
    if (APawn* Pawn = PC->GetPawn())
    {
        if (SellerInv->GetOwner() != Pawn)
        {
            Client_ShopActionResult(Shop, false, NSLOCTEXT("YOLOInventory", "Shop_Sell_WrongOwner", "Invalid inventory owner"));
            return;
        }
    }

    Shop->ServerSellItem(SourceIndex, Count, SellerInv);
    Client_ShopActionResult(Shop, true, NSLOCTEXT("YOLOInventory", "Shop_Sell_OK", "Sale requested"));
}

bool UYITradeInteractionComponent::Server_RequestShopSell_Validate(UYIShopComponent* Shop, int32 SourceIndex, int32 Count, UYIInventoryComponent* SellerInv)
{
    return IsOwnerValidForTrade(false) && Shop != nullptr && SellerInv != nullptr && Count > 0;
}

void UYITradeInteractionComponent::Server_TransferItem_Implementation(ETradeSide FromSide, ETradeSide ToSide, int32 SourceIndex, FIntPoint DestPos, int32 Count)
{
    if (!IsOwnerValidForTrade(false) || !CurrentSession)
    {
        return;
    }

    APlayerController* PC = GetOwningPC();
    if (!PC || !PC->PlayerState)
    {
        return;
    }

    // Only allow participants to move items.
    const ETradeSide CallerSide = CurrentSession->GetSideForPlayer(PC->PlayerState);
    if (CallerSide != ETradeSide::SideA && CallerSide != ETradeSide::SideB)
    {
        return;
    }

    // Allow any direction during an active trade (shop-style or free trade).
    CurrentSession->ServerTransferItemBetweenSides(FromSide, ToSide, SourceIndex, DestPos, Count);
}

bool UYITradeInteractionComponent::Server_TransferItem_Validate(ETradeSide FromSide, ETradeSide ToSide, int32 SourceIndex, FIntPoint DestPos, int32 Count)
{
    return IsOwnerValidForTrade(false);
}

bool UYITradeInteractionComponent::ValidateTradeRequest_Implementation(AActor* Target, bool bTargetIsNPC) const
{
    // Default implementation: allow if both actors exist and target is not the same pawn.
    if (!Target) return false;
    if (const APawn* MyPawn = GetOwningPC() ? GetOwningPC()->GetPawn() : nullptr)
    {
        if (Target == MyPawn) return false; // disallow self-trade by default
    }
    return true;
}

bool UYITradeInteractionComponent::ValidateShopRequest_Implementation(UYIShopComponent* Shop) const
{
    return Shop != nullptr;
}

void UYITradeInteractionComponent::Client_TradeSessionStarted_Implementation(AYITradeSessionActor* Session)
{
    // Let replicated CurrentSession + OnRep drive UI once the actor is fully resolved client-side.
    // This avoids null pointers when the RPC arrives before actor replication is complete.
}

void UYITradeInteractionComponent::Client_TradeSessionFailed_Implementation(const FText& Reason)
{
    OnTradeFailed.Broadcast(Reason);
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Red, Reason.ToString());
    }
}

void UYITradeInteractionComponent::Client_ShopStockReady_Implementation(UYIShopComponent* Shop, const TArray<FYINetBagItem>& Stock, FIntPoint Size)
{
    CurrentShop = Shop;
    CurrentShopStock = Stock;
    CurrentShopStockSize = Size;
    OnShopStockUpdated.Broadcast(Shop);

    if (GetOwningPC() && GetOwningPC()->IsLocalController())
    {
        if (APawn* P = GetOwningPC()->GetPawn())
        {
            if (UYIInventoryComponent* Inv = P->FindComponentByClass<UYIInventoryComponent>())
            {
                // Always update if a shop screen is already open.
                Inv->UpdateShopScreen(Shop, Inv->GetBag(), Stock, Size);
                if (bAutoShowShopWidget)
                {
                    Inv->OpenShopScreen(Shop, Inv->GetBag(), Stock, Size);
                }

                // Ensure focus/input stays on this local controller window
                GetOwningPC()->bShowMouseCursor = true;
                FInputModeGameAndUI Mode;
                Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                Mode.SetHideCursorDuringCapture(false);
                GetOwningPC()->SetInputMode(Mode);
            }
        }
        if (bAutoShowShopWidget && AutoShopWidgetClass)
        {
            if (UShopScreenWidget* WidgetInst = CreateWidget<UShopScreenWidget>(GetOwningPC(), AutoShopWidgetClass))
            {
                WidgetInst->AddToViewport();
                UYIInventoryBag* LocalBag = nullptr;
                if (APawn* P = GetOwningPC()->GetPawn())
                {
                    if (UYIInventoryComponent* Inv = P->FindComponentByClass<UYIInventoryComponent>())
                    {
                        LocalBag = Inv->GetBag();
                    }
                }
                WidgetInst->SetShop(Shop, LocalBag, Stock, Size);
            }
        }
    }

    if (CurrentShop)
    {
        if (!bShopOpened)
        {
            OnShopOpened.Broadcast(CurrentShop);
            bShopOpened = true;
        }
        SetComponentTickEnabled(true);
    }
}

void UYITradeInteractionComponent::Client_ShopActionResult_Implementation(UYIShopComponent* Shop, bool bSuccess, const FText& Reason)
{
    OnShopActionResult.Broadcast(Shop, bSuccess, Reason);
    if (bDebugTradeInteraction && GEngine)
    {
        const FColor Color = bSuccess ? FColor::Green : FColor::Red;
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, Color, Reason.ToString());
    }
}

APlayerController* UYITradeInteractionComponent::GetOwningPC() const
{
    return Cast<APlayerController>(GetOwner());
}

bool UYITradeInteractionComponent::IsOwnerValidForTrade(bool bLogWarning) const
{
    const bool bValid = GetOwningPC() != nullptr;
    if (!bValid && bLogWarning)
    {
        UE_LOG(LogTemp, Warning, TEXT("YITradeInteractionComponent owner is not a PlayerController (%s)"), *GetNameSafe(GetOwner()));
    }
    return bValid;
}

void UYITradeInteractionComponent::OnRep_CurrentSession()
{
	if (CurrentSession)
	{
		OnTradeSessionReady.Broadcast(CurrentSession);
        OnTradeOpened.Broadcast(CurrentSession);

        if (BoundSession.IsValid())
        {
            BoundSession->OnTradeCancelled.RemoveAll(this);
            BoundSession->OnTradeCommitted.RemoveAll(this);
            BoundSession->OnTradeFailed.RemoveAll(this);
        }
        BoundSession = CurrentSession;
        CurrentSession->OnTradeCancelled.AddDynamic(this, &UYITradeInteractionComponent::HandleTradeEnded);
        CurrentSession->OnTradeCommitted.AddDynamic(this, &UYITradeInteractionComponent::HandleTradeEnded);
        CurrentSession->OnTradeFailed.AddDynamic(this, &UYITradeInteractionComponent::HandleTradeEnded);
        SetComponentTickEnabled(true);

		if (bAutoShowWidget && GetOwningPC() && GetOwningPC()->IsLocalController())
		{
			// Prefer central UI helper on the inventory component
			if (APawn* P = GetOwningPC()->GetPawn())
			{
				if (UYIInventoryComponent* Inv = P->FindComponentByClass<UYIInventoryComponent>())
				{
					Inv->OpenTradeScreen(CurrentSession, Inv->GetBag());

					// Ensure focus/input stays on this local controller window
					GetOwningPC()->bShowMouseCursor = true;
					FInputModeGameAndUI Mode;
					Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
					Mode.SetHideCursorDuringCapture(false);
					GetOwningPC()->SetInputMode(Mode);
					return;
				}
			}
            // Fallback to legacy auto widget path if no inventory component
            if (AutoTradeWidgetClass)
            {
                if (UTradingScreenWidget* WidgetInst = CreateWidget<UTradingScreenWidget>(GetOwningPC(), AutoTradeWidgetClass))
                {
                    WidgetInst->AddToViewport();
                    UYIInventoryBag* LocalBag = nullptr;
                    if (APawn* P = GetOwningPC()->GetPawn())
                    {
                        if (UYIInventoryComponent* Inv = P->FindComponentByClass<UYIInventoryComponent>())
                        {
                            LocalBag = Inv->GetBag();
                        }
                    }
                    WidgetInst->SetSession(CurrentSession, LocalBag, nullptr);
			}
		}
	}
}
}

void UYITradeInteractionComponent::ServerAssignSession(AYITradeSessionActor* Session)
{
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;
	CurrentSession = Session;
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->ForceNetUpdate();
	}
	if (APlayerController* PC = GetOwningPC())
	{
		if (PC->IsLocalController())
		{
			OnRep_CurrentSession();
		}
	}
}

void UYITradeInteractionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UYITradeInteractionComponent, CurrentSession, COND_OwnerOnly);
}

void UYITradeInteractionComponent::OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
    // Unbind old pawn bag delegates
    if (OldPawn)
    {
        if (UYIInventoryComponent* Inv = OldPawn->FindComponentByClass<UYIInventoryComponent>())
        {
            if (UYIInventoryBag* Bag = Inv->GetBag())
            {
                Bag->OnItemAdded.RemoveDynamic(this, &UYITradeInteractionComponent::HandleBagItemAdded);
                Bag->OnItemRemoved.RemoveDynamic(this, &UYITradeInteractionComponent::HandleBagItemRemoved);
                Bag->OnItemMoved.RemoveDynamic(this, &UYITradeInteractionComponent::HandleBagItemMoved);
                Bag->OnItemRotated.RemoveDynamic(this, &UYITradeInteractionComponent::HandleBagItemRotated);
                Bag->OnItemTransferred.RemoveDynamic(this, &UYITradeInteractionComponent::HandleBagItemTransferred);
            }
        }
    }

    // Bind new pawn bag delegates
    if (NewPawn)
    {
        if (UYIInventoryComponent* Inv = NewPawn->FindComponentByClass<UYIInventoryComponent>())
        {
            if (UYIInventoryBag* Bag = Inv->GetBag())
            {
                Bag->OnItemAdded.AddDynamic(this, &UYITradeInteractionComponent::HandleBagItemAdded);
                Bag->OnItemRemoved.AddDynamic(this, &UYITradeInteractionComponent::HandleBagItemRemoved);
                Bag->OnItemMoved.AddDynamic(this, &UYITradeInteractionComponent::HandleBagItemMoved);
                Bag->OnItemRotated.AddDynamic(this, &UYITradeInteractionComponent::HandleBagItemRotated);
                Bag->OnItemTransferred.AddDynamic(this, &UYITradeInteractionComponent::HandleBagItemTransferred);
            }
        }
    }
}

void UYITradeInteractionComponent::HandleBagItemAdded(int32 Index, FYIBagItem Item)
{
    if (bDebugTradeInteraction && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Green,
            FString::Printf(TEXT("[TradeInt] Added idx %d count %d"), Index, Item.Item.Count));
    }
    OnBagItemAdded.Broadcast(Index, Item);
}

void UYITradeInteractionComponent::HandleBagItemRemoved(int32 Index, FYIBagItem Item)
{
    if (bDebugTradeInteraction && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Orange,
            FString::Printf(TEXT("[TradeInt] Removed idx %d"), Index));
    }
    OnBagItemRemoved.Broadcast(Index, Item);
}

void UYITradeInteractionComponent::HandleBagItemMoved(int32 Index, FIntPoint NewPos)
{
    if (bDebugTradeInteraction && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Cyan,
            FString::Printf(TEXT("[TradeInt] Moved idx %d -> (%d,%d)"), Index, NewPos.X, NewPos.Y));
    }
    OnBagItemMoved.Broadcast(Index, NewPos);
}

void UYITradeInteractionComponent::HandleBagItemRotated(int32 Index)
{
    if (bDebugTradeInteraction && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Yellow,
            FString::Printf(TEXT("[TradeInt] Rotated idx %d"), Index));
    }
    OnBagItemRotated.Broadcast(Index);
}

void UYITradeInteractionComponent::HandleBagItemTransferred(UYIInventoryBag* Src, UYIInventoryBag* Dest, int32 SrcIdx, int32 DestIdx)
{
    if (bDebugTradeInteraction && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::White,
            FString::Printf(TEXT("[TradeInt] Transfer %p:%d -> %p:%d"), Src, SrcIdx, Dest, DestIdx));
    }
    OnBagItemTransferred.Broadcast(Src, Dest, SrcIdx, DestIdx);
}

bool UYITradeInteractionComponent::IsWithinDistance(const AActor* Target, float MaxDistance) const
{
    if (!Target || !GetOwningPC()) return false;
    const APawn* MyPawn = GetOwningPC()->GetPawn();
    if (!MyPawn) return false;
    const float DistSq = FVector::DistSquared(MyPawn->GetActorLocation(), Target->GetActorLocation());
    return DistSq <= FMath::Square(MaxDistance);
}

AActor* UYITradeInteractionComponent::GetOtherTradeActor() const
{
    if (!CurrentSession || !GetOwningPC() || !GetOwningPC()->PlayerState) return nullptr;
    const ETradeSide MySide = CurrentSession->GetSideForPlayer(GetOwningPC()->PlayerState);
    if (MySide == ETradeSide::SideA)
    {
        return CurrentSession->bSideBIsNPC ? Cast<AActor>(CurrentSession->NPCPawn) : Cast<AActor>(CurrentSession->PawnB);
    }
    if (MySide == ETradeSide::SideB)
    {
        return Cast<AActor>(CurrentSession->PawnA);
    }
    return nullptr;
}

void UYITradeInteractionComponent::CloseTradeLocal(bool bCancelServer)
{
    if (CurrentSession)
    {
        if (bCancelServer)
        {
            CurrentSession->ServerCancel();
        }
        if (APlayerController* PC = GetOwningPC())
        {
            if (APawn* P = PC->GetPawn())
            {
                if (UYIInventoryComponent* Inv = P->FindComponentByClass<UYIInventoryComponent>())
                {
                    Inv->CloseTradeScreen();
                }
            }
        }
    }
    if (BoundSession.IsValid())
    {
        BoundSession->OnTradeCancelled.RemoveAll(this);
        BoundSession->OnTradeCommitted.RemoveAll(this);
        BoundSession->OnTradeFailed.RemoveAll(this);
        BoundSession = nullptr;
    }
    CurrentSession = nullptr;
    OnTradeClosed.Broadcast();
    if (!CurrentSession && !CurrentShop)
    {
        SetComponentTickEnabled(false);
    }
}

void UYITradeInteractionComponent::CloseShopLocal()
{
    if (CurrentShop)
    {
        if (APlayerController* PC = GetOwningPC())
        {
            if (APawn* P = PC->GetPawn())
            {
                if (UYIInventoryComponent* Inv = P->FindComponentByClass<UYIInventoryComponent>())
                {
                    Inv->CloseShopScreen();
                }
            }
        }
    }
    CurrentShop = nullptr;
    CurrentShopStock.Reset();
    CurrentShopStockSize = FIntPoint(0, 0);
    OnShopClosed.Broadcast();
    bShopOpened = false;
    if (!CurrentSession && !CurrentShop)
    {
        SetComponentTickEnabled(false);
    }
}

void UYITradeInteractionComponent::HandleTradeEnded()
{
    CloseTradeLocal(false);
}
