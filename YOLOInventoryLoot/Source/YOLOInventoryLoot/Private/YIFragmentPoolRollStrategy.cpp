#include "YIFragmentPoolRollStrategy.h"

#include "YIFragmentAsset.h"
#include "YIFragmentPoolAsset.h"

bool UYIFragmentPoolRollStrategy::ApplyGeneratedFragments_Implementation(const UYIItemDefinition* ItemDefinition, int32 Level, int32 Seed, FYIBagItem& InOutItem) const
{
	(void)ItemDefinition;

	UYIFragmentPoolAsset* FragmentPool = Pool.IsValid() ? Pool.Get() : Pool.LoadSynchronous();
	if (!FragmentPool)
	{
		return false;
	}

	const int32 Low = FMath::Max(0, FMath::Min(MinRolls, MaxRolls));
	const int32 High = FMath::Max(0, FMath::Max(MinRolls, MaxRolls));
	if (High <= 0)
	{
		return false;
	}

	FRandomStream RNG(Seed);
	const int32 Rolls = (High > Low) ? RNG.RandRange(Low, High) : Low;
	if (Rolls <= 0)
	{
		return false;
	}

	bool bAppliedAny = false;
	TSet<const UScriptStruct*> AddedStructTypes;
	for (const FInstancedStruct& Existing : InOutItem.Item.Fragments)
	{
		if (const UScriptStruct* Struct = Existing.GetScriptStruct())
		{
			AddedStructTypes.Add(Struct);
		}
	}

	for (int32 RollIndex = 0; RollIndex < Rolls; ++RollIndex)
	{
		TSoftObjectPtr<UYIFragmentAsset> RolledAssetSoft;
		if (!FragmentPool->RollEntry(Level, RNG.RandRange(1, TNumericLimits<int32>::Max()), RolledAssetSoft))
		{
			continue;
		}

		UYIFragmentAsset* FragmentAsset = RolledAssetSoft.IsValid() ? RolledAssetSoft.Get() : RolledAssetSoft.LoadSynchronous();
		if (!FragmentAsset)
		{
			continue;
		}

		for (const FInstancedStruct& Fragment : FragmentAsset->ItemInstanceFragments)
		{
			const UScriptStruct* StructType = Fragment.GetScriptStruct();
			if (!StructType)
			{
				continue;
			}
			if (bPreventDuplicateStructTypes && AddedStructTypes.Contains(StructType))
			{
				continue;
			}

			InOutItem.Item.Fragments.Add(Fragment);
			AddedStructTypes.Add(StructType);
			bAppliedAny = true;
		}
	}

	return bAppliedAny;
}

