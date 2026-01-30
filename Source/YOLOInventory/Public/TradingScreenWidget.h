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

	/** Assign the two bags that back the left and right grids. */
	UFUNCTION(BlueprintCallable, Category="Trading")
	void SetBags(class UYIInventoryBag* InLeftBag, class UYIInventoryBag* InRightBag);

	/** Bind a trade session and the two live bags (ours and theirs). OtherPartyBag can be null to use the replicated mirror. */
	UFUNCTION(BlueprintCallable, Category="Trading")
	void SetSession(class AYITradeSessionActor* InSession, class UYIInventoryBag* LocalPlayerBag, class UYIInventoryBag* OtherPartyBag);

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
	UInventoryTooltipView* SharedTooltip = nullptr;
	// Global drag overlay (optional)
	UPROPERTY(meta=(BindWidgetOptional))
	class UInventoryDragOverlayUserWidget* DragOverlay = nullptr;

	// Optional: track which grid has keyboard/gamepad focus for navigation
	UPROPERTY(Transient)
	UInventoryGridWidget* FocusedGrid = nullptr;

	// Session wiring
	UPROPERTY(Transient)
	TObjectPtr<class AYITradeSessionActor> Session = nullptr;

	// Runtime mirror bags for each side's offers/inventory
	UPROPERTY(Transient)
	TObjectPtr<class UYIInventoryBag> LeftMirrorBag = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<class UYIInventoryBag> RightMirrorBag = nullptr;

	// The local player's live bag used for offering items
	UPROPERTY(Transient)
	TObjectPtr<class UYIInventoryBag> LocalBag = nullptr;

	// The other party's live bag (if available). If null, we show their replicated mirror.
	UPROPERTY(Transient)
	TObjectPtr<class UYIInventoryBag> OtherBag = nullptr;

	// Set this to true if the local player is actually SideB (for AI-initiated trades etc.)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trading")
	bool bLocalIsSideA = true;

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

	// Session callbacks
	UFUNCTION()
	void HandleOffersUpdated();
	UFUNCTION()
	void HandleTradeCommitted();
	UFUNCTION()
	void HandleTradeCancelled();
	UFUNCTION()
	void HandleTradeFailed();

	void RefreshOffers();
	class UYIInventoryBag* BuildMirrorFromOffer(const struct FYITradeOffer& Offer);
	class UYIInventoryBag* BuildMirrorFromInventory(const TArray<struct FYINetBagItem>& View, FIntPoint GridSize);
};
