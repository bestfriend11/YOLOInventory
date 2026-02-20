#include "YIGridContainer.h"

#include "YIItemDefinition.h"

static FIntPoint GetSize(const FYIItemInstance& I)
{
	// Size is derived from the definition's DefaultSize; take rotation into account
	if (UYIItemDefinition* Def = I.Definition.LoadSynchronous())
	{
		FIntPoint S = Def->GetEffectiveDefaultSize();
		if (I.bRotated) { S = FIntPoint(S.Y, S.X); }
		return S;
	}
	// Fallback
	return I.bRotated ? FIntPoint(1,1) : FIntPoint(1,1);
} 

bool UYIGridContainer::FindFirstFit(const FYIItemInstance& Instance, FIntPoint& OutPos) const
{
	FIntPoint S = GetSize(Instance);
	for (int32 y=0; y<=GridSize.Y - S.Y; ++y)
	{
		for (int32 x=0; x<=GridSize.X - S.X; ++x)
		{
			FIntPoint P(x,y);
			if (CanPlaceAt(P, Instance)) { OutPos = P; return true; }
		}
	}
	return false;
}

bool UYIGridContainer::CanPlaceAt(const FIntPoint& Pos, const FYIItemInstance& Instance) const
{
	FIntPoint S = GetSize(Instance);
	// bounds
	if (Pos.X < 0 || Pos.Y < 0 || Pos.X + S.X > GridSize.X || Pos.Y + S.Y > GridSize.Y) return false;
	// overlap
	for (const FYIGridEntry& E : Items)
	{
		FIntPoint ES = GetSize(E.Instance);
		const FIntRect A(Pos, Pos + S);
		const FIntRect B(E.Pos, E.Pos + ES);
		if (A.Intersect(B)) return false;
	}
	return true;
}

bool UYIGridContainer::AddItem_Implementation(const FYIItemInstance& Instance)
{
	UYIItemDefinition* Def = Instance.Definition.LoadSynchronous();
	if (!Def)
	{
		return false;
	}

	// Enforce unique-per-type: if Def->bUniquePerType, only allow one stack for this ItemType
	if (Def->bUniquePerType)
	{
		for (const FYIGridEntry& E : Items)
		{
			if (UYIItemDefinition* EDef = E.Instance.Definition.LoadSynchronous())
			{
				if (EDef->ItemType == Def->ItemType)
				{
					return false; // already present
				}
			}
		}
	}

	int32 Remaining = Instance.Count;
	// Try to merge into existing compatible stacks up to MaxStackCount
	for (FYIGridEntry& E : Items)
	{
		if (Remaining <= 0) break;
		if (E.Instance.Definition == Instance.Definition && E.Instance.CustomStackKey == Instance.CustomStackKey)
		{
			const int32 Capacity = Def->IsEffectiveStackingEnabled() ? Def->GetEffectiveMaxStackCount() : 1;
			int32 CanAdd = Capacity - E.Instance.Count;
			if (CanAdd > 0)
			{
				int32 ToAdd = FMath::Min(CanAdd, Remaining);
				E.Instance.Count += ToAdd;
				Remaining -= ToAdd;
			}
		}
	}
	if (Remaining <= 0)
	{
		return true;
	}

	// Place new stacks for any remaining count (respect stack capacity)
	while (Remaining > 0)
	{
		FYIItemInstance NewInst = Instance;
		const int32 Capacity = Def->IsEffectiveStackingEnabled() ? Def->GetEffectiveMaxStackCount() : 1;
		int32 ToPlace = FMath::Min(Capacity, Remaining);
		NewInst.Count = ToPlace;
		// Size is derived from the item definition; rotation flag controls orientation (no per-instance size override)
		FIntPoint Pos;
		if (!FindFirstFit(NewInst, Pos))
		{
			return false; // not enough space for remainder
		}
		FYIGridEntry NewE; NewE.Instance = NewInst; NewE.Pos = Pos;
		Items.Add(MoveTemp(NewE));
		Remaining -= ToPlace;
	}
	return true;
}

bool UYIGridContainer::RemoveItemById_Implementation(const FGuid& InstanceId, int32 Count)
{
	for (int32 i=0;i<Items.Num();++i)
	{
		FYIGridEntry& E = Items[i];
		if (E.Instance.InstanceId == InstanceId)
		{
			if (Count <= 0 || Count >= E.Instance.Count)
			{
				Items.RemoveAt(i);
				return true;
			}
			E.Instance.Count -= Count;
			return true;
		}
	}
	return false;
}

bool UYIGridContainer::TransferTo_Implementation(const TScriptInterface<IYIContainerInterface>& Other, const FGuid& InstanceId, int32 Count)
{
	for (int32 i=0;i<Items.Num();++i)
	{
		FYIGridEntry& E = Items[i];
		if (E.Instance.InstanceId == InstanceId)
		{
			FYIItemInstance Copy = E.Instance;
			if (Count > 0 && Count < Copy.Count)
			{
				Copy.Count = Count;
			}
			if (Other.GetInterface() && IYIContainerInterface::Execute_AddItem(Other.GetObject(), Copy))
			{
				return RemoveItemById_Implementation(InstanceId, Count);
			}
			return false;
		}
	}
	return false;
}
