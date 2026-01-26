#pragma once
#include "CoreMinimal.h"
#include "WidgetScreen.h"
#include "InventoryGridWidget.h"
#include "Widgets/InventoryTooltipView.h"
#include "TradingScreenWidget.generated.h"

/**
 * UTradingScreenWidget
 *
 * A layered UI ScreenWidget that displays two independent inventory grids side by side
 * and supports drag-and-drop of items between them for trading/transfer.
 *
 * Usage:
 * - Create a UMG widget with two child InventoryGridWidgets named LeftGrid and RightGrid
 *   (and optional tooltips named LeftTooltip and RightTooltip) then set this class as the widget logic.
 * - Call SetBags to assign two UYIInventoryBag instances to the grids (or wire via Blueprints).
 */
UCLASS(meta=(DisplayName="YOLO Trading Screen"))
class YOLOINVENTORY_API UTradingScreenWidget : public UWidgetScreen
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Trading")
	UInventoryGridWidget* GetLeftGrid() const { return LeftGrid; }
	UFUNCTION(BlueprintCallable, Category="Trading")
	UInventoryGridWidget* GetRightGrid() const { return RightGrid; }

	UFUNCTION(BlueprintCallable, Category="Trading")
	UInventoryTooltipView* GetLeftTooltip() const { return LeftTooltip; }
	UFUNCTION(BlueprintCallable, Category="Trading")
	UInventoryTooltipView* GetRightTooltip() const { return RightTooltip; }

	/** Assign the two bags that back the left and right grids. */
	UFUNCTION(BlueprintCallable, Category="Trading")
	void SetBags(class UYIInventoryBag* InLeftBag, class UYIInventoryBag* InRightBag);

	/** Convenience: transfer current selection from left to right (or vice versa). Returns true on success and gives new index. */
	UFUNCTION(BlueprintCallable, Category="Trading")
	bool TransferSelection(bool bLeftToRight, int32 Count, int32& OutDestIndex);

protected:
	// BindWidget references
	UPROPERTY(meta=(BindWidget))
	UInventoryGridWidget* LeftGrid = nullptr;
	UPROPERTY(meta=(BindWidget))
	UInventoryGridWidget* RightGrid = nullptr;
	UPROPERTY(meta=(BindWidget))
	UInventoryTooltipView* LeftTooltip = nullptr;
	UPROPERTY(meta=(BindWidget))
	UInventoryTooltipView* RightTooltip = nullptr;
	// Global drag overlay (optional)
	UPROPERTY(meta=(BindWidgetOptional))
	class UInventoryDragOverlayUserWidget* DragOverlay = nullptr;

	// Optional: track which grid has keyboard/gamepad focus for navigation
	UPROPERTY(Transient)
	UInventoryGridWidget* FocusedGrid = nullptr;

	// Input handlers used when this screen is top of UILayered stack
	virtual bool HandleMoveUp() override;
	virtual bool HandleMoveDown() override;
	virtual bool HandleMoveLeft() override;
	virtual bool HandleMoveRight() override;
	virtual bool HandleConfirm() override;
	virtual bool HandleCancel() override;
	virtual bool HandleOpenMenu() override;

	void SetFocusedGrid(UInventoryGridWidget* NewFocused);

	// Called when either grid changes selection to refresh tooltips
	UFUNCTION()
	void OnLeftCellSelected(FIntPoint NewCell);
	UFUNCTION()
	void OnRightCellSelected(FIntPoint NewCell);
};
