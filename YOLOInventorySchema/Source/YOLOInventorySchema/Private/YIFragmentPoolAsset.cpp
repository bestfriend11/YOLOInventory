#include "YIFragmentPoolAsset.h"

bool UYIFragmentPoolAsset::RollEntry(int32 Level, int32 Seed, TSoftObjectPtr<UYIFragmentAsset>& OutFragmentAsset) const
{
	OutFragmentAsset = nullptr;

	TArray<const FYIFragmentPoolEntry*> Candidates;
	float TotalWeight = 0.f;
	for (const FYIFragmentPoolEntry& Entry : Entries)
	{
		if (!Entry.FragmentAsset.ToSoftObjectPath().IsValid() || Entry.Weight <= 0.f)
		{
			continue;
		}

		const int32 MinLevel = FMath::Min(Entry.MinLevel, Entry.MaxLevel);
		const int32 MaxLevel = FMath::Max(Entry.MinLevel, Entry.MaxLevel);
		if (Level < MinLevel || Level > MaxLevel)
		{
			continue;
		}

		Candidates.Add(&Entry);
		TotalWeight += Entry.Weight;
	}

	if (Candidates.Num() == 0 || TotalWeight <= 0.f)
	{
		return false;
	}

	FRandomStream RNG(Seed);
	const float Pick = RNG.FRandRange(0.f, TotalWeight);
	float Accum = 0.f;
	for (const FYIFragmentPoolEntry* Entry : Candidates)
	{
		Accum += Entry->Weight;
		if (Pick <= Accum)
		{
			OutFragmentAsset = Entry->FragmentAsset;
			return true;
		}
	}

	OutFragmentAsset = Candidates.Last()->FragmentAsset;
	return OutFragmentAsset.ToSoftObjectPath().IsValid();
}

