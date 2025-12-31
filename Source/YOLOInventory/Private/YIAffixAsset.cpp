#include "YIAffixAsset.h"

#if WITH_EDITOR
#include "Misc/MTAccessDetector.h"
#endif

#if WITH_EDITOR
float UYIAffixAsset::SampleRoll(int32 Level, int32 Seed) const
{
    if (MinValue == MaxValue)
    {
        return MinValue;
    }
    // Deterministic rng
    FRandomStream Stream(Seed);
    float Base = Stream.FRandRange(MinValue, MaxValue);
    if (ValueByLevel.GetRichCurveConst())
    {
        Base *= ValueByLevel.GetRichCurveConst()->Eval(Level, 1.f);
    }
    return Base;
}
#endif
