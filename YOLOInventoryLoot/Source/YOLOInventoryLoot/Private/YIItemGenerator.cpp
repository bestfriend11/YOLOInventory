#include "YIItemGenerator.h"
#include "YILootTable.h"
#include "YIRarityProfile.h"
#include "YIItemDefinition.h"
#include "YIItemSchemaResolver.h"
#include "YIAffixPoolAsset.h"
#include "YIAffixAsset.h"
#include "YIFragmentRollStrategy.h"
#include "YIItemFeatureResolverRegistry.h"
#include "YIInventoryBlueprintLibrary.h"
#include "YILootPolicyResolver.h"

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

static bool YIArrayContainsStringIgnoreCase(const TArray<FString>& InArray, const FString& Value)
{
	for (const FString& Entry : InArray)
	{
		if (Entry.Equals(Value, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}
	return false;
}

bool UYIItemGenerator::DoesAffixPassCriteria(const UYIAffixAsset* Affix, const UYIItemDefinition* ItemDef, int32 ItemLevel, const FYIAffixRollCriteria& Criteria, EYIAffixKind ExpectedKind)
{
	if (!Affix)
	{
		return false;
	}

	FYIAffixResolvedDefinitionData AffixData;
	Affix->GetEffectiveDefinitionData(AffixData);

	if (AffixData.Kind != ExpectedKind)
	{
		return false;
	}

	if (Criteria.bEnabled)
	{
		const int32 MinTier = FMath::Min(Criteria.MinTier, Criteria.MaxTier);
		const int32 MaxTier = FMath::Max(Criteria.MinTier, Criteria.MaxTier);
		if (AffixData.Tier < MinTier || AffixData.Tier > MaxTier)
		{
			return false;
		}

		int32 MinPower = Criteria.MinPowerLevel;
		int32 MaxPower = Criteria.MaxPowerLevel;
		if (Criteria.bUseItemLevelAsPowerBaseline)
		{
			const int32 LowOffset = FMath::Min(Criteria.MinPowerLevelOffset, Criteria.MaxPowerLevelOffset);
			const int32 HighOffset = FMath::Max(Criteria.MinPowerLevelOffset, Criteria.MaxPowerLevelOffset);
			MinPower = FMath::Max(0, ItemLevel + LowOffset);
			MaxPower = FMath::Max(0, ItemLevel + HighOffset);
		}
		else
		{
			MinPower = FMath::Max(0, FMath::Min(Criteria.MinPowerLevel, Criteria.MaxPowerLevel));
			MaxPower = FMath::Max(0, FMath::Max(Criteria.MinPowerLevel, Criteria.MaxPowerLevel));
		}

		if (AffixData.PowerLevel < MinPower || AffixData.PowerLevel > MaxPower)
		{
			return false;
		}

		const uint8 AffixQuality = static_cast<uint8>(AffixData.Quality);
		const uint8 MinQuality = static_cast<uint8>(Criteria.MinQuality);
		const uint8 MaxQuality = static_cast<uint8>(Criteria.MaxQuality);
		const uint8 QualityLow = FMath::Min(MinQuality, MaxQuality);
		const uint8 QualityHigh = FMath::Max(MinQuality, MaxQuality);
		if (AffixQuality < QualityLow || AffixQuality > QualityHigh)
		{
			return false;
		}

		if (!Criteria.AllowedTemplateIds.IsEmpty() && !YIArrayContainsStringIgnoreCase(Criteria.AllowedTemplateIds, Affix->TemplateId))
		{
			return false;
		}

		if (!Criteria.BlockedTemplateIds.IsEmpty() && YIArrayContainsStringIgnoreCase(Criteria.BlockedTemplateIds, Affix->TemplateId))
		{
			return false;
		}

		if (!AffixData.ConflictGroup.IsNone() && Criteria.BlockedConflictGroups.Contains(AffixData.ConflictGroup))
		{
			return false;
		}
	}

	// Respect item compatibility tags before trying to add.
	if (ItemDef && !AffixData.AllowedItemTags.IsEmpty())
	{
		FGameplayTagContainer EffectiveTags;
		YIItemSchema::GetTags(ItemDef, EffectiveTags);
		if (!EffectiveTags.HasAny(AffixData.AllowedItemTags))
		{
			return false;
		}
	}

	return true;
}

int32 UYIItemGenerator::RollAffixesFromPool(UYIAffixPoolAsset* Pool, const UYIItemDefinition* ItemDef, FYIBagItem& Item, int32 Count, int32 Level, FRandomStream& RNG, const FYIAffixRollCriteria& Criteria, EYIAffixKind ExpectedKind)
{
	if (!Pool || Count <= 0)
	{
		return 0;
	}

	TArray<FYIAffixPoolEntry> Candidates;
	Candidates.Reserve(Pool->Entries.Num());
	float TotalWeight = 0.f;
	for (const FYIAffixPoolEntry& Entry : Pool->Entries)
	{
		if (Entry.Weight <= 0.f)
		{
			continue;
		}
		if (Level < Entry.MinLevel || Level > Entry.MaxLevel)
		{
			continue;
		}

		UYIAffixAsset* Affix = Entry.Affix.IsValid() ? Entry.Affix.Get() : Entry.Affix.LoadSynchronous();
		if (!Affix)
		{
			continue;
		}
		FYIAffixResolvedDefinitionData AffixData;
		Affix->GetEffectiveDefinitionData(AffixData);
		if (static_cast<uint8>(AffixData.Quality) < static_cast<uint8>(Entry.MinQuality))
		{
			continue;
		}
		if (!DoesAffixPassCriteria(Affix, ItemDef, Level, Criteria, ExpectedKind))
		{
			continue;
		}

		Candidates.Add(Entry);
		TotalWeight += Entry.Weight;
	}

	if (Candidates.Num() == 0 || TotalWeight <= 0.f)
	{
		return 0;
	}

	int32 Added = 0;
	int32 Attempts = 0;
	const int32 MaxAttempts = FMath::Max(8, Count * 6);
	while (Added < Count && Attempts < MaxAttempts)
	{
		const float Pick = RNG.FRandRange(0.f, TotalWeight);
		float Accum = 0.f;
		UYIAffixAsset* Affix = nullptr;
		for (const FYIAffixPoolEntry& Entry : Candidates)
		{
			Accum += Entry.Weight;
			if (Pick <= Accum)
			{
				Affix = Entry.Affix.IsValid() ? Entry.Affix.Get() : Entry.Affix.LoadSynchronous();
				break;
			}
		}
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

	if (const TSharedPtr<IYILootPolicyResolver, ESPMode::ThreadSafe> LootResolver =
		FYIItemFeatureResolverRegistry::Get().FindResolverTyped<IYILootPolicyResolver>(YIItemFeatureKeys::LootPolicy))
	{
		FYILootPolicyContext PolicyContext;
		PolicyContext.LootLevel = UseLevel;

		FYILootPolicyResult PolicyResult;
		if (!LootResolver->EvaluateLootPolicy(Def, PolicyContext, PolicyResult))
		{
			return false;
		}

		if (PolicyResult.MinGeneratedCount > 0)
		{
			Count = FMath::Max(Count, PolicyResult.MinGeneratedCount);
		}
		if (PolicyResult.MaxGeneratedCount > 0)
		{
			Count = FMath::Min(Count, PolicyResult.MaxGeneratedCount);
		}
	}

	OutItem.Item.Definition = DefSoft;
	OutItem.Item.Count = FMath::Max(1, Count);
	OutItem.Size = YIItemSchema::GetDefaultSize(Def);

	if (bApplyTemplateAffixes)
	{
		UYIInventoryBlueprintLibrary::ApplyTemplateAffixesToInstance(Def, OutItem.Item);
	}

	UYIFragmentRollStrategy* FragmentStrategy = FragmentRollStrategy.IsValid()
		? FragmentRollStrategy.Get()
		: FragmentRollStrategy.LoadSynchronous();
	auto RunFragmentStrategy = [&]()
	{
		if (FragmentStrategy)
		{
			FragmentStrategy->ApplyGeneratedFragments(Def, UseLevel, Seed ^ 0x31FA, OutItem);
		}
	};

	if (bRunFragmentStrategyBeforeLegacyAffixes)
	{
		RunFragmentStrategy();
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

	if (bUseLegacyAffixGeneration && bGenerateRandomAffixes && (OutPrefixes > 0 || OutSuffixes > 0))
	{
		TArray<TSoftObjectPtr<UYIAffixAsset>> UnusedTemplateAffixes;
		int32 UnusedMinMods = 0;
		int32 UnusedMaxMods = 0;
		TSoftObjectPtr<UYIAffixPoolAsset> EffectivePrefixPool;
		TSoftObjectPtr<UYIAffixPoolAsset> EffectiveSuffixPool;
		const FYIItemSchemaSnapshot& Snapshot = YIItemSchema::ResolveSnapshot(Def);
		UnusedMinMods = Snapshot.Affix.MinRandomModifiers;
		UnusedMaxMods = Snapshot.Affix.MaxRandomModifiers;
		for (const FSoftObjectPath& Path : Snapshot.Affix.TemplateAffixes)
		{
			UnusedTemplateAffixes.Add(TSoftObjectPtr<UYIAffixAsset>(Path));
		}
		EffectivePrefixPool = TSoftObjectPtr<UYIAffixPoolAsset>(Snapshot.Affix.PrefixPool);
		EffectiveSuffixPool = TSoftObjectPtr<UYIAffixPoolAsset>(Snapshot.Affix.SuffixPool);
		(void)UnusedTemplateAffixes;
		(void)UnusedMinMods;
		(void)UnusedMaxMods;

		const TSoftObjectPtr<UYIAffixPoolAsset> PrefixPoolSoft =
			ResolvePoolOverride(RarityRule.PrefixPoolOverride, PrefixPoolOverride, EffectivePrefixPool, bUseDefinitionAffixPools);
		const TSoftObjectPtr<UYIAffixPoolAsset> SuffixPoolSoft =
			ResolvePoolOverride(RarityRule.SuffixPoolOverride, SuffixPoolOverride, EffectiveSuffixPool, bUseDefinitionAffixPools);

		FRandomStream RNG(Seed ^ 0x1234);
		UYIAffixPoolAsset* PrefixPool = PrefixPoolSoft.IsValid() ? PrefixPoolSoft.Get() : PrefixPoolSoft.LoadSynchronous();
		UYIAffixPoolAsset* SuffixPool = SuffixPoolSoft.IsValid() ? SuffixPoolSoft.Get() : SuffixPoolSoft.LoadSynchronous();

		OutPrefixes = RollAffixesFromPool(PrefixPool, Def, OutItem, OutPrefixes, UseLevel, RNG, PrefixCriteria, EYIAffixKind::Prefix);
		OutSuffixes = RollAffixesFromPool(SuffixPool, Def, OutItem, OutSuffixes, UseLevel, RNG, SuffixCriteria, EYIAffixKind::Suffix);
	}

	if (!bRunFragmentStrategyBeforeLegacyAffixes)
	{
		RunFragmentStrategy();
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
