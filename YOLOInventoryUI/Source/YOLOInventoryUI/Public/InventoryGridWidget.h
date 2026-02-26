#pragma once
#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "GameplayTagContainer.h"
#include "YIInventoryBag.h"
#include "InventoryGridWidget.generated.h"

class UYIInventoryBag;
class SInventoryGridWidget;
class UAbilitySystemComponent;
struct FYIRequirementContext;
class AYITradeSessionActor;
class USoundBase;
class UYIItemSFXLibrary;
class UYIEquipmentComponent;
enum class ETradeSide : uint8;
class UYIShopComponent;
class UYIInventoryComponent;
class UYIInventoryGridStyleAsset;

/**
 * UInventoryGridWidget
 *
 * Runtime UMG widget that visualizes a UYIInventoryBag as a selectable grid.
 * Designed for designers to be able to drive selection via mouse/gamepad and bind a tooltip widget.
 *
 * Designer tips:
 * - Bind a tooltip widget (e.g., UInventoryTooltipView or custom UUserWidget with OnTooltipDataUpdated/OnTooltipCleared) using SetBoundTooltipWidget to show item details for the selected cell.
 * - Use MoveSelection* helpers to drive focus with input mappings (gamepad/keyboard).
 */
UCLASS(meta=(DisplayName="YOLO Inventory Grid"))
class YOLOINVENTORYUI_API UInventoryGridWidget : public UWidget
{
	GENERATED_BODY()
public:
	/** The bag asset this widget displays (must be set for the widget to show items). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(ToolTip="The inventory bag asset to display"))
	UYIInventoryBag* Bag;

	/** Runtime owner inventory component used to resolve context/id/role bindings. Internal binding state (not designer-authored). */
	UPROPERTY(Transient)
	TObjectPtr<UYIInventoryComponent> BoundInventoryComponent = nullptr;

	/** Runtime bag id binding. Internal binding state set by SetBagBindingById at runtime. */
	FGuid BoundBagId;

	/** Optional role tag binding when BoundBagId is not set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Binding", meta=(EditCondition="!bBindToActiveBagContext", EditConditionHides, ToolTip="Designer-facing binding by bag role tag (used when not binding to active context)."))
	FGameplayTag BoundBagRoleTag;

	/** If true, this grid always resolves from inventory active bag contexts instead of a fixed role/id. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Binding", meta=(ToolTip="Bind this grid to the inventory component's active bag context map instead of a fixed bag role."))
	bool bBindToActiveBagContext = false;

	/** Optional semantic active-context tag. Empty = primary active bag (ActiveBagId); set to bind this grid to a named context bag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Binding", meta=(EditCondition="bBindToActiveBagContext", EditConditionHides, ToolTip="Context tag this grid listens to when active-context binding is enabled. Empty = primary active bag."))
	FGameplayTag BoundActiveContextTag;

	/** Set/replace the bag shown by this grid at runtime. Ensures Slate and delegates are rebound. */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void SetBag(class UYIInventoryBag* InBag);

	UFUNCTION(BlueprintCallable, Category="Inventory|Binding")
	void SetBagBindingById(UYIInventoryComponent* InInventoryComponent, const FGuid& InBagId);

	UFUNCTION(BlueprintCallable, Category="Inventory|Binding")
	void SetBagBindingByRole(UYIInventoryComponent* InInventoryComponent, FGameplayTag InBagRoleTag);

	UFUNCTION(BlueprintCallable, Category="Inventory|Binding")
	void SetBagBindingToActiveContext(UYIInventoryComponent* InInventoryComponent, FGameplayTag InContextTag);

	UFUNCTION(BlueprintCallable, Category="Inventory|Binding")
	void ClearBagBinding();

	/** Reuse the grid's existing binding configuration (active-context/id/role) but swap the inventory component source. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Binding")
	void SetBindingInventoryComponent(UYIInventoryComponent* InInventoryComponent);

	/** Size of one grid cell in pixels. Designers can clamp this between 8 and 128 for readability. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(ClampMin="8.0", ClampMax="128.0", ToolTip="Pixel size of each grid cell"))
	float CellPixelSize = 32.f;

	UFUNCTION(BlueprintCallable, Category="Inventory|Geometry")
	float GetCellPixelSize() const { return CellPixelSize; }

	/** When true, individual grids will not draw their own drag ghost; instead, a global overlay widget should render the ghost once for the whole screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Visuals", meta=(ToolTip="If enabled, suppress per-grid ghost drawing; use a global overlay to draw drag ghost"))
	bool bUseGlobalDragGhost = false;

	/** Optional explicit style override for this grid widget instance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Style", meta=(ToolTip="Optional visual style override for this grid widget"))
	TSoftObjectPtr<UYIInventoryGridStyleAsset> GridStyleOverride;

	/** If true, bag-level style is preferred before project default style when override is not set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Style", meta=(ToolTip="Use bag GridStyleAsset when widget override is empty"))
	bool bUseBagStyleAsset = true;

	/** If true, the grid will track hover and draw hover highlights. Disabled by default for gamepad-first setups. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Visuals", meta=(ToolTip="Enable per-cell hover tracking/highlight"))
	bool bEnableCellHover = true;

	/** If true, mouse clicks will update selection. Disable for mouse-driven PC inventories that don't need grid selection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Input", meta=(ToolTip="Allow mouse clicks to change the selected cell"))
	bool bEnableMouseSelection = false;

	/** Optional SFX when drag starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Audio|General", meta=(ToolTip="Enable all inventory UI sounds for this grid"))
	bool bEnableInventorySounds = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Audio|Hover", meta=(ToolTip="Enable hover sounds for slots"))
	bool bEnableHoverSounds = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Audio|Drag", meta=(ToolTip="Enable drag highlight hover sounds"))
	bool bEnableDragHoverSounds = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Audio|Drag", meta=(ToolTip="Enable drag start/drop/cancel sounds"))
	bool bEnableDragSounds = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Audio|Invalid", meta=(ToolTip="Enable invalid move sounds"))
	bool bEnableInvalidMoveSounds = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Audio|Drag", meta=(ToolTip="Play drag hover sounds even when the footprint is out of bounds"))
	bool bPlayDragHoverOutOfBounds = false;

	/** Optional SFX when drag starts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Audio|Drag")
	TObjectPtr<USoundBase> DragStartSound = nullptr;

	/** Optional SFX when a drop succeeds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Audio|Drag")
	TObjectPtr<USoundBase> DropSound = nullptr;

	/** Optional SFX when a drag is canceled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Audio|Drag")
	TObjectPtr<USoundBase> CancelDragSound = nullptr;

	/** Optional SFX when hovering a slot containing an item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Audio|Hover")
	TObjectPtr<USoundBase> HoverSlotSound = nullptr;

	/** Optional SFX when hovering an empty slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Audio|Hover")
	TObjectPtr<USoundBase> HoverEmptySound = nullptr;

	/** Optional SFX when dragging over a valid highlighted cell. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Audio|Drag")
	TObjectPtr<USoundBase> DragHoverSound = nullptr;

	/** Optional SFX when dragging over an invalid highlighted cell. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Audio|Drag")
	TObjectPtr<USoundBase> DragHoverInvalidSound = nullptr;

	/** Optional SFX when a move/drop is invalid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Audio|Invalid")
	TObjectPtr<USoundBase> InvalidMoveSound = nullptr;

	/** Optional SFX library for per-item sounds (override for this grid). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Audio|General")
	TSoftObjectPtr<UYIItemSFXLibrary> ItemSFXLibrary;

	/** Allow loading definition assets for SFX lookup when not already loaded. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Audio|General")
	bool bLoadDefinitionForSFX = true;

	/** Optional ASC/tags/XP for evaluating requirements in tooltips. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Tooltip", meta=(ToolTip="Ability System used to evaluate item requirements for tooltips"))
	TWeakObjectPtr<class UAbilitySystemComponent> RequirementAbilitySystem;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Tooltip")
	FGameplayTagContainer RequirementOwnedTags;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Tooltip")
	int32 RequirementXP = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Tooltip")
	TMap<FName,float> RequirementPreviewAttributes;

	/** Fired when tooltip data is produced (non-empty). */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTooltipDataUpdated, const FYITooltipData&, Data);
	UPROPERTY(BlueprintAssignable, Category="Inventory|Tooltip")
	FOnTooltipDataUpdated OnTooltipDataUpdated;
	/** Fired when tooltip is cleared/hidden. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTooltipCleared);
	UPROPERTY(BlueprintAssignable, Category="Inventory|Tooltip")
	FOnTooltipCleared OnTooltipCleared;

	UFUNCTION(BlueprintCallable, Category="Inventory|Visuals")
	void SetUseGlobalDragGhost(bool bEnable);
	UFUNCTION(BlueprintCallable, Category="Inventory|Visuals")
	void SetEnableCellHover(bool bEnable);
	UFUNCTION(BlueprintCallable, Category="Inventory|Input")
	void SetEnableMouseSelection(bool bEnable);
	UFUNCTION(BlueprintCallable, Category="Inventory|Style")
	void SetGridStyleOverride(UYIInventoryGridStyleAsset* InStyle);

	/** Resolve effective style: Widget override -> Bag style (optional) -> Project settings default. */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category="Inventory|Style")
	UYIInventoryGridStyleAsset* GetResolvedGridStyleAsset() const;
	/** Set runtime context used for requirement evaluation in tooltips. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Tooltip")
	void SetTooltipRequirementContext(class UAbilitySystemComponent* InASC, int32 InXP, const FGameplayTagContainer& InOwnedTags);
	UFUNCTION(BlueprintCallable, Category="Inventory|Tooltip")
	void SetTooltipPreviewAttributes(const TMap<FName,float>& InAttributes);

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
	void SetBoundTooltipWidget(class UUserWidget* Widget);

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
	bool RequestOpenActionMenu(class UUserWidget* Menu);

	/** Event fired when the grid wants an action menu opened for the current selection (provides the selected item index). UI owners can bind this to show their own menu or call RequestOpenActionMenu. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActionMenuRequested, int32, SelectedItemIndex);
	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnActionMenuRequested OnActionMenuRequested;

	/** Optional action menu bound to this grid; if Set and bAutoOpenActionMenuOnSelect is true the menu will automatically open when a new selection occurs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(ToolTip="Optional action menu to auto-open on selection"))
	UUserWidget* BoundActionMenu = nullptr;

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

	/** Allow or block moving items within this same grid (useful for read-only views). */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void SetAllowSelfMove(bool bInAllow) { bAllowSelfMove = bInAllow; }

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

	// Detailed hover event: includes whether the hovered slot has an item
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHoverSlotChanged, int32, HoveredItemIndex, bool, bHasItem);
	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnHoverSlotChanged OnHoverSlotChanged;
	// Drag & Drop events
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemDragStarted, UInventoryGridWidget*, SourceGrid, int32, SourceIndex);
	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnItemDragStarted OnItemDragStarted;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnItemDropped, UInventoryGridWidget*, DestGrid, int32, SourceIndex, FIntPoint, DestCell, bool, bSucceeded);
	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnItemDropped OnItemDropped;

	/** Fired when a drag is canceled; bDroppedToWorld indicates a forced drop due to no fit. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnItemDragCancelled, UInventoryGridWidget*, SourceGrid, FYIItemInstance, Item, bool, bDroppedToWorld);
	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnItemDragCancelled OnItemDragCancelled;

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
	/** Returns true if a global inventory drag is active (filtered by context world if provided). */
	static bool IsItemDragActive(const UWorld* ContextWorld = nullptr);
	/** Get the currently dragged item (if any) and the source bag for context; returns false if no drag active. */
	static bool GetActiveDraggedItem(struct FYIBagItem& OutItem, class UYIInventoryBag*& OutSourceBag, const UWorld* ContextWorld = nullptr);
	/** Equip the currently dragged inventory item into the requested equipment slot. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Drag")
	static bool TryEquipActiveDraggedItem(class UYIEquipmentComponent* EquipmentComponent, FGameplayTag RequestedSlotTag);
	/** Locate a live runtime grid currently bound to the provided bag. */
	static UInventoryGridWidget* FindRegisteredGridForBag(UYIInventoryBag* InBag, const UWorld* ContextWorld = nullptr);
	/** Starts drag for a specific item index from the provided bag using a registered runtime grid. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Drag")
	static bool BeginDragFromBagItem(UYIInventoryBag* InBag, int32 ItemIndex, const UWorld* ContextWorld = nullptr);
	/** Starts a detached drag by immediately removing the source item from bag and carrying it until dropped/canceled. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Drag")
	static bool BeginDetachedDragFromBagItem(UYIInventoryBag* InBag, int32 ItemIndex, const UWorld* ContextWorld = nullptr);

	/** Optional: route cross-owner transfers through a trade session (server-authoritative). */
	UFUNCTION(BlueprintCallable, Category="Inventory|Trade")
	void SetTradeSession(AYITradeSessionActor* InSession) { ActiveTradeSession = InSession; bHasTradeSide = false; }
	/** Set trade context (session + side) so cross-bag drops can route through server. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Trade")
	void SetTradeContext(AYITradeSessionActor* InSession, ETradeSide InSide);
	/** Assign shop context for drag/drop purchases or selling. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Shop")
	void SetShopContext(UYIShopComponent* InShop, bool bStockGrid);
	/** Fill OutData for the currently selected cell if an item exists there (returns true on success). */
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Get tooltip data for the currently selected cell"))
	bool GetSelectedCellTooltipData(struct FYITooltipData& OutData, const struct FYIRequirementContext& RequirementContext) const;

protected:
	virtual void OnWidgetRebuilt() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;
	FVector2D GetDesiredSize() const;

	/** Release owned Slate resources and unregister any delegates to avoid leaking SWidgets on teardown. */
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void BeginDestroy() override;

	// Global registry for all live grids (for overlay queries)
public:
	static void ForEachRegisteredGrid(TFunctionRef<void(UInventoryGridWidget*)> Callback);

	/** UI helper used by Slate drawing to tint equipment-locked items. */
	bool IsItemIndexLockedForUI(int32 ItemIndex) const;

private:
	UPROPERTY(Transient)
	TObjectPtr<AYITradeSessionActor> ActiveTradeSession = nullptr;
	UPROPERTY(Transient)
	ETradeSide TradeSide = static_cast<ETradeSide>(0);
	UPROPERTY(Transient)
	bool bHasTradeSide = false;
	UPROPERTY(Transient)
	bool bAllowSelfMove = true;
	static TSet<TWeakObjectPtr<UInventoryGridWidget>> GRegisteredGrids;
	TSharedPtr<SInventoryGridWidget> MySlateWidget;

	/** Optional tooltip widget to bind for runtime (set from BP/owner). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory", meta=(AllowPrivateAccess="true", ToolTip="Optional runtime tooltip widget to display item details"))
	class UUserWidget* BoundTooltipWidget = nullptr;

	// Internal click handler bound from Slate
	void HandleCellClicked(const FIntPoint& Cell);

	// Cached bag change handle so we can update tooltip when items change
	FDelegateHandle BagChangedHandle;
	UYIInventoryBag* CachedBag = nullptr;
	TWeakObjectPtr<UYIInventoryComponent> CachedBoundInventoryComponent;
	int32 HoveredItemIndexCached = INDEX_NONE;
	FIntPoint HoveredCellCached = FIntPoint(-1, -1);

	// Helper to update the bound tooltip when selection changes
	void UpdateBoundTooltip();

	// Called when the bound Bag fires OnChanged
	void OnBagChanged();
	void RebindInventoryContextDelegates();
	void RefreshBagFromBinding();

	UFUNCTION()
	void HandleInventoryBagOpened(UYIInventoryBag* InBag);
	UFUNCTION()
	void HandleInventoryBagClosed(UYIInventoryBag* InBag);

	// Internal handlers invoked from Slate callbacks
	void HandleSelectionChanged(const FIntPoint& NewCell);
	void HandleHoverChanged(int32 HoveredIndex);
	void HandleHoverCellChanged(const FIntPoint& NewCell);
	void HandleGhostPlacementChanged(const FIntPoint& TopLeftCell, bool bValid, bool bOutOfBounds);

	bool IsInventorySoundEnabled() const;
	bool IsHoverSoundEnabled() const;
	bool IsDragSoundEnabled() const;
	bool IsDragHoverSoundEnabled() const;
	bool IsInvalidSoundEnabled() const;

	UPROPERTY(Transient)
	TObjectPtr<UYIShopComponent> ActiveShopComponent = nullptr;
	UPROPERTY(Transient)
	bool bIsShopStockGrid = false;
};
