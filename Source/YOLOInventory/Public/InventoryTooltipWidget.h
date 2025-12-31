#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "YIInventoryBlueprintLibrary.h"
#include "InventoryTooltipWidget.generated.h"

/**
 * UInventoryTooltipWidget
 *
 * Designer-facing tooltip widget used to display item metadata at runtime.
 * - Bind this widget to UInventoryGridWidget via SetBoundTooltipWidget to receive selected-item data.
 * - Implement OnTooltipDataChanged in Blueprint to update visuals (icon, name, description, affixes, etc.).
 */
UCLASS(meta=(DisplayName="YOLO Inventory Tooltip"))
class YOLOINVENTORY_API UInventoryTooltipWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Current tooltip data populated from a selected cell or hovered cell. */
	UPROPERTY(BlueprintReadOnly, Category="Tooltip", meta=(ToolTip="Struct containing item info: name, description, stats, affixes, etc."))
	FYITooltipData TooltipData;

	/** Set tooltip data (updates TooltipData and triggers OnTooltipDataChanged). */
	UFUNCTION(BlueprintCallable, Category="Tooltip", meta=(ToolTip="Populate the tooltip with FYITooltipData and notify the widget to update visuals"))
	void SetTooltipData(const FYITooltipData& Data)
	{
		TooltipData = Data;
		OnTooltipDataChanged();
	}

	/** Implement in Blueprint to react to data changes and refresh the visual layout. */
	UFUNCTION(BlueprintImplementableEvent, Category="Tooltip")
	void OnTooltipDataChanged();
};