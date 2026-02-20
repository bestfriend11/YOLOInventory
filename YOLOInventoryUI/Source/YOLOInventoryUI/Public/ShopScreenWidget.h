#pragma once

#include "CoreMinimal.h"
#include "WidgetScreen.h"
#include "InventoryGridWidget.h"
#include "Widgets/InventoryTooltipView.h"
#include "YIShopComponent.h"
#include "ShopScreenWidget.generated.h"

/**
 * UShopScreenWidget
 *
 * Simple shop UI: left grid is the player's inventory, right grid is the shop stock mirror.
 * Drag from right -> left to buy. (Selling can be added later.)
 */
UCLASS(meta=(DisplayName="YOLO Shop Screen"))
class YOLOINVENTORYUI_API UShopScreenWidget : public UWidgetScreen
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Shop")
	UInventoryGridWidget* GetLeftGrid() const { return LeftGrid; }
	UFUNCTION(BlueprintCallable, Category="Shop")
	UInventoryGridWidget* GetRightGrid() const { return RightGrid; }

	/** Assign the shop + player bag + current stock mirror. */
	UFUNCTION(BlueprintCallable, Category="Shop")
	void SetShop(UYIShopComponent* InShop, UYIInventoryBag* LocalPlayerBag, const TArray<FYINetBagItem>& Stock, FIntPoint StockSize);

protected:
	UPROPERTY(meta=(BindWidget))
	UInventoryGridWidget* LeftGrid = nullptr;
	UPROPERTY(meta=(BindWidget))
	UInventoryGridWidget* RightGrid = nullptr;
	UPROPERTY(meta=(BindWidget))
	UInventoryTooltipView* SharedTooltip = nullptr;
	UPROPERTY(meta=(BindWidgetOptional))
	class UInventoryDragOverlayUserWidget* DragOverlay = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UYIShopComponent> Shop = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UYIInventoryBag> LocalBag = nullptr;
	UPROPERTY(Transient)
	TObjectPtr<UYIInventoryBag> ShopMirrorBag = nullptr;

	UFUNCTION()
	void OnLeftCellSelected(FIntPoint NewCell);
	UFUNCTION()
	void OnRightCellSelected(FIntPoint NewCell);

	UFUNCTION()
	void HandleShopMirrorUpdated();

	UYIInventoryBag* BuildMirrorFromStock(const TArray<FYINetBagItem>& View, FIntPoint GridSize);
};
