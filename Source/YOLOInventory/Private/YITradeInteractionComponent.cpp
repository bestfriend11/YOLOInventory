#include "YITradeInteractionComponent.h"

#include "YIInventoryBlueprintLibrary.h"
#include "YITradeSessionActor.h"
#include "TradingScreenWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

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

    // If this is a local controller (standalone or listen server owning client), drive UI immediately.
    if (PC->IsLocalController())
    {
        OnRep_CurrentSession();
    }
    else
    {
        Client_TradeSessionStarted(Session); // remote client will still rely on OnRep once actor resolves
    }
}

bool UYITradeInteractionComponent::Server_RequestTrade_Validate(AActor* Target, bool bTargetIsNPC)
{
    // Keep validation permissive to avoid disconnects; do real checks in _Implementation.
    // Only sanity-check that we have a PC owner; if not, reject to prevent spoofing.
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

        if (bAutoShowWidget && AutoTradeWidgetClass && GetOwningPC())
        {
            UYIInventoryBag* LocalBag = nullptr;
            if (APawn* P = GetOwningPC()->GetPawn())
            {
                if (UYIInventoryComponent* Inv = P->FindComponentByClass<UYIInventoryComponent>())
                {
                    LocalBag = Inv->GetBag();
                }
            }
            if (UTradingScreenWidget* Widget = ActiveWidget.Get())
            {
                Widget->SetSession(CurrentSession, LocalBag, nullptr);
            }
            else
            {
                UTradingScreenWidget* WidgetInst = CreateWidget<UTradingScreenWidget>(GetOwningPC(), AutoTradeWidgetClass);
                if (WidgetInst)
                {
                    ActiveWidget = WidgetInst;
                    WidgetInst->AddToViewport();
                    WidgetInst->SetSession(CurrentSession, LocalBag, nullptr);
                }
            }
        }
    }
}

void UYITradeInteractionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UYITradeInteractionComponent, CurrentSession, COND_OwnerOnly);
}
#include "Net/UnrealNetwork.h"
#include "YIInventoryComponent.h"
