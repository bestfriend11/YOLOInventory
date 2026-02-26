#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UYIInventoryBag;
class UInventoryGridWidget;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UYIInventoryGridStyleAsset;
struct FYIGridStyleBrushSlot;
class YOLOINVENTORYGRID_API SInventoryGridWidget : public SCompoundWidget
{
public:
	// Callbacks used by owning UInventoryGridWidget to react to hover/selection changes
	DECLARE_DELEGATE_OneParam(FOnHoveredItemChanged, int32 /*HoveredItemIndex*/);
	DECLARE_DELEGATE_OneParam(FOnHoveredCellChanged, const FIntPoint& /*HoveredCell*/);
	DECLARE_DELEGATE_OneParam(FOnSelectedCellChanged, const FIntPoint& /*SelectedCell*/);
	DECLARE_DELEGATE_OneParam(FOnCellClicked, const FIntPoint& /*ClickedCell*/);
	DECLARE_DELEGATE_ThreeParams(FOnGhostPlacementChanged, const FIntPoint& /*TopLeftCell*/, bool /*bValid*/, bool /*bOutOfBounds*/);

	SLATE_BEGIN_ARGS(SInventoryGridWidget){}
		SLATE_ARGUMENT(UInventoryGridWidget*, OwnerWidget)
		SLATE_ARGUMENT(UYIInventoryBag*, Bag)
		SLATE_ARGUMENT(float, CellPixelSize)
		SLATE_ARGUMENT(bool, bWholeItemHover)
		SLATE_ARGUMENT(bool, bWholeItemSelection)
		SLATE_ARGUMENT(bool, bWrapNavigation)
		SLATE_ARGUMENT(bool, bEnableCellHover)
		SLATE_ARGUMENT(bool, bEnableMouseSelection)
		SLATE_EVENT(FOnHoveredItemChanged, OnHoveredItemChanged)
		SLATE_EVENT(FOnHoveredCellChanged, OnHoveredCellChanged)
		SLATE_EVENT(FOnSelectedCellChanged, OnSelectedCellChanged)
		SLATE_EVENT(FOnGhostPlacementChanged, OnGhostPlacementChanged)
		// Called when a cell is explicitly clicked by the user (mouse button down)
		SLATE_EVENT(FOnCellClicked, OnCellClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// SWidget
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

public:
	// Click callback (set during Construct)
	FOnCellClicked OnCellClicked;

	/**
	 * Assign a new bag and immediately rebuild internal occupancy / hover caches
	 * so hit-testing, hover and drag detection work right after the swap.
	 */
	void SetBag(UYIInventoryBag* InBag)
	{
		Bag = InBag;
		RebuildOccupancy();
		// Reset hover/selection so stale indices from the previous bag don't linger.
		HoverCell = FIntPoint(-1, -1);
		LastHoverCell = FIntPoint(-1, -1);
		LastGhostTopLeft = FIntPoint(-1, -1);
		bLastGhostValid = false;
		bLastGhostOutOfBounds = false;
		HoveredItemIndex = INDEX_NONE;
		HoveredItemTopLeft = FIntPoint(-1, -1);
		HoveredItemSize = FIntPoint::ZeroValue;
		SelectedCell = FIntPoint(-1, -1);
		UpdateHoverSelection();
	}
	void SetCellPixelSize(float InSize) { CellSize = FVector2D(InSize, InSize); }
	void SetWholeItemHover(bool bEnable) { bWholeItemHover = bEnable; }
	void SetWholeItemSelection(bool bEnable) { bWholeItemSelection = bEnable; }
	void SetWrapNavigation(bool bEnable) { bWrapNavigation = bEnable; }
	void SetUseGlobalDragGhost(bool bEnable) { bUseGlobalDragGhost = bEnable; }
	void SetCellHoverEnabled(bool bEnable) { bEnableCellHover = bEnable; if (!bEnable) { HoverCell = FIntPoint(-1,-1); UpdateHoverSelection(); } }
	void SetMouseSelectionEnabled(bool bEnable) { bEnableMouseSelection = bEnable; if (!bEnable) { SelectedCell = FIntPoint(-1,-1); } }
	/** Rebuild occupancy & invalidate layout; call after bag item mutations to refresh hover/drag hit tests. */
	void RefreshFromBag()
	{
		RebuildOccupancy();
		Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);
		UpdateHoverSelection();
	}

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// Provide preferred size to parent layout so SizeBox/Canvas allocate expected space
	virtual FVector2D ComputeDesiredSize(float) const override;

	void SetSelectedCell(const FIntPoint& Cell) { SelectedCell = Cell; }
	bool MoveSelection(const FIntPoint& Delta);
	FIntPoint GetSelectedCell() const { return SelectedCell; }

private:
	TWeakObjectPtr<UYIInventoryBag> Bag;
	TWeakObjectPtr<UInventoryGridWidget> OwnerWidget;
	FVector2D CellSize = FVector2D(32,32);

	// Cursor tracking
	mutable FIntPoint HoverCell = FIntPoint::ZeroValue;
	mutable FIntPoint LastHoverCell = FIntPoint(-1, -1);
	mutable FIntPoint LastGhostTopLeft = FIntPoint(-1, -1);
	mutable bool bLastGhostValid = false;
	mutable bool bLastGhostOutOfBounds = false;
	FIntPoint SelectedCell = FIntPoint(-1,-1);

	// Whole-item hover selection
	int32 HoveredItemIndex = INDEX_NONE;
	FIntPoint HoveredItemTopLeft = FIntPoint(-1,-1);
	FIntPoint HoveredItemSize = FIntPoint::ZeroValue;
	TArray<int32> CellToItemIndex; // size = GridSize.X * GridSize.Y, INDEX_NONE for empty

	// Helpers
	FVector2D ToPixel(const FIntPoint& Cell) const { return FVector2D(Cell)*CellSize; }
	FIntPoint ToCell(const FVector2D& LocalPos) const { return FIntPoint(FMath::FloorToInt(LocalPos.X/CellSize.X), FMath::FloorToInt(LocalPos.Y/CellSize.Y)); }
	FIntPoint ToCellLocal(const FVector2D& LocalPos, const FVector2D& LocalCell) const { return FIntPoint(FMath::FloorToInt(LocalPos.X/LocalCell.X), FMath::FloorToInt(LocalPos.Y/LocalCell.Y)); }
	void RebuildOccupancy();
	int32 GetItemIndexAtCell(const FIntPoint& Cell) const;
	void UpdateHoverSelection();

	// Behavior flags
	bool bWholeItemHover = true;
	bool bWholeItemSelection = true;
	bool bWrapNavigation = false;
	bool bUseGlobalDragGhost = false;
	bool bEnableCellHover = false;
	bool bEnableMouseSelection = false;

	// Callbacks (set during Construct)
	FOnHoveredItemChanged OnHoveredItemChanged;
	FOnHoveredCellChanged OnHoveredCellChanged;
	FOnSelectedCellChanged OnSelectedCellChanged;
	FOnGhostPlacementChanged OnGhostPlacementChanged;

	// Ghost drag visual (click-to-drag without holding)
	mutable bool bGhostActive = false;
	mutable bool bGhostPlacementValid = false;
	mutable bool bGhostOutOfBounds = false;
	mutable int32 GhostOverlapIndex = INDEX_NONE;
	mutable int32 GhostIgnoreIndex = INDEX_NONE;
	mutable FVector2D GhostSize = FVector2D(32.f, 32.f);
	mutable FVector2D GhostCursorLocal = FVector2D::ZeroVector;
	mutable FIntPoint GhostFootprint = FIntPoint::ZeroValue;
	mutable FIntPoint GhostTopLeft = FIntPoint::ZeroValue;
	mutable FSlateBrush GhostBrush;
	mutable bool bGhostHasIcon = false;
	mutable TMap<int32, TStrongObjectPtr<UMaterialInstanceDynamic>> StyleSlotMIDs;
	mutable TMap<int32, TWeakObjectPtr<UMaterialInterface>> StyleSlotSourceMaterials;
	mutable TMap<int32, FSlateBrush> StyleSlotBrushCache;

	void UpdateGhostPlacement(const FVector2D& LocalCursor, const FVector2D& LocalCell);
	bool EvaluateGhostPlacement(const FIntPoint& TopLeft, int32& OutOverlapIdx) const;
	const FSlateBrush* ResolveBrushForStyleSlot(
		const UYIInventoryGridStyleAsset* GridStyle,
		const FYIGridStyleBrushSlot& Slot,
		int32 SlotKey,
		float HoverAmount,
		float SelectedAmount,
		float InvalidAmount,
		float MarqueeAmount) const;
};
