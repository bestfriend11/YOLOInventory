#include "YIItemGenerator.h"
#include "YILootTable.h"
#include "YIRarityProfile.h"
#include "YIItemDefinition.h"
#include "YIAffixPoolAsset.h"
#include "YIInventoryBlueprintLibrary.h"

static int32 ClampLevel(int32 Level, int32 MinLevel, int32 MaxLevel)
{
	const int32 ClampedMin = FMath::Max(1, MinLevel);
	const int32 ClampedMax = FMath::Max(ClampedMin, MaxLevel);
	return FMath::Clamp(Level, ClampedMin, ClampedMax);
}

TSoftObjectPtr<UYIAffixPoolAsset> UYIItemGenerator::ResolvePoolOverride(
	const TSoftObjectPtr<UYIAffixPoolAsset>& RuleOverride,
	const TSoftObjectPtr<UYIAffixPoolAsset>& GeneratorOverride,
	const TSoftObjectPtr<UYIAffixPoolAsset>& DefinitionPool,
	bool bUseDefinitionPools)
{
	if (RuleOverride.ToSoftObjectPath().IsValid())
	{
		return RuleOverride;
	}
	if (GeneratorOverride.ToSoftObjectPath().IsValid())
	{
		return GeneratorOverride;
	}
	return bUseDefinitionPools ? DefinitionPool : nullptr;
}

int32 UYIItemGenerator::RollAffixesFromPool(UYIAffixPoolAsset* Pool, FYIBagItem& Item, int32 Count, int32 Level, FRandomStream& RNG)
{
	if (!Pool || Count <= 0)
	{
		return 0;
	}

	int32 Added = 0;
	int32 Attempts = 0;
	const int32 MaxAttempts = FMath::Max(8, Count * 6);
	while (Added < Count && Attempts < MaxAttempts)
	{
		UYIAffixAsset* Affix = Pool->SampleAffix(RNG, Level);
		if (!Affix)
		{
			++Attempts;
			continue;
		}

		const int32 Seed = RNG.RandRange(1, INT32_MAX);
		float Rolled = 0.f;
		if (UYIInventoryBlueprintLibrary::AddRolledAffix(Item, Affix, Level, Seed, Rolled))
		{
			++Added;
		}
		++Attempts;
	}
	return Added;
}

bool UYIItemGenerator::GenerateItem(int32 Level, int32 Seed, FYIBagItem& OutItem, FGameplayTag& OutRarity, int32& OutPrefixes, int32& OutSuffixes) const
{
	OutItem = FYIBagItem();
	OutRarity = DefaultRarityTag;
	OutPrefixes = 0;
	OutSuffixes = 0;

	UYILootTable* Table = LootTable.IsValid() ? LootTable.Get() : LootTable.LoadSynchronous();
	if (!Table)
	{
		return false;
	}

	const int32 UseLevel = bClampLevel ? ClampLevel(Level, MinItemLevel, MaxItemLevel) : Level;
	TSoftObjectPtr<UYIItemDefinition> DefSoft;
	int32 Count = 0;
	if (!Table->RollDefinition(UseLevel, Seed, DefSoft, Count))
	{
		return false;
	}

	UYIItemDefinition* Def = DefSoft.IsValid() ? DefSoft.Get() : DefSoft.LoadSynchronous();
	if (!Def)
	{
		return false;
	}

	OutItem.Item.Definition = DefSoft;
	OutItem.Item.Count = FMath::Max(1, Count);
	OutItem.Size = Def->DefaultSize;

	if (bApplyTemplateAffixes)
	{
		UYIInventoryBlueprintLibrary::ApplyTemplateAffixesToInstance(Def, OutItem.Item);
	}

	FYIRarityRule RarityRule;
	if (UYIRarityProfile* Profile = RarityProfile.IsValid() ? RarityProfile.Get() : RarityProfile.LoadSynchronous())
	{
		if (Profile->RollRarity(UseLevel, Seed ^ 0xA51C, RarityRule))
		{
			OutRarity = RarityRule.RarityTag.IsValid() ? RarityRule.RarityTag : Profile->DefaultRarityTag;
			OutPrefixes = (RarityRule.MaxPrefixes > RarityRule.MinPrefixes)
				? FRandomStream(Seed ^ 0xD1A4).RandRange(RarityRule.MinPrefixes, RarityRule.MaxPrefixes)
				: RarityRule.MinPrefixes;
			OutSuffixes = (RarityRule.MaxSuffixes > RarityRule.MinSuffixes)
				? FRandomStream(Seed ^ 0xBEEF).RandRange(RarityRule.MinSuffixes, RarityRule.MaxSuffixes)
				: RarityRule.MinSuffixes;
		}
	}

	if (bGenerateRandomAffixes && (OutPrefixes > 0 || OutSuffixes > 0))
	{
		const TSoftObjectPtr<UYIAffixPoolAsset> PrefixPoolSoft =
			ResolvePoolOverride(RarityRule.PrefixPoolOverride, PrefixPoolOverride, Def->PrefixPool, bUseDefinitionAffixPools);
		const TSoftObjectPtr<UYIAffixPoolAsset> SuffixPoolSoft =
			ResolvePoolOverride(RarityRule.SuffixPoolOverride, SuffixPoolOverride, Def->SuffixPool, bUseDefinitionAffixPools);

		FRandomStream RNG(Seed ^ 0x1234);
		UYIAffixPoolAsset* PrefixPool = PrefixPoolSoft.IsValid() ? PrefixPoolSoft.Get() : PrefixPoolSoft.LoadSynchronous();
		UYIAffixPoolAsset* SuffixPool = SuffixPoolSoft.IsValid() ? SuffixPoolSoft.Get() : SuffixPoolSoft.LoadSynchronous();

		OutPrefixes = RollAffixesFromPool(PrefixPool, OutItem, OutPrefixes, UseLevel, RNG);
		OutSuffixes = RollAffixesFromPool(SuffixPool, OutItem, OutSuffixes, UseLevel, RNG);
	}

	UYIInventoryBlueprintLibrary::UpdateCustomStackKey(OutItem.Item);
	return true;
}

FYIItemGenerationResult UYIItemGenerator::GenerateItemResult(int32 Level, int32 Seed) const
{
	FYIItemGenerationResult Result;
	Result.bSuccess = GenerateItem(Level, Seed, Result.Item, Result.RarityTag, Result.NumPrefixes, Result.NumSuffixes);
	return Result;
}
