#include "ShopScreenWidget.h"

#include "InventoryDragOverlayUserWidget.h"
#include "UILayerSubsystem.h"
#include "YIInventoryBag.h"
#include "YIItemBlueprintLibrary.h"

void UShopScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (LeftGrid && SharedTooltip) { LeftGrid->SetBoundTooltipWidget(SharedTooltip); }
	if (RightGrid && SharedTooltip) { RightGrid->SetBoundTooltipWidget(SharedTooltip); }
	if (LeftGrid) { LeftGrid->OnCellSelected.AddDynamic(this, &UShopScreenWidget::OnLeftCellSelected); }
	if (RightGrid) { RightGrid->OnCellSelected.AddDynamic(this, &UShopScreenWidget::OnRightCellSelected); }
	if (LeftGrid) LeftGrid->RefreshBoundTooltip();
	if (RightGrid) RightGrid->RefreshBoundTooltip();

	// Always render drag ghosts/highlights inside each grid to keep drag/drop simple.
	if (LeftGrid) LeftGrid->SetUseGlobalDragGhost(false);
	if (RightGrid) RightGrid->SetUseGlobalDragGhost(false);

	// If a legacy overlay exists in the UMG layout, collapse it to avoid duplicate drawing/overhead.
	if (DragOverlay) { DragOverlay->SetVisibility(ESlateVisibility::Collapsed); }

	RequestPush(true);
}

void UShopScreenWidget::NativeDestruct()
{
	if (LeftGrid) { LeftGrid->OnCellSelected.RemoveAll(this); }
	if (RightGrid) { RightGrid->OnCellSelected.RemoveAll(this); }
	if (Shop)
	{
		Shop->OnStockMirrorUpdated.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UShopScreenWidget::SetShop(UYIShopComponent* InShop, UYIInventoryBag* LocalPlayerBag, const TArray<FYINetBagItem>& Stock, FIntPoint StockSize)
{
	if (Shop)
	{
		Shop->OnStockMirrorUpdated.RemoveAll(this);
	}

	Shop = InShop;
	LocalBag = LocalPlayerBag;

	if (LeftGrid)
	{
		LeftGrid->SetBag(LocalBag);
		LeftGrid->SetShopContext(Shop, false);
		LeftGrid->RefreshBoundTooltip();
	}

	ShopMirrorBag = BuildMirrorFromStock(Stock, StockSize);
	if (RightGrid)
	{
		RightGrid->SetBag(ShopMirrorBag);
		RightGrid->SetShopContext(Shop, true);
		RightGrid->SetAllowSelfMove(false);
		RightGrid->RefreshBoundTooltip();
	}

	if (Shop && Shop->StockMode == EYIShopStockMode::SharedStock)
	{
		Shop->OnStockMirrorUpdated.AddDynamic(this, &UShopScreenWidget::HandleShopMirrorUpdated);
	}
}

UYIInventoryBag* UShopScreenWidget::BuildMirrorFromStock(const TArray<FYINetBagItem>& View, FIntPoint GridSize)
{
	UYIInventoryBag* Bag = NewObject<UYIInventoryBag>(this);
	if (!Bag) return nullptr;
	Bag->GridSize = GridSize;
	for (const FYINetBagItem& Net : View)
	{
		if (Net.Code == 0 || Net.Count <= 0) continue;
		FYIBagItem Item;
		Item.Item = UYIItemBlueprintLibrary::MakeItemInstanceByCode(Net.Code, Net.Count);
		Item.Item.CustomStackKey = Net.CustomStackKey;
		Item.Pos = Net.Pos;
		Item.Size = Net.Size;
		Bag->Items.Add(Item);
	}
	return Bag;
}

void UShopScreenWidget::HandleShopMirrorUpdated()
{
	if (!Shop || Shop->StockMode != EYIShopStockMode::SharedStock) return;

	const TArray<FYINetBagItem> Stock = Shop->GetStockMirror();
	const FIntPoint Size = Shop->GetStockMirrorSize();
	ShopMirrorBag = BuildMirrorFromStock(Stock, Size);

	if (RightGrid)
	{
		RightGrid->SetBag(ShopMirrorBag);
		RightGrid->SetShopContext(Shop, true);
		RightGrid->SetAllowSelfMove(false);
		RightGrid->RefreshBoundTooltip();
	}
}

void UShopScreenWidget::OnLeftCellSelected(FIntPoint NewCell)
{
	if (LeftGrid) { LeftGrid->RefreshBoundTooltip(); }
}

void UShopScreenWidget::OnRightCellSelected(FIntPoint NewCell)
{
	if (RightGrid) { RightGrid->RefreshBoundTooltip(); }
}
