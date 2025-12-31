#include "YIEvolutionLibrary.h"
#include "YIEvolutionPath.h"
#include "YIItemDefinition.h"

TArray<UYIItemDefinition*> UYIEvolutionLibrary::EvaluateEvolution(const UYIEvolutionPath* Path, const TMap<FName,float>& Attributes, const FGameplayTagContainer& OwnedTags, int32 XP)
{
	TArray<UYIItemDefinition*> Out;
	if (!Path) return Out;
	for (const FYIEvoEdge& E : Path->Edges)
	{
		if (E.From == INDEX_NONE || E.To == INDEX_NONE) continue;
		if (!Path->Nodes.IsValidIndex(E.To)) continue;
		// Unlock criteria removed; accept edges as valid transitions
		if (UYIItemDefinition* Def = Path->Nodes[E.To].Item.LoadSynchronous())
		{
			Out.Add(Def);
			if (Path->Policy == EYIEvolutionPolicy::FirstValid) break;
		}
	}
	if (Path->Policy == EYIEvolutionPolicy::HighestIndex && Out.Num() > 1)
	{
		// Take the last encountered definition to approximate highest index policy
		UYIItemDefinition* Last = Out.Last();
		Out.Reset(1);
		Out.Add(Last);
	}
	return Out;
}
