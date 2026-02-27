#include "YITradeInteractionComponent.h"

#include "YIInventoryComponent.h"
#include "YIShopComponent.h"
#include "YIInventoryBag.h"
#include "YITradeSessionActor.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"
#include "YIDebugLibrary.h"

namespace YITradeInteractionUIBindings
{
	struct FYITradeScreenBindParams
	{
		AYITradeSessionActor* InSession = nullptr;
		UYIInventoryBag* LocalPlayerBag = nullptr;
		UYIInventoryBag* OtherPartyBag = nullptr;
	};

	struct FYIShopScreenBindParams
	{
		UYIShopComponent* InShop = nullptr;
		UYIInventoryBag* LocalPlayerBag = nullptr;
		TArray<FYINetBagItem> Stock;
		FIntPoint StockSize = FIntPoint::ZeroValue;
	};

	static void YITradeComp_BindTradeScreenWidget(UUserWidget* Widget, AYITradeSessionActor* Session, UYIInventoryBag* LocalBag)
	{
		if (!Widget || !Session)
		{
			return;
		}

		if (UFunction* Fn = Widget->FindFunction(TEXT("SetSession")))
		{
			FYITradeScreenBindParams Params;
			Params.InSession = Session;
			Params.LocalPlayerBag = LocalBag;
			Params.OtherPartyBag = nullptr;
			Widget->ProcessEvent(Fn, &Params);
		}
	}

	static void YITradeComp_BindShopScreenWidget(UUserWidget* Widget, UYIShopComponent* Shop, UYIInventoryBag* LocalBag, const TArray<FYINetBagItem>& Stock, FIntPoint StockSize)
	{
		if (!Widget || !Shop)
		{
			return;
		}

		if (UFunction* Fn = Widget->FindFunction(TEXT("SetShop")))
		{
			FYIShopScreenBindParams Params;
			Params.InShop = Shop;
			Params.LocalPlayerBag = LocalBag;
			Params.Stock = Stock;
			Params.StockSize = StockSize;
			Widget->ProcessEvent(Fn, &Params);
		}
	}
}

using namespace YITradeInteractionUIBindings;

namespace
{
	static FYITradeOpResult MakeTradeOpResult(EYITradeOpKind OpKind, const FGuid& RequestId, bool bAccepted, bool bSucceeded, EYITradeOpError Error, const FText& Message)
	{
		FYITradeOpResult Result;
		Result.OpKind = OpKind;
		Result.RequestId = RequestId;
		Result.bRequestAccepted = bAccepted;
		Result.bSucceeded = bSucceeded;
		Result.Error = Error;
		Result.Message = Message;
		return Result;
	}

	static FYIShopOpResult MakeShopOpResult(UYIShopComponent* Shop, EYIShopOpKind OpKind, const FGuid& RequestId, bool bAccepted, bool bSucceeded, EYIShopOpError Error, const FText& Message)
	{
		FYIShopOpResult Result;
		Result.Shop = Shop;
		Result.OpKind = OpKind;
		Result.RequestId = RequestId;
		Result.bRequestAccepted = bAccepted;
		Result.bSucceeded = bSucceeded;
		Result.Error = Error;
		Result.Message = Message;
		return Result;
	}
}

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

FYITradeOpResult UYITradeInteractionComponent::RequestTradeEx(const FYITradeOpenRequest& InRequest)
{
	FYITradeOpenRequest Request = InRequest;
	if (!Request.RequestId.IsValid())
	{
		Request.RequestId = FGuid::NewGuid();
	}

	if (!IsOwnerValidForTrade(true))
	{
		const FYITradeOpResult Result = MakeTradeOpResult(EYITradeOpKind::Open, Request.RequestId, false, false, EYITradeOpError::InvalidOwner,
			NSLOCTEXT("YOLOInventory", "Trade_InvalidOwner", "Trade component must be on PlayerController"));
		PendingTradeOpenRequestId = Request.RequestId;
		Client_TradeSessionFailed(Result.Message);
		return Result;
	}
	if (!Request.Target)
	{
		const FYITradeOpResult Result = MakeTradeOpResult(EYITradeOpKind::Open, Request.RequestId, false, false, EYITradeOpError::InvalidTarget,
			NSLOCTEXT("YOLOInventory", "Trade_NoTarget", "No trade target"));
		PendingTradeOpenRequestId = Request.RequestId;
		Client_TradeSessionFailed(Result.Message);
		return Result;
	}
	if (!ValidateTradeRequest(Request.Target, Request.bTargetIsNPC))
	{
		const FYITradeOpResult Result = MakeTradeOpResult(EYITradeOpKind::Open, Request.RequestId, false, false, EYITradeOpError::ValidationFailed,
			NSLOCTEXT("YOLOInventory", "Trade_ClientRejected", "Trade request rejected"));
		PendingTradeOpenRequestId = Request.RequestId;
		Client_TradeSessionFailed(Result.Message);
		return Result;
	}
	if (!IsWithinDistance(Request.Target, TradeInteractionDistance))
	{
		const FYITradeOpResult Result = MakeTradeOpResult(EYITradeOpKind::Open, Request.RequestId, false, false, EYITradeOpError::TooFar,
			NSLOCTEXT("YOLOInventory", "Trade_TooFar", "Target is too far"));
		PendingTradeOpenRequestId = Request.RequestId;
		Client_TradeSessionFailed(Result.Message);
		return Result;
	}

	FYITradeOpResult Accepted = MakeTradeOpResult(EYITradeOpKind::Open, Request.RequestId, true, false, EYITradeOpError::None, FText::GetEmpty());
	PendingTradeOpenRequestId = Request.RequestId;
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_RequestTradeEx(Request);
		return Accepted;
	}

	Server_RequestTradeEx_Implementation(Request);
	return Accepted;
}

FYITradeOpResult UYITradeInteractionComponent::RequestTradeTransferEx(const FYITradeTransferRequest& InRequest)
{
	FYITradeTransferRequest Request = InRequest;
	if (!Request.RequestId.IsValid())
	{
		Request.RequestId = FGuid::NewGuid();
	}
	if (!IsOwnerValidForTrade(true))
	{
		FYITradeOpResult Result = MakeTradeOpResult(EYITradeOpKind::Transfer, Request.RequestId, false, false, EYITradeOpError::InvalidOwner,
			NSLOCTEXT("YOLOInventory", "Trade_InvalidOwner", "Trade component must be on PlayerController"));
		OnTradeOpResultReceived.Broadcast(Result);
		return Result;
	}
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_RequestTradeTransferEx(Request);
		return MakeTradeOpResult(EYITradeOpKind::Transfer, Request.RequestId, true, false, EYITradeOpError::None, FText::GetEmpty());
	}
	Server_RequestTradeTransferEx_Implementation(Request);
	return MakeTradeOpResult(EYITradeOpKind::Transfer, Request.RequestId, true, false, EYITradeOpError::None, FText::GetEmpty());
}

FYITradeOpResult UYITradeInteractionComponent::RequestTradeAddOfferEx(const FYITradeAddOfferRequest& InRequest)
{
	FYITradeAddOfferRequest Request = InRequest;
	if (!Request.RequestId.IsValid())
	{
		Request.RequestId = FGuid::NewGuid();
	}
	if (!IsOwnerValidForTrade(true))
	{
		FYITradeOpResult Result = MakeTradeOpResult(EYITradeOpKind::AddOffer, Request.RequestId, false, false, EYITradeOpError::InvalidOwner,
			NSLOCTEXT("YOLOInventory", "Trade_InvalidOwner", "Trade component must be on PlayerController"));
		OnTradeOpResultReceived.Broadcast(Result);
		return Result;
	}
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_RequestTradeAddOfferEx(Request);
		return MakeTradeOpResult(EYITradeOpKind::AddOffer, Request.RequestId, true, false, EYITradeOpError::None, FText::GetEmpty());
	}
	Server_RequestTradeAddOfferEx_Implementation(Request);
	return MakeTradeOpResult(EYITradeOpKind::AddOffer, Request.RequestId, true, false, EYITradeOpError::None, FText::GetEmpty());
}

FYITradeOpResult UYITradeInteractionComponent::RequestTradeRemoveOfferEx(const FYITradeRemoveOfferRequest& InRequest)
{
	FYITradeRemoveOfferRequest Request = InRequest;
	if (!Request.RequestId.IsValid())
	{
		Request.RequestId = FGuid::NewGuid();
	}
	if (!IsOwnerValidForTrade(true))
	{
		FYITradeOpResult Result = MakeTradeOpResult(EYITradeOpKind::RemoveOffer, Request.RequestId, false, false, EYITradeOpError::InvalidOwner,
			NSLOCTEXT("YOLOInventory", "Trade_InvalidOwner", "Trade component must be on PlayerController"));
		OnTradeOpResultReceived.Broadcast(Result);
		return Result;
	}
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_RequestTradeRemoveOfferEx(Request);
		return MakeTradeOpResult(EYITradeOpKind::RemoveOffer, Request.RequestId, true, false, EYITradeOpError::None, FText::GetEmpty());
	}
	Server_RequestTradeRemoveOfferEx_Implementation(Request);
	return MakeTradeOpResult(EYITradeOpKind::RemoveOffer, Request.RequestId, true, false, EYITradeOpError::None, FText::GetEmpty());
}

FYITradeOpResult UYITradeInteractionComponent::RequestTradeSetResourceEx(const FYITradeSetResourceRequest& InRequest)
{
	FYITradeSetResourceRequest Request = InRequest;
	if (!Request.RequestId.IsValid())
	{
		Request.RequestId = FGuid::NewGuid();
	}
	if (!IsOwnerValidForTrade(true))
	{
		FYITradeOpResult Result = MakeTradeOpResult(EYITradeOpKind::SetResource, Request.RequestId, false, false, EYITradeOpError::InvalidOwner,
			NSLOCTEXT("YOLOInventory", "Trade_InvalidOwner", "Trade component must be on PlayerController"));
		OnTradeOpResultReceived.Broadcast(Result);
		return Result;
	}
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_RequestTradeSetResourceEx(Request);
		return MakeTradeOpResult(EYITradeOpKind::SetResource, Request.RequestId, true, false, EYITradeOpError::None, FText::GetEmpty());
	}
	Server_RequestTradeSetResourceEx_Implementation(Request);
	return MakeTradeOpResult(EYITradeOpKind::SetResource, Request.RequestId, true, false, EYITradeOpError::None, FText::GetEmpty());
}

FYITradeOpResult UYITradeInteractionComponent::RequestTradeSetReadyEx(const FYITradeSetReadyRequest& InRequest)
{
	FYITradeSetReadyRequest Request = InRequest;
	if (!Request.RequestId.IsValid())
	{
		Request.RequestId = FGuid::NewGuid();
	}
	if (!IsOwnerValidForTrade(true))
	{
		FYITradeOpResult Result = MakeTradeOpResult(EYITradeOpKind::SetReady, Request.RequestId, false, false, EYITradeOpError::InvalidOwner,
			NSLOCTEXT("YOLOInventory", "Trade_InvalidOwner", "Trade component must be on PlayerController"));
		OnTradeOpResultReceived.Broadcast(Result);
		return Result;
	}
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_RequestTradeSetReadyEx(Request);
		return MakeTradeOpResult(EYITradeOpKind::SetReady, Request.RequestId, true, false, EYITradeOpError::None, FText::GetEmpty());
	}
	Server_RequestTradeSetReadyEx_Implementation(Request);
	return MakeTradeOpResult(EYITradeOpKind::SetReady, Request.RequestId, true, false, EYITradeOpError::None, FText::GetEmpty());
}

FYIShopOpResult UYITradeInteractionComponent::RequestShopOpenEx(const FYIShopOpenRequest& InRequest)
{
	FYIShopOpenRequest Request = InRequest;
	if (!Request.RequestId.IsValid())
	{
		Request.RequestId = FGuid::NewGuid();
	}
	if (!IsOwnerValidForTrade(true))
	{
		FYIShopOpResult Result = MakeShopOpResult(Request.Shop, EYIShopOpKind::Open, Request.RequestId, false, false, EYIShopOpError::InvalidRequest,
			NSLOCTEXT("YOLOInventory", "Shop_InvalidOwner", "Shop interaction requires PlayerController owner"));
		OnShopOpResultReceived.Broadcast(Result);
		return Result;
	}
	if (!Request.Shop)
	{
		FYIShopOpResult Result = MakeShopOpResult(nullptr, EYIShopOpKind::Open, Request.RequestId, false, false, EYIShopOpError::InvalidShop,
			NSLOCTEXT("YOLOInventory", "Shop_InvalidShop", "Invalid shop"));
		OnShopOpResultReceived.Broadcast(Result);
		return Result;
	}
	if (!ValidateShopRequest(Request.Shop))
	{
		FYIShopOpResult Result = MakeShopOpResult(Request.Shop, EYIShopOpKind::Open, Request.RequestId, false, false, EYIShopOpError::ValidationFailed,
			NSLOCTEXT("YOLOInventory", "Shop_RequestRejected", "Shop request rejected"));
		OnShopOpResultReceived.Broadcast(Result);
		return Result;
	}
	if (Request.Shop->GetOwner() && !IsWithinDistance(Request.Shop->GetOwner(), ShopInteractionDistance))
	{
		FYIShopOpResult Result = MakeShopOpResult(Request.Shop, EYIShopOpKind::Open, Request.RequestId, false, false, EYIShopOpError::TooFar,
			NSLOCTEXT("YOLOInventory", "Shop_TooFar", "Shop is too far"));
		OnShopOpResultReceived.Broadcast(Result);
		return Result;
	}
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_RequestShopOpenEx(Request);
		return MakeShopOpResult(Request.Shop, EYIShopOpKind::Open, Request.RequestId, true, false, EYIShopOpError::None, FText::GetEmpty());
	}
	Server_RequestShopOpenEx_Implementation(Request);
	return MakeShopOpResult(Request.Shop, EYIShopOpKind::Open, Request.RequestId, true, true, EYIShopOpError::None, FText::GetEmpty());
}

FYIShopOpResult UYITradeInteractionComponent::RequestShopBuyEx(const FYIShopBuyRequest& InRequest)
{
	FYIShopBuyRequest Request = InRequest;
	if (!Request.RequestId.IsValid())
	{
		Request.RequestId = FGuid::NewGuid();
	}
	if (!IsOwnerValidForTrade(true) || !Request.Shop || !Request.BuyerInv || Request.Count <= 0)
	{
		FYIShopOpResult Result = MakeShopOpResult(Request.Shop, EYIShopOpKind::Buy, Request.RequestId, false, false, EYIShopOpError::InvalidRequest,
			NSLOCTEXT("YOLOInventory", "Shop_Buy_InvalidReq", "Invalid buy request"));
		OnShopOpResultReceived.Broadcast(Result);
		return Result;
	}
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_RequestShopBuyEx(Request);
		return MakeShopOpResult(Request.Shop, EYIShopOpKind::Buy, Request.RequestId, true, false, EYIShopOpError::None, FText::GetEmpty());
	}
	Server_RequestShopBuyEx_Implementation(Request);
	return MakeShopOpResult(Request.Shop, EYIShopOpKind::Buy, Request.RequestId, true, true, EYIShopOpError::None, FText::GetEmpty());
}

FYIShopOpResult UYITradeInteractionComponent::RequestShopSellEx(const FYIShopSellRequest& InRequest)
{
	FYIShopSellRequest Request = InRequest;
	if (!Request.RequestId.IsValid())
	{
		Request.RequestId = FGuid::NewGuid();
	}
	if (!IsOwnerValidForTrade(true) || !Request.Shop || !Request.SellerInv || Request.Count <= 0)
	{
		FYIShopOpResult Result = MakeShopOpResult(Request.Shop, EYIShopOpKind::Sell, Request.RequestId, false, false, EYIShopOpError::InvalidRequest,
			NSLOCTEXT("YOLOInventory", "Shop_Sell_InvalidReq", "Invalid sell request"));
		OnShopOpResultReceived.Broadcast(Result);
		return Result;
	}
	if (GetOwner() && !GetOwner()->HasAuthority())
	{
		Server_RequestShopSellEx(Request);
		return MakeShopOpResult(Request.Shop, EYIShopOpKind::Sell, Request.RequestId, true, false, EYIShopOpError::None, FText::GetEmpty());
	}
	Server_RequestShopSellEx_Implementation(Request);
	return MakeShopOpResult(Request.Shop, EYIShopOpKind::Sell, Request.RequestId, true, true, EYIShopOpError::None, FText::GetEmpty());
}

void UYITradeInteractionComponent::Server_RequestTradeEx_Implementation(FYITradeOpenRequest Request)
{
	APlayerController* PC = GetOwningPC();
	if (!PC || !PC->PlayerState)
	{
		Client_TradeSessionFailed(NSLOCTEXT("YOLOInventory", "Trade_NoPC", "Invalid player controller"));
		return;
	}
	if (!Request.Target)
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

	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_Client)
	{
		Client_TradeSessionFailed(NSLOCTEXT("YOLOInventory", "Trade_NoWorld", "Trade can only start on server"));
		return;
	}

	AYITradeSessionActor* Session = World->SpawnActor<AYITradeSessionActor>();
	if (!Session)
	{
		Client_TradeSessionFailed(NSLOCTEXT("YOLOInventory", "Trade_SpawnFail", "Could not start trade"));
		return;
	}

	Session->PlayerA = PC->PlayerState;
	Session->PawnA = InitiatorPawn;

	if (Request.bTargetIsNPC)
	{
		Session->bSideBIsNPC = true;
		Session->NPCPawn = Cast<APawn>(Request.Target);
	}
	else
	{
		APawn* TargetPawn = Cast<APawn>(Request.Target);
		APlayerController* TargetPC = TargetPawn ? Cast<APlayerController>(TargetPawn->GetController()) : nullptr;
		if (TargetPC && TargetPC->PlayerState)
		{
			Session->PlayerB = TargetPC->PlayerState;
			Session->PawnB = TargetPawn;
		}
	}

	Session->RefreshInventoryViews();
	Session->SetOwner(PC); // ensure relevance
	Session->ForceNetUpdate();

	CurrentSession = Session;
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->ForceNetUpdate(); // push replicated property to owning client
	}

	if (PC->IsLocalController())
	{
		OnRep_CurrentSession();
	}

	if (!Request.bTargetIsNPC)
	{
		APawn* TargetPawn = Cast<APawn>(Request.Target);
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

void UYITradeInteractionComponent::Server_RequestShopOpenEx_Implementation(FYIShopOpenRequest Request)
{
	FYIShopOpResult Result = MakeShopOpResult(Request.Shop, EYIShopOpKind::Open, Request.RequestId, false, false, EYIShopOpError::InvalidRequest, FText::GetEmpty());
	if (!IsOwnerValidForTrade(false) || !Request.Shop)
	{
		Result.Error = Request.Shop ? EYIShopOpError::InvalidRequest : EYIShopOpError::InvalidShop;
		Result.Message = Request.Shop
			? NSLOCTEXT("YOLOInventory", "Shop_Open_InvalidReq", "Invalid shop request")
			: NSLOCTEXT("YOLOInventory", "Shop_Open_InvalidShop", "Invalid shop");
		Client_ShopOpResult(Result);
		return;
	}
	Result.bRequestAccepted = true;
	APlayerController* PC = GetOwningPC();
	if (!PC || !PC->PlayerState)
	{
		Result.Error = EYIShopOpError::InvalidRequest;
		Result.Message = NSLOCTEXT("YOLOInventory", "Shop_Open_NoPC", "No player controller");
		Client_ShopOpResult(Result);
		return;
	}

	TArray<FYINetBagItem> OutItems;
	FIntPoint OutSize = FIntPoint(0,0);
	Request.Shop->GetStockMirrorForPlayer(PC->PlayerState, OutItems, OutSize);
	Client_ShopStockReady(Request.Shop, OutItems, OutSize);

	Result.bSucceeded = true;
	Result.Error = EYIShopOpError::None;
	Client_ShopOpResult(Result);
}

void UYITradeInteractionComponent::Server_RequestTradeAddOfferEx_Implementation(FYITradeAddOfferRequest Request)
{
	FYITradeOpResult Result = MakeTradeOpResult(EYITradeOpKind::AddOffer, Request.RequestId, false, false, EYITradeOpError::InvalidRequest, FText::GetEmpty());
	if (!IsOwnerValidForTrade(false) || !CurrentSession)
	{
		Result.Error = EYITradeOpError::NoSession;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_NoSession", "No active trade session");
		Client_TradeOpResult(Result);
		return;
	}

	APlayerController* PC = GetOwningPC();
	if (!PC || !PC->PlayerState)
	{
		Result.Error = EYITradeOpError::InvalidOwner;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_InvalidOwner", "Invalid player controller");
		Client_TradeOpResult(Result);
		return;
	}
	const ETradeSide CallerSide = CurrentSession->GetSideForPlayer(PC->PlayerState);
	if (CallerSide != ETradeSide::SideA && CallerSide != ETradeSide::SideB)
	{
		Result.Error = EYITradeOpError::NotParticipant;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_NotParticipant", "You are not part of this trade");
		Client_TradeOpResult(Result);
		return;
	}
	if (Request.Side != CallerSide)
	{
		Result.Error = EYITradeOpError::ValidationFailed;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_AddOffer_WrongSide", "Cannot edit the other side offer");
		Client_TradeOpResult(Result);
		return;
	}

	if (!Request.SourceInventory)
	{
		Request.SourceInventory = GetOwningInventoryComponent();
	}
	Result.bRequestAccepted = true;
	FText AddError;
	Result.bSucceeded = CurrentSession->TryAddOfferItem(Request.Side, Request.SourceInventory, Request.SourceIndex, Request.Count, PC, AddError);
	Result.Error = Result.bSucceeded ? EYITradeOpError::None : EYITradeOpError::ValidationFailed;
	Result.Message = Result.bSucceeded ? FText::GetEmpty()
		: (AddError.IsEmpty()
			? NSLOCTEXT("YOLOInventory", "Trade_AddOffer_Failed", "Failed to add offer entry")
			: AddError);
	Client_TradeOpResult(Result);
}

void UYITradeInteractionComponent::Server_RequestTradeRemoveOfferEx_Implementation(FYITradeRemoveOfferRequest Request)
{
	FYITradeOpResult Result = MakeTradeOpResult(EYITradeOpKind::RemoveOffer, Request.RequestId, false, false, EYITradeOpError::InvalidRequest, FText::GetEmpty());
	if (!IsOwnerValidForTrade(false) || !CurrentSession)
	{
		Result.Error = EYITradeOpError::NoSession;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_NoSession", "No active trade session");
		Client_TradeOpResult(Result);
		return;
	}

	APlayerController* PC = GetOwningPC();
	if (!PC || !PC->PlayerState)
	{
		Result.Error = EYITradeOpError::InvalidOwner;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_InvalidOwner", "Invalid player controller");
		Client_TradeOpResult(Result);
		return;
	}
	const ETradeSide CallerSide = CurrentSession->GetSideForPlayer(PC->PlayerState);
	if (CallerSide != ETradeSide::SideA && CallerSide != ETradeSide::SideB)
	{
		Result.Error = EYITradeOpError::NotParticipant;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_NotParticipant", "You are not part of this trade");
		Client_TradeOpResult(Result);
		return;
	}
	if (Request.Side != CallerSide)
	{
		Result.Error = EYITradeOpError::ValidationFailed;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_RemoveOffer_WrongSide", "Cannot edit the other side offer");
		Client_TradeOpResult(Result);
		return;
	}

	Result.bRequestAccepted = true;
	FText RemoveError;
	Result.bSucceeded = CurrentSession->TryRemoveOfferItem(Request.Side, Request.OfferIndex, PC, RemoveError);
	Result.Error = Result.bSucceeded ? EYITradeOpError::None : EYITradeOpError::ValidationFailed;
	Result.Message = Result.bSucceeded ? FText::GetEmpty()
		: (RemoveError.IsEmpty()
			? NSLOCTEXT("YOLOInventory", "Trade_RemoveOffer_Failed", "Failed to remove offer entry")
			: RemoveError);
	Client_TradeOpResult(Result);
}

void UYITradeInteractionComponent::Server_RequestTradeSetResourceEx_Implementation(FYITradeSetResourceRequest Request)
{
	FYITradeOpResult Result = MakeTradeOpResult(EYITradeOpKind::SetResource, Request.RequestId, false, false, EYITradeOpError::InvalidRequest, FText::GetEmpty());
	if (!IsOwnerValidForTrade(false) || !CurrentSession)
	{
		Result.Error = EYITradeOpError::NoSession;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_NoSession", "No active trade session");
		Client_TradeOpResult(Result);
		return;
	}

	APlayerController* PC = GetOwningPC();
	if (!PC || !PC->PlayerState)
	{
		Result.Error = EYITradeOpError::InvalidOwner;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_InvalidOwner", "Invalid player controller");
		Client_TradeOpResult(Result);
		return;
	}
	const ETradeSide CallerSide = CurrentSession->GetSideForPlayer(PC->PlayerState);
	if (CallerSide != ETradeSide::SideA && CallerSide != ETradeSide::SideB)
	{
		Result.Error = EYITradeOpError::NotParticipant;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_NotParticipant", "You are not part of this trade");
		Client_TradeOpResult(Result);
		return;
	}
	if (Request.Side != CallerSide)
	{
		Result.Error = EYITradeOpError::ValidationFailed;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_SetResource_WrongSide", "Cannot edit the other side offer");
		Client_TradeOpResult(Result);
		return;
	}

	Result.bRequestAccepted = true;
	FText ResourceError;
	Result.bSucceeded = CurrentSession->TrySetResourceOffer(Request.Side, Request.Resource, Request.Amount, PC, ResourceError);
	Result.Error = Result.bSucceeded ? EYITradeOpError::None : EYITradeOpError::ValidationFailed;
	Result.Message = Result.bSucceeded ? FText::GetEmpty()
		: (ResourceError.IsEmpty()
			? NSLOCTEXT("YOLOInventory", "Trade_SetResource_Failed", "Failed to set resource offer")
			: ResourceError);
	Client_TradeOpResult(Result);
}

void UYITradeInteractionComponent::Server_RequestShopBuyEx_Implementation(FYIShopBuyRequest Request)
{
	FYIShopOpResult Result = MakeShopOpResult(Request.Shop, EYIShopOpKind::Buy, Request.RequestId, false, false, EYIShopOpError::InvalidRequest, FText::GetEmpty());
	if (!IsOwnerValidForTrade(false) || !Request.Shop || !Request.BuyerInv || Request.Count <= 0)
	{
		Result.Error = EYIShopOpError::InvalidRequest;
		Result.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_Invalid", "Invalid buy request");
		Client_ShopActionResult(Request.Shop, false, Result.Message);
		Client_ShopOpResult(Result);
		return;
	}

	APlayerController* PC = GetOwningPC();
	if (!PC || !PC->PlayerState)
	{
		Result.Error = EYIShopOpError::InvalidRequest;
		Result.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_NoPC", "No player controller");
		Client_ShopActionResult(Request.Shop, false, Result.Message);
		Client_ShopOpResult(Result);
		return;
	}
	if (APawn* Pawn = PC->GetPawn())
	{
		if (Request.BuyerInv->GetOwner() != Pawn)
		{
			Result.Error = EYIShopOpError::InvalidOwner;
			Result.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_WrongOwner", "Invalid inventory owner");
			Client_ShopActionResult(Request.Shop, false, Result.Message);
			Client_ShopOpResult(Result);
			return;
		}
	}

	Result.bRequestAccepted = true;
	Request.Shop->ExecuteBuyRequest(Request, Result);

	TArray<FYINetBagItem> OutItems;
	FIntPoint OutSize = FIntPoint::ZeroValue;
	Request.Shop->GetStockMirrorForPlayer(PC->PlayerState, OutItems, OutSize);
	Client_ShopStockReady(Request.Shop, OutItems, OutSize);

	Client_ShopActionResult(Request.Shop, Result.bSucceeded, Result.Message);
	Client_ShopOpResult(Result);
}

void UYITradeInteractionComponent::Server_RequestShopSellEx_Implementation(FYIShopSellRequest Request)
{
	FYIShopOpResult Result = MakeShopOpResult(Request.Shop, EYIShopOpKind::Sell, Request.RequestId, false, false, EYIShopOpError::InvalidRequest, FText::GetEmpty());
	if (!IsOwnerValidForTrade(false) || !Request.Shop || !Request.SellerInv || Request.Count <= 0)
	{
		Result.Error = EYIShopOpError::InvalidRequest;
		Result.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_Invalid", "Invalid sell request");
		Client_ShopActionResult(Request.Shop, false, Result.Message);
		Client_ShopOpResult(Result);
		return;
	}

	APlayerController* PC = GetOwningPC();
	if (!PC || !PC->PlayerState)
	{
		Result.Error = EYIShopOpError::InvalidRequest;
		Result.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_NoPC", "No player controller");
		Client_ShopActionResult(Request.Shop, false, Result.Message);
		Client_ShopOpResult(Result);
		return;
	}
	if (APawn* Pawn = PC->GetPawn())
	{
		if (Request.SellerInv->GetOwner() != Pawn)
		{
			Result.Error = EYIShopOpError::InvalidOwner;
			Result.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_WrongOwner", "Invalid inventory owner");
			Client_ShopActionResult(Request.Shop, false, Result.Message);
			Client_ShopOpResult(Result);
			return;
		}
	}

	Result.bRequestAccepted = true;
	Request.Shop->ExecuteSellRequest(Request, Result);

	TArray<FYINetBagItem> OutItems;
	FIntPoint OutSize = FIntPoint::ZeroValue;
	Request.Shop->GetStockMirrorForPlayer(PC->PlayerState, OutItems, OutSize);
	Client_ShopStockReady(Request.Shop, OutItems, OutSize);

	Client_ShopActionResult(Request.Shop, Result.bSucceeded, Result.Message);
	Client_ShopOpResult(Result);
}

void UYITradeInteractionComponent::Server_RequestTradeTransferEx_Implementation(FYITradeTransferRequest Request)
{
	FYITradeOpResult Result = MakeTradeOpResult(EYITradeOpKind::Transfer, Request.RequestId, false, false, EYITradeOpError::InvalidRequest, FText::GetEmpty());
	if (!IsOwnerValidForTrade(false) || !CurrentSession)
	{
		Result.Error = EYITradeOpError::NoSession;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_NoSession", "No active trade session");
		Client_TradeOpResult(Result);
		return;
	}

	APlayerController* PC = GetOwningPC();
	if (!PC || !PC->PlayerState)
	{
		Result.Error = EYITradeOpError::InvalidOwner;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_InvalidOwner", "Invalid player controller");
		Client_TradeOpResult(Result);
		return;
	}
	const ETradeSide CallerSide = CurrentSession->GetSideForPlayer(PC->PlayerState);
	if (CallerSide != ETradeSide::SideA && CallerSide != ETradeSide::SideB)
	{
		Result.Error = EYITradeOpError::NotParticipant;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_NotParticipant", "You are not part of this trade");
		Client_TradeOpResult(Result);
		return;
	}
	Result.bRequestAccepted = true;
	FText TransferError;
	Result.bSucceeded = CurrentSession->TryTransferItemBetweenSides(Request.FromSide, Request.ToSide, Request.SourceIndex, Request.DestPos, Request.Count, TransferError);
	if (!Result.bSucceeded)
	{
		Result.Error = EYITradeOpError::ValidationFailed;
		Result.Message = TransferError.IsEmpty() ? NSLOCTEXT("YOLOInventory", "Trade_TransferFailed", "Trade transfer failed") : TransferError;
	}
	else
	{
		Result.Error = EYITradeOpError::None;
	}
	Client_TradeOpResult(Result);
}

void UYITradeInteractionComponent::Server_RequestTradeSetReadyEx_Implementation(FYITradeSetReadyRequest Request)
{
	FYITradeOpResult Result = MakeTradeOpResult(EYITradeOpKind::SetReady, Request.RequestId, false, false, EYITradeOpError::InvalidRequest, FText::GetEmpty());
	if (!IsOwnerValidForTrade(false) || !CurrentSession)
	{
		Result.Error = EYITradeOpError::NoSession;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_NoSession", "No active trade session");
		Client_TradeOpResult(Result);
		return;
	}

	APlayerController* PC = GetOwningPC();
	if (!PC || !PC->PlayerState)
	{
		Result.Error = EYITradeOpError::InvalidOwner;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_InvalidOwner", "Invalid player controller");
		Client_TradeOpResult(Result);
		return;
	}

	const ETradeSide CallerSide = CurrentSession->GetSideForPlayer(PC->PlayerState);
	if (CallerSide != ETradeSide::SideA && CallerSide != ETradeSide::SideB)
	{
		Result.Error = EYITradeOpError::NotParticipant;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_NotParticipant", "You are not part of this trade");
		Client_TradeOpResult(Result);
		return;
	}

	if (Request.Side != CallerSide)
	{
		Result.Error = EYITradeOpError::ValidationFailed;
		Result.Message = NSLOCTEXT("YOLOInventory", "Trade_SetReady_WrongSide", "Cannot change readiness for the other side");
		Client_TradeOpResult(Result);
		return;
	}

	Result.bRequestAccepted = true;
	FText SetReadyError;
	Result.bSucceeded = CurrentSession->TrySetReady(Request.Side, Request.bReady, PC, SetReadyError);
	Result.Error = Result.bSucceeded ? EYITradeOpError::None : EYITradeOpError::ValidationFailed;
	Result.Message = Result.bSucceeded ? FText::GetEmpty()
		: (SetReadyError.IsEmpty()
			? NSLOCTEXT("YOLOInventory", "Trade_SetReady_Failed", "Failed to change trade readiness")
			: SetReadyError);
	Client_TradeOpResult(Result);
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
    FYITradeOpResult Structured = MakeTradeOpResult(EYITradeOpKind::Open, PendingTradeOpenRequestId, true, false, EYITradeOpError::ValidationFailed, Reason);
    OnTradeOpResultReceived.Broadcast(Structured);
    PendingTradeOpenRequestId.Invalidate();
    OnTradeFailed.Broadcast(Reason);
    UYIDebugLibrary::EmitDebugMessage(
        this,
        EYIDebugChannel::Trade,
        Reason.ToString(),
        FLinearColor(FColor::Red),
        true,
        true,
        2.0f,
        false,
        false,
        TEXT("Trade"));
}

void UYITradeInteractionComponent::Client_TradeOpResult_Implementation(const FYITradeOpResult& Result)
{
	OnTradeOpResultReceived.Broadcast(Result);
}

void UYITradeInteractionComponent::Client_ShopStockReady_Implementation(UYIShopComponent* Shop, const TArray<FYINetBagItem>& Stock, FIntPoint Size)
{
    CurrentShop = Shop;
    CurrentShopStock = Stock;
    CurrentShopStockSize = Size;
    OnShopStockUpdated.Broadcast(Shop);

    if (GetOwningPC() && GetOwningPC()->IsLocalController())
    {
        // Always refresh an already-open widget if present.
        if (ActiveShopWidget.IsValid())
        {
            OpenOrRefreshShopWidget(Shop, Stock, Size);
        }

        if (bAutoShowShopWidget)
        {
            OpenOrRefreshShopWidget(Shop, Stock, Size);
        }

        if (ActiveShopWidget.IsValid())
        {
            // Ensure focus/input stays on this local controller window.
            GetOwningPC()->bShowMouseCursor = true;
            FInputModeGameAndUI Mode;
            Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            Mode.SetHideCursorDuringCapture(false);
            GetOwningPC()->SetInputMode(Mode);
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
    UYIDebugLibrary::EmitDebugMessage(
        this,
        EYIDebugChannel::Shop,
        Reason.ToString(),
        FLinearColor(bSuccess ? FColor::Green : FColor::Red),
        bDebugTradeInteraction,
        bDebugTradeInteraction,
        2.0f,
        false,
        false,
        TEXT("Trade"));
}

void UYITradeInteractionComponent::Client_ShopOpResult_Implementation(const FYIShopOpResult& Result)
{
	OnShopOpResultReceived.Broadcast(Result);
}

APlayerController* UYITradeInteractionComponent::GetOwningPC() const
{
    return Cast<APlayerController>(GetOwner());
}

UYIInventoryComponent* UYITradeInteractionComponent::GetOwningInventoryComponent() const
{
    if (const APlayerController* PC = GetOwningPC())
    {
        if (APawn* Pawn = PC->GetPawn())
        {
            return Pawn->FindComponentByClass<UYIInventoryComponent>();
        }
    }
    return nullptr;
}

UYIInventoryBag* UYITradeInteractionComponent::GetOwningInventoryBag() const
{
    if (UYIInventoryComponent* Inv = GetOwningInventoryComponent())
    {
        return Inv->GetBag();
    }
    return nullptr;
}

void UYITradeInteractionComponent::OpenOrRefreshTradeWidget(AYITradeSessionActor* Session)
{
    APlayerController* PC = GetOwningPC();
    if (!PC || !PC->IsLocalController() || !Session || !AutoTradeWidgetClass)
    {
        return;
    }

    UUserWidget* Widget = ActiveTradeWidget.Get();
    if (!Widget)
    {
        Widget = CreateWidget<UUserWidget>(PC, AutoTradeWidgetClass);
        if (!Widget)
        {
            return;
        }
        Widget->AddToViewport();
        ActiveTradeWidget = Widget;
    }

    YITradeComp_BindTradeScreenWidget(Widget, Session, GetOwningInventoryBag());
}

void UYITradeInteractionComponent::OpenOrRefreshShopWidget(UYIShopComponent* Shop, const TArray<FYINetBagItem>& Stock, FIntPoint Size)
{
    APlayerController* PC = GetOwningPC();
    if (!PC || !PC->IsLocalController() || !Shop || !AutoShopWidgetClass)
    {
        return;
    }

    UUserWidget* Widget = ActiveShopWidget.Get();
    if (!Widget)
    {
        Widget = CreateWidget<UUserWidget>(PC, AutoShopWidgetClass);
        if (!Widget)
        {
            return;
        }
        Widget->AddToViewport();
        ActiveShopWidget = Widget;
    }

    YITradeComp_BindShopScreenWidget(Widget, Shop, GetOwningInventoryBag(), Stock, Size);
}

void UYITradeInteractionComponent::CloseTradeWidget()
{
    if (UUserWidget* Widget = ActiveTradeWidget.Get())
    {
        Widget->RemoveFromParent();
    }
    ActiveTradeWidget.Reset();
}

void UYITradeInteractionComponent::CloseShopWidget()
{
    if (UUserWidget* Widget = ActiveShopWidget.Get())
    {
        Widget->RemoveFromParent();
    }
    ActiveShopWidget.Reset();
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
		FYITradeOpResult Structured = MakeTradeOpResult(EYITradeOpKind::Open, PendingTradeOpenRequestId, true, true, EYITradeOpError::None, FText::GetEmpty());
		OnTradeOpResultReceived.Broadcast(Structured);
		PendingTradeOpenRequestId.Invalidate();
		OnTradeSessionReady.Broadcast(CurrentSession);
        OnTradeOpened.Broadcast(CurrentSession);

        if (BoundSession.IsValid())
        {
            BoundSession->OnTradeCancelled.RemoveAll(this);
            BoundSession->OnTradeCommitted.RemoveAll(this);
            BoundSession->OnTradeFailed.RemoveAll(this);
        }
        BoundSession = CurrentSession;
        CurrentSession->OnTradeCancelled.AddDynamic(this, &UYITradeInteractionComponent::HandleTradeCancelled);
        CurrentSession->OnTradeCommitted.AddDynamic(this, &UYITradeInteractionComponent::HandleTradeCommittedSession);
        CurrentSession->OnTradeFailed.AddDynamic(this, &UYITradeInteractionComponent::HandleTradeFailedSession);
        SetComponentTickEnabled(true);

		if (bAutoShowWidget && GetOwningPC() && GetOwningPC()->IsLocalController())
		{
            OpenOrRefreshTradeWidget(CurrentSession);
            if (ActiveTradeWidget.IsValid())
            {
                // Ensure focus/input stays on this local controller window.
                GetOwningPC()->bShowMouseCursor = true;
                FInputModeGameAndUI Mode;
                Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
                Mode.SetHideCursorDuringCapture(false);
                GetOwningPC()->SetInputMode(Mode);
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
    UYIDebugLibrary::EmitDebugMessage(
        this,
        EYIDebugChannel::Trade,
        FString::Printf(TEXT("Added idx=%d count=%d"), Index, Item.Item.Count),
        FLinearColor(FColor::Green),
        bDebugTradeInteraction,
        bDebugTradeInteraction,
        2.0f,
        false,
        false,
        TEXT("Trade"));
    OnBagItemAdded.Broadcast(Index, Item);
}

void UYITradeInteractionComponent::HandleBagItemRemoved(int32 Index, FYIBagItem Item)
{
    UYIDebugLibrary::EmitDebugMessage(
        this,
        EYIDebugChannel::Trade,
        FString::Printf(TEXT("Removed idx=%d"), Index),
        FLinearColor(FColor::Orange),
        bDebugTradeInteraction,
        bDebugTradeInteraction,
        2.0f,
        false,
        false,
        TEXT("Trade"));
    OnBagItemRemoved.Broadcast(Index, Item);
}

void UYITradeInteractionComponent::HandleBagItemMoved(int32 Index, FIntPoint NewPos)
{
    UYIDebugLibrary::EmitDebugMessage(
        this,
        EYIDebugChannel::Trade,
        FString::Printf(TEXT("Moved idx=%d to (%d,%d)"), Index, NewPos.X, NewPos.Y),
        FLinearColor(FColor::Cyan),
        bDebugTradeInteraction,
        bDebugTradeInteraction,
        2.0f,
        false,
        false,
        TEXT("Trade"));
    OnBagItemMoved.Broadcast(Index, NewPos);
}

void UYITradeInteractionComponent::HandleBagItemRotated(int32 Index)
{
    UYIDebugLibrary::EmitDebugMessage(
        this,
        EYIDebugChannel::Trade,
        FString::Printf(TEXT("Rotated idx=%d"), Index),
        FLinearColor(FColor::Yellow),
        bDebugTradeInteraction,
        bDebugTradeInteraction,
        2.0f,
        false,
        false,
        TEXT("Trade"));
    OnBagItemRotated.Broadcast(Index);
}

void UYITradeInteractionComponent::HandleBagItemTransferred(UYIInventoryBag* Src, UYIInventoryBag* Dest, int32 SrcIdx, int32 DestIdx)
{
    UYIDebugLibrary::EmitDebugMessage(
        this,
        EYIDebugChannel::Trade,
        FString::Printf(TEXT("Transfer %p:%d -> %p:%d"), Src, SrcIdx, Dest, DestIdx),
        FLinearColor(FColor::White),
        bDebugTradeInteraction,
        bDebugTradeInteraction,
        2.0f,
        false,
        false,
        TEXT("Trade"));
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
    }
    CloseTradeWidget();
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
    CloseShopWidget();
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

void UYITradeInteractionComponent::HandleTradeCancelled()
{
	FYITradeOpResult Result = MakeTradeOpResult(EYITradeOpKind::Close, FGuid(), true, true, EYITradeOpError::None,
		NSLOCTEXT("YOLOInventory", "Trade_Cancelled", "Trade cancelled"));
	OnTradeOpResultReceived.Broadcast(Result);
	CloseTradeLocal(false);
}

void UYITradeInteractionComponent::HandleTradeCommittedSession()
{
	FYITradeOpResult Result = MakeTradeOpResult(EYITradeOpKind::Commit, FGuid(), true, true, EYITradeOpError::None,
		NSLOCTEXT("YOLOInventory", "Trade_Committed", "Trade committed"));
	OnTradeOpResultReceived.Broadcast(Result);
	CloseTradeLocal(false);
}

void UYITradeInteractionComponent::HandleTradeFailedSession()
{
	FText Reason = BoundSession.IsValid() ? BoundSession->GetLastFailureReason() : FText();
	if (Reason.IsEmpty())
	{
		Reason = NSLOCTEXT("YOLOInventory", "Trade_Failed", "Trade failed");
	}
	FYITradeOpResult Result = MakeTradeOpResult(EYITradeOpKind::Commit, FGuid(), true, false, EYITradeOpError::ValidationFailed, Reason);
	OnTradeOpResultReceived.Broadcast(Result);
	CloseTradeLocal(false);
}
