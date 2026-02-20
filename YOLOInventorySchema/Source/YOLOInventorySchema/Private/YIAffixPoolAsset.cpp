#include "YIAffixPoolAsset.h"

UYIAffixAsset* UYIAffixPoolAsset::SampleAffix(FRandomStream& RNG, int32 Level) const
{
    // Build a filtered list and compute total weight
    float Total = 0.f;
    for (const FYIAffixPoolEntry& E : Entries)
    {
        if (E.Weight <= 0.f) continue;
        if (Level < E.MinLevel || Level > E.MaxLevel) continue;
        // Filter by minimum affix quality if set
        UYIAffixAsset* Candidate = E.Affix.IsValid() ? E.Affix.Get() : E.Affix.LoadSynchronous();
        if (!Candidate) continue;
        if (static_cast<uint8>(Candidate->Quality) < static_cast<uint8>(E.MinQuality)) continue;
        Total += E.Weight;
    }
    if (Total <= 0.f) return nullptr;

    float Pick = RNG.FRandRange(0.f, Total);
    float Acc = 0.f;
    for (const FYIAffixPoolEntry& E : Entries)
    {
        if (E.Weight <= 0.f) continue;
        if (Level < E.MinLevel || Level > E.MaxLevel) continue;
        UYIAffixAsset* Candidate = E.Affix.IsValid() ? E.Affix.Get() : E.Affix.LoadSynchronous();
        if (!Candidate) continue;
        if (static_cast<uint8>(Candidate->Quality) < static_cast<uint8>(E.MinQuality)) continue;
        Acc += E.Weight;
        if (Pick <= Acc)
        {
            return Candidate;
        }
    }
    return nullptr;
}