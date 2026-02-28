#include "YIEquipmentPolicyResolver.h"

#include "YIEquipmentFragments.h"
#include "YIItemDefinition.h"
#include "YIItemSchemaResolver.h"

#define LOCTEXT_NAMESPACE "YIEquipPolicyResolver"

bool FYIDefaultEquipPolicyResolver::EvaluateEquipPolicy(const FYIItemInstance& Item, const FYIEquipPolicyContext& Context, FYIEquipPolicyResult& OutResult) const
{
	OutResult = FYIEquipPolicyResult();
	OutResult.bAllowed = true;

	const UYIItemDefinition* Definition = Item.Definition.IsValid() ? Item.Definition.Get() : Item.Definition.LoadSynchronous();
	if (!Definition)
	{
		OutResult.bAllowed = false;
		OutResult.DenyReason = EYIEquipPolicyDenyReason::MissingDefinition;
		OutResult.Message = LOCTEXT("MissingDefinition", "Item definition is missing");
		return false;
	}

	const FYIItemSchemaSnapshot& Snapshot = YIItemSchema::ResolveSnapshot(Definition);
	if (const FYIItemEquipRequirementsFragment* Requirements = Snapshot.FindResolvedFragment<FYIItemEquipRequirementsFragment>())
	{
		if (Context.ActorLevel < Requirements->MinLevel)
		{
			OutResult.bAllowed = false;
			OutResult.DenyReason = EYIEquipPolicyDenyReason::LevelTooLow;
			OutResult.Message = FText::Format(
				LOCTEXT("LevelTooLow", "Requires level {0}"),
				FText::AsNumber(Requirements->MinLevel));
			return false;
		}

		if (!Requirements->RequiredTags.IsEmpty() && !Context.ActorTags.HasAll(Requirements->RequiredTags))
		{
			OutResult.bAllowed = false;
			OutResult.DenyReason = EYIEquipPolicyDenyReason::MissingRequiredTags;
			OutResult.Message = LOCTEXT("MissingRequiredTags", "Missing required equipment tags");
			return false;
		}

		if (!Requirements->BlockedTags.IsEmpty() && Context.ActorTags.HasAny(Requirements->BlockedTags))
		{
			OutResult.bAllowed = false;
			OutResult.DenyReason = EYIEquipPolicyDenyReason::BlockedByTags;
			OutResult.Message = LOCTEXT("BlockedTags", "Item is blocked by current tags");
			return false;
		}
	}

	return true;
}

#undef LOCTEXT_NAMESPACE

