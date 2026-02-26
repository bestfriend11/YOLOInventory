#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Delegates/DelegateCombinations.h"
#include "StructUtils/InstancedStruct.h"
#include "GameplayTagContainer.h"
#include "YIInventoryBagNetTypes.h"
#include "YIItemInstance.h" // for FYIItemInstance
// #include "YIContainerInterface.h"
#include "YIInventoryBag.generated.h"

class UYIItemDefinition;

USTRUCT(BlueprintType)
struct YOLOINVENTORYCONTAINERS_API FYIBagItem
{
	GENERATED_BODY()
public:
	/** Instance of an item (definition + per-instance data like stack count/custom keys). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag", meta=(ToolTip="Stored item instance (definition + instance data)"))
	FYIItemInstance Item;

	/** Top-left grid coordinate within the bag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag", meta=(ToolTip="Top-left coordinate of this item in the bag grid"))
	FIntPoint Pos = FIntPoint::ZeroValue;

	/** Size in cells (width,height) the item occupies when placed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag", meta=(ToolTip="Width and height in grid cells for this item"))
	FIntPoint Size = FIntPoint(1,1);

	// Stack count if the item supports stacking
	// Count moved to Item.Count
	UPROPERTY(meta=(DeprecatedProperty, DisplayAfter="Item"))
	int32 Count_DEPRECATED = 1;

	// Legacy capability state removed
	UPROPERTY()
	TMap<FName, int32> DeprecatedCapabilityState_DoNotUse;

protected:
	// Only mark persistent assets dirty; runtime/PIE instances skip to avoid dirtying templates.
	bool ShouldMarkDirty() const;
};

/**
 * UYIInventoryBag
 *
 * Serializable bag asset that stores a grid of items and exposes designer-facing behaviors for packing, stacking and constraints.
 *
 * Designer tips:
 * - Set GridSize to 0 in one dimension to switch to list mode.
 * - Use bAutoMergeOnAdd to control whether the editor/runtime merges stacks automatically when new items are added.
 */
UCLASS(BlueprintType)
class YOLOINVENTORYCONTAINERS_API UYIInventoryBag : public UObject
{
	GENERATED_BODY()
public:
	/** Stable runtime identifier used by network/persistence layers. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Bag|Identity")
	FGuid BagId;

	/** Monotonic runtime revision incremented on bag content/layout changes. Used for optimistic request validation and UI mirror reconciliation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="Bag|Runtime")
	int32 RuntimeRevision = 0;

	/** Optional semantic role (for example Bag.Role.Main, Bag.Role.Context.Secondary). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag|Identity")
	FGameplayTag BagRoleTag;

	// Fired when items or layout change; editors can subscribe for live repaint
	FSimpleMulticastDelegate OnChanged;

	// Fine-grained bag events (Blueprint-assignable)
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBagItemAdded, int32, Index, FYIBagItem, Item);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBagItemRemoved, int32, Index, FYIBagItem, Item);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBagItemMoved, int32, Index, FIntPoint, NewPos);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBagItemRotated, int32, Index);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnBagItemTransferred, UYIInventoryBag*, Source, UYIInventoryBag*, Dest, int32, SourceIndex, int32, DestIndex);

	/** Broadcast when a new item stack is created in this bag. */
	UPROPERTY(BlueprintAssignable, Category="Bag|Events")
	FOnBagItemAdded OnItemAdded;

	/** Broadcast when an item stack is removed from this bag. */
	UPROPERTY(BlueprintAssignable, Category="Bag|Events")
	FOnBagItemRemoved OnItemRemoved;

	/** Broadcast when an item is moved within this bag (index and new position). */
	UPROPERTY(BlueprintAssignable, Category="Bag|Events")
	FOnBagItemMoved OnItemMoved;

	/** Broadcast when an item's rotation changed. */
	UPROPERTY(BlueprintAssignable, Category="Bag|Events")
	FOnBagItemRotated OnItemRotated;

	/** Broadcast when an item was transferred between bags. Source bag will broadcast after transfer; Dest bag will also broadcast with details. */
	UPROPERTY(BlueprintAssignable, Category="Bag|Events")
	FOnBagItemTransferred OnItemTransferred;

	/** Human-readable name for the bag used in editors and UI. */
	UPROPERTY(EditAnywhere, Category="Bag", meta=(ToolTip="Designer-friendly name for this bag"))
	FText DisplayName;

	/** Grid size; set either dimension to zero to treat the bag as a list. */
	UPROPERTY(EditAnywhere, Category="Bag|Layout", meta=(ToolTip="Grid dimensions (set a component to 0 to enable list mode)"))
	FIntPoint GridSize = FIntPoint(10,10);

	/** Base cell pixel size (designer adjustable). Keep between 8 and 128 for reasonable readability. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag|Layout", meta=(ClampMin="8.0", ClampMax="128.0", ToolTip="Pixel size for each grid cell"))
	float CellPixelSize = 32.f;

	/** Allow item rotation within this bag. */
	UPROPERTY(EditAnywhere, Category="Bag|Layout", meta=(ToolTip="Enable rotation for items placed in this bag"))
	bool bAllowRotation = true;

	/** Visual minify scale (editor preview concept). 1.0 = normal. May reduce item sizes visually and cause drops when decreased. */
	UPROPERTY(EditAnywhere, Category="Bag|Preview", meta=(ClampMin="0.1", ClampMax="1.0", ToolTip="Visual minify scale for previewing compact layouts"))
	float MinifyScale = 1.0f;

	// Grid visuals (designer selectable)
	/** Optional grid style asset used by runtime UMG/Slate grids. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag|Style", meta=(AllowedClasses="/Script/YOLOInventoryGrid.YIInventoryGridStyleAsset", ToolTip="Optional style asset for rendering this bag in runtime grids"))
	TSoftObjectPtr<UObject> GridStyleAsset;

	UPROPERTY(EditAnywhere, Category="Bag|Style", meta=(ToolTip="Color for inner grid lines")) FLinearColor GridLineColor = FLinearColor(0.1f,0.1f,0.1f,1);
	UPROPERTY(EditAnywhere, Category="Bag|Style", meta=(ToolTip="Color for outer outline")) FLinearColor OuterLineColor = FLinearColor(0.2f,0.2f,0.2f,1);
	UPROPERTY(EditAnywhere, Category="Bag|Style", meta=(ToolTip="Background color for cells")) FLinearColor CellBgColor = FLinearColor(0.02f,0.02f,0.02f,1);
	UPROPERTY(EditAnywhere, Category="Bag|Style", meta=(ClampMin="0.5", ClampMax="4.0", ToolTip="Thickness for grid lines")) float GridThickness = 1.0f;

	// Feature toggles (perf-friendly)
	UPROPERTY(EditAnywhere, Category="Bag|Features", meta=(ToolTip="Show per-cell tooltips in the editor and runtime when available")) bool bShowCellTooltips = true;
	UPROPERTY(EditAnywhere, Category="Bag|Features", meta=(ToolTip="Show column/row sorting headers in the Info tab")) bool bShowSortingHeaders = true;
	UPROPERTY(EditAnywhere, Category="Bag|Features", meta=(ToolTip="Enable drawing of thumbnails for items (may cost memory)")) bool bEnableThumbnails = true;
	UPROPERTY(EditAnywhere, Category="Bag|Features", meta=(ToolTip="Enable hover highlight for clearer feedback when hovering items")) bool bEnableHoverHighlight = true;

	// Palette filters (optional)
	UPROPERTY(EditAnywhere, Category="Bag|Palette", meta=(ToolTip="If true, only items matching TagFilters will be shown in the palette")) bool bUseTagFilter = false;
	UPROPERTY(EditAnywhere, Category="Bag|Palette", meta=(ToolTip="Tags used to filter the palette view")) TArray<FName> TagFilters;
	UPROPERTY(EditAnywhere, Category="Bag|Palette", meta=(ToolTip="If true, only assets in FolderFilters will be shown in the palette")) bool bUseFolderFilter = false;
	UPROPERTY(EditAnywhere, Category="Bag|Palette", meta=(ToolTip="Folder paths to include in the palette")) TArray<FDirectoryPath> FolderFilters;

	// Items currently stored
	UPROPERTY(EditAnywhere, Category="Bag", meta=(ToolTip="The items currently stored in this bag (grid or list)"))
	TArray<FYIBagItem> Items;

	/** If true, AddBagItem will attempt to merge into existing compatible stacks. Turn off to force new stacks (useful for designer testing). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag|Rules", meta=(ToolTip="Auto-merge incoming items into existing stacks when compatible"))
	bool bAutoMergeOnAdd = true;

	/** Enables hard acceptance rules so this bag can act like a rule-constrained slot set (equipment/context-specific). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag|Rules|Acceptance", meta=(ToolTip="If enabled, incoming items must pass acceptance filters below"))
	bool bEnforceAcceptanceRules = false;

	/** Allowed primary item types (uses ItemDefinition.ItemType). Empty means any type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag|Rules|Acceptance", meta=(EditCondition="bEnforceAcceptanceRules", ToolTip="Allowed ItemType tags; child tags are accepted"))
	FGameplayTagContainer AllowedItemTypes;

	/** Required tags that must exist on the item (checks ItemDefinition.Tags + ItemType). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag|Rules|Acceptance", meta=(EditCondition="bEnforceAcceptanceRules", ToolTip="All of these tags must exist on the item"))
	FGameplayTagContainer RequiredItemTags;

	/** Tags that disqualify the item (checks ItemDefinition.Tags + ItemType). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag|Rules|Acceptance", meta=(EditCondition="bEnforceAcceptanceRules", ToolTip="Any of these tags will block the item"))
	FGameplayTagContainer BlockedItemTags;

	/** Optional class filter for advanced setups; if set, item definition must be one of these classes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag|Rules|Acceptance", meta=(EditCondition="bEnforceAcceptanceRules", ToolTip="Optional allowed item definition classes"))
	TArray<TSoftClassPtr<UYIItemDefinition>> AllowedDefinitionClasses;

	/** Returns true if this definition passes acceptance rules configured on the bag. */
	UFUNCTION(BlueprintCallable, Category="Bag|Rules")
	bool CanAcceptItemDefinition(const UYIItemDefinition* Definition) const;

	/** Ensure bag has a valid BagId (called automatically by runtime systems). */
	UFUNCTION(BlueprintCallable, Category="Bag|Identity")
	void EnsureBagId();

	// Returns true if rectangle [Pos, Pos+Size) is free (no overlap) and inside grid
	UFUNCTION(BlueprintCallable, Category="Bag")
	bool CanPlaceAt(const FIntPoint Pos, const FIntPoint Size) const;
	// Same as CanPlaceAt but ignores a specific item index when checking overlap
	bool CanPlaceAtIgnoring(const FIntPoint& Pos, const FIntPoint& Size, int32 IgnoreIndex) const;

	// Returns true if rectangle would fit with minify scaling applied
	UFUNCTION(BlueprintCallable, Category="Bag")
	bool CanPlaceAtWithScale(const FIntPoint Pos, const FIntPoint Size) const;

	// Attempt to move an item within the bag
	UFUNCTION(BlueprintCallable, Category="Bag")
	bool MoveItem(int32 Index, const FIntPoint NewPos);

	// Rotate item (swap Size) if allowed
	UFUNCTION(BlueprintCallable, Category="Bag")
	bool RotateItem(int32 Index);

	// Apply a new minify scale and drop items that no longer fit; list is empty in list mode
	UFUNCTION(BlueprintCallable, Category="Bag")
	void ApplyMinifyScale(float NewScale, TArray<FYIBagItem>& DroppedItems);

	// Add a new item (returns index or -1). Respects item stacking/uniqueness rules.
	UFUNCTION(BlueprintCallable, Category="Bag")
	int32 AddBagItem(const FYIBagItem& NewItem);

	// Combine two stacks if they are of the same item type; returns true if any change
	UFUNCTION(BlueprintCallable, Category="Bag")
	bool CombineStacks(int32 IndexA, int32 IndexB);

	// Split a stack by amount into a new stack placed at Position if possible; returns new index or -1
	UFUNCTION(BlueprintCallable, Category="Bag")
	int32 SplitStack(int32 Index, int32 Amount, const FIntPoint Position);

	// Find existing stack index matching the same item asset (if any), or -1
	UFUNCTION(BlueprintCallable, Category="Bag")
	int32 FindExistingStackIndex(UYIItemDefinition* Definition) const;
	// Find existing stack matching both asset and per-instance stack key (returns INDEX_NONE if NewItem.CustomStackKey == 0)
	int32 FindExistingStackIndexForItem(const FYIBagItem& NewItem) const;
	// Remove item by index
	UFUNCTION(BlueprintCallable, Category="Bag")
	bool RemoveItem(int32 Index);

	// Atomically swap two items by index (both must be valid); returns true on success
	UFUNCTION(BlueprintCallable, Category="Bag")
	bool SwapItems(int32 IndexA, int32 IndexB);

	// Find first cell that can fit the given size; returns true and OutPos if found
	UFUNCTION(BlueprintCallable, Category="Bag")
	bool FindFirstFit(const FIntPoint Size, FIntPoint& OutPos) const;

	// Auto-pack all items row-first using current MinifyScale; preserves order where possible
	UFUNCTION(BlueprintCallable, Category="Bag")
	void AutoPack();

	// Utility to get effective size considering MinifyScale (at least 1x1)
	UFUNCTION(BlueprintCallable, Category="Bag")
	FIntPoint GetEffectiveSize(const FIntPoint InSize) const;

protected:
	/** Only mark package dirty when this is a persistent asset (not PIE/runtime clone). */
	bool ShouldMarkDirty() const;
	void MarkBagChanged();

	virtual void PostLoad() override;
};

