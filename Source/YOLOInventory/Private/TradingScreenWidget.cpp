#include "TradingScreenWidget.h"
#include "UILayerSubsystem.h"
#include "InventoryScreenWidget.h"
#include "InventoryDragOverlayUserWidget.h"
#include "YIInventoryBag.h"

void UTradingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// Ensure both grids exist and are independent
	if (LeftGrid && LeftTooltip) { LeftGrid->SetBoundTooltipWidget(LeftTooltip); }
	if (RightGrid && RightTooltip) { RightGrid->SetBoundTooltipWidget(RightTooltip); }
	if (LeftGrid) { LeftGrid->OnCellSelected.AddDynamic(this, &UTradingScreenWidget::OnLeftCellSelected); }
	if (RightGrid) { RightGrid->OnCellSelected.AddDynamic(this, &UTradingScreenWidget::OnRightCellSelected); }
	if (LeftGrid) LeftGrid->RefreshBoundTooltip();
	if (RightGrid) RightGrid->RefreshBoundTooltip();
	// Always render drag ghosts/highlights inside each grid to keep drag/drop simple
	if (LeftGrid) LeftGrid->SetUseGlobalDragGhost(false);
	if (RightGrid) RightGrid->SetUseGlobalDragGhost(false);
	// If a legacy overlay exists in the UMG layout, collapse it to avoid duplicate drawing/overhead
	if (DragOverlay) { DragOverlay->SetVisibility(ESlateVisibility::Collapsed); }
	FocusedGrid = LeftGrid ? LeftGrid : RightGrid;
	// Auto push this screen to layered UI stack
	RequestPush(true);
}

void UTradingScreenWidget::NativeDestruct()
{
	if (LeftGrid) { LeftGrid->OnCellSelected.RemoveAll(this); }
	if (RightGrid) { RightGrid->OnCellSelected.RemoveAll(this); }
	Super::NativeDestruct();
}

void UTradingScreenWidget::SetBags(UYIInventoryBag* InLeftBag, UYIInventoryBag* InRightBag)
{
	if (LeftGrid) { LeftGrid->SetBag(InLeftBag); LeftGrid->RefreshBoundTooltip(); }
	if (RightGrid) { RightGrid->SetBag(InRightBag); RightGrid->RefreshBoundTooltip(); }
}

bool UTradingScreenWidget::TransferSelection(bool bLeftToRight, int32 Count, int32& OutDestIndex)
{
	OutDestIndex = INDEX_NONE;
	UInventoryGridWidget* Src = bLeftToRight ? LeftGrid : RightGrid;
	UInventoryGridWidget* Dst = bLeftToRight ? RightGrid : LeftGrid;
	if (!Src || !Dst || !Src->Bag || !Dst->Bag) return false;
	int32 Sel = Src->GetSelectedItemIndex();
	if (Sel == INDEX_NONE) return false;

	// Reuse InventoryScreenWidget helper behavior if available
	// (Equivalent to manual Remove from Src and Add into Dst at highlighted placement via drag-drop rules.)
	FYIBagItem Item = Src->Bag->Items.IsValidIndex(Sel) ? Src->Bag->Items[Sel] : FYIBagItem();
	if (!Src->Bag->RemoveItem(Sel)) return false;
	// Try to place at Dst current selected cell if valid (approximation for preferred drop), else use first-fit
	FIntPoint DropCell = Dst->SelectedCell;
	if (!Dst->Bag->CanPlaceAt(DropCell, Item.Size))
	{
		// fallback to find first fit
		int32 TmpIdx = Dst->Bag->AddBagItem(Item);
		if (TmpIdx == INDEX_NONE) { /* rollback */ Src->Bag->AddBagItem(Item); return false; }
		OutDestIndex = TmpIdx; return true;
	}
	Item.Pos = DropCell;
	int32 NewIdx = Dst->Bag->AddBagItem(Item);
	if (NewIdx == INDEX_NONE) { Src->Bag->AddBagItem(Item); return false; }
	OutDestIndex = NewIdx;
	return true;
}

void UTradingScreenWidget::SetFocusedGrid(UInventoryGridWidget* NewFocused)
{
	FocusedGrid = NewFocused ? NewFocused : FocusedGrid;
}

void UTradingScreenWidget::OnLeftCellSelected(FIntPoint NewCell)
{
	if (LeftGrid) { LeftGrid->RefreshBoundTooltip(); }
}

void UTradingScreenWidget::OnRightCellSelected(FIntPoint NewCell)
{
	if (RightGrid) { RightGrid->RefreshBoundTooltip(); }
}

bool UTradingScreenWidget::HandleMoveUp()
{
	if (FocusedGrid) { FocusedGrid->MoveSelectionUp(); return true; }
	return false;
}
bool UTradingScreenWidget::HandleMoveDown()
{
	if (FocusedGrid) { FocusedGrid->MoveSelectionDown(); return true; }
	return false;
}
bool UTradingScreenWidget::HandleMoveLeft()
{
	if (FocusedGrid == RightGrid && LeftGrid) { SetFocusedGrid(LeftGrid); return true; }
	if (FocusedGrid) { FocusedGrid->MoveSelectionLeft(); return true; }
	return false;
}
bool UTradingScreenWidget::HandleMoveRight()
{
	if (FocusedGrid == LeftGrid && RightGrid) { SetFocusedGrid(RightGrid); return true; }
	if (FocusedGrid) { FocusedGrid->MoveSelectionRight(); return true; }
	return false;
}
bool UTradingScreenWidget::HandleConfirm()
{
	// optional: open action menu for focused grid if present (reusing InventoryScreenWidget pattern)
	return false;
}
bool UTradingScreenWidget::HandleCancel()
{
	// Let Blueprint close the screen by default
	return false;
}
bool UTradingScreenWidget::HandleOpenMenu()
{
	return HandleConfirm();
}
