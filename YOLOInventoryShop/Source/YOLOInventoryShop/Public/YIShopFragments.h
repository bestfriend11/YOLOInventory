#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YIItemFragments.h"
#include "YIShopFragments.generated.h"

/**
 * Selects which level source should drive scalable pricing.
 * Values are resolved server-side and remain feature-agnostic.
 */
UENUM(BlueprintType)
enum class EYIShopPriceLevelSource : uint8
{
	None UMETA(DisplayName="None"),
	Item UMETA(DisplayName="Item Level"),
	Buyer UMETA(DisplayName="Buyer Level"),
	Seller UMETA(DisplayName="Seller Level")
};

/**
 * Fixed-point price rule (integer currency) with optional level/quality scaling.
 * Uses basis points for percentage multipliers to avoid float-only economy logic.
 */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSHOP_API FYIShopScaledPriceRule
{
	GENERATED_BODY()

	/** Baseline amount for one unit before optional scaling/multipliers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price")
	int64 BaseAmount = 0;

	/** If true, apply PerLevelDelta for each level above BaseLevel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price|Scale")
	bool bScaleByLevel = false;

	/** Level where scaling starts (inclusive). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price|Scale", meta=(EditCondition="bScaleByLevel", ClampMin="1"))
	int32 BaseLevel = 1;

	/** Additive delta applied per level step. Supports negative values. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price|Scale", meta=(EditCondition="bScaleByLevel"))
	int64 PerLevelDelta = 0;

	/** If true, apply PerQualityDelta for each quality tier above BaseQuality. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price|Scale")
	bool bScaleByQuality = false;

	/** Quality tier where scaling starts (inclusive). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price|Scale", meta=(EditCondition="bScaleByQuality"))
	int32 BaseQuality = 0;

	/** Additive delta applied per quality step. Supports negative values. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price|Scale", meta=(EditCondition="bScaleByQuality"))
	int64 PerQualityDelta = 0;

	/** Multiply by request count (stack count) after scaling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price|Scale")
	bool bMultiplyByCount = true;

	/** Additional percentage multiplier in basis points (10000 = 100%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price|Scale", meta=(ClampMin="0"))
	int32 MultiplierBasisPoints = 10000;

	/** Enable explicit minimum clamp for the final amount. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price|Clamp")
	bool bClampMin = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price|Clamp", meta=(EditCondition="bClampMin"))
	int64 MinAmount = 0;

	/** Enable explicit maximum clamp for the final amount. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price|Clamp")
	bool bClampMax = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price|Clamp", meta=(EditCondition="bClampMax"))
	int64 MaxAmount = 0;
};

/** One currency/resource entry for shop pricing. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSHOP_API FYIShopFragmentPriceEntry
{
	GENERATED_BODY()

	/** Resource/currency key (Gold, Credits, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price")
	FName Resource = NAME_None;

	/** Buy rule used when purchasing from shop stock. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price")
	FYIShopScaledPriceRule Buy;

	/** Sell rule used when selling an item to a shop. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price")
	FYIShopScaledPriceRule Sell;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price")
	bool bAllowBuy = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price")
	bool bAllowSell = true;
};

/**
 * Static definition fragment that describes economy pricing in a non-opinionated way.
 * Feature plugins consume this at runtime; core schema remains generic.
 */
USTRUCT(BlueprintType, meta=(DisplayName="Shop Price Definition"))
struct YOLOINVENTORYSHOP_API FYIItemPriceDefinitionFragment : public FYIItemDefinitionFragmentBase
{
	GENERATED_BODY()

	/** Currency/resource pricing entries. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Price")
	TArray<FYIShopFragmentPriceEntry> Prices;

	/** Level source used for buy evaluation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Price")
	EYIShopPriceLevelSource BuyLevelSource = EYIShopPriceLevelSource::Buyer;

	/** Level source used for sell evaluation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Price")
	EYIShopPriceLevelSource SellLevelSource = EYIShopPriceLevelSource::Seller;

	/** Item-level fallback when level source is Item or player level is unavailable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Price", meta=(ClampMin="1"))
	int32 StaticItemLevel = 1;

	/** Item quality tier used by optional quality scaling rules. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Price")
	int32 StaticItemQuality = 0;
};

/**
 * Static shop policy fragment for visibility and transaction eligibility.
 */
USTRUCT(BlueprintType, meta=(DisplayName="Shop Policy"))
struct YOLOINVENTORYSHOP_API FYIItemShopPolicyFragment : public FYIItemDefinitionFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Policy")
	bool bVisibleInShop = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Policy")
	bool bBuyable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Policy")
	bool bSellable = true;

	/** Hide from stock UI if no valid price is resolved. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Policy")
	bool bRequirePriceForVisibility = false;

	/** Reject buy requests when no valid price is resolved. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Policy")
	bool bRequirePriceForBuy = false;

	/** Reject sell requests when no valid price is resolved. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Policy")
	bool bRequirePriceForSell = false;

	/** Optional context tags that must all be present on the shop component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Policy")
	FGameplayTagContainer RequiredShopTags;

	/** Optional context tags that block visibility/transactions if present on the shop component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Policy")
	FGameplayTagContainer BlockedShopTags;
};

/**
 * Dynamic runtime shop price modifiers (instance-owned).
 * Server can mutate these for discounts/events without changing definitions.
 */
USTRUCT(BlueprintType, meta=(DisplayName="Shop Price Runtime"))
struct YOLOINVENTORYSHOP_API FYIItemPriceRuntimeFragment : public FYIItemFragmentBase
{
	GENERATED_BODY()

	/** Runtime buy multiplier in basis points (10000 = 100%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Runtime", meta=(ClampMin="0"))
	int32 BuyMultiplierBasisPoints = 10000;

	/** Runtime sell multiplier in basis points (10000 = 100%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Runtime", meta=(ClampMin="0"))
	int32 SellMultiplierBasisPoints = 10000;

	/** Runtime additive buy delta after scaling/multipliers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Runtime")
	int64 BuyFlatDelta = 0;

	/** Runtime additive sell delta after scaling/multipliers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Runtime")
	int64 SellFlatDelta = 0;

	/** Optional runtime level override; <=0 means no override. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Runtime")
	int32 OverrideLevel = 0;

	/** Optional runtime quality override; INDEX_NONE means no override. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Runtime")
	int32 OverrideQuality = INDEX_NONE;
};
