#include "YIGridContainer.h"
#include "YIItemDefinition.h"
#include "YIInventoryBlueprintLibrary.h"

static FIntPoint Local_GetSize(const FYIItemInstance& I)
{
	if (UYIItemDefinition* Def = I.Definition.LoadSynchronous())
	{
		FIntPoint S = Def->GetEffectiveDefaultSize();
		if (I.bRotated) S = FIntPoint(S.Y, S.X);
		return S;
	}
	return I.bRotated ? FIntPoint(1,1) : FIntPoint(1,1);
} 

bool UYIGridContainer::MoveItem(const FGuid& InstanceId, const FIntPoint& NewPos)
{
	for (int32 i=0;i<Items.Num();++i)
	{
		FYIGridEntry Tmp = Items[i];
		if (Tmp.Instance.InstanceId == InstanceId)
		{
			Items.RemoveAtSwap(i);
			bool bOk = CanPlaceAt(NewPos, Tmp.Instance);
			Items.Insert(Tmp, i);
			if (!bOk) return false;
			Items[i].Pos = NewPos;
			return true;
		}
	}
	return false;
}

bool UYIGridContainer::RotateItem(const FGuid& InstanceId)
{
	for (int32 i=0;i<Items.Num();++i)
	{
		FYIGridEntry& E = Items[i];
		if (E.Instance.InstanceId == InstanceId)
		{
			UYIItemDefinition* Def = E.Instance.Definition.LoadSynchronous();
			if (!Def || !Def->IsEffectiveRotationAllowed()) return false;
			// Toggle rotation and validate placement using the definition-derived size
			FYIItemInstance Tmp = E.Instance;
			Tmp.bRotated = !E.Instance.bRotated;
			FYIGridEntry Saved = E;
			Items.RemoveAtSwap(i);
			bool bOk = CanPlaceAt(Saved.Pos, Tmp);
			Items.Insert(Saved, i);
			if (!bOk) return false;
			E.Instance.bRotated = !E.Instance.bRotated;
			// Update key after rotation
			UYIInventoryBlueprintLibrary::UpdateCustomStackKey(E.Instance);
			return true;
		}
	}
	return false;
}

bool UYIGridContainer::CombineStacks(const FGuid& A, const FGuid& B)
{
	int32 IA=-1, IB=-1;
	for (int32 i=0;i<Items.Num();++i)
	{
		if (Items[i].Instance.InstanceId == A) IA=i;
		if (Items[i].Instance.InstanceId == B) IB=i;
	}
	if (IA<0 || IB<0 || IA==IB) return false;
	FYIItemInstance& AInst = Items[IA].Instance;
	FYIItemInstance& BInst = Items[IB].Instance;
	if (AInst.Definition != BInst.Definition || AInst.CustomStackKey != BInst.CustomStackKey) return false;
	UYIItemDefinition* Def = AInst.Definition.LoadSynchronous(); if (!Def) return false;
	int32 Capacity = Def->bAllowStacking ? FMath::Max(1, Def->MaxStackCount) : 1;
	int32 CanAdd = Capacity - AInst.Count;
	if (CanAdd <= 0) return false;
	int32 Moved = FMath::Min(CanAdd, BInst.Count);
	AInst.Count += Moved;
	BInst.Count -= Moved;
	if (BInst.Count <= 0) { Items.RemoveAt(IB); }
	return true;
}

bool UYIGridContainer::SplitStack(const FGuid& InstanceId, int32 Amount)
{
	if (Amount <= 0) return false;
	for (int32 i=0;i<Items.Num();++i)
	{
		FYIGridEntry& E = Items[i];
		if (E.Instance.InstanceId == InstanceId && E.Instance.Count > Amount)
		{
			FYIItemInstance NewInst = E.Instance;
			NewInst.InstanceId = FGuid::NewGuid();
			NewInst.StackId = FGuid::NewGuid();
			NewInst.Count = Amount;
			FIntPoint Pos;
			if (!FindFirstFit(NewInst, Pos)) return false;
			E.Instance.Count -= Amount;
			FYIGridEntry NewE; NewE.Instance = NewInst; NewE.Pos = Pos;
			Items.Add(MoveTemp(NewE));
			return true;
		}
	}
	return false;
}
