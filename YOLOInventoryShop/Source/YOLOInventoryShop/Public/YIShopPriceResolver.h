#pragma once

#include "CoreMinimal.h"
#include "YIShopFragments.h"

class APlayerState;
class UYIItemDefinition;
class UYIShopComponent;
struct FYIItemInstance;

/** Buy vs sell price computation path. */
enum class EYIShopResolvedPriceKind : uint8
{
	Buy,
	Sell
};

/** Lightweight resolved price row used by shop runtime and UI. */
struct FYIShopResolvedPriceRow
{
	FName Resource = NAME_None;
	int64 Amount = 0;
};

/** Price resolver output. */
struct FYIShopResolvedPriceResult
{
	TArray<FYIShopResolvedPriceRow> Rows;
	int32 EffectiveLevel = 1;
	int32 EffectiveQuality = 0;
	bool bUsedRuntimeModifier = false;
};

/** Input context for fragment price resolver. */
struct FYIShopPriceResolverContext
{
	const UYIShopComponent* Shop = nullptr;
	const UYIItemDefinition* Definition = nullptr;
	const FYIItemInstance* ItemInstance = nullptr;
	APlayerState* BuyerPlayerState = nullptr;
	APlayerState* SellerPlayerState = nullptr;
	int32 Count = 1;
	EYIShopResolvedPriceKind PriceKind = EYIShopResolvedPriceKind::Buy;
};

/**
 * Server-authoritative fragment price resolver.
 * Computes integer currency prices from static + runtime fragments.
 */
class YOLOINVENTORYSHOP_API FYIShopPriceResolver
{
public:
	static bool ResolveFragmentPrice(const FYIShopPriceResolverContext& Context, FYIShopResolvedPriceResult& OutResult);
};
