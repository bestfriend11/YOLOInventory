#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/IToolTip.h"

class UYIInventoryBag;

/**
 * SBagEditor
 *
 * Designer-facing Slate widget that renders an editable inventory bag grid inside the editor.
 * - Renders the grid, item thumbnails, stacks, selection, and hover outlines.
 * - Handles drag-and-drop from the palette, placing and stacking items.
 * - Exposes a minimal API for other editor panels (Info tab) to introspect state (hover/selection/preview cell).
 *
 * Notes for designers:
 * - Hover tooltips are shown only when the grid has a valid hovered index and the bag's bShowCellTooltips is enabled.
 * - Dragging with Alt held will force a new stack (useful for testing split behavior).
 */
class YOLOINVENTORYEDITOR_API SBagEditor : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_OneParam(FOnSelectionChanged, int32);

	SLATE_BEGIN_ARGS(SBagEditor) {}
		/** The inventory bag asset this widget edits. */
		SLATE_ARGUMENT(UYIInventoryBag*, Bag)
		/** Fired when the selected item index changes. */
		SLATE_EVENT(FOnSelectionChanged, OnSelectionChanged)
	SLATE_END_ARGS()

	/** Construct the widget with the given bag. */
	void Construct(const FArguments& InArgs);
	~SBagEditor();

	// ------------------------------------------------------------------
	// State accessors for designer-facing panels (Info Tab, automation)
	// ------------------------------------------------------------------

	/** Returns the currently previewed cell while hovering or dragging. Useful for the Info tab. */
	FIntPoint GetPreviewCell() const { return PreviewCell; }

	/** Returns the index of the currently hovered item in Bag->Items, or INDEX_NONE. */
	int32 GetHoveredIndex() const { return HoveredIndex; }

	/** Returns the index of the currently selected item in Bag->Items, or INDEX_NONE. */
	int32 GetSelectedIndex() const { return SelectedIndex; }

	/** Returns the size of a single cell in pixels (from the bag when available). */
	float GetCellPixelSize() const { return Bag.IsValid() ? Bag->CellPixelSize : CellSize.X; }

	/** Set the cell pixel size (updates the bag and refreshes the widget). */
	void SetCellPixelSize(float V) { if (Bag.IsValid()) { Bag->CellPixelSize = V; CellSize = FVector2D(V, V); Invalidate(EInvalidateWidgetReason::PaintAndVolatility); } }

	/** Get the current minify scale (a visual compacting factor). */
	float GetMinifyScale() const { return Bag.IsValid() ? Bag->MinifyScale : 1.0f; }

	/** Apply a minify scale; some items may be dropped when shrinking — the bag's ApplyMinifyScale reports dropped items. */
	void SetMinifyScale(float V) { if (Bag.IsValid()) { TArray<FYIBagItem> Dropped; Bag->ApplyMinifyScale(V, Dropped); } }

	/** Available sort options presented in the editor Info tab. */
	const TArray<TSharedPtr<FString>>& GetSortOptions() const { return SortOptions; }

	/** Currently selected sort option text (or invalid). */
	TSharedPtr<FString> GetSelectedSort() const { return SelectedSort; }

	/** Set the selected sort option and trigger a stable sort+pack on the bag. */
	void SetSelectedSort(TSharedPtr<FString> In) { SelectedSort = In; RequestSort(); }

	/** Whether sorting is ascending or descending. */
	bool IsSortAscending() const { return bSortAscending; }

	/** Toggle sort direction and re-run sort/pack. */
	void ToggleSortAscending() { bSortAscending = !bSortAscending; RequestSort(); }

private:
	// Underlying bag asset (may be invalid during teardown). Use weak pointer to avoid dangling refs.
	TWeakObjectPtr<UYIInventoryBag> Bag;
	FOnSelectionChanged OnSelectionChanged;

	// Current visual cell size used for painting. Initialized from Bag->CellPixelSize at Construct.
	FVector2D CellSize = FVector2D(32.f, 32.f);

	// Selection / dragging state
	int32 SelectedIndex = INDEX_NONE;
	int32 DraggingIndex = INDEX_NONE;
	int32 HoveredIndex = INDEX_NONE;
	FVector2D DragOffset = FVector2D::ZeroVector;
	FIntPoint DragStartPos = FIntPoint::ZeroValue;
	int32 HitIndexAtRelease(const FIntPoint& Cell) const;

	// ------------------------------------------------------------------
	// Transactional & preview state (internal)
	// ------------------------------------------------------------------

	// Dynamic tooltip used for hover; created at Construct and invoked/hidden by UpdateTooltipVisibility.
	TSharedPtr<IToolTip> DynamicToolTip;

	/** Clear hover state when the mouse leaves the widget (hides tooltip and refreshes visuals). */
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override;

	/** Update whether the dynamic tooltip should be shown (based on HoveredIndex and bag flags). */
	void UpdateTooltipVisibility();

	// Transaction tracking for click-to-move style interactions
	bool bTransactionActive = false;
	int32 PickedIndex = INDEX_NONE;
	FIntPoint PickedOriginalPos = FIntPoint::ZeroValue;
	int32 DisplacedIndex = INDEX_NONE;
	FIntPoint DisplacedOriginalPos = FIntPoint::ZeroValue;

	// Handle used to listen for bag asset changes; keep to unregister at teardown.
	FDelegateHandle BagChangedHandle;

	// Sorting UI state (designer friendly defaults shown in Info tab)
	TArray<TSharedPtr<FString>> SortOptions { MakeShared<FString>(TEXT("Rarity")), MakeShared<FString>(TEXT("Size")), MakeShared<FString>(TEXT("Stack")), MakeShared<FString>(TEXT("Price")) };
	TSharedPtr<FString> SelectedSort;
	bool bSortAscending = true;

	/** Request an editor-side sort/pack operation using the currently selected sort option. */
	void RequestSort();

	// Preview when dragging an asset from the palette. Marked mutable so const painting/accessors can inspect it.
	mutable bool bPreviewActive = false;
	mutable bool bPreviewOk = false; // Whether the previewed placement is valid
	mutable FIntPoint PreviewCell = FIntPoint::ZeroValue;
	mutable FIntPoint PreviewSize = FIntPoint(1,1);

	// Input / painting overrides (primary editor interactions)
	FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override { bPreviewActive = false; UpdateTooltipVisibility(); }
	virtual FReply OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override;
	int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override; // implemented in cpp

	// Helpers
	FIntPoint ToCell(const FVector2D& LocalPos) const;
	FVector2D ToPixel(const FIntPoint& Cell) const { return FVector2D(Cell)*CellSize; }

	/** Show a context menu for an item index (or invalid index to show generic bag actions). */
	void ShowItemContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, int32 HitIndex);
};
