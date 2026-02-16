#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "YIInventoryGridStyleAsset.generated.h"

/**
 * Generic brush + tint pair used by grid style slots.
 * If bEnabled is false the renderer ignores this slot.
 */
USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIGridStyleBrushSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	bool bEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style", meta = (ToolTip = "Optional image/material brush. If empty and enabled, tint-only fill is used when possible."))
	FSlateBrush Brush;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	FLinearColor Tint = FLinearColor::White;
};

/**
 * Grid rendering style asset used by runtime inventory grids.
 * Keep this focused on the grid itself (cells/items/ghost/highlights).
 */
UCLASS(BlueprintType)
class YOLOINVENTORY_API UYIInventoryGridStyleAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	UYIInventoryGridStyleAsset();

	/**
	 * If enabled, grid widgets automatically push theme/state scalar parameters
	 * into material brushes used by this style asset.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Automation")
	bool bAutoDriveThemeMaterialParameters = true;

	/** 0 = dark fantasy, 1 = sci-fi (or your own blend convention). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Automation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ThemeBlend = 0.0f;

	/** Scalar parameter names expected by your UI material instances. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Automation")
	FName ThemeBlendParameterName = TEXT("ThemeBlend");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Automation")
	FName TimeParameterName = TEXT("Time");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Automation")
	FName HoverAmountParameterName = TEXT("HoverAmount");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Automation")
	FName SelectedAmountParameterName = TEXT("SelectedAmount");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Automation")
	FName InvalidAmountParameterName = TEXT("InvalidAmount");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Automation")
	FName MarqueeAmountParameterName = TEXT("MarqueeAmount");

	/** Optional slot id scalar so one material can branch by slot role. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Automation")
	bool bSetSlotStateIdParameter = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Theme Automation")
	FName SlotStateIdParameterName = TEXT("SlotStateId");

	/** Optional style label for content browser readability. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
	FText DisplayName;

	/** Base cell fill; falls back to bag CellBgColor if disabled. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Cells")
	FYIGridStyleBrushSlot CellFill;

	/** Per-cell line color/thickness. Falls back to bag values when disabled by style usage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Lines")
	FLinearColor GridLineColor = FLinearColor(0.1f, 0.1f, 0.1f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Lines", meta = (ClampMin = "0.5", ClampMax = "8.0"))
	float GridLineThickness = 1.f;

	/** Optional outer border around the full grid area. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid|Lines")
	FYIGridStyleBrushSlot OuterBorder;

	/** Optional item background under icons (per item footprint). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items|Body")
	FYIGridStyleBrushSlot ItemFill;

	/** Item border/frame tint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items|Body")
	FLinearColor ItemBorderColor = FLinearColor(0.8f, 0.8f, 0.8f, 0.65f);

	/** Optional brush frame for item footprint. If disabled, line border is used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items|Body")
	FYIGridStyleBrushSlot ItemFrame;

	/** Draw item icon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items|Icon")
	bool bDrawItemIcon = true;

	/** If true icon stretches to item footprint; otherwise icon keeps ratio and fits. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items|Icon")
	bool bStretchItemIconToBounds = true;

	/** Padding applied to item icon in local pixel units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items|Icon")
	FMargin ItemIconPadding = FMargin(0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items|Icon")
	FLinearColor ItemIconTint = FLinearColor::White;

	/** Optional hovered-cell overlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States|Hover")
	FYIGridStyleBrushSlot HoveredCellOverlay;

	/** Optional hovered-item overlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States|Hover")
	FYIGridStyleBrushSlot HoveredItemOverlay;

	/** Optional selected-cell overlay (fill). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States|Selection")
	FYIGridStyleBrushSlot SelectedCellOverlay;

	/** Selected-cell outline settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States|Selection")
	FLinearColor SelectedCellOutlineColor = FLinearColor(0.2f, 0.6f, 1.f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States|Selection", meta = (ClampMin = "0.5", ClampMax = "8.0"))
	float SelectedCellOutlineThickness = 2.5f;

	/** Overlay shown for locked/equipped items in inventory grid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States|Locked")
	FYIGridStyleBrushSlot LockedItemOverlay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States|Locked")
	FText LockedLabel = NSLOCTEXT("YOLOInventory", "GridLockedLabelDefault", "EQUIPPED");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States|Locked")
	FLinearColor LockedLabelColor = FLinearColor(0.95f, 0.98f, 1.f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States|Locked", meta = (ClampMin = "6", ClampMax = "36"))
	int32 LockedLabelFontSize = 9;

	/** Stack count text styling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items|Stack")
	FLinearColor StackCountColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Items|Stack", meta = (ClampMin = "6", ClampMax = "36"))
	int32 StackCountFontSize = 10;

	/** Drag footprint overlays (always shown while dragging over this grid). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States|Ghost")
	FYIGridStyleBrushSlot GhostPlacementValidOverlay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States|Ghost")
	FYIGridStyleBrushSlot GhostPlacementInvalidOverlay;

	/** Drag icon tint and outline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States|Ghost")
	FLinearColor GhostIconTint = FLinearColor(1.f, 1.f, 1.f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States|Ghost")
	FLinearColor GhostOutlineColor = FLinearColor(0.2f, 0.8f, 1.f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "States|Ghost", meta = (ClampMin = "0.5", ClampMax = "8.0"))
	float GhostOutlineThickness = 1.5f;
};
