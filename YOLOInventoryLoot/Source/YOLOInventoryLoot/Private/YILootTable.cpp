#include "YILootTable.h"
#include "YIItemDefinition.h"

bool UYILootTable::GetEligibleEntries(int32 Level, TArray<const FYILootTableEntry*>& OutEntries) const
{
	OutEntries.Reset();
	for (const FYILootTableEntry& Entry : Entries)
	{
		if (Entry.Weight <= 0.f)
		{
			continue;
		}
		if (Level < Entry.MinLevel || Level > Entry.MaxLevel)
		{
			continue;
		}
		if (!Entry.Definition.ToSoftObjectPath().IsValid())
		{
			continue;
		}
		if (!Entry.RequiredTags.IsEmpty())
		{
			UYIItemDefinition* Def = Entry.Definition.IsValid() ? Entry.Definition.Get() : Entry.Definition.LoadSynchronous();
			if (!Def || !Def->Tags.HasAny(Entry.RequiredTags))
			{
				continue;
			}
		}
		OutEntries.Add(&Entry);
	}
	return OutEntries.Num() > 0;
}

bool UYILootTable::RollDefinition(int32 Level, int32 Seed, TSoftObjectPtr<UYIItemDefinition>& OutDefinition, int32& OutCount) const
{
	OutDefinition = nullptr;
	OutCount = 0;

	TArray<const FYILootTableEntry*> Eligible;
	if (!GetEligibleEntries(Level, Eligible))
	{
		return false;
	}

	float TotalWeight = 0.f;
	for (const FYILootTableEntry* Entry : Eligible)
	{
		TotalWeight += Entry ? Entry->Weight : 0.f;
	}
	if (TotalWeight <= 0.f)
	{
		return false;
	}

	FRandomStream RNG(Seed);
	const float Pick = RNG.FRandRange(0.f, TotalWeight);
	float Accum = 0.f;
	const FYILootTableEntry* Chosen = nullptr;
	for (const FYILootTableEntry* Entry : Eligible)
	{
		if (!Entry) { continue; }
		Accum += Entry->Weight;
		if (Pick <= Accum)
		{
			Chosen = Entry;
			break;
		}
	}
	if (!Chosen)
	{
		Chosen = Eligible.Last();
	}

	OutDefinition = Chosen->Definition;
	const int32 MinCount = FMath::Max(1, Chosen->MinCount);
	const int32 MaxCount = FMath::Max(MinCount, Chosen->MaxCount);
	OutCount = (MaxCount > MinCount) ? RNG.RandRange(MinCount, MaxCount) : MinCount;
	return OutDefinition.ToSoftObjectPath().IsValid();
}
