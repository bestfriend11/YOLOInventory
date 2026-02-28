#include "YILootPolicyResolver.h"

#include "YIItemSchemaResolver.h"
#include "YILootPolicyFragments.h"

#define LOCTEXT_NAMESPACE "YILootPolicyResolver"

bool FYIDefaultLootPolicyResolver::EvaluateLootPolicy(const UYIItemDefinition* Definition, const FYILootPolicyContext& Context, FYILootPolicyResult& OutResult) const
{
	OutResult = FYILootPolicyResult();
	OutResult.bEligible = true;

	if (!Definition)
	{
		OutResult.bEligible = false;
		OutResult.DenyReason = EYILootPolicyDenyReason::NotEligible;
		OutResult.Message = LOCTEXT("MissingDefinition", "Item definition is missing");
		return false;
	}

	const FYIItemSchemaSnapshot& Snapshot = YIItemSchema::ResolveSnapshot(Definition);
	if (const FYIItemLootEligibilityFragment* Eligibility = Snapshot.FindResolvedFragment<FYIItemLootEligibilityFragment>())
	{
		if (!Eligibility->bLootEligible)
		{
			OutResult.bEligible = false;
			OutResult.DenyReason = EYILootPolicyDenyReason::NotEligible;
			OutResult.Message = LOCTEXT("NotEligible", "Item is not eligible for loot");
			return false;
		}

		if (Context.LootLevel < Eligibility->MinLootLevel ||
			(Eligibility->MaxLootLevel > 0 && Context.LootLevel > Eligibility->MaxLootLevel))
		{
			OutResult.bEligible = false;
			OutResult.DenyReason = EYILootPolicyDenyReason::LevelOutOfRange;
			OutResult.Message = LOCTEXT("LevelOutOfRange", "Item is not eligible at this loot level");
			return false;
		}

		if (!Eligibility->RequiredLootTags.IsEmpty() && !Context.LootContextTags.HasAll(Eligibility->RequiredLootTags))
		{
			OutResult.bEligible = false;
			OutResult.DenyReason = EYILootPolicyDenyReason::MissingRequiredTags;
			OutResult.Message = LOCTEXT("MissingRequiredTags", "Loot context is missing required tags");
			return false;
		}

		if (!Eligibility->BlockedLootTags.IsEmpty() && Context.LootContextTags.HasAny(Eligibility->BlockedLootTags))
		{
			OutResult.bEligible = false;
			OutResult.DenyReason = EYILootPolicyDenyReason::BlockedByTags;
			OutResult.Message = LOCTEXT("BlockedByTags", "Loot context blocks this item");
			return false;
		}
	}

	if (const FYIItemRollPolicyFragment* RollPolicy = Snapshot.FindResolvedFragment<FYIItemRollPolicyFragment>())
	{
		OutResult.WeightScaleBps = FMath::Max(0, RollPolicy->WeightScaleBps);
		OutResult.MinGeneratedCount = FMath::Max(1, RollPolicy->MinGeneratedCount);
		OutResult.MaxGeneratedCount = FMath::Max(0, RollPolicy->MaxGeneratedCount);
	}

	return true;
}

#undef LOCTEXT_NAMESPACE

