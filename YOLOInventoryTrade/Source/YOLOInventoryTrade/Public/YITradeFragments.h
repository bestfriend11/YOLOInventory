#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YIItemFragments.h"
#include "YITradeFragments.generated.h"

/** Static policy fragment for trade visibility/eligibility. */
USTRUCT(BlueprintType, meta=(DisplayName="Trade Policy"))
struct YOLOINVENTORYTRADE_API FYIItemTradePolicyFragment : public FYIItemDefinitionFragmentBase
{
	GENERATED_BODY()

	/** Master tradable flag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade|Policy")
	bool bTradable = true;

	/** Show this item in trade UIs/offer pickers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade|Policy")
	bool bVisibleInTrade = true;

	/** Optional required tags for a trade context. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade|Policy")
	FGameplayTagContainer RequiredTradeTags;

	/** Optional blocked tags for a trade context. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade|Policy")
	FGameplayTagContainer BlockedTradeTags;
};

/** Runtime binding/lock payload for anti-exploit trade rules. */
USTRUCT(BlueprintType, meta=(DisplayName="Trade Bind Runtime"))
struct YOLOINVENTORYTRADE_API FYIItemBindStateRuntimeFragment : public FYIItemFragmentBase
{
	GENERATED_BODY()

	/** Item cannot be traded away from owning account when true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade|Runtime")
	bool bAccountBound = false;

	/** Item cannot be traded away from owning character when true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade|Runtime")
	bool bCharacterBound = false;

	/** Optional absolute server world time when trade lock expires. <=0 means no timer lock. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade|Runtime")
	double TradeLockedUntilServerTime = 0.0;
};
