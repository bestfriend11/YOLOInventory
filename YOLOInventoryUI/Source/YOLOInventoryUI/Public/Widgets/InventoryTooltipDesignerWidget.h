#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "YIInventoryBlueprintLibrary.h"
#include "InventoryTooltipDesignerWidget.generated.h"

/**
 * Base class for designer-authored tooltip widgets.
 * Extend this in UMG and implement OnTooltipDataUpdated to drive your own layout.
 */
UCLASS(Abstract, Blueprintable)
class YOLOINVENTORYUI_API UInventoryTooltipDesignerWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintImplementableEvent, Category="Tooltip")
	void OnTooltipDataUpdated(const FYITooltipData& Data);
};

