#include "YIInventoryGridGameplayAdapter.h"

#include "InventoryGridWidget.h"
#include "YIEquipmentComponent.h"
#include "YIShopComponent.h"
#include "YITradeInteractionComponent.h"
#include "YITradeSessionActor.h"
#include "YIInventoryComponent.h"
#include "YIInventoryBlueprintLibrary.h"
#include "YIItemDescriptionResolver.h"
#include "YIWorldLootBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

void UYIInventoryGridGameplayAdapter::SetTradeSession(AYITradeSessionActor* InSession)
{
	ActiveTradeSession = InSession;
	bHasTradeSide = false;
}

void UYIInventoryGridGameplayAdapter::SetTradeContext(AYITradeSessionActor* InSession, ETradeSide InSide)
{
	ActiveTradeSession = InSession;
	if (InSession)
	{
		TradeSide = InSide;
		bHasTradeSide = true;
	}
	else
	{
		bHasTradeSide = false;
	}
}

void UYIInventoryGridGameplayAdapter::SetShopContext(UYIShopComponent* InShop, bool bStockGrid)
{
	ActiveShopComponent = InShop;
	bIsShopStockGrid = bStockGrid;
}

EYIInventoryGridExternalOpResult UYIInventoryGridGameplayAdapter::TryHandleTransferRequest(
	const FYIInventoryGridTransferRequest& Request,
	int32& OutDestIndex)
{
	OutDestIndex = INDEX_NONE;
	UInventoryGridWidget* DestGrid = Request.DestGrid;
	UInventoryGridWidget* SourceGrid = Request.SourceGrid;
	if (!DestGrid || !SourceGrid)
	{
		return EYIInventoryGridExternalOpResult::NotHandled;
	}

	UYIInventoryGridGameplayAdapter* SourceAdapter = Cast<UYIInventoryGridGameplayAdapter>(SourceGrid->GetFeatureAdapter());
	const int32 SourceIndex = Request.SourceIndex;
	const int32 TransferCount = Request.Count > 0 ? Request.Count : FMath::Max(1, Request.Item.Item.Count);
	const FIntPoint DestCell = Request.bHasDestCell
		? Request.DestCell
		: ((DestGrid->SelectedCell.X >= 0 && DestGrid->SelectedCell.Y >= 0) ? DestGrid->SelectedCell : FIntPoint(0, 0));

	// Shop flow interception.
	if (ActiveShopComponent && SourceAdapter && SourceAdapter->ActiveShopComponent == ActiveShopComponent)
	{
		UYIInventoryComponent* DestOwnerComp = DestGrid->Bag ? DestGrid->Bag->GetTypedOuter<UYIInventoryComponent>() : nullptr;

		// Buy: shop stock -> player inventory.
		if (SourceAdapter->bIsShopStockGrid && !bIsShopStockGrid && SourceIndex != INDEX_NONE)
		{
			APlayerController* PC = DestGrid->GetOwningPlayer();
			if (!PC && DestGrid->GetWorld())
			{
				PC = UGameplayStatics::GetPlayerController(DestGrid->GetWorld(), 0);
			}
			if (PC)
			{
				if (UYITradeInteractionComponent* TradeComp = PC->FindComponentByClass<UYITradeInteractionComponent>())
				{
					FYIShopBuyRequest Req;
					Req.Shop = ActiveShopComponent;
					Req.StockIndex = SourceIndex;
					Req.StockItemInstanceId = Request.Item.Item.InstanceId;
					Req.Count = TransferCount;
					Req.BuyerInv = DestOwnerComp;
					Req.DestPos = DestCell;
					TradeComp->RequestShopBuyEx(Req);
					return EYIInventoryGridExternalOpResult::HandledSucceeded;
				}
			}
			return EYIInventoryGridExternalOpResult::HandledFailed;
		}

		// Sell: player inventory -> shop stock.
		if (!SourceAdapter->bIsShopStockGrid && bIsShopStockGrid && SourceIndex != INDEX_NONE)
		{
			APlayerController* PC = DestGrid->GetOwningPlayer();
			if (!PC && DestGrid->GetWorld())
			{
				PC = UGameplayStatics::GetPlayerController(DestGrid->GetWorld(), 0);
			}
			if (PC)
			{
				if (UYITradeInteractionComponent* TradeComp = PC->FindComponentByClass<UYITradeInteractionComponent>())
				{
					UYIInventoryComponent* ShopSourceComp = SourceGrid->Bag ? SourceGrid->Bag->GetTypedOuter<UYIInventoryComponent>() : nullptr;
					FYIShopSellRequest Req;
					Req.Shop = ActiveShopComponent;
					Req.SourceIndex = SourceIndex;
					Req.SourceItemInstanceId = Request.Item.Item.InstanceId;
					Req.Count = TransferCount;
					Req.SellerInv = ShopSourceComp;
					TradeComp->RequestShopSellEx(Req);
					return EYIInventoryGridExternalOpResult::HandledSucceeded;
				}
			}
			return EYIInventoryGridExternalOpResult::HandledFailed;
		}

		// Block moves into/within stock grid.
		return EYIInventoryGridExternalOpResult::HandledFailed;
	}

	// Trade session flow interception.
	if (ActiveTradeSession && SourceAdapter &&
		SourceAdapter->ActiveTradeSession == ActiveTradeSession &&
		SourceAdapter->bHasTradeSide && bHasTradeSide && SourceIndex != INDEX_NONE)
	{
		APlayerController* PC = DestGrid->GetOwningPlayer();
		if (!PC && DestGrid->GetWorld())
		{
			PC = UGameplayStatics::GetPlayerController(DestGrid->GetWorld(), 0);
		}
		if (PC)
		{
			if (UYITradeInteractionComponent* TradeComp = PC->FindComponentByClass<UYITradeInteractionComponent>())
			{
				FYITradeTransferRequest Req;
				Req.FromSide = SourceAdapter->TradeSide;
				Req.ToSide = TradeSide;
				Req.SourceIndex = SourceIndex;
				Req.DestPos = DestCell;
				Req.Count = Request.Count;
				TradeComp->RequestTradeTransferEx(Req);
				return EYIInventoryGridExternalOpResult::HandledSucceeded;
			}
		}

		return EYIInventoryGridExternalOpResult::HandledFailed;
	}

	return EYIInventoryGridExternalOpResult::NotHandled;
}

void UYIInventoryGridGameplayAdapter::AugmentTooltipData(
	UInventoryGridWidget* Grid,
	const UYIInventoryBag* Bag,
	int32 ItemIndex,
	FYITooltipData& InOutTooltipData)
{
	if (!Bag || !Bag->Items.IsValidIndex(ItemIndex))
	{
		return;
	}

	const APlayerController* PC = Grid ? Grid->GetOwningPlayer() : nullptr;
	FYIItemDescriptionContext Context;
	Context.Item = &Bag->Items[ItemIndex].Item;
	Context.Bag = Bag;
	Context.Shop = ActiveShopComponent;
	Context.ViewerPlayerState = PC ? PC->PlayerState : nullptr;
	Context.bShopBuyContext = (ActiveShopComponent != nullptr) && bIsShopStockGrid;
	Context.Count = 1;
	FYIItemDescriptionResolver::AugmentTooltip(Context, InOutTooltipData);
}

bool UYIInventoryGridGameplayAdapter::TryEquipItemFromInventory(
	UObject* EquipmentContextObject,
	UYIInventoryComponent* SourceInventory,
	int32 SourceIndex,
	FGameplayTag RequestedSlotTag)
{
	if (UYIEquipmentComponent* EquipmentComponent = Cast<UYIEquipmentComponent>(EquipmentContextObject))
	{
		if (!SourceInventory)
		{
			return false;
		}
		FYIEquipFromInventoryRequest Req;
		Req.SourceInventory = SourceInventory;
		Req.SourceIndex = SourceIndex;
		Req.RequestedSlotTag = RequestedSlotTag;
		const FYIEquipmentOpResult Result = EquipmentComponent->RequestEquip(Req);
		return (EquipmentComponent->GetOwner() && EquipmentComponent->GetOwner()->HasAuthority()) ? Result.bSucceeded : Result.bRequestAccepted;
	}
	return false;
}

bool UYIInventoryGridGameplayAdapter::TrySpawnWorldDropFromInstance(
	UObject* WorldContextObject,
	const FYIItemInstance& Item,
	const FTransform& SpawnTransform)
{
	return UYIWorldLootBlueprintLibrary::SpawnItemPickupFromInstance(WorldContextObject, Item, SpawnTransform) != nullptr;
}
