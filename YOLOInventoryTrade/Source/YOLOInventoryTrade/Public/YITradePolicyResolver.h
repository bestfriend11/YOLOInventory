#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YIItemFeatureResolverRegistry.h"
#include "YIItemInstance.h"
#include "YITradePolicyResolver.generated.h"

UENUM(BlueprintType)
enum class EYITradePolicyDenyReason : uint8
{
	None = 0,
	NotTradable,
	HiddenInTrade,
	MissingRequiredTags,
	BlockedByTags,
	AccountBound,
	CharacterBound,
	TradeLocked
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYTRADE_API FYITradePolicyContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	FGameplayTagContainer TradeContextTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	double ServerTimeSeconds = 0.0;

	/** Enforce bVisibleInTrade from static policy fragment. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	bool bRequireVisibility = false;

	/** Ignore dynamic bind state checks (account/character/time lock). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	bool bIgnoreRuntimeBindState = false;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYTRADE_API FYITradePolicyResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	bool bAllowed = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	EYITradePolicyDenyReason DenyReason = EYITradePolicyDenyReason::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	FText Message;
};

class YOLOINVENTORYTRADE_API IYITradePolicyResolver : public IYIItemFeatureResolver
{
public:
	virtual bool EvaluateTradePolicy(const FYIItemInstance& Item, const FYITradePolicyContext& Context, FYITradePolicyResult& OutResult) const = 0;
};

class YOLOINVENTORYTRADE_API FYIDefaultTradePolicyResolver : public IYITradePolicyResolver
{
public:
	virtual FName GetResolverKey() const override { return YIItemFeatureKeys::TradePolicy; }
	virtual bool EvaluateTradePolicy(const FYIItemInstance& Item, const FYITradePolicyContext& Context, FYITradePolicyResult& OutResult) const override;
};

