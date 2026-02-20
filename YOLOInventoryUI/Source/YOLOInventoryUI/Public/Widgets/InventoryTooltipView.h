#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "YIInventoryBlueprintLibrary.h"
#include "InventoryTooltipView.generated.h"

class UInventoryTooltipDesignerWidget;
class SInventoryTooltipWidget;

/**
 * Runtime tooltip view that can render either the default Slate tooltip or a designer-authored UMG widget.
 */
UCLASS(meta=(DisplayName="YOLO Inventory Tooltip View"))
class YOLOINVENTORYUI_API UInventoryTooltipView : public UUserWidget
{
	GENERATED_BODY()
public:
UInventoryTooltipView(const FObjectInitializer& ObjectInitializer);

	/** Tooltip data being displayed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip", meta=(ExposeOnSpawn=true))
	FYITooltipData TooltipData;

	/** If true and DesignerTooltipClass is set, the designer widget will be used instead of the default Slate tooltip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip", meta=(ExposeOnSpawn=true))
	bool bUseDesignerTooltip = false;

	/** Designer-authored tooltip widget class (must derive from UInventoryTooltipDesignerWidget for data updates). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip", meta=(ExposeOnSpawn=true))
	TSubclassOf<UInventoryTooltipDesignerWidget> DesignerTooltipClass;

	/** Update tooltip with new data. */
	UFUNCTION(BlueprintCallable, Category="Tooltip")
	void SetTooltipData(const FYITooltipData& InData);

	/** Clear tooltip visuals. */
	UFUNCTION(BlueprintCallable, Category="Tooltip")
	void ClearTooltip();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void SynchronizeProperties() override;

private:
	TSharedPtr<SInventoryTooltipWidget> SlateTooltip;
	UPROPERTY(Transient) TObjectPtr<UInventoryTooltipDesignerWidget> DesignerTooltipInstance;

	void PushDataToCurrentView();
};
