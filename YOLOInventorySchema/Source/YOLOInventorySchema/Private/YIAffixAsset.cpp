#include "YIAffixAsset.h"

#if WITH_EDITOR
#include "Misc/MTAccessDetector.h"
#endif

void UYIAffixAsset::GetEffectiveDefinitionData(FYIAffixResolvedDefinitionData& OutData) const
{
	OutData.DisplayName = DisplayName;
	OutData.Description = Description;
	OutData.TooltipFormat = TooltipFormat;
	OutData.Kind = Kind;
	OutData.Quality = Quality;
	OutData.Tier = Tier;
	OutData.Weight = Weight;
	OutData.AttributeMods = AttributeMods;
	OutData.MinValue = MinValue;
	OutData.MaxValue = MaxValue;
	OutData.PowerLevel = PowerLevel;
	OutData.ValueByLevel = ValueByLevel;
	OutData.AllowedItemTags = AllowedItemTags;
	OutData.ConflictGroup = ConflictGroup;
}

#if WITH_EDITOR
float UYIAffixAsset::SampleRoll(int32 Level, int32 Seed) const
{
	FYIAffixResolvedDefinitionData Effective;
	GetEffectiveDefinitionData(Effective);

    if (Effective.MinValue == Effective.MaxValue)
    {
        return Effective.MinValue;
    }
    // Deterministic rng
    FRandomStream Stream(Seed);
    float Base = Stream.FRandRange(Effective.MinValue, Effective.MaxValue);
    if (Effective.ValueByLevel.GetRichCurveConst())
    {
        Base *= Effective.ValueByLevel.GetRichCurveConst()->Eval(Level, 1.f);
    }
	Base *= FMath::Max(1, Effective.PowerLevel);
    return Base;
}
#endif
