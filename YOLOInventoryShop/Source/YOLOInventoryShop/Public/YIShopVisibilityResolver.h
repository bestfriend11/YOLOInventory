#pragma once

#include "CoreMinimal.h"

class UYIItemDefinition;
class UYIShopComponent;

/** Input context for visibility/policy resolution. */
struct FYIShopVisibilityResolverContext
{
	const UYIShopComponent* Shop = nullptr;
	const UYIItemDefinition* Definition = nullptr;
};

/** Resolved static shop policy flags. */
struct FYIShopResolvedVisibility
{
	bool bVisibleInShop = true;
	bool bBuyable = true;
	bool bSellable = true;
	bool bRequirePriceForVisibility = false;
	bool bRequirePriceForBuy = false;
	bool bRequirePriceForSell = false;
	bool bHasPolicyFragment = false;
};

/**
 * Server-authoritative policy resolver for shop visibility and transaction flags.
 */
class YOLOINVENTORYSHOP_API FYIShopVisibilityResolver
{
public:
	static FYIShopResolvedVisibility ResolvePolicy(const FYIShopVisibilityResolverContext& Context);
};
