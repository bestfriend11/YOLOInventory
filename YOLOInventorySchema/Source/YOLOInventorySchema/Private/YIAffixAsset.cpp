#include "YIAffixAsset.h"

#if WITH_EDITOR
#include "Misc/MTAccessDetector.h"
#endif

namespace
{
	template<typename TFragment>
	static const TFragment* YI_FindAffixDefinitionFragment(const TArray<FInstancedStruct>& Fragments)
	{
		for (const FInstancedStruct& Fragment : Fragments)
		{
			if (const TFragment* Value = Fragment.GetPtr<TFragment>())
			{
				return Value;
			}
		}
		return nullptr;
	}
}

const FYIStaticAffixDefinitionFragment* UYIAffixAsset::GetStaticDefinitionFragment() const
{
	return YI_FindAffixDefinitionFragment<FYIStaticAffixDefinitionFragment>(DefinitionFragments);
}

const FInstancedStruct* UYIAffixAsset::FindDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct) const
{
	if (!FragmentStruct)
	{
		return nullptr;
	}

	for (const FInstancedStruct& Fragment : DefinitionFragments)
	{
		if (Fragment.GetScriptStruct() == FragmentStruct)
		{
			return &Fragment;
		}
	}
	return nullptr;
}

FInstancedStruct* UYIAffixAsset::FindOrAddDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct)
{
	if (!FragmentStruct)
	{
		return nullptr;
	}

	for (FInstancedStruct& Fragment : DefinitionFragments)
	{
		if (Fragment.GetScriptStruct() == FragmentStruct)
		{
			return &Fragment;
		}
	}

	FInstancedStruct& NewFragment = DefinitionFragments.AddDefaulted_GetRef();
	NewFragment.InitializeAs(FragmentStruct);
	return &NewFragment;
}

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

	if (const FYIStaticAffixDefinitionFragment* Fragment = GetStaticDefinitionFragment())
	{
		if (!Fragment->bOverrideLegacyFields)
		{
			return;
		}

		OutData.DisplayName = Fragment->DisplayName;
		OutData.Description = Fragment->Description;
		OutData.TooltipFormat = Fragment->TooltipFormat;
		OutData.Kind = Fragment->Kind;
		OutData.Quality = Fragment->Quality;
		OutData.Tier = Fragment->Tier;
		OutData.Weight = Fragment->Weight;
		OutData.AttributeMods = Fragment->AttributeMods;
		OutData.MinValue = Fragment->MinValue;
		OutData.MaxValue = Fragment->MaxValue;
		OutData.PowerLevel = Fragment->PowerLevel;
		OutData.ValueByLevel = Fragment->ValueByLevel;
		OutData.AllowedItemTags = Fragment->AllowedItemTags;
		OutData.ConflictGroup = Fragment->ConflictGroup;
	}
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
