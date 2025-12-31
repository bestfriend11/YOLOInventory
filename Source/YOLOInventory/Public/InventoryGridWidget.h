#pragma once
#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "InventoryGridWidget.generated.h"

class UYIInventoryBag;
class SInventoryGridWidget;

/**
 * UInventoryGridWidget
 *
 * Runtime UMG widget that visualizes a UYIInventoryBag as a selectable grid.
 * Designed for designers to be able to drive selection via mouse/gamepad and bind a tooltip widget.
 *
 * Designer tips:
 * - Bind a UInventoryTooltipWidget using SetBoundTooltipWidget to show item details for the selected cell.
 * - Use MoveSelection* helpers to drive focus with input mappings (gamepad/keyboard).
 */
UCLASS(meta=(DisplayName="YOLO Inventory Grid"))
class YOLOINVENTORY_API UInventoryGridWidget : public UWidget
{
	GENERATED_BODY()
public:
	/** The bag asset this widget displays (must be set for the widget to show items). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(ToolTip="The inventory bag asset to display"))
	UYIInventoryBag* Bag;

	/** Size of one grid cell in pixels. Designers can clamp this between 8 and 128 for readability. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(ClampMin="8.0", ClampMax="128.0", ToolTip="Pixel size of each grid cell"))
	float CellPixelSize = 32.f;

	/** Currently selected cell (runtime). Returns (-1,-1) if nothing selected. */
	UPROPERTY(BlueprintReadOnly, Category="Inventory", meta=(ToolTip="Currently selected cell in the grid"))
	FIntPoint SelectedCell = FIntPoint(-1,-1);

	/** Move the selection by a delta (e.g., Left = (-1,0)). Returns true if the selection moved. */
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Move selected cell by Delta (returns true if changed)"))
	bool MoveSelection(FIntPoint Delta);

	/** Returns the index of the item at the currently selected cell, or INDEX_NONE. */
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Get item index at selected cell or INDEX_NONE"))
	int32 GetSelectedItemIndex() const;

	// Convenience helpers for input (blueprint friendly)
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Move selection up"))
	bool MoveSelectionUp();
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Move selection down"))
	bool MoveSelectionDown();
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Move selection left"))
	bool MoveSelectionLeft();
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Move selection right"))
	bool MoveSelectionRight();

	/** Bind a runtime tooltip widget that will be populated as selection changes. */
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Attach a tooltip widget to display selected cell info"))
	void SetBoundTooltipWidget(class UInventoryTooltipWidget* Widget);

	/** Force the bound tooltip to refresh (useful after programmatic changes). */
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Refresh the currently bound tooltip widget"))
	void RefreshBoundTooltip();

	/** Action IDs used by action menus. Keep stable so designer authored menus and blueprints can rely on them. */
	static const int32 ACTION_USE = 0;
	static const int32 ACTION_ROTATE = 1;
	static const int32 ACTION_DROP = 2;
	static const int32 ACTION_COMBINE = 3;
	static const int32 ACTION_SELL = 4;
	static const int32 ACTION_TRANSFER = 5;
	static const int32 ACTION_INSPECT = 6;
	static const int32 ACTION_EQUIP = 7;
	static const int32 ACTION_GRAB = 8;

	/** Transfer the currently selected item stack to another grid's bag. Count==0 moves the whole stack; otherwise move at most Count. Returns true on success and OutDestIndex contains the dest slot index. */
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Transfer selected item to Other grid (returns true if transfer succeeded)"))
	bool TransferSelectedItemTo(UInventoryGridWidget* Other, int32 Count, int32& OutDestIndex);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventoryGridItemTransferred, UInventoryGridWidget*, SourceGrid, int32, SourceIndex, int32, DestIndex);
	/** Broadcast when this grid successfully transfers an item to another grid/bag. */
	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnInventoryGridItemTransferred OnItemTransferred;

	/** Ask the grid to compute available actions for the currently selected item (returns false if no selection). */
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Get available actions for the currently selected item"))
	bool GetAvailableActionsForSelectedItem(TArray<FText>& OutActions, TArray<int32>& OutActionIds) const;

	/** Request that the provided ActionMenu be opened for the current selection. Returns false if menu couldn't be opened (no selection or Menu==nullptr). */
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Populates and shows the provided ActionMenu for the current selection"))
	bool RequestOpenActionMenu(class UInventoryActionMenuWidget* Menu);

	/** Event fired when the grid wants an action menu opened for the current selection (provides the selected item index). UI owners can bind this to show their own menu or call RequestOpenActionMenu. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionMenuRequested, int32, SelectedItemIndex);
	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnActionMenuRequested OnActionMenuRequested;

	/** Optional action menu bound to this grid; if Set and bAutoOpenActionMenuOnSelect is true the menu will automatically open when a new selection occurs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(ToolTip="Optional action menu to auto-open on selection"))
	UInventoryActionMenuWidget* BoundActionMenu = nullptr;

	/** If true and BoundActionMenu is set, opening selection will automatically open the bound menu. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(ToolTip="Auto-open the bound action menu when selection changes"))
	bool bAutoOpenActionMenuOnSelect = false;

	/** Wrap navigation when moving past edges (left/right/top/bottom). When enabled, moving off one edge continues on the opposite side. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(ToolTip="Wrap navigation when moving past grid edges"))
	bool bWrapNavigation = false;

	/** Enable or disable wrap navigation at runtime. */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void SetWrapNavigation(bool bEnable);

	/** Explicitly set the currently selected cell. */
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Set the selected cell explicitly"))
	void SetSelectedCell(FIntPoint Cell);

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCellSelected, FIntPoint, Cell);
	/** Broadcast when a cell is selected (useful for other UI to react). */
	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnCellSelected OnCellSelected;

	// More detailed selection changed event with selected item index (INDEX_NONE if none)
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSelectionChanged, FIntPoint, NewCell, int32, NewItemIndex);
	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnSelectionChanged OnSelectionChanged;

	// Hover change event: broadcast with hovered item index (INDEX_NONE when hovering empty cell)
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemHoverChanged, int32, HoveredItemIndex);
	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnItemHoverChanged OnItemHoverChanged;
	// Drag & Drop events
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemDragStarted, UInventoryGridWidget*, SourceGrid, int32, SourceIndex);
	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnItemDragStarted OnItemDragStarted;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnItemDropped, UInventoryGridWidget*, DestGrid, int32, SourceIndex, FIntPoint, DestCell, bool, bSucceeded);
	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnItemDropped OnItemDropped;

	/** Start a drag using the item at Cell (returns false if no item at cell) */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool BeginDragFromCell(FIntPoint Cell);

	/** Begin drag from the currently selected cell */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool BeginDragFromSelectedCell();

	/** Attempt to drop currently dragged item at Cell. Returns true on success. */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool DropDraggedItemAtCell(FIntPoint Cell);

	/** Cancel any active drag operation */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void CancelDrag();
	/** Returns true if a global inventory drag is active. */
	static bool IsItemDragActive();
	/** Get the currently dragged item (if any) and the source bag for context; returns false if no drag active. */
	static bool GetActiveDraggedItem(struct FYIBagItem& OutItem, class UYIInventoryBag*& OutSourceBag);
	/** Fill OutData for the currently selected cell if an item exists there (returns true on success). */
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Get tooltip data for the currently selected cell"))
	bool GetSelectedCellTooltipData(struct FYITooltipData& OutData) const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;
	FVector2D GetDesiredSize() const;

	/** Release owned Slate resources and unregister any delegates to avoid leaking SWidgets on teardown. */
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;


private:
	TSharedPtr<SInventoryGridWidget> MySlateWidget;

	/** Optional tooltip widget to bind for runtime (set from BP/owner). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(AllowPrivateAccess="true", ToolTip="Optional runtime tooltip widget to display item details"))
	class UInventoryTooltipWidget* BoundTooltipWidget = nullptr;

	// Internal click handler bound from Slate
	void HandleCellClicked(const FIntPoint& Cell);

	// Cached bag change handle so we can update tooltip when items change
	FDelegateHandle BagChangedHandle;
	UYIInventoryBag* CachedBag = nullptr;

	// Helper to update the bound tooltip when selection changes
	void UpdateBoundTooltip();

	// Called when the bound Bag fires OnChanged
	void OnBagChanged();

	// Internal handlers invoked from Slate callbacks
	void HandleSelectionChanged(const FIntPoint& NewCell);
	void HandleHoverChanged(int32 HoveredIndex);
};
