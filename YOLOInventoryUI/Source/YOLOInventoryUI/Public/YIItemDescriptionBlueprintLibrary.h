#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YIInventoryBlueprintLibrary.h"
#include "YIItemDescriptionBlueprintLibrary.generated.h"

class APlayerState;
class UYIInventoryBag;
class UYIShopComponent;

/**
 * Blueprint helpers for rich tooltip/description generation in non-grid views.
 * Use these for list views, item panels, and custom inspect windows.
 */
UCLASS()
class YOLOINVENTORYUI_API UYIItemDescriptionBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|UI|Description")
	static bool BuildRichTooltipForBagItem(
		const UYIInventoryBag* Bag,
		int32 ItemIndex,
		FYITooltipData& OutTooltipData,
		APlayerState* ViewerPlayerState = nullptr,
		UYIShopComponent* Shop = nullptr,
		bool bShopBuyContext = false,
		int32 Count = 1);
};

