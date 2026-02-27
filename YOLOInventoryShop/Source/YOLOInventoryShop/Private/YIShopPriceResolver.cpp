#include "YIShopPriceResolver.h"

#include "Components/ActorComponent.h"
#include "GameFramework/PlayerState.h"
#include "UObject/UnrealType.h"
#include "YIItemDefinition.h"
#include "YIItemInstance.h"
#include "YIItemSchemaResolver.h"

namespace
{
	static int32 ClampBasisPoints(const int32 InBasisPoints)
	{
		return FMath::Max(0, InBasisPoints);
	}

	static int64 ApplyBasisPoints(const int64 Value, const int32 BasisPoints)
	{
		const int32 SafeBasisPoints = ClampBasisPoints(BasisPoints);
		if (Value == 0 || SafeBasisPoints == 10000)
		{
			return Value;
		}
		if (SafeBasisPoints == 0)
		{
			return 0;
		}

		const int64 Sign = Value < 0 ? -1 : 1;
		const int64 AbsValue = FMath::Abs(Value);
		const int64 Whole = (AbsValue / 10000) * static_cast<int64>(SafeBasisPoints);
		const int64 Fraction = ((AbsValue % 10000) * static_cast<int64>(SafeBasisPoints) + 5000) / 10000;
		return Sign * (Whole + Fraction);
	}

	static bool TryInvokeNumericNoArgs(UObject* Target, const FName FunctionName, int32& OutValue)
	{
		if (!Target)
		{
			return false;
		}

		UFunction* Function = Target->FindFunction(FunctionName);
		if (!Function || Function->NumParms != 1)
		{
			return false;
		}

		FProperty* ReturnProperty = Function->GetReturnProperty();
		if (!ReturnProperty)
		{
			return false;
		}

		TArray<uint8> Params;
		Params.SetNumZeroed(Function->ParmsSize);
		Target->ProcessEvent(Function, Params.GetData());
		const void* ReturnValuePtr = ReturnProperty->ContainerPtrToValuePtr<void>(Params.GetData());

		if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(ReturnProperty))
		{
			if (NumericProperty->IsInteger())
			{
				OutValue = static_cast<int32>(NumericProperty->GetSignedIntPropertyValue(ReturnValuePtr));
				return true;
			}
			if (NumericProperty->IsFloatingPoint())
			{
				OutValue = FMath::RoundToInt(NumericProperty->GetFloatingPointPropertyValue(ReturnValuePtr));
				return true;
			}
		}
		return false;
	}

	static int32 ResolvePlayerLevel(APlayerState* PlayerState)
	{
		if (!PlayerState)
		{
			return 1;
		}

		static const FName LevelFunctionNames[] =
		{
			TEXT("GetShopPricingLevel"),
			TEXT("GetCharacterLevel"),
			TEXT("GetPlayerLevel"),
			TEXT("GetLevel")
		};

		int32 ResolvedLevel = 1;
		for (const FName FunctionName : LevelFunctionNames)
		{
			if (TryInvokeNumericNoArgs(PlayerState, FunctionName, ResolvedLevel))
			{
				return FMath::Max(1, ResolvedLevel);
			}
		}

		TInlineComponentArray<UActorComponent*> Components(PlayerState);
		for (UActorComponent* Component : Components)
		{
			if (!Component)
			{
				continue;
			}
			for (const FName FunctionName : LevelFunctionNames)
			{
				if (TryInvokeNumericNoArgs(Component, FunctionName, ResolvedLevel))
				{
					return FMath::Max(1, ResolvedLevel);
				}
			}
		}

		return 1;
	}

	static int32 ResolveEffectiveLevel(
		const FYIItemPriceDefinitionFragment& PriceFragment,
		const FYIItemPriceRuntimeFragment* RuntimeFragment,
		const APlayerState* BuyerPlayerState,
		const APlayerState* SellerPlayerState,
		const EYIShopResolvedPriceKind PriceKind)
	{
		if (RuntimeFragment && RuntimeFragment->OverrideLevel > 0)
		{
			return RuntimeFragment->OverrideLevel;
		}

		const EYIShopPriceLevelSource Source = (PriceKind == EYIShopResolvedPriceKind::Buy)
			? PriceFragment.BuyLevelSource
			: PriceFragment.SellLevelSource;

		switch (Source)
		{
		case EYIShopPriceLevelSource::Buyer:
			return ResolvePlayerLevel(const_cast<APlayerState*>(BuyerPlayerState));
		case EYIShopPriceLevelSource::Seller:
			return ResolvePlayerLevel(const_cast<APlayerState*>(SellerPlayerState));
		case EYIShopPriceLevelSource::Item:
		case EYIShopPriceLevelSource::None:
		default:
			return FMath::Max(1, PriceFragment.StaticItemLevel);
		}
	}

	static int32 ResolveEffectiveQuality(
		const FYIItemPriceDefinitionFragment& PriceFragment,
		const FYIItemPriceRuntimeFragment* RuntimeFragment)
	{
		if (RuntimeFragment && RuntimeFragment->OverrideQuality != INDEX_NONE)
		{
			return RuntimeFragment->OverrideQuality;
		}
		return PriceFragment.StaticItemQuality;
	}

	static int64 ResolveRuleAmount(const FYIShopScaledPriceRule& Rule, const int32 EffectiveLevel, const int32 EffectiveQuality, const int32 Count)
	{
		int64 Amount = Rule.BaseAmount;

		if (Rule.bScaleByLevel)
		{
			const int32 LevelDelta = EffectiveLevel - Rule.BaseLevel;
			if (LevelDelta != 0)
			{
				Amount += static_cast<int64>(LevelDelta) * Rule.PerLevelDelta;
			}
		}

		if (Rule.bScaleByQuality)
		{
			const int32 QualityDelta = EffectiveQuality - Rule.BaseQuality;
			if (QualityDelta != 0)
			{
				Amount += static_cast<int64>(QualityDelta) * Rule.PerQualityDelta;
			}
		}

		Amount = ApplyBasisPoints(Amount, Rule.MultiplierBasisPoints);
		if (Rule.bMultiplyByCount)
		{
			Amount *= static_cast<int64>(FMath::Max(1, Count));
		}

		if (Rule.bClampMin)
		{
			Amount = FMath::Max(Amount, Rule.MinAmount);
		}
		if (Rule.bClampMax)
		{
			Amount = FMath::Min(Amount, Rule.MaxAmount);
		}

		return FMath::Max<int64>(0, Amount);
	}
}

bool FYIShopPriceResolver::ResolveFragmentPrice(const FYIShopPriceResolverContext& Context, FYIShopResolvedPriceResult& OutResult)
{
	OutResult = FYIShopResolvedPriceResult();
	if (!Context.Definition || !Context.ItemInstance || Context.Count <= 0)
	{
		return false;
	}

	const FYIItemSchemaSnapshot& Snapshot = YIItemSchema::ResolveSnapshot(Context.Definition);
	const FYIItemPriceDefinitionFragment* PriceFragment = Snapshot.FindResolvedFragment<FYIItemPriceDefinitionFragment>();
	if (!PriceFragment || PriceFragment->Prices.Num() == 0)
	{
		return false;
	}

	const FYIItemPriceRuntimeFragment* RuntimeFragment = nullptr;
	if (const FInstancedStruct* RuntimeFragmentStruct = Context.ItemInstance->FindFragmentByStruct(FYIItemPriceRuntimeFragment::StaticStruct()))
	{
		RuntimeFragment = RuntimeFragmentStruct->GetPtr<FYIItemPriceRuntimeFragment>();
	}

	OutResult.EffectiveLevel = ResolveEffectiveLevel(
		*PriceFragment,
		RuntimeFragment,
		Context.BuyerPlayerState,
		Context.SellerPlayerState,
		Context.PriceKind);
	OutResult.EffectiveQuality = ResolveEffectiveQuality(*PriceFragment, RuntimeFragment);

	const bool bIsBuy = Context.PriceKind == EYIShopResolvedPriceKind::Buy;
	OutResult.Rows.Reserve(PriceFragment->Prices.Num());
	for (const FYIShopFragmentPriceEntry& Entry : PriceFragment->Prices)
	{
		if (Entry.Resource.IsNone())
		{
			continue;
		}
		if (bIsBuy && !Entry.bAllowBuy)
		{
			continue;
		}
		if (!bIsBuy && !Entry.bAllowSell)
		{
			continue;
		}

		const FYIShopScaledPriceRule& Rule = bIsBuy ? Entry.Buy : Entry.Sell;
		int64 Amount = ResolveRuleAmount(Rule, OutResult.EffectiveLevel, OutResult.EffectiveQuality, Context.Count);

		if (RuntimeFragment)
		{
			const int32 RuntimeMultiplierBps = bIsBuy ? RuntimeFragment->BuyMultiplierBasisPoints : RuntimeFragment->SellMultiplierBasisPoints;
			const int64 RuntimeDelta = bIsBuy ? RuntimeFragment->BuyFlatDelta : RuntimeFragment->SellFlatDelta;
			Amount = ApplyBasisPoints(Amount, RuntimeMultiplierBps);
			Amount = FMath::Max<int64>(0, Amount + RuntimeDelta);
			OutResult.bUsedRuntimeModifier = true;
		}

		if (Amount <= 0)
		{
			continue;
		}

		FYIShopResolvedPriceRow& ResolvedRow = OutResult.Rows.AddDefaulted_GetRef();
		ResolvedRow.Resource = Entry.Resource;
		ResolvedRow.Amount = Amount;
	}

	OutResult.Rows.Sort([](const FYIShopResolvedPriceRow& A, const FYIShopResolvedPriceRow& B)
	{
		return A.Resource.LexicalLess(B.Resource);
	});

	return OutResult.Rows.Num() > 0;
}
