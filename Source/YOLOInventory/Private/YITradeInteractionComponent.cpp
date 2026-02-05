#include "YITradeInteractionComponent.h"

#include "YIInventoryBlueprintLibrary.h"
#include "YIInventoryComponent.h"
#include "YIInventoryBag.h"
#include "YITradeSessionActor.h"
#include "TradingScreenWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"
#include "YIInventoryComponent.h"

UYITradeInteractionComponent::UYITradeInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
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
