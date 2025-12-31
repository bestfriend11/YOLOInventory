#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UYIInventoryBag;
class UInventoryGridWidget;
class YOLOINVENTORY_API SInventoryGridWidget : public SCompoundWidget
{
public:
	// Callbacks used by owning UInventoryGridWidget to react to hover/selection changes
	DECLARE_DELEGATE_OneParam(FOnHoveredItemChanged, int32 /*HoveredItemIndex*/);
	DECLARE_DELEGATE_OneParam(FOnSelectedCellChanged, const FIntPoint& /*SelectedCell*/);
	DECLARE_DELEGATE_OneParam(FOnCellClicked, const FIntPoint& /*ClickedCell*/);

	SLATE_BEGIN_ARGS(SInventoryGridWidget){}
		SLATE_ARGUMENT(UInventoryGridWidget*, OwnerWidget)
		SLATE_ARGUMENT(UYIInventoryBag*, Bag)
		SLATE_ARGUMENT(float, CellPixelSize)
		SLATE_ARGUMENT(bool, bWholeItemHover)
		SLATE_ARGUMENT(bool, bWholeItemSelection)
		SLATE_ARGUMENT(bool, bWrapNavigation)
		SLATE_EVENT(FOnHoveredItemChanged, OnHoveredItemChanged)
		SLATE_EVENT(FOnSelectedCellChanged, OnSelectedCellChanged)
		// Called when a cell is explicitly clicked by the user (mouse button down)
		SLATE_EVENT(FOnCellClicked, OnCellClicked)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

public:
	// Click callback (set during Construct)
	FOnCellClicked OnCellClicked;

	void SetBag(UYIInventoryBag* InBag) { Bag = InBag; }
	void SetCellPixelSize(float InSize) { CellSize = FVector2D(InSize, InSize); }
	void SetWholeItemHover(bool bEnable) { bWholeItemHover = bEnable; }
	void SetWholeItemSelection(bool bEnable) { bWholeItemSelection = bEnable; }
	void SetWrapNavigation(bool bEnable) { bWrapNavigation = bEnable; }

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
	FIntPoint SelectedCell = FIntPoint(-1,-1);

	// Whole-item hover selection
	int32 HoveredItemIndex = INDEX_NONE;
	FIntPoint HoveredItemTopLeft = FIntPoint(-1,-1);
	FIntPoint HoveredItemSize = FIntPoint::ZeroValue;
	TArray<int32> CellToItemIndex; // size = GridSize.X * GridSize.Y, INDEX_NONE for empty

	// Helpers
	FVector2D ToPixel(const FIntPoint& Cell) const { return FVector2D(Cell)*CellSize; }
	FIntPoint ToCell(const FVector2D& LocalPos) const { return FIntPoint(FMath::FloorToInt(LocalPos.X/CellSize.X), FMath::FloorToInt(LocalPos.Y/CellSize.Y)); }
	void RebuildOccupancy();
	int32 GetItemIndexAtCell(const FIntPoint& Cell) const;
	void UpdateHoverSelection();

	// Behavior flags
	bool bWholeItemHover = true;
	bool bWholeItemSelection = true;
	bool bWrapNavigation = false;

	// Callbacks (set during Construct)
	FOnHoveredItemChanged OnHoveredItemChanged;
	FOnSelectedCellChanged OnSelectedCellChanged;

	// Ghost drag visual (click-to-drag without holding)
	mutable bool bGhostActive = false;
	mutable bool bGhostPlacementValid = false;
	mutable int32 GhostOverlapIndex = INDEX_NONE;
	mutable int32 GhostIgnoreIndex = INDEX_NONE;
	mutable FVector2D GhostSize = FVector2D(32.f, 32.f);
	mutable FVector2D GhostCursorLocal = FVector2D::ZeroVector;
	mutable FIntPoint GhostFootprint = FIntPoint::ZeroValue;
	mutable FIntPoint GhostTopLeft = FIntPoint::ZeroValue;
	mutable FSlateBrush GhostBrush;
	mutable bool bGhostHasIcon = false;

	void UpdateGhostPlacement(const FVector2D& LocalCursor);
	bool EvaluateGhostPlacement(const FIntPoint& TopLeft, int32& OutOverlapIdx) const;
};
