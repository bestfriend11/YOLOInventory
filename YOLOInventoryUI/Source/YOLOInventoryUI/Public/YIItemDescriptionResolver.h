#pragma once

#include "CoreMinimal.h"

class APlayerState;
class UYIInventoryBag;
class UYIShopComponent;
struct FYIItemInstance;
struct FYITooltipData;

/**
 * Context passed to description resolver.
 * Keep this feature-agnostic and cheap to construct from UI paths.
 */
struct YOLOINVENTORYUI_API FYIItemDescriptionContext
{
	const FYIItemInstance* Item = nullptr;
	const UYIInventoryBag* Bag = nullptr;
	const UYIShopComponent* Shop = nullptr;
	APlayerState* ViewerPlayerState = nullptr;
	bool bShopBuyContext = false;
	int32 Count = 1;
};

/**
 * Fast tooltip/description augmentation service.
 * Uses cacheable fragment-derived lines and avoids per-frame heavy work.
 */
class YOLOINVENTORYUI_API FYIItemDescriptionResolver
{
public:
	static void AugmentTooltip(const FYIItemDescriptionContext& Context, FYITooltipData& InOutTooltipData);
	static void InvalidateCacheForItem(const FGuid& ItemInstanceId);
	static void InvalidateAllCaches();
};
