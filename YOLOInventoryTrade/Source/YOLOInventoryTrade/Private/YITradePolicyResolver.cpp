#include "YITradePolicyResolver.h"

#include "YIItemDefinition.h"
#include "YIItemSchemaResolver.h"
#include "YITradeFragments.h"

#define LOCTEXT_NAMESPACE "YITradePolicyResolver"

bool FYIDefaultTradePolicyResolver::EvaluateTradePolicy(const FYIItemInstance& Item, const FYITradePolicyContext& Context, FYITradePolicyResult& OutResult) const
{
	OutResult = FYITradePolicyResult();
	OutResult.bAllowed = true;

	const UYIItemDefinition* Definition = Item.Definition.IsValid() ? Item.Definition.Get() : Item.Definition.LoadSynchronous();
	if (!Definition)
	{
		OutResult.bAllowed = false;
		OutResult.DenyReason = EYITradePolicyDenyReason::NotTradable;
		OutResult.Message = LOCTEXT("MissingDefinition", "Item definition is missing");
		return false;
	}

	const FYIItemSchemaSnapshot& Snapshot = YIItemSchema::ResolveSnapshot(Definition);
	if (const FYIItemTradePolicyFragment* Policy = Snapshot.FindResolvedFragment<FYIItemTradePolicyFragment>())
	{
		if (!Policy->bTradable)
		{
			OutResult.bAllowed = false;
			OutResult.DenyReason = EYITradePolicyDenyReason::NotTradable;
			OutResult.Message = LOCTEXT("NotTradable", "Item is not tradable");
			return false;
		}

		if (Context.bRequireVisibility && !Policy->bVisibleInTrade)
		{
			OutResult.bAllowed = false;
			OutResult.DenyReason = EYITradePolicyDenyReason::HiddenInTrade;
			OutResult.Message = LOCTEXT("HiddenInTrade", "Item is hidden in trade");
			return false;
		}

		if (!Policy->RequiredTradeTags.IsEmpty() && !Context.TradeContextTags.HasAll(Policy->RequiredTradeTags))
		{
			OutResult.bAllowed = false;
			OutResult.DenyReason = EYITradePolicyDenyReason::MissingRequiredTags;
			OutResult.Message = LOCTEXT("MissingRequiredTradeTags", "Item is missing required trade context tags");
			return false;
		}

		if (!Policy->BlockedTradeTags.IsEmpty() && Context.TradeContextTags.HasAny(Policy->BlockedTradeTags))
		{
			OutResult.bAllowed = false;
			OutResult.DenyReason = EYITradePolicyDenyReason::BlockedByTags;
			OutResult.Message = LOCTEXT("BlockedTradeTags", "Item is blocked in this trade context");
			return false;
		}
	}

	if (!Context.bIgnoreRuntimeBindState)
	{
		if (const FInstancedStruct* BindStateStruct = Item.FindFragmentByStruct(FYIItemBindStateRuntimeFragment::StaticStruct()))
		{
			if (const FYIItemBindStateRuntimeFragment* BindState = BindStateStruct->GetPtr<FYIItemBindStateRuntimeFragment>())
			{
				if (BindState->bAccountBound)
				{
					OutResult.bAllowed = false;
					OutResult.DenyReason = EYITradePolicyDenyReason::AccountBound;
					OutResult.Message = LOCTEXT("AccountBound", "Item is account bound");
					return false;
				}

				if (BindState->bCharacterBound)
				{
					OutResult.bAllowed = false;
					OutResult.DenyReason = EYITradePolicyDenyReason::CharacterBound;
					OutResult.Message = LOCTEXT("CharacterBound", "Item is character bound");
					return false;
				}

				if (BindState->TradeLockedUntilServerTime > Context.ServerTimeSeconds)
				{
					OutResult.bAllowed = false;
					OutResult.DenyReason = EYITradePolicyDenyReason::TradeLocked;
					OutResult.Message = LOCTEXT("TradeLocked", "Item is temporarily trade locked");
					return false;
				}
			}
		}
	}

	return true;
}

#undef LOCTEXT_NAMESPACE

