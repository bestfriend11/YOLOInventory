#include "YIWorldPickupPolicyResolver.h"

#include "YIItemDefinition.h"
#include "YIItemSchemaResolver.h"
#include "YIWorldPolicyFragments.h"

#define LOCTEXT_NAMESPACE "YIWorldPickupPolicyResolver"

bool FYIDefaultWorldPickupPolicyResolver::EvaluatePickupPolicy(const FYIItemInstance& Item, const FYIWorldPickupPolicyContext& Context, FYIWorldPickupPolicyResult& OutResult) const
{
	OutResult = FYIWorldPickupPolicyResult();
	OutResult.bAllowed = true;

	const UYIItemDefinition* Definition = Item.Definition.IsValid() ? Item.Definition.Get() : Item.Definition.LoadSynchronous();
	if (!Definition)
	{
		return true;
	}

	const FYIItemSchemaSnapshot& Snapshot = YIItemSchema::ResolveSnapshot(Definition);
	if (const FYIItemPickupPolicyFragment* Policy = Snapshot.FindResolvedFragment<FYIItemPickupPolicyFragment>())
	{
		OutResult.bAutoPickup = Policy->bAutoPickup;
		if (!Policy->bAllowPickup)
		{
			OutResult.bAllowed = false;
			OutResult.DenyReason = EYIWorldPickupDenyReason::PickupDisabled;
			OutResult.Message = LOCTEXT("PickupDisabled", "Item cannot be picked up");
			return false;
		}

		if (Policy->bOwnerOnlyPickup && !Context.bIsOwner)
		{
			OutResult.bAllowed = false;
			OutResult.DenyReason = EYIWorldPickupDenyReason::OwnerOnly;
			OutResult.Message = LOCTEXT("OwnerOnly", "Only owner can pick up this item");
			return false;
		}

		if (!Policy->RequiredPickupTags.IsEmpty() && !Context.PickerTags.HasAll(Policy->RequiredPickupTags))
		{
			OutResult.bAllowed = false;
			OutResult.DenyReason = EYIWorldPickupDenyReason::MissingRequiredTags;
			OutResult.Message = LOCTEXT("MissingRequiredTags", "Missing required pickup tags");
			return false;
		}

		if (!Policy->BlockedPickupTags.IsEmpty() && Context.PickerTags.HasAny(Policy->BlockedPickupTags))
		{
			OutResult.bAllowed = false;
			OutResult.DenyReason = EYIWorldPickupDenyReason::BlockedByTags;
			OutResult.Message = LOCTEXT("BlockedByTags", "Pickup blocked by tags");
			return false;
		}
	}

	return true;
}

#undef LOCTEXT_NAMESPACE

