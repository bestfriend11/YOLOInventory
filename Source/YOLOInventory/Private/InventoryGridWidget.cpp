#include "InventoryGridWidget.h"
#include "SInventoryGridWidget.h"
#include "YIInventoryBag.h"
#include "YIInventoryBlueprintLibrary.h"
#include "InventoryTooltipWidget.h"
#include "InventoryUtils.h"
#include "YIItemDefinition.h"
#include "InventoryActionMenuWidget.h"

// Global drag state used to track click-to-pickup drags across grids
static struct FInventoryGlobalDrag
{
	UInventoryGridWidget* SourceGrid = nullptr;
	int32 SourceIndex = INDEX_NONE; // original index at pickup time (may be invalid after removal)
	FIntPoint SourcePos = FIntPoint(-1,-1);
	FYIBagItem Item;
	bool bRemovedFromSource = false; // true if we removed the item from its bag when drag started
	bool bActive = false;
	void Reset() { SourceGrid = nullptr; SourceIndex = INDEX_NONE; SourcePos = FIntPoint(-1,-1); Item = FYIBagItem(); bRemovedFromSource = false; bActive = false; }
} GInventoryDrag;

// Check if placing a footprint at Pos would overlap at most one other item (ignoring SourceIdx). Returns that item index or INDEX_NONE.
static bool FindSingleOverlap(const UYIInventoryBag* Bag, int32 SourceIdx, const FIntPoint& Pos, const FIntPoint& Footprint, int32& OutOverlapIdx)
{
	OutOverlapIdx = INDEX_NONE;
	if (!Bag) return false;
	// bounds check
	if (Pos.X < 0 || Pos.Y < 0 || Pos.X + Footprint.X > Bag->GridSize.X || Pos.Y + Footprint.Y > Bag->GridSize.Y) return false;
	for (int32 i=0;i<Bag->Items.Num();++i)
	{
		if (i == SourceIdx) continue;
		const FYIBagItem& It = Bag->Items[i];
		const FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
		if (RectsOverlap(Pos, Footprint, It.Pos, Eff))
		{
			if (OutOverlapIdx == INDEX_NONE) { OutOverlapIdx = i; }
			else if (OutOverlapIdx != i) { OutOverlapIdx = INDEX_NONE; return false; }
		}
	}
	return true;
}

// Helper: get item index at an arbitrary cell in a bag (returns INDEX_NONE)
static int32 GetItemIndexAtCell(const UYIInventoryBag* Bag, const FIntPoint& Cell)
{
	if (!Bag) return INDEX_NONE;
	for (int32 i = 0; i < Bag->Items.Num(); ++i)
	{
		const auto& It = Bag->Items[i];
		if (It.Pos.X < 0 || It.Pos.Y < 0) continue;
		FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
		if (Cell.X >= It.Pos.X && Cell.Y >= It.Pos.Y && Cell.X < It.Pos.X + Eff.X && Cell.Y < It.Pos.Y + Eff.Y)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

TSharedRef<SWidget> UInventoryGridWidget::RebuildWidget()
{
	// Hook Slate callbacks so Slate can notify the owning UWidget about hover/selection changes
	MySlateWidget = SNew(SInventoryGridWidget)
		.OwnerWidget(this)
		.Bag(Bag)
		.CellPixelSize(CellPixelSize)
		.OnHoveredItemChanged(SInventoryGridWidget::FOnHoveredItemChanged::CreateLambda([WeakThis = TWeakObjectPtr<UInventoryGridWidget>(this)](int32 Idx)
		{
			if (WeakThis.IsValid()) { WeakThis->HandleHoverChanged(Idx); }
		}))
		.OnSelectedCellChanged(SInventoryGridWidget::FOnSelectedCellChanged::CreateLambda([WeakThis = TWeakObjectPtr<UInventoryGridWidget>(this)](const FIntPoint& Cell)
		{
			if (WeakThis.IsValid()) { WeakThis->HandleSelectionChanged(Cell); }
		}))
		.OnCellClicked(SInventoryGridWidget::FOnCellClicked::CreateLambda([WeakThis = TWeakObjectPtr<UInventoryGridWidget>(this)](const FIntPoint& Cell)
		{
			if (WeakThis.IsValid()) { WeakThis->HandleCellClicked(Cell); }
		}));
	// Apply wrap setting
	if (MySlateWidget.IsValid()) MySlateWidget->SetWrapNavigation(bWrapNavigation);
	return MySlateWidget.ToSharedRef();
}

void UInventoryGridWidget::SetWrapNavigation(bool bEnable)
{
	bWrapNavigation = bEnable;
	if (MySlateWidget.IsValid()) MySlateWidget->SetWrapNavigation(bWrapNavigation);
}

void UInventoryGridWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (MySlateWidget)
	{
		MySlateWidget->SetBag(Bag);
		MySlateWidget->SetCellPixelSize(CellPixelSize);
		MySlateWidget->SetWrapNavigation(bWrapNavigation);
	}

	// Bind to bag change events so tooltips are updated when items change
	if (CachedBag != Bag)
	{
		if (CachedBag)
		{
			CachedBag->OnChanged.Remove(BagChangedHandle);
		}
		if (Bag)
		{
			BagChangedHandle = Bag->OnChanged.AddLambda([this]() { OnBagChanged(); });
		}
		CachedBag = Bag;
	}

	// Ensure tooltip reflects the current selection
	UpdateBoundTooltip();
}

FVector2D UInventoryGridWidget::GetDesiredSize() const
{
	if (MySlateWidget.IsValid()) return MySlateWidget->GetDesiredSize();
	if (Bag) return FVector2D(Bag->GridSize) * FVector2D(CellPixelSize, CellPixelSize);
	return FVector2D::Zero();
}

bool UInventoryGridWidget::MoveSelection(FIntPoint Delta)
{
	if (!MySlateWidget.IsValid()) return false;
	// Let Slate change selection and notify via the slate callback which calls HandleSelectionChanged
	return MySlateWidget->MoveSelection(Delta);
}

bool UInventoryGridWidget::MoveSelectionUp()
{
	return MoveSelection(FIntPoint(0, -1));
}

bool UInventoryGridWidget::MoveSelectionDown()
{
	return MoveSelection(FIntPoint(0, 1));
}

bool UInventoryGridWidget::MoveSelectionLeft()
{
	return MoveSelection(FIntPoint(-1, 0));
}

bool UInventoryGridWidget::MoveSelectionRight()
{
	return MoveSelection(FIntPoint(1, 0));
}

void UInventoryGridWidget::SetSelectedCell(FIntPoint Cell)
{
	if (MySlateWidget.IsValid())
	{
		MySlateWidget->SetSelectedCell(Cell);
		// Slate may not trigger the callback when set programmatically; ensure we process the change here
		HandleSelectionChanged(Cell);
	}
}

bool UInventoryGridWidget::GetSelectedCellTooltipData(FYITooltipData& OutData) const
{
	if (!Bag) return false;
	if (SelectedCell.X < 0 || SelectedCell.Y < 0) return false;
	for (int32 i = 0; i < Bag->Items.Num(); ++i)
	{
		const auto& It = Bag->Items[i];
		if (It.Pos.X < 0 || It.Pos.Y < 0) continue;
		FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
		if (SelectedCell.X >= It.Pos.X && SelectedCell.Y >= It.Pos.Y && SelectedCell.X < It.Pos.X + Eff.X && SelectedCell.Y < It.Pos.Y + Eff.Y)
		{
			return UYIInventoryBlueprintLibrary::GetItemTooltipData(Bag, i, OutData);
		}
	}
	return false;
}

int32 UInventoryGridWidget::GetSelectedItemIndex() const
{
	if (!Bag) return INDEX_NONE;
	if (SelectedCell.X < 0 || SelectedCell.Y < 0) return INDEX_NONE;
	for (int32 i = 0; i < Bag->Items.Num(); ++i)
	{
		const auto& It = Bag->Items[i];
		if (It.Pos.X < 0 || It.Pos.Y < 0) continue;
		FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
		if (SelectedCell.X >= It.Pos.X && SelectedCell.Y >= It.Pos.Y && SelectedCell.X < It.Pos.X + Eff.X && SelectedCell.Y < It.Pos.Y + Eff.Y)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

// Called when the bound bag changes (items added/removed/modified)
void UInventoryGridWidget::OnBagChanged()
{
	// Bag changed (add/remove/etc.) — ensure selection hover/tooltips remain correct
	// Recompute selection-related state by re-broadcasting the current selection
	HandleSelectionChanged(SelectedCell);
	UpdateBoundTooltip();
}

void UInventoryGridWidget::HandleSelectionChanged(const FIntPoint& NewCell)
{
	SelectedCell = NewCell;
	// Determine item index now selected
	int32 ItemIndex = GetSelectedItemIndex();
	// Legacy simple cell selected event
	OnCellSelected.Broadcast(SelectedCell);
	// New detailed event
	OnSelectionChanged.Broadcast(SelectedCell, ItemIndex);
	// Update tooltip to reflect new selection
	UpdateBoundTooltip();

	// Optionally request UI to show action menu
	OnActionMenuRequested.Broadcast(ItemIndex);
	if (bAutoOpenActionMenuOnSelect && BoundActionMenu && ItemIndex != INDEX_NONE)
	{
		RequestOpenActionMenu(BoundActionMenu);
	}
}

bool UInventoryGridWidget::GetAvailableActionsForSelectedItem(TArray<FText>& OutActions, TArray<int32>& OutActionIds) const
{
	OutActions.Reset(); OutActionIds.Reset();
	if (!Bag) return false;
	int32 Index = GetSelectedItemIndex();
	if (Index == INDEX_NONE) return false;

	// Default actions
	OutActions.Add(NSLOCTEXT("YOLOInventory", "UseAction", "Use")); OutActionIds.Add(ACTION_USE);
	if (Bag->bAllowRotation) { OutActions.Add(NSLOCTEXT("YOLOInventory", "RotateAction", "Rotate")); OutActionIds.Add(ACTION_ROTATE); }
	OutActions.Add(NSLOCTEXT("YOLOInventory", "DropAction", "Drop")); OutActionIds.Add(ACTION_DROP);
	int32 Found = Bag->FindExistingStackIndexForItem(Bag->Items[Index]);
	if (Found != INDEX_NONE && Found != Index) { OutActions.Add(NSLOCTEXT("YOLOInventory", "CombineAction", "Combine")); OutActionIds.Add(ACTION_COMBINE); }
	OutActions.Add(NSLOCTEXT("YOLOInventory", "SellAction", "Sell")); OutActionIds.Add(ACTION_SELL);

	// Extra actions: transfer, inspect, equip/grab
	OutActions.Add(NSLOCTEXT("YOLOInventory", "TransferAction", "Transfer")); OutActionIds.Add(ACTION_TRANSFER);
	OutActions.Add(NSLOCTEXT("YOLOInventory", "InspectAction", "Inspect")); OutActionIds.Add(ACTION_INSPECT);
	OutActions.Add(NSLOCTEXT("YOLOInventory", "EquipAction", "Equip")); OutActionIds.Add(ACTION_EQUIP);
	OutActions.Add(NSLOCTEXT("YOLOInventory", "GrabAction", "Grab")); OutActionIds.Add(ACTION_GRAB);

	return true;
}

bool UInventoryGridWidget::RequestOpenActionMenu(UInventoryActionMenuWidget* Menu)
{
	if (!Menu) return false;
	TArray<FText> Actions; TArray<int32> ActionIds;
	if (!GetAvailableActionsForSelectedItem(Actions, ActionIds)) return false;
	Menu->ShowActions(Actions, ActionIds);
	return true;
}

void UInventoryGridWidget::HandleHoverChanged(int32 HoveredIndex)
{
	// Broadcast hover state to Blueprint listeners
	OnItemHoverChanged.Broadcast(HoveredIndex);
	// Optionally refresh tooltip if there is no explicit selection but we want hover to drive tooltip (kept to selection-only for now)
}

bool UInventoryGridWidget::BeginDragFromCell(FIntPoint Cell)
{
	if (!Bag) return false;
	int32 Idx = GetItemIndexAtCell(Bag, Cell);
	if (Idx == INDEX_NONE) return false;
	GInventoryDrag.SourceGrid = this;
	GInventoryDrag.SourceIndex = Idx;
	GInventoryDrag.Item = Bag->Items[Idx];
	GInventoryDrag.SourcePos = GInventoryDrag.Item.Pos;
	// Remove it from the bag immediately so the slot is visually free during drag
	if (Bag->RemoveItem(Idx))
	{
		GInventoryDrag.bRemovedFromSource = true;
		// Adjust SourceIndex if needed: after removal, index may shift; set to INDEX_NONE to signal unattached during drag
		GInventoryDrag.SourceIndex = INDEX_NONE;
	}
	GInventoryDrag.bActive = true;
	// Notify listeners
	OnItemDragStarted.Broadcast(this, Idx);
	// Clear selection so UI reflects pickup
	SetSelectedCell(FIntPoint(-1, -1));
	UpdateBoundTooltip();
	return true;
}

bool UInventoryGridWidget::BeginDragFromSelectedCell()
{
	if (SelectedCell.X < 0 || SelectedCell.Y < 0) return false;
	return BeginDragFromCell(SelectedCell);
}

bool UInventoryGridWidget::DropDraggedItemAtCell(FIntPoint Cell)
{
	if (!GInventoryDrag.bActive) return false;
	if (!Bag) return false;

	// Same bag: if this drag originated here and we removed from source, we are placing an unattached item now
	if (GInventoryDrag.SourceGrid == this)
	{
		// When pickup removed the item, SourceIndex is INDEX_NONE and the bag no longer contains it. Treat as add-at-cell or swap.
		if (GInventoryDrag.bRemovedFromSource && GInventoryDrag.SourceIndex == INDEX_NONE)
		{
			FYIBagItem ToPlace = GInventoryDrag.Item; ToPlace.Pos = Cell;
			// Enforce exact placement at the highlighted cell; do not allow AddBagItem to relocate to first-fit
			if (Bag->CanPlaceAt(Cell, ToPlace.Size))
			{
				int32 NewIdx = Bag->AddBagItem(ToPlace);
				if (NewIdx != INDEX_NONE)
				{
					OnItemDropped.Broadcast(this, INDEX_NONE, Cell, true);
					// Drag ends because no victim was displaced
					GInventoryDrag.Reset();
					UpdateBoundTooltip();
					return true;
				}
			}
			// If we can't place exactly at Cell, try displacing a single overlapped item and continue dragging that victim
			int32 Victim = INDEX_NONE; const FIntPoint Foot = Bag->GetEffectiveSize(ToPlace.Size);
			if (!FindSingleOverlap(Bag, INDEX_NONE, Cell, Foot, Victim) || Victim == INDEX_NONE)
			{
				OnItemDropped.Broadcast(this, INDEX_NONE, Cell, false);
				return false;
			}
			// Displace victim: remove it from the bag and continue dragging it (no swap/backfill)
			FYIBagItem SavedVictim = Bag->Items[Victim];
			if (!Bag->RemoveItem(Victim))
			{
				OnItemDropped.Broadcast(this, INDEX_NONE, Cell, false);
				return false;
			}
			// Place dragged item at Cell (exact)
			int32 NewIdx = INDEX_NONE;
			if (Bag->CanPlaceAt(Cell, ToPlace.Size))
			{
				NewIdx = Bag->AddBagItem(ToPlace);
			}
			if (NewIdx == INDEX_NONE)
			{
				// Rollback: reinsert victim to its original spot
				SavedVictim.Pos = SavedVictim.Pos; // unchanged
				Bag->AddBagItem(SavedVictim);
				OnItemDropped.Broadcast(this, INDEX_NONE, Cell, false);
				return false;
			}
			OnItemDropped.Broadcast(this, INDEX_NONE, Cell, true);
			// Continue dragging the displaced item (victim) as UNATTACHED (no lingering visual)
			GInventoryDrag.SourceGrid = this;
			GInventoryDrag.SourceIndex = INDEX_NONE;
			GInventoryDrag.Item = SavedVictim;
			GInventoryDrag.bRemovedFromSource = true;
			GInventoryDrag.bActive = true;
			OnItemDragStarted.Broadcast(this, INDEX_NONE);
			UpdateBoundTooltip();
			return true;
		}
		if (!Bag->MoveItem(GInventoryDrag.SourceIndex, Cell))
		{
			// Allow displacing a single overlapped item if the footprint only hits that one
			const FYIBagItem& Src = Bag->Items[GInventoryDrag.SourceIndex];
			const FIntPoint Foot = Bag->GetEffectiveSize(Src.Size);
			int32 Victim = INDEX_NONE;
			if (!FindSingleOverlap(Bag, GInventoryDrag.SourceIndex, Cell, Foot, Victim) || Victim == INDEX_NONE)
			{
				OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
				return false;
			}
			// Remove victim, adjust source index if needed, then attempt move again
			FYIBagItem SavedVictim = Bag->Items[Victim];
			Bag->RemoveItem(Victim);
			if (Victim < GInventoryDrag.SourceIndex)
			{
				GInventoryDrag.SourceIndex -= 1;
			}
			if (!Bag->MoveItem(GInventoryDrag.SourceIndex, Cell))
			{
			// Failed even after clearing victim; restore it in-place to avoid merge/stack side-effects
			Bag->Items.Insert(SavedVictim, Victim);
			Bag->MarkPackageDirty(); Bag->OnChanged.Broadcast();
				OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
				return false;
			}
			// Start a new drag with the displaced item
			GInventoryDrag.SourceGrid = nullptr;
			GInventoryDrag.SourceIndex = INDEX_NONE;
			GInventoryDrag.Item = SavedVictim;
			GInventoryDrag.bActive = true;
			OnItemDragStarted.Broadcast(this, INDEX_NONE);
		}
		OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, true);
		if (GInventoryDrag.SourceGrid != nullptr || GInventoryDrag.SourceIndex != INDEX_NONE)
		{
			GInventoryDrag.Reset();
		}
		UpdateBoundTooltip();
		return true;
	}

	// Cross-bag: perform an atomic swap (drag item takes victim's slot, victim becomes active drag or drops)
	FYIBagItem ToPlace = GInventoryDrag.Item;
	ToPlace.Pos = Cell;
	
	// Check if placement at Cell is possible or if we need a victim swap
	int32 VictimIdx = INDEX_NONE;
	const FIntPoint Foot = Bag->GetEffectiveSize(ToPlace.Size);
	
	// First try to place without a victim
	if (Bag->CanPlaceAt(Cell, ToPlace.Size))
	{
		// Clean placement: just add and remove from source
		int32 NewIdx = Bag->AddBagItem(ToPlace);
		if (NewIdx != INDEX_NONE)
		{
			if (GInventoryDrag.SourceGrid && GInventoryDrag.SourceGrid->Bag)
{
// If item was already removed at pickup, skip removing now
if (GInventoryDrag.bRemovedFromSource || GInventoryDrag.SourceIndex == INDEX_NONE || GInventoryDrag.SourceGrid->Bag->RemoveItem(GInventoryDrag.SourceIndex))
				{
					OnItemTransferred.Broadcast(GInventoryDrag.SourceGrid, GInventoryDrag.SourceIndex, NewIdx);
					if (GInventoryDrag.SourceGrid != this) GInventoryDrag.SourceGrid->OnItemTransferred.Broadcast(GInventoryDrag.SourceGrid, GInventoryDrag.SourceIndex, NewIdx);
					GInventoryDrag.SourceGrid->RefreshBoundTooltip();
					RefreshBoundTooltip();
					OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, true);
					GInventoryDrag.Reset();
					return true;
				}
				else
				{
					// Remove from source failed; undo the add
					Bag->RemoveItem(NewIdx);
					OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
					GInventoryDrag.Reset();
					return false;
				}
			}
			else
			{
				// Unattached drag (picked up from world) — accept the placement
				UE_LOG(LogTemp, Warning, TEXT("Inventory: Placed unattached dragged item into bag at index %d."), NewIdx);
				RefreshBoundTooltip();
				OnItemDropped.Broadcast(this, INDEX_NONE, Cell, true);
				GInventoryDrag.Reset();
				return true;
			}
		}
		// If we get here with NewIdx == INDEX_NONE, fall through to victim logic
	}
	
	// Placement failed or AddBagItem returned INDEX_NONE; try victim swap
	if (!FindSingleOverlap(Bag, INDEX_NONE, Cell, Foot, VictimIdx) || VictimIdx == INDEX_NONE)
	{
		OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
		return false;
	}
	
	// We have a victim. Now perform an ATOMIC swap across bags:
	// 1. Save both items
	FYIBagItem SavedDragged = GInventoryDrag.Item;
	FYIBagItem SavedVictim = Bag->Items[VictimIdx];
	
	// Debug: log pre-swap state
	auto GetDefName = [](const FYIBagItem& It)->FString { return It.Item.Definition.IsValid() ? It.Item.Definition.Get()->GetName() : It.Item.Definition.ToSoftObjectPath().ToString(); };
	UE_LOG(LogTemp, Warning, TEXT("Inventory Swap START: SrcGrid=%s SrcIdx=%d SrcBagCount=%d DestBagCount=%d VictimIdx=%d Cell=(%d,%d) Dragged=%s Victim=%s"),
		(GInventoryDrag.SourceGrid? *GInventoryDrag.SourceGrid->GetName() : TEXT("null")),
		GInventoryDrag.SourceIndex,
		(GInventoryDrag.SourceGrid && GInventoryDrag.SourceGrid->Bag) ? GInventoryDrag.SourceGrid->Bag->Items.Num() : -1,
		Bag->Items.Num(),
		VictimIdx,
		Cell.X, Cell.Y,
		*GetDefName(SavedDragged), *GetDefName(SavedVictim));

	// 2. Place dragged at Cell with victim's position as fallback
	ToPlace.Pos = Cell;
	SavedDragged.Pos = Cell;
	
	// 3. Perform swap in destination bag
	Bag->Items[VictimIdx] = SavedDragged;
	Bag->Items[VictimIdx].Pos = Cell;
	Bag->MarkPackageDirty();
	Bag->OnChanged.Broadcast();
	UE_LOG(LogTemp, Warning, TEXT("Inventory Swap: Placed dragged into dest at idx %d. DestCount=%d"), VictimIdx, Bag->Items.Num());
	
	// 4. Remove the original dragged item from its source bag (if not already this grid and not the same as victim)
	if (GInventoryDrag.SourceGrid && GInventoryDrag.SourceGrid->Bag)
	{
		UYIInventoryBag* SourceBag = GInventoryDrag.SourceGrid->Bag;
		const int32 SourceIdx = GInventoryDrag.SourceIndex;
		// Only remove if source is not the same as destination victim index
		if (!(SourceBag == Bag && SourceIdx == VictimIdx))
		{
			UE_LOG(LogTemp, Warning, TEXT("Inventory Swap: Removing original dragged item from source bag. SourceIdx=%d SourceBagCountBefore=%d"), SourceIdx, SourceBag->Items.Num());
			if (!SourceBag->RemoveItem(SourceIdx))
			{
				// Source removal failed; FULLY REVERT the destination change
				Bag->Items[VictimIdx] = SavedVictim;
				Bag->MarkPackageDirty();
				Bag->OnChanged.Broadcast();
				UE_LOG(LogTemp, Error, TEXT("Inventory Swap: Failed to remove original dragged item from source. Reverting. DestCount=%d"), Bag->Items.Num());
				OnItemDropped.Broadcast(this, SourceIdx, Cell, false);
				GInventoryDrag.Reset();
				return false;
			}
			UE_LOG(LogTemp, Warning, TEXT("Inventory Swap: Removed original dragged item from source. SourceBagCountAfter=%d"), SourceBag->Items.Num());
		}
		// Success! Notify both grids
		OnItemTransferred.Broadcast(GInventoryDrag.SourceGrid, SourceIdx, VictimIdx);
		if (GInventoryDrag.SourceGrid != this) 
		{
			GInventoryDrag.SourceGrid->OnItemTransferred.Broadcast(GInventoryDrag.SourceGrid, SourceIdx, VictimIdx);
		}
		GInventoryDrag.SourceGrid->RefreshBoundTooltip();
	}
	
	// 5. Set victim as active drag for the next placement
	RefreshBoundTooltip();
	OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, true);
	UE_LOG(LogTemp, Warning, TEXT("Inventory Swap COMPLETE: NewDragItem=%s NewDragActive=%d DestCount=%d"), *GetDefName(SavedVictim), (int)GInventoryDrag.bActive, Bag->Items.Num());

	// Update drag state: the victim is now at VictimIdx in this bag (linked, not unattached)
	// If BeginDragFromCell is called again, it will just pick up the victim from its new location
	GInventoryDrag.SourceGrid = this;
	GInventoryDrag.SourceIndex = VictimIdx;
	GInventoryDrag.Item = SavedVictim;
	GInventoryDrag.bActive = true;
	OnItemDragStarted.Broadcast(this, VictimIdx);
	return true;
}

void UInventoryGridWidget::CancelDrag()
{
	if (GInventoryDrag.bActive)
	{
		// If we removed the item from its source bag at pickup, try to restore it at its original position
		if (GInventoryDrag.bRemovedFromSource && GInventoryDrag.SourceGrid && GInventoryDrag.SourceGrid->Bag)
		{
			UYIInventoryBag* SrcBag = GInventoryDrag.SourceGrid->Bag;
			FYIBagItem Restore = GInventoryDrag.Item; Restore.Pos = GInventoryDrag.SourcePos;
			// Try to add back; if it fails (blocked now), try to add anywhere to avoid loss
			if (SrcBag->AddBagItem(Restore) == INDEX_NONE)
			{
				// Fallback: place at first available cell
				for (int y=0; y<SrcBag->GridSize.Y; ++y)
				{
					bool bBreak=false;
					for (int x=0; x<SrcBag->GridSize.X; ++x)
					{
						Restore.Pos = FIntPoint(x,y);
						if (SrcBag->AddBagItem(Restore) != INDEX_NONE) { bBreak=true; break; }
					}
					if (bBreak) break;
				}
			}
		}
		GInventoryDrag.Reset();
	}
}

bool UInventoryGridWidget::IsItemDragActive()
{
	return GInventoryDrag.bActive;
}

bool UInventoryGridWidget::GetActiveDraggedItem(FYIBagItem& OutItem, UYIInventoryBag*& OutSourceBag)
{
	if (!GInventoryDrag.bActive) { OutSourceBag = nullptr; return false; }
	OutItem = GInventoryDrag.Item;
	OutSourceBag = GInventoryDrag.SourceGrid ? GInventoryDrag.SourceGrid->Bag : nullptr;
	return true;
}

void UInventoryGridWidget::HandleCellClicked(const FIntPoint& Cell)
{
	// If a drag is active, attempt drop; otherwise start a drag from the clicked cell
	if (GInventoryDrag.bActive)
	{
		// Attempt drop on this cell
		DropDraggedItemAtCell(Cell);
		return;
	}
	// No active drag: click only updates selection; dragging starts on drag detect
}

void UInventoryGridWidget::UpdateBoundTooltip()
{
	if (!BoundTooltipWidget) return;
	FYITooltipData Data;
	if (GetSelectedCellTooltipData(Data))
	{
		BoundTooltipWidget->SetTooltipData(Data);
	}
	else
	{
		// Clear tooltip by sending empty data
		BoundTooltipWidget->SetTooltipData(FYITooltipData());
	}
}

void UInventoryGridWidget::RefreshBoundTooltip()
{
	UpdateBoundTooltip();
}

bool UInventoryGridWidget::TransferSelectedItemTo(UInventoryGridWidget* Other, int32 Count, int32& OutDestIndex)
{
	OutDestIndex = INDEX_NONE;
	if (!Other || !Bag || !Other->Bag) return false;
	int32 SourceIndex = GetSelectedItemIndex();
	if (SourceIndex == INDEX_NONE) return false;
	bool b = UYIInventoryBlueprintLibrary::TransferItemBetweenBags(Bag, Other->Bag, SourceIndex, Count, OutDestIndex);
	if (b)
	{
		// Refresh our tooltip and the other grid's tooltip
		UpdateBoundTooltip();
		Other->RefreshBoundTooltip();
		// Notify grid-level listeners
		OnItemTransferred.Broadcast(this, SourceIndex, OutDestIndex);
		Other->OnItemTransferred.Broadcast(this, SourceIndex, OutDestIndex);
	}
	return b;
}

void UInventoryGridWidget::SetBoundTooltipWidget(UInventoryTooltipWidget* Widget)
{
	BoundTooltipWidget = Widget;
	UpdateBoundTooltip();
}

void UInventoryGridWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	// Unregister bag change delegate
	if (CachedBag)
	{
		if (BagChangedHandle.IsValid())
		{
			CachedBag->OnChanged.Remove(BagChangedHandle);
			BagChangedHandle = FDelegateHandle();
		}
		CachedBag = nullptr;
	}

	// Ensure the underlying Slate widget is removed from parent and released
	if (MySlateWidget.IsValid())
	{
		MySlateWidget.Reset();
	}

	// Clear any bound tooltip pointer so it cannot keep Slate/UWidget references alive
	BoundTooltipWidget = nullptr;
}
