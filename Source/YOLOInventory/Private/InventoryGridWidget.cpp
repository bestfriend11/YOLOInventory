#include "InventoryGridWidget.h"
#include "SInventoryGridWidget.h"
#include "YIInventoryBag.h"
#include "YIInventoryComponent.h"
#include "YIInventoryBlueprintLibrary.h"
#include "Widgets/InventoryTooltipView.h"
#include "InventoryUtils.h"
#include "YIItemDefinition.h"
#include "InventoryActionMenuWidget.h"
#include "AbilitySystemComponent.h"
#include "YIRequirement.h"
#include "YITradeSessionActor.h"
#include "YITradeInteractionComponent.h"
#include "YIShopComponent.h"
#include "Kismet/GameplayStatics.h"
#include "YIItemPickup.h"
#include "YIItemSFXLibrary.h"

// Global drag state used to track click-to-pickup drags across grids
TSet<TWeakObjectPtr<UInventoryGridWidget>> UInventoryGridWidget::GRegisteredGrids;

static struct FInventoryGlobalDrag
{
	UInventoryGridWidget* SourceGrid = nullptr;
	int32 SourceIndex = INDEX_NONE; // original index at pickup time (may be invalid after removal)
	FIntPoint SourcePos = FIntPoint(-1,-1);
	FYIBagItem Item;
	bool bRemovedFromSource = false; // true if we removed the item from its bag when drag started
	bool bActive = false;
	bool bFromExchange = false;
	TWeakObjectPtr<UGameInstance> DragGI;
	void Reset() { SourceGrid = nullptr; SourceIndex = INDEX_NONE; SourcePos = FIntPoint(-1,-1); Item = FYIBagItem(); bRemovedFromSource = false; bActive = false; bFromExchange = false; DragGI.Reset(); }
} GInventoryDrag;

static FYIItemInstanceNet MakeNetItem(const FYIItemInstance& Item)
{
	FYIItemInstanceNet Net;
	Net.Definition = Item.Definition;
	Net.Count = Item.Count;
	Net.CustomStackKey = Item.CustomStackKey;
	Net.bRotated = Item.bRotated;
	Net.Affixes = Item.Affixes;
	Net.Attributes.Reset();
	for (const TPair<FName, float>& KV : Item.Attributes)
	{
		FYIAttributeKV Entry;
		Entry.Name = KV.Key;
		Entry.Value = KV.Value;
		Net.Attributes.Add(Entry);
	}
	return Net;
}

static const UYIItemSFXLibrary* ResolveSFXLibrary(const UInventoryGridWidget* Grid)
{
	if (!Grid)
	{
		return nullptr;
	}
	if (Grid->ItemSFXLibrary.IsValid())
	{
		return Grid->ItemSFXLibrary.Get();
	}
	if (Grid->ItemSFXLibrary.ToSoftObjectPath().IsValid())
	{
		return Grid->ItemSFXLibrary.LoadSynchronous();
	}
	if (Grid->Bag)
	{
		if (const UYIInventoryComponent* OwnerComp = Grid->Bag->GetTypedOuter<UYIInventoryComponent>())
		{
			if (OwnerComp->ItemSFXLibrary.IsValid())
			{
				return OwnerComp->ItemSFXLibrary.Get();
			}
			if (OwnerComp->ItemSFXLibrary.ToSoftObjectPath().IsValid())
			{
				return OwnerComp->ItemSFXLibrary.LoadSynchronous();
			}
		}
	}
	return nullptr;
}

static UYIItemDefinition* ResolveItemDefForSFX(const FYIItemInstance& Item, bool bAllowLoad)
{
	UYIItemDefinition* Def = Item.Definition.Get();
	if (!Def && bAllowLoad && Item.Definition.ToSoftObjectPath().IsValid())
	{
		Def = Item.Definition.LoadSynchronous();
	}
	return Def;
}

static USoundBase* ResolveItemSoundForEvent(const UInventoryGridWidget* Grid, const FYIItemInstance& Item, EYIItemSFXEvent Event)
{
	const UYIItemSFXLibrary* Library = ResolveSFXLibrary(Grid);
	UYIItemDefinition* Def = ResolveItemDefForSFX(Item, Grid ? Grid->bLoadDefinitionForSFX : false);
	return UYIInventoryBlueprintLibrary::ResolveItemSFXSound(Def, Library, Event);
}

// Check if placing a footprint at Pos would overlap at most one other item (ignoring SourceIdx). Returns that item index or INDEX_NONE.
static bool FindSingleOverlap(const UYIInventoryBag* Bag, int32 SourceIdx, const FIntPoint& Pos, const FIntPoint& Footprint, int32& OutOverlapIdx)
{
	OutOverlapIdx = INDEX_NONE;
	if (!Bag) return false;
	UYIInventoryComponent* OwnerComp = Bag->GetTypedOuter<UYIInventoryComponent>();
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

void UInventoryGridWidget::OnWidgetRebuilt()
{
	Super::OnWidgetRebuilt();
	GRegisteredGrids.Add(this);
}

void UInventoryGridWidget::BeginDestroy()
{
	GRegisteredGrids.Remove(this);
	Super::BeginDestroy();
}

void UInventoryGridWidget::ForEachRegisteredGrid(TFunctionRef<void(UInventoryGridWidget*)> Callback)
{
	UGameInstance* TargetGI = GInventoryDrag.DragGI.Get();
	for (auto It = GRegisteredGrids.CreateIterator(); It; ++It)
	{
		if (UInventoryGridWidget* Grid = It->Get())
		{
			if (!TargetGI || (Grid->GetWorld() && Grid->GetWorld()->GetGameInstance() == TargetGI))
			{
				Callback(Grid);
			}
		}
	}
}

TSharedRef<SWidget> UInventoryGridWidget::RebuildWidget()
{
	// Hook Slate callbacks so Slate can notify the owning UWidget about hover/selection changes
	MySlateWidget = SNew(SInventoryGridWidget)
		.OwnerWidget(this)
		.Bag(Bag)
		.CellPixelSize(CellPixelSize)
		.bEnableCellHover(bEnableCellHover)
		.bEnableMouseSelection(bEnableMouseSelection)
		.OnHoveredItemChanged(SInventoryGridWidget::FOnHoveredItemChanged::CreateLambda([WeakThis = TWeakObjectPtr<UInventoryGridWidget>(this)](int32 Idx)
		{
			if (WeakThis.IsValid()) { WeakThis->HandleHoverChanged(Idx); }
		}))
		.OnHoveredCellChanged(SInventoryGridWidget::FOnHoveredCellChanged::CreateLambda([WeakThis = TWeakObjectPtr<UInventoryGridWidget>(this)](const FIntPoint& Cell)
		{
			if (WeakThis.IsValid()) { WeakThis->HandleHoverCellChanged(Cell); }
		}))
		.OnGhostPlacementChanged(SInventoryGridWidget::FOnGhostPlacementChanged::CreateLambda([WeakThis = TWeakObjectPtr<UInventoryGridWidget>(this)](const FIntPoint& Cell, bool bValid, bool bOutOfBounds)
		{
			if (WeakThis.IsValid()) { WeakThis->HandleGhostPlacementChanged(Cell, bValid, bOutOfBounds); }
		}))
		.OnSelectedCellChanged(SInventoryGridWidget::FOnSelectedCellChanged::CreateLambda([WeakThis = TWeakObjectPtr<UInventoryGridWidget>(this)](const FIntPoint& Cell)
		{
			if (WeakThis.IsValid()) { WeakThis->HandleSelectionChanged(Cell); }
		}))
		.OnCellClicked(SInventoryGridWidget::FOnCellClicked::CreateLambda([WeakThis = TWeakObjectPtr<UInventoryGridWidget>(this)](const FIntPoint& Cell)
		{
			if (WeakThis.IsValid()) { WeakThis->HandleCellClicked(Cell); }
		}));
	// Apply wrap/ghost settings
	if (MySlateWidget.IsValid()) { MySlateWidget->SetWrapNavigation(bWrapNavigation); MySlateWidget->SetUseGlobalDragGhost(bUseGlobalDragGhost); MySlateWidget->SetCellHoverEnabled(bEnableCellHover); MySlateWidget->SetMouseSelectionEnabled(bEnableMouseSelection); MySlateWidget->Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);}
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
		MySlateWidget->SetUseGlobalDragGhost(bUseGlobalDragGhost);
		MySlateWidget->SetCellHoverEnabled(bEnableCellHover);
		MySlateWidget->SetMouseSelectionEnabled(bEnableMouseSelection);
		MySlateWidget->Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);
		MySlateWidget->Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);
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

void UInventoryGridWidget::SetUseGlobalDragGhost(bool bEnable)
{
	bUseGlobalDragGhost = bEnable; 
	if (MySlateWidget.IsValid())
		MySlateWidget->Invalidate(EInvalidateWidgetReason::Paint);
}

void UInventoryGridWidget::SetEnableCellHover(bool bEnable)
{
	bEnableCellHover = bEnable;
	if (MySlateWidget.IsValid())
	{
		MySlateWidget->SetCellHoverEnabled(bEnableCellHover);
		MySlateWidget->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void UInventoryGridWidget::SetEnableMouseSelection(bool bEnable)
{
	bEnableMouseSelection = bEnable;
	if (MySlateWidget.IsValid())
	{
		MySlateWidget->SetMouseSelectionEnabled(bEnableMouseSelection);
		MySlateWidget->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void UInventoryGridWidget::SetTooltipRequirementContext(UAbilitySystemComponent* InASC, int32 InXP, const FGameplayTagContainer& InOwnedTags)
{
	RequirementAbilitySystem = InASC;
	RequirementXP = InXP;
	RequirementOwnedTags = InOwnedTags;
	UpdateBoundTooltip();
}

void UInventoryGridWidget::SetTooltipPreviewAttributes(const TMap<FName,float>& InAttributes)
{
	RequirementPreviewAttributes = InAttributes;
	UpdateBoundTooltip();
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

bool UInventoryGridWidget::GetSelectedCellTooltipData(FYITooltipData& OutData, const FYIRequirementContext& RequirementContext) const
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
			return UYIInventoryBlueprintLibrary::GetItemTooltipData(Bag, i, OutData, RequirementContext);
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
	// Bag changed (add/remove/etc.) — ensure selection/hover/tooltips stay correct
	if (MySlateWidget.IsValid())
	{ MySlateWidget->RefreshFromBag(); }
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
	const int32 PrevHovered = HoveredItemIndexCached;
	// Broadcast hover state to Blueprint listeners
	OnItemHoverChanged.Broadcast(HoveredIndex);
	const bool bHasItem = HoveredIndex != INDEX_NONE;
	OnHoverSlotChanged.Broadcast(HoveredIndex, bHasItem);
	HoveredItemIndexCached = HoveredIndex;

	if (PrevHovered != HoveredIndex)
	{
		if (IsHoverSoundEnabled() && bHasItem && Bag && Bag->Items.IsValidIndex(HoveredIndex))
		{
			if (USoundBase* ItemSound = ResolveItemSoundForEvent(this, Bag->Items[HoveredIndex].Item, EYIItemSFXEvent::HoverItem))
			{
				UGameplayStatics::PlaySound2D(this, ItemSound);
			}
			else if (HoverSlotSound)
			{
				UGameplayStatics::PlaySound2D(this, HoverSlotSound);
			}
		}
	}
	UpdateBoundTooltip();
	// Optionally refresh tooltip if there is no explicit selection but we want hover to drive tooltip (kept to selection-only for now)
}

void UInventoryGridWidget::HandleHoverCellChanged(const FIntPoint& NewCell)
{
	const FIntPoint PrevCell = HoveredCellCached;
	HoveredCellCached = NewCell;

	if (!bEnableCellHover || PrevCell == NewCell)
	{
		return;
	}

	if (GInventoryDrag.bActive)
	{
		return;
	}

	if (!Bag)
	{
		return;
	}

	const int32 HoverIdx = GetItemIndexAtCell(Bag, NewCell);
	// Only play empty-slot hover sounds when we are hovering an empty cell.
	if (IsHoverSoundEnabled() && HoverIdx == INDEX_NONE && HoverEmptySound)
	{
		UGameplayStatics::PlaySound2D(this, HoverEmptySound);
	}
}

void UInventoryGridWidget::HandleGhostPlacementChanged(const FIntPoint& TopLeftCell, bool bValid, bool bOutOfBounds)
{
	(void)TopLeftCell;
	// Only play ghost highlight SFX when dragging.
	if (!GInventoryDrag.bActive)
	{
		return;
	}
	if (!IsDragHoverSoundEnabled())
	{
		return;
	}
	if (bOutOfBounds && !bPlayDragHoverOutOfBounds)
	{
		return;
	}

	if (bValid)
	{
		if (USoundBase* ItemSound = ResolveItemSoundForEvent(this, GInventoryDrag.Item.Item, EYIItemSFXEvent::HoverItem))
		{
			UGameplayStatics::PlaySound2D(this, ItemSound);
		}
		else if (DragHoverSound)
		{
			UGameplayStatics::PlaySound2D(this, DragHoverSound);
		}
		else if (HoverEmptySound)
		{
			UGameplayStatics::PlaySound2D(this, HoverEmptySound);
		}
	}
	else
	{
		if (USoundBase* ItemSound = ResolveItemSoundForEvent(this, GInventoryDrag.Item.Item, EYIItemSFXEvent::InvalidMove))
		{
			UGameplayStatics::PlaySound2D(this, ItemSound);
		}
		else if (DragHoverInvalidSound)
		{
			UGameplayStatics::PlaySound2D(this, DragHoverInvalidSound);
		}
		else if (IsInvalidSoundEnabled() && InvalidMoveSound)
		{
			UGameplayStatics::PlaySound2D(this, InvalidMoveSound);
		}
	}
}

void UInventoryGridWidget::SetShopContext(UYIShopComponent* InShop, bool bStockGrid)
{
	ActiveShopComponent = InShop;
	bIsShopStockGrid = bStockGrid;
}

bool UInventoryGridWidget::IsInventorySoundEnabled() const
{
	if (!bEnableInventorySounds)
	{
		return false;
	}
	if (Bag)
	{
		if (const UYIInventoryComponent* OwnerComp = Bag->GetTypedOuter<UYIInventoryComponent>())
		{
			return OwnerComp->bEnableInventorySounds;
		}
	}
	return true;
}

bool UInventoryGridWidget::IsHoverSoundEnabled() const
{
	return IsInventorySoundEnabled() && bEnableHoverSounds;
}

bool UInventoryGridWidget::IsDragSoundEnabled() const
{
	return IsInventorySoundEnabled() && bEnableDragSounds;
}

bool UInventoryGridWidget::IsDragHoverSoundEnabled() const
{
	return IsInventorySoundEnabled() && bEnableDragHoverSounds;
}

bool UInventoryGridWidget::IsInvalidSoundEnabled() const
{
	return IsInventorySoundEnabled() && bEnableInvalidMoveSounds;
}

bool UInventoryGridWidget::BeginDragFromCell(FIntPoint Cell)
{
	if (!Bag) return false;
	int32 Idx = GetItemIndexAtCell(Bag, Cell);
	if (Idx == INDEX_NONE) return false;
	// Only allow drag within this game instance (prevents cross-PIE bleed)
	if (UWorld* World = GetWorld())
	{
		GInventoryDrag.DragGI = World->GetGameInstance();
	}
	GInventoryDrag.SourceGrid = this;
	GInventoryDrag.SourceIndex = Idx;
	GInventoryDrag.Item = Bag->Items[Idx];
	GInventoryDrag.SourcePos = GInventoryDrag.Item.Pos;
	GInventoryDrag.bRemovedFromSource = false; // keep item in bag until drop is confirmed (prevents other clients seeing it vanish mid-drag)
	GInventoryDrag.bActive = true;
	GInventoryDrag.bFromExchange = false;
	// Notify listeners
	OnItemDragStarted.Broadcast(this, Idx);
	if (IsDragSoundEnabled())
	{
		if (USoundBase* ItemSound = ResolveItemSoundForEvent(this, GInventoryDrag.Item.Item, EYIItemSFXEvent::DragStart))
		{
			UGameplayStatics::PlaySound2D(this, ItemSound);
		}
		else if (DragStartSound)
		{
			UGameplayStatics::PlaySound2D(this, DragStartSound);
		}
	}
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
	if (UWorld* World = GetWorld())
	{
		if (!GInventoryDrag.DragGI.IsValid() || GInventoryDrag.DragGI.Get() != World->GetGameInstance())
		{
			// Drag from another PIE instance; ignore
			return false;
		}
	}
	UYIInventoryComponent* OwnerComp = Bag->GetTypedOuter<UYIInventoryComponent>();
	auto PlayDropSound = [this]()
	{
		if (!IsDragSoundEnabled())
		{
			return;
		}
		if (USoundBase* ItemSound = ResolveItemSoundForEvent(this, GInventoryDrag.Item.Item, EYIItemSFXEvent::Drop))
		{
			UGameplayStatics::PlaySound2D(this, ItemSound);
		}
		else if (DropSound)
		{
			UGameplayStatics::PlaySound2D(this, DropSound);
		}
	};
	auto PlayInvalidMoveSound = [this]()
	{
		if (!IsInvalidSoundEnabled())
		{
			return;
		}
		if (USoundBase* ItemSound = ResolveItemSoundForEvent(this, GInventoryDrag.Item.Item, EYIItemSFXEvent::InvalidMove))
		{
			UGameplayStatics::PlaySound2D(this, ItemSound);
		}
		else if (InvalidMoveSound)
		{
			UGameplayStatics::PlaySound2D(this, InvalidMoveSound);
		}
	};

	// Same bag: if this drag originated here and we removed from source, we are placing an unattached item now
	if (GInventoryDrag.SourceGrid == this)
	{
		if (!bAllowSelfMove)
		{
			OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
			PlayInvalidMoveSound();
			return false;
		}
		// When pickup removed the item, SourceIndex is INDEX_NONE and the bag no longer contains it. Treat as add-at-cell or swap.
		if (GInventoryDrag.bRemovedFromSource && GInventoryDrag.SourceIndex == INDEX_NONE)
		{
			FYIBagItem ToPlace = GInventoryDrag.Item; ToPlace.Pos = Cell;
			// Enforce exact placement at the highlighted cell; do not allow AddBagItem to relocate to first-fit
			if (Bag->CanPlaceAt(Cell, ToPlace.Size))
			{
				// Temporarily disable auto-merge to enforce exact placement at target cell
				bool bSavedAutoMerge = Bag->bAutoMergeOnAdd; Bag->bAutoMergeOnAdd = false;
				int32 NewIdx = OwnerComp ? OwnerComp->AddBagItem(ToPlace) : Bag->AddBagItem(ToPlace);
				Bag->bAutoMergeOnAdd = bSavedAutoMerge;
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
				PlayInvalidMoveSound();
				return false;
			}
			// Displace victim: remove it from the bag and continue dragging it (no swap/backfill)
			FYIBagItem SavedVictim = Bag->Items[Victim];
			if (!Bag->RemoveItem(Victim))
			{
				OnItemDropped.Broadcast(this, INDEX_NONE, Cell, false);
				PlayInvalidMoveSound();
				return false;
			}
			// Place dragged item at Cell (exact)
			int32 NewIdx = INDEX_NONE;
			if (Bag->CanPlaceAt(Cell, ToPlace.Size))
			{
				NewIdx = OwnerComp ? OwnerComp->AddBagItem(ToPlace) : Bag->AddBagItem(ToPlace);
			}
			if (NewIdx == INDEX_NONE)
			{
				// Rollback: reinsert victim to its original spot
				SavedVictim.Pos = SavedVictim.Pos; // unchanged
				if (OwnerComp) OwnerComp->AddBagItem(SavedVictim); else Bag->AddBagItem(SavedVictim);
				OnItemDropped.Broadcast(this, INDEX_NONE, Cell, false);
				PlayInvalidMoveSound();
				return false;
			}
			OnItemDropped.Broadcast(this, INDEX_NONE, Cell, true);
			// Continue dragging the displaced item (victim) as UNATTACHED (no lingering visual)
			GInventoryDrag.SourceGrid = this;
			GInventoryDrag.SourceIndex = INDEX_NONE;
			GInventoryDrag.Item = SavedVictim;
			GInventoryDrag.bRemovedFromSource = true;
			GInventoryDrag.bFromExchange = true;
			GInventoryDrag.SourcePos = SavedVictim.Pos;
			GInventoryDrag.bActive = true;
			OnItemDragStarted.Broadcast(this, INDEX_NONE);
			UpdateBoundTooltip();
			return true;
		}
		if (!(OwnerComp ? OwnerComp->MoveItem(GInventoryDrag.SourceIndex, Cell) : Bag->MoveItem(GInventoryDrag.SourceIndex, Cell)))
		{
			// Non-authority clients should not attempt in-place swaps; let the server resolve.
			if (OwnerComp && OwnerComp->GetOwner() && !OwnerComp->GetOwner()->HasAuthority())
			{
				OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
				PlayInvalidMoveSound();
				return false;
			}
			// Allow displacing a single overlapped item if the footprint only hits that one
			const FYIBagItem& Src = Bag->Items[GInventoryDrag.SourceIndex];
			const FIntPoint Foot = Bag->GetEffectiveSize(Src.Size);
			int32 Victim = INDEX_NONE;
			if (!FindSingleOverlap(Bag, GInventoryDrag.SourceIndex, Cell, Foot, Victim) || Victim == INDEX_NONE)
			{
				OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
				PlayInvalidMoveSound();
				return false;
			}
			// Remove victim, adjust source index if needed, then attempt move again
			FYIBagItem SavedVictim = Bag->Items[Victim];
			if (OwnerComp) OwnerComp->RemoveItem(Victim); else Bag->RemoveItem(Victim);
			if (Victim < GInventoryDrag.SourceIndex)
			{
				GInventoryDrag.SourceIndex -= 1;
			}
			if (!(OwnerComp ? OwnerComp->MoveItem(GInventoryDrag.SourceIndex, Cell) : Bag->MoveItem(GInventoryDrag.SourceIndex, Cell)))
			{
			// Failed even after clearing victim; restore it in-place to avoid merge/stack side-effects
			Bag->Items.Insert(SavedVictim, Victim);
			Bag->MarkPackageDirty(); Bag->OnChanged.Broadcast();
				OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
				PlayInvalidMoveSound();
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
		PlayDropSound();
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
	UYIInventoryComponent* SourceComp = (GInventoryDrag.SourceGrid && GInventoryDrag.SourceGrid->Bag) ? GInventoryDrag.SourceGrid->Bag->GetTypedOuter<UYIInventoryComponent>() : nullptr;

	// If this grid participates in a shop session, route drag from shop stock to buyer inventory via shop RPCs.
	if (ActiveShopComponent && GInventoryDrag.SourceGrid)
	{
		const UInventoryGridWidget* SourceGrid = GInventoryDrag.SourceGrid;
		if (SourceGrid->ActiveShopComponent == ActiveShopComponent)
		{
			// Only allow drag FROM shop stock INTO player bag.
			if (SourceGrid->bIsShopStockGrid && !bIsShopStockGrid && GInventoryDrag.SourceIndex != INDEX_NONE)
			{
				APlayerController* PC = GetOwningPlayer();
				if (!PC && GetWorld())
				{
					PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
				}
				if (PC)
				{
					if (UYITradeInteractionComponent* TradeComp = PC->FindComponentByClass<UYITradeInteractionComponent>())
					{
						const int32 BuyCount = FMath::Max(1, GInventoryDrag.Item.Item.Count);
						TradeComp->RequestShopBuy(ActiveShopComponent, GInventoryDrag.SourceIndex, BuyCount, OwnerComp, Cell);
						OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, true);
						PlayDropSound();
						GInventoryDrag.Reset();
						RefreshBoundTooltip();
						return true;
					}
				}
				OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
				PlayInvalidMoveSound();
				return false;
			}
			// Allow drag FROM player bag INTO shop stock (sell).
			if (!SourceGrid->bIsShopStockGrid && bIsShopStockGrid && GInventoryDrag.SourceIndex != INDEX_NONE)
			{
				APlayerController* PC = GetOwningPlayer();
				if (!PC && GetWorld())
				{
					PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
				}
				if (PC)
				{
					if (UYITradeInteractionComponent* TradeComp = PC->FindComponentByClass<UYITradeInteractionComponent>())
					{
						UYIInventoryComponent* ShopSourceComp = (GInventoryDrag.SourceGrid && GInventoryDrag.SourceGrid->Bag)
							? GInventoryDrag.SourceGrid->Bag->GetTypedOuter<UYIInventoryComponent>() : nullptr;
						const int32 SellCount = FMath::Max(1, GInventoryDrag.Item.Item.Count);
						TradeComp->RequestShopSell(ActiveShopComponent, GInventoryDrag.SourceIndex, SellCount, ShopSourceComp);
						OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, true);
						PlayDropSound();
						GInventoryDrag.Reset();
						RefreshBoundTooltip();
						return true;
					}
				}
				OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
				PlayInvalidMoveSound();
				return false;
			}
			// Block moving items into or within the shop stock grid.
			OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
			PlayInvalidMoveSound();
			return false;
		}
	}

	// If this grid participates in a trade session, route cross-bag transfer through the session (server authoritative).
	if (ActiveTradeSession && GInventoryDrag.SourceGrid)
	{
		const UInventoryGridWidget* SourceGrid = GInventoryDrag.SourceGrid;
		if (SourceGrid->ActiveTradeSession == ActiveTradeSession && SourceGrid->bHasTradeSide && bHasTradeSide && GInventoryDrag.SourceIndex != INDEX_NONE)
		{
			if (OwnerComp && OwnerComp->GetOwner() && OwnerComp->GetOwner()->HasAuthority())
			{
				ActiveTradeSession->ServerTransferItemBetweenSides(SourceGrid->TradeSide, TradeSide, GInventoryDrag.SourceIndex, Cell, 0);
				OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, true);
				PlayDropSound();
				GInventoryDrag.Reset();
				RefreshBoundTooltip();
				return true;
			}
			APlayerController* PC = GetOwningPlayer();
			if (!PC && GetWorld())
			{
				PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
			}
			if (PC)
			{
				if (UYITradeInteractionComponent* TradeComp = PC->FindComponentByClass<UYITradeInteractionComponent>())
				{
					TradeComp->RequestTradeTransfer(SourceGrid->TradeSide, TradeSide, GInventoryDrag.SourceIndex, Cell, 0);
					OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, true);
					PlayDropSound();
					GInventoryDrag.Reset();
					RefreshBoundTooltip();
					return true;
				}
			}
			OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
			PlayInvalidMoveSound();
			return false;
		}
	}
	
	// If we are not authoritative, do not mutate bags directly for cross-bag operations.
	if (OwnerComp && OwnerComp->GetOwner() && !OwnerComp->GetOwner()->HasAuthority())
	{
		OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
		PlayInvalidMoveSound();
		return false;
	}
	
	// Check if placement at Cell is possible or if we need a victim swap
	int32 VictimIdx = INDEX_NONE;
	const FIntPoint Foot = Bag->GetEffectiveSize(ToPlace.Size);
	
	// First try to place without a victim
	if (Bag->CanPlaceAt(Cell, ToPlace.Size))
	{
		// Clean placement: just add and remove from source
		// Temporarily disable auto-merge to avoid merging dragged item into existing stacks during cross-bag direct placement
		int32 NewIdx; { bool bSavedAutoMerge = Bag->bAutoMergeOnAdd; Bag->bAutoMergeOnAdd = false; NewIdx = OwnerComp ? OwnerComp->AddBagItem(ToPlace) : Bag->AddBagItem(ToPlace); Bag->bAutoMergeOnAdd = bSavedAutoMerge; }
		if (NewIdx != INDEX_NONE)
		{
			if (GInventoryDrag.SourceGrid && GInventoryDrag.SourceGrid->Bag)
{
// If item was already removed at pickup, skip removing now
				if (GInventoryDrag.bRemovedFromSource || GInventoryDrag.SourceIndex == INDEX_NONE || (SourceComp ? SourceComp->RemoveItem(GInventoryDrag.SourceIndex) : GInventoryDrag.SourceGrid->Bag->RemoveItem(GInventoryDrag.SourceIndex)))
				{
					OnItemTransferred.Broadcast(GInventoryDrag.SourceGrid, GInventoryDrag.SourceIndex, NewIdx);
					if (GInventoryDrag.SourceGrid != this) GInventoryDrag.SourceGrid->OnItemTransferred.Broadcast(GInventoryDrag.SourceGrid, GInventoryDrag.SourceIndex, NewIdx);
					GInventoryDrag.SourceGrid->RefreshBoundTooltip();
					RefreshBoundTooltip();
					OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, true);
					PlayDropSound();
					GInventoryDrag.Reset();
					return true;
				}
				else
				{
					// Remove from source failed; undo the add
					Bag->RemoveItem(NewIdx);
					OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
					PlayInvalidMoveSound();
					GInventoryDrag.Reset();
					return false;
				}
			}
			else
			{
				// Unattached drag (picked up from world) â€” accept the placement
				UE_LOG(LogTemp, Warning, TEXT("Inventory: Placed unattached dragged item into bag at index %d."), NewIdx);
				RefreshBoundTooltip();
				OnItemDropped.Broadcast(this, INDEX_NONE, Cell, true);
				PlayDropSound();
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
		PlayInvalidMoveSound();
		return false;
	}

	// Non-authority clients should not perform in-place swaps; trigger server ops instead.
	if (OwnerComp && OwnerComp->GetOwner() && !OwnerComp->GetOwner()->HasAuthority())
	{
		// Try removing victim then adding dragged via RPC
		UYIInventoryComponent* SrcCompInner = SourceComp;
		if (SrcCompInner) SrcCompInner->RemoveItem(VictimIdx); // victim in dest bag index; safe because server will validate
		OwnerComp->AddBagItem(ToPlace);
		OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, true);
		PlayDropSound();
		GInventoryDrag.Reset();
		return true;
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
				UYIInventoryComponent* SrcCompInner = SourceBag->GetTypedOuter<UYIInventoryComponent>();
				const int32 SourceIdx = GInventoryDrag.SourceIndex;
				// Skip removal if the item was already removed at pickup or SourceIdx is invalid
				if (!GInventoryDrag.bRemovedFromSource && SourceIdx != INDEX_NONE)
				{
					// Only remove if source is not the same as destination victim index
					if (!(SourceBag == Bag && SourceIdx == VictimIdx))
					{
						UE_LOG(LogTemp, Warning, TEXT("Inventory Swap: Removing original dragged item from source bag. SourceIdx=%d SourceBagCountBefore=%d"), SourceIdx, SourceBag->Items.Num());
						const bool bRemoved = SrcCompInner ? SrcCompInner->RemoveItem(SourceIdx) : SourceBag->RemoveItem(SourceIdx);
						if (!bRemoved)
						{
							// Source removal failed; FULLY REVERT the destination change
					Bag->Items[VictimIdx] = SavedVictim;
					Bag->MarkPackageDirty();
					Bag->OnChanged.Broadcast();
					UE_LOG(LogTemp, Error, TEXT("Inventory Swap: Failed to remove original dragged item from source. Reverting. DestCount=%d"), Bag->Items.Num());
					OnItemDropped.Broadcast(this, SourceIdx, Cell, false);
					PlayInvalidMoveSound();
					GInventoryDrag.Reset();
					return false;
				}
				UE_LOG(LogTemp, Warning, TEXT("Inventory Swap: Removed original dragged item from source. SourceBagCountAfter=%d"), SourceBag->Items.Num());
			}
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
	PlayDropSound();
	UE_LOG(LogTemp, Warning, TEXT("Inventory Swap COMPLETE: NewDragItem=%s NewDragActive=%d DestCount=%d"), *GetDefName(SavedVictim), (int)GInventoryDrag.bActive, Bag->Items.Num());

	// Update drag state: the victim is now at VictimIdx in this bag (linked, not unattached)
	// If BeginDragFromCell is called again, it will just pick up the victim from its new location
	GInventoryDrag.SourceGrid = this;
	GInventoryDrag.SourceIndex = INDEX_NONE;
	GInventoryDrag.Item = SavedVictim;
	GInventoryDrag.bRemovedFromSource = true; // victim no longer exists in any bag after being displaced
	GInventoryDrag.bFromExchange = true;
	GInventoryDrag.SourcePos = SavedVictim.Pos;
	GInventoryDrag.bActive = true;
	OnItemDragStarted.Broadcast(this, INDEX_NONE);
	return true;
}

void UInventoryGridWidget::CancelDrag()
{
	if (GInventoryDrag.bActive)
	{
		if (IsDragSoundEnabled())
		{
			if (USoundBase* ItemSound = ResolveItemSoundForEvent(this, GInventoryDrag.Item.Item, EYIItemSFXEvent::Cancel))
			{
				UGameplayStatics::PlaySound2D(this, ItemSound);
			}
			else if (CancelDragSound)
			{
				UGameplayStatics::PlaySound2D(this, CancelDragSound);
			}
		}

		bool bDroppedToWorld = false;
		// If we removed the item from its source bag at pickup, try to restore it at its original position
		if (GInventoryDrag.bRemovedFromSource && GInventoryDrag.SourceGrid && GInventoryDrag.SourceGrid->Bag)
		{
			UYIInventoryBag* SrcBag = GInventoryDrag.SourceGrid->Bag;
			FYIBagItem Restore = GInventoryDrag.Item;

			if (GInventoryDrag.bFromExchange)
			{
				// Exchange drag: try original victim cell, then any fit, else drop to world
				bool bPlaced = false;
				if (SrcBag->CanPlaceAt(GInventoryDrag.SourcePos, Restore.Size))
				{
					Restore.Pos = GInventoryDrag.SourcePos;
					bPlaced = (SrcBag->AddBagItem(Restore) != INDEX_NONE);
				}
				if (!bPlaced)
				{
					FIntPoint FitPos;
					if (SrcBag->FindFirstFit(Restore.Size, FitPos))
					{
						Restore.Pos = FitPos;
						bPlaced = (SrcBag->AddBagItem(Restore) != INDEX_NONE);
					}
				}
				if (!bPlaced)
				{
					if (UYIInventoryComponent* OwnerComp = SrcBag->GetTypedOuter<UYIInventoryComponent>())
					{
						if (AActor* Owner = OwnerComp->GetOwner())
						{
							const FVector SpawnLoc = Owner->GetActorLocation() + Owner->GetActorForwardVector() * 80.f;
							const FTransform SpawnTransform(Owner->GetActorRotation(), SpawnLoc);
							const FYIItemInstanceNet NetItem = MakeNetItem(Restore.Item);
							if (OwnerComp->DropItemToWorld(NetItem, SpawnTransform))
							{
								bDroppedToWorld = true;
							}
						}
					}
				}
			}
			else
			{
				Restore.Pos = GInventoryDrag.SourcePos;
				// Try to add back; if it fails (blocked now), try to add anywhere to avoid loss
				if (SrcBag->AddBagItem(Restore) == INDEX_NONE)
				{
					FIntPoint FitPos;
					if (SrcBag->FindFirstFit(Restore.Size, FitPos))
					{
						Restore.Pos = FitPos;
						SrcBag->AddBagItem(Restore);
					}
				}
			}
		}
		OnItemDragCancelled.Broadcast(GInventoryDrag.SourceGrid, GInventoryDrag.Item.Item, bDroppedToWorld);
		GInventoryDrag.Reset();
	}
}

bool UInventoryGridWidget::IsItemDragActive(const UWorld* ContextWorld)
{
	if (!GInventoryDrag.bActive)
	{
		return false;
	}
	if (ContextWorld && GInventoryDrag.DragGI.IsValid() && ContextWorld->GetGameInstance() != GInventoryDrag.DragGI.Get())
	{
		return false;
	}
	return true;
}

bool UInventoryGridWidget::GetActiveDraggedItem(FYIBagItem& OutItem, UYIInventoryBag*& OutSourceBag, const UWorld* ContextWorld)
{
	if (!GInventoryDrag.bActive) { OutSourceBag = nullptr; return false; }
	if (ContextWorld && GInventoryDrag.DragGI.IsValid() && ContextWorld->GetGameInstance() != GInventoryDrag.DragGI.Get())
	{
		OutSourceBag = nullptr;
		return false;
	}
	OutItem = GInventoryDrag.Item;
	OutSourceBag = GInventoryDrag.SourceGrid ? GInventoryDrag.SourceGrid->Bag : nullptr;
	return true;
}

void UInventoryGridWidget::SetTradeContext(AYITradeSessionActor* InSession, ETradeSide InSide)
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
	FYITooltipData Data;
	FYIRequirementContext Ctx;
	if (RequirementAbilitySystem.IsValid()) { Ctx.AbilitySystem = RequirementAbilitySystem; }
	Ctx.OwnedTags = RequirementOwnedTags;
	Ctx.XP = RequirementXP;
	Ctx.PreviewAttributes = RequirementPreviewAttributes;

	bool bGot = GetSelectedCellTooltipData(Data, Ctx);
	if (!bGot && Bag && HoveredItemIndexCached != INDEX_NONE)
	{
		bGot = UYIInventoryBlueprintLibrary::GetItemTooltipData(Bag, HoveredItemIndexCached, Data, Ctx);
	}

	if (bGot)
	{
		if (BoundTooltipWidget)
		{
			if (UInventoryTooltipView* View = Cast<UInventoryTooltipView>(BoundTooltipWidget))
			{
				View->SetTooltipData(Data);
			}
			else if (UFunction* Fn = BoundTooltipWidget->FindFunction(TEXT("OnTooltipDataUpdated")))
			{
				BoundTooltipWidget->ProcessEvent(Fn, &Data);
			}
		}
		OnTooltipDataUpdated.Broadcast(Data);
	}
	else
	{
		// Clear tooltip by sending empty data
		if (BoundTooltipWidget)
		{
			if (UInventoryTooltipView* View = Cast<UInventoryTooltipView>(BoundTooltipWidget))
			{
				View->ClearTooltip();
			}
			else if (UFunction* Fn = BoundTooltipWidget->FindFunction(TEXT("OnTooltipCleared")))
			{
				BoundTooltipWidget->ProcessEvent(Fn, nullptr);
			}
		}
		OnTooltipCleared.Broadcast();
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

	// Trade session path: route transfer through server so both sides stay in sync.
	if (ActiveTradeSession && Other->ActiveTradeSession == ActiveTradeSession && bHasTradeSide && Other->bHasTradeSide)
	{
		const FIntPoint DestCell = (Other->SelectedCell.X >= 0 && Other->SelectedCell.Y >= 0) ? Other->SelectedCell : FIntPoint(0, 0);
		ActiveTradeSession->ServerTransferItemBetweenSides(TradeSide, Other->TradeSide, SourceIndex, DestCell, Count);
		UpdateBoundTooltip();
		Other->RefreshBoundTooltip();
		OnItemTransferred.Broadcast(this, SourceIndex, INDEX_NONE);
		Other->OnItemTransferred.Broadcast(this, SourceIndex, INDEX_NONE);
		return true;
	}

	// Non-trade: only allow direct transfer on authority.
	UYIInventoryComponent* OwnerComp = Bag->GetTypedOuter<UYIInventoryComponent>();
	if (OwnerComp && OwnerComp->GetOwner() && !OwnerComp->GetOwner()->HasAuthority())
	{
		return false;
	}

	const bool b = UYIInventoryBlueprintLibrary::TransferItemBetweenBags(Bag, Other->Bag, SourceIndex, Count, OutDestIndex);
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

void UInventoryGridWidget::SetBoundTooltipWidget(UUserWidget* Widget)
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

void UInventoryGridWidget::SetBag(UYIInventoryBag* InBag)
{
	Bag = InBag;
	// Rebind Slate to the new bag immediately
	if (MySlateWidget.IsValid())
	{
		MySlateWidget->SetBag(Bag);
		MySlateWidget->RefreshFromBag();
	}
	// Rebind bag-changed delegate like SynchronizeProperties
	if (CachedBag != Bag)
	{
		if (CachedBag)
		{
			if (BagChangedHandle.IsValid())
			{
				CachedBag->OnChanged.Remove(BagChangedHandle);
				BagChangedHandle = FDelegateHandle();
			}
		}
		if (Bag)
		{
			BagChangedHandle = Bag->OnChanged.AddLambda([this]() { OnBagChanged(); });
		}
		CachedBag = Bag;
	}
	// Ensure tooltip reflects current selection and new bag
	UpdateBoundTooltip();
}

