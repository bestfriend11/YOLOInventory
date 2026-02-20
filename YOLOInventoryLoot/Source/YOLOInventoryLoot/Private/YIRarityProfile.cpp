#include "YIRarityProfile.h"

bool UYIRarityProfile::RollRarity(int32 Level, int32 Seed, FYIRarityRule& OutRule) const
{
	TArray<const FYIRarityRule*> Eligible;
	for (const FYIRarityRule& Rule : Rules)
	{
		if (Rule.Weight <= 0.f)
		{
			continue;
		}
		if (Level < Rule.MinLevel || Level > Rule.MaxLevel)
		{
			continue;
		}
		Eligible.Add(&Rule);
	}
	if (Eligible.Num() == 0)
	{
		return false;
	}

	float TotalWeight = 0.f;
	for (const FYIRarityRule* Rule : Eligible)
	{
		TotalWeight += Rule ? Rule->Weight : 0.f;
	}
	if (TotalWeight <= 0.f)
	{
		return false;
	}

	FRandomStream RNG(Seed);
	const float Pick = RNG.FRandRange(0.f, TotalWeight);
	float Accum = 0.f;
	const FYIRarityRule* Chosen = nullptr;
	for (const FYIRarityRule* Rule : Eligible)
	{
		if (!Rule) { continue; }
		Accum += Rule->Weight;
		if (Pick <= Accum)
		{
			Chosen = Rule;
			break;
		}
	}
	if (!Chosen)
	{
		Chosen = Eligible.Last();
	}
	OutRule = *Chosen;
	return true;
}
