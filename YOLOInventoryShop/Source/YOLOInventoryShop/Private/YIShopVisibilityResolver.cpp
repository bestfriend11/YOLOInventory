#include "YIShopVisibilityResolver.h"

#include "YIItemDefinition.h"
#include "YIItemSchemaResolver.h"
#include "YIShopComponent.h"
#include "YIShopFragments.h"

FYIShopResolvedVisibility FYIShopVisibilityResolver::ResolvePolicy(const FYIShopVisibilityResolverContext& Context)
{
	FYIShopResolvedVisibility Result;
	if (!Context.Definition)
	{
		return Result;
	}

	const FYIItemSchemaSnapshot& Snapshot = YIItemSchema::ResolveSnapshot(Context.Definition);
	const FYIItemShopPolicyFragment* PolicyFragment = Snapshot.FindResolvedFragment<FYIItemShopPolicyFragment>();
	if (!PolicyFragment)
	{
		return Result;
	}

	Result.bHasPolicyFragment = true;
	Result.bVisibleInShop = PolicyFragment->bVisibleInShop;
	Result.bBuyable = PolicyFragment->bBuyable;
	Result.bSellable = PolicyFragment->bSellable;
	Result.bRequirePriceForVisibility = PolicyFragment->bRequirePriceForVisibility;
	Result.bRequirePriceForBuy = PolicyFragment->bRequirePriceForBuy;
	Result.bRequirePriceForSell = PolicyFragment->bRequirePriceForSell;

	const FGameplayTagContainer* ShopTags = Context.Shop ? &Context.Shop->GetShopContextTags() : nullptr;
	if (PolicyFragment->RequiredShopTags.Num() > 0)
	{
		const bool bHasRequiredTags = ShopTags && ShopTags->HasAll(PolicyFragment->RequiredShopTags);
		if (!bHasRequiredTags)
		{
			Result.bVisibleInShop = false;
			Result.bBuyable = false;
			Result.bSellable = false;
			return Result;
		}
	}

	if (ShopTags && PolicyFragment->BlockedShopTags.Num() > 0 && ShopTags->HasAny(PolicyFragment->BlockedShopTags))
	{
		Result.bVisibleInShop = false;
		Result.bBuyable = false;
		Result.bSellable = false;
	}

	return Result;
}
