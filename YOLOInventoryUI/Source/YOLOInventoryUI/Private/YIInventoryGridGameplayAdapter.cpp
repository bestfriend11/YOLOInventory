#include "YIInventoryGridGameplayAdapter.h"

#include "InventoryGridWidget.h"
#include "YIEquipmentComponent.h"
#include "YIShopComponent.h"
#include "YITradeInteractionComponent.h"
#include "YITradeSessionActor.h"
#include "YIInventoryComponent.h"
#include "YIWorldLootBlueprintLibrary.h"
#include "Kismet/GameplayStatics.h"

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
					TradeComp->RequestShopBuy(ActiveShopComponent, SourceIndex, BuyCount, DestOwnerComp, DestCell);
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
					TradeComp->RequestShopSell(ActiveShopComponent, SourceIndex, SellCount, ShopSourceComp);
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
		UYIInventoryComponent* DestOwnerComp = DestGrid->Bag ? DestGrid->Bag->GetTypedOuter<UYIInventoryComponent>() : nullptr;
		if (DestOwnerComp && DestOwnerComp->GetOwner() && DestOwnerComp->GetOwner()->HasAuthority())
		{
			ActiveTradeSession->ServerTransferItemBetweenSides(SourceAdapter->TradeSide, TradeSide, SourceIndex, DestCell, 0);
			return EYIInventoryGridExternalOpResult::HandledSucceeded;
		}

		APlayerController* PC = DestGrid->GetOwningPlayer();
		if (!PC && DestGrid->GetWorld())
		{
			PC = UGameplayStatics::GetPlayerController(DestGrid->GetWorld(), 0);
		}
		if (PC)
		{
			if (UYITradeInteractionComponent* TradeComp = PC->FindComponentByClass<UYITradeInteractionComponent>())
			{
				TradeComp->RequestTradeTransfer(SourceAdapter->TradeSide, TradeSide, SourceIndex, DestCell, 0);
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
		ActiveTradeSession->ServerTransferItemBetweenSides(TradeSide, DestAdapter->TradeSide, SourceIndex, DestCell, Count);
		return EYIInventoryGridExternalOpResult::HandledSucceeded;
	}

	return EYIInventoryGridExternalOpResult::NotHandled;
}

bool UYIInventoryGridGameplayAdapter::TryEquipItemFromInventory(
	UObject* EquipmentContextObject,
	UYIInventoryComponent* SourceInventory,
	int32 SourceIndex,
	FGameplayTag RequestedSlotTag)
{
	if (UYIEquipmentComponent* EquipmentComponent = Cast<UYIEquipmentComponent>(EquipmentContextObject))
	{
		return SourceInventory && EquipmentComponent->EquipFromInventory(SourceInventory, SourceIndex, RequestedSlotTag);
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
