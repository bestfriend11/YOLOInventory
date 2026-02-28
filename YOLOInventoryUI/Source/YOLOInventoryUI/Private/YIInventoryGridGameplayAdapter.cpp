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

EYIInventoryGridExternalOpResult UYIInventoryGridGameplayAdapter::TryHandleCrossGridDrop(
	UInventoryGridWidget* DestGrid,
	UInventoryGridWidget* SourceGrid,
	int32 SourceIndex,
	const FYIBagItem& DraggedItem,
	const FIntPoint& DestCell)
{
	if (!DestGrid || !SourceGrid)
	{
		return EYIInventoryGridExternalOpResult::NotHandled;
	}

	UYIInventoryGridGameplayAdapter* SourceAdapter = Cast<UYIInventoryGridGameplayAdapter>(SourceGrid->GetFeatureAdapter());

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
					const int32 BuyCount = FMath::Max(1, DraggedItem.Item.Count);
					FYIShopBuyRequest Req;
					Req.Shop = ActiveShopComponent;
					Req.StockIndex = SourceIndex;
					Req.StockItemInstanceId = DraggedItem.Item.InstanceId;
					Req.Count = BuyCount;
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
					const int32 SellCount = FMath::Max(1, DraggedItem.Item.Count);
					FYIShopSellRequest Req;
					Req.Shop = ActiveShopComponent;
					Req.SourceIndex = SourceIndex;
					Req.SourceItemInstanceId = DraggedItem.Item.InstanceId;
					Req.Count = SellCount;
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
				Req.Count = 0;
				TradeComp->RequestTradeTransferEx(Req);
				return EYIInventoryGridExternalOpResult::HandledSucceeded;
			}
		}

		return EYIInventoryGridExternalOpResult::HandledFailed;
	}

	return EYIInventoryGridExternalOpResult::NotHandled;
}

EYIInventoryGridExternalOpResult UYIInventoryGridGameplayAdapter::TryHandleTransferSelectedTo(
	UInventoryGridWidget* SourceGrid,
	UInventoryGridWidget* DestGrid,
	int32 SourceIndex,
	int32 Count,
	int32& OutDestIndex)
{
	OutDestIndex = INDEX_NONE;
	if (!SourceGrid || !DestGrid)
	{
		return EYIInventoryGridExternalOpResult::NotHandled;
	}

	UYIInventoryGridGameplayAdapter* DestAdapter = Cast<UYIInventoryGridGameplayAdapter>(DestGrid->GetFeatureAdapter());
	if (ActiveTradeSession && DestAdapter &&
		DestAdapter->ActiveTradeSession == ActiveTradeSession &&
		bHasTradeSide && DestAdapter->bHasTradeSide)
	{
		const FIntPoint DestCell = (DestGrid->SelectedCell.X >= 0 && DestGrid->SelectedCell.Y >= 0)
			? DestGrid->SelectedCell
			: FIntPoint(0, 0);
		APlayerController* PC = SourceGrid->GetOwningPlayer();
		if (!PC && SourceGrid->GetWorld())
		{
			PC = UGameplayStatics::GetPlayerController(SourceGrid->GetWorld(), 0);
		}
		if (PC)
		{
			if (UYITradeInteractionComponent* TradeComp = PC->FindComponentByClass<UYITradeInteractionComponent>())
			{
				FYITradeTransferRequest Req;
				Req.FromSide = TradeSide;
				Req.ToSide = DestAdapter->TradeSide;
				Req.SourceIndex = SourceIndex;
				Req.DestPos = DestCell;
				Req.Count = Count;
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
