#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "YIInventoryBlueprintLibrary.h"
#include "InventoryUtils.h"
#include "UObject/Package.h"

void UYIInventoryBag::PostLoad()
{
	Super::PostLoad();
	EnsureBagId();
	for (FYIBagItem& ItemEntry : Items)
	{
		if (!ItemEntry.Item.InstanceId.IsValid())
		{
			ItemEntry.Item.InstanceId = FGuid::NewGuid();
		}
		if (!ItemEntry.Item.StackId.IsValid())
		{
			ItemEntry.Item.StackId = FGuid::NewGuid();
		}
	}
}

void UYIInventoryBag::EnsureBagId()
{
	if (!BagId.IsValid())
	{
		BagId = FGuid::NewGuid();
	}
}

bool UYIInventoryBag::CanAcceptItemDefinition(const UYIItemDefinition* Definition) const
{
	if (!Definition)
	{
		return false;
	}

	if (!bEnforceAcceptanceRules)
	{
		return true;
	}

	// Optional class-level filter.
	if (AllowedDefinitionClasses.Num() > 0)
	{
		bool bAllowedClass = false;
		for (const TSoftClassPtr<UYIItemDefinition>& AllowedClassPtr : AllowedDefinitionClasses)
		{
			UClass* AllowedClass = AllowedClassPtr.IsValid() ? AllowedClassPtr.Get() : AllowedClassPtr.LoadSynchronous();
			if (AllowedClass && Definition->IsA(AllowedClass))
			{
				bAllowedClass = true;
				break;
			}
		}
		if (!bAllowedClass)
		{
			return false;
		}
	}

	// ItemType allow-list with hierarchical match (e.g. Item.Spell accepts Item.Spell.Fire).
	if (AllowedItemTypes.Num() > 0)
	{
		const FGameplayTag ItemType = Definition->ItemType;
		bool bTypeAllowed = false;
		if (ItemType.IsValid())
		{
			for (const FGameplayTag& AllowedType : AllowedItemTypes)
			{
				if (ItemType.MatchesTag(AllowedType))
				{
					bTypeAllowed = true;
					break;
				}
			}
		}
		if (!bTypeAllowed)
		{
			return false;
		}
	}

	FGameplayTagContainer EffectiveTags = Definition->Tags;
	if (Definition->ItemType.IsValid())
	{
		EffectiveTags.AddTag(Definition->ItemType);
	}

	if (RequiredItemTags.Num() > 0 && !EffectiveTags.HasAll(RequiredItemTags))
	{
		return false;
	}

	if (BlockedItemTags.Num() > 0 && EffectiveTags.HasAny(BlockedItemTags))
	{
		return false;
	}

	return true;
}

FIntPoint UYIInventoryBag::GetEffectiveSize(const FIntPoint InSize) const
{
	FVector2D S = FVector2D(InSize) * FMath::Clamp(MinifyScale, 0.1f, 1.0f);
	FIntPoint Out(FMath::Max(1, FMath::RoundToInt(S.X)), FMath::Max(1, FMath::RoundToInt(S.Y)));
	return Out;
}

bool UYIInventoryBag::CanPlaceAt(const FIntPoint Pos, const FIntPoint Size) const
{
	if (GridSize.X <= 0 || GridSize.Y <= 0)
	{
		// list mode: no spatial constraint
		return true;
	}
	// Always evaluate using effective sizes with current MinifyScale
	const FIntPoint EffSize = GetEffectiveSize(Size);
	if (Pos.X < 0 || Pos.Y < 0 || Pos.X + EffSize.X > GridSize.X || Pos.Y + EffSize.Y > GridSize.Y)
	{
		return false;
	}
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		const FYIBagItem& It = Items[i];
		const FIntPoint OtherEff = GetEffectiveSize(It.Size);
		if (RectsOverlap(Pos, EffSize, It.Pos, OtherEff))
		{
			return false;
		}
	}
	return true;
}

bool UYIInventoryBag::MoveItem(int32 Index, const FIntPoint NewPos)
{
	if (!Items.IsValidIndex(Index)) return false;
	FYIBagItem Tmp = Items[Index];
	// Temporarily remove for overlap test
	Items.RemoveAt(Index);
	const bool bCan = CanPlaceAt(NewPos, Tmp.Size);
	Items.Insert(Tmp, Index);
	if (!bCan) return false;
	Items[Index].Pos = NewPos;
	if (ShouldMarkDirty()) { MarkPackageDirty(); }
	OnChanged.Broadcast();
	OnItemMoved.Broadcast(Index, NewPos);
	return true;
}

int32 UYIInventoryBag::FindExistingStackIndex(UYIItemDefinition* Definition) const
{
	if (!Definition) return INDEX_NONE;
	for (int32 i=0;i<Items.Num();++i)
	{
		if (Items[i].Item.Definition.ToSoftObjectPath() == TSoftObjectPtr<UYIItemDefinition>(Definition).ToSoftObjectPath())
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int32 UYIInventoryBag::FindExistingStackIndexForItem(const FYIBagItem& NewItem) const
{
	if (NewItem.Item.CustomStackKey == 0) return INDEX_NONE;
	for (int32 i=0;i<Items.Num();++i)
	{
		if (Items[i].Item.Definition.ToSoftObjectPath() == NewItem.Item.Definition.ToSoftObjectPath()
			&& Items[i].Item.CustomStackKey == NewItem.Item.CustomStackKey)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

int32 UYIInventoryBag::AddBagItem(const FYIBagItem& NewItem)
{
	UYIItemDefinition* Def = NewItem.Item.Definition.IsValid() ? NewItem.Item.Definition.Get() : NewItem.Item.Definition.LoadSynchronous();
	if (!Def)
	{
		return INDEX_NONE;
	}

	if (!CanAcceptItemDefinition(Def))
	{
		return INDEX_NONE;
	}

	FYIBagItem NormalizedNewItem = NewItem;
	if (Def->bIsContainerItem)
	{
		NormalizedNewItem.Item.Count = 1;
	}
	if (!NormalizedNewItem.Item.InstanceId.IsValid())
	{
		NormalizedNewItem.Item.InstanceId = FGuid::NewGuid();
	}
	if (!NormalizedNewItem.Item.StackId.IsValid())
	{
		NormalizedNewItem.Item.StackId = FGuid::NewGuid();
	}

	// Enforce Dungeon Siege style uniqueness-per-type: only one stack per ItemType in this bag when flagged on the incoming item
	if (Def->bUniquePerType)
	{
		for (const FYIBagItem& E : Items)
		{
			if (UYIItemDefinition* EDef = (E.Item.Definition.IsValid() ? E.Item.Definition.Get() : E.Item.Definition.LoadSynchronous()))
			{
				if (EDef->ItemType == Def->ItemType)
				{
					// Another item of the same ItemType already exists; reject placement
					return INDEX_NONE;
				}
			}
		}
	}

	// Stacking logic: if enabled and there is an existing stack and stacking allowed, try to merge; otherwise create another stack
	if (bAutoMergeOnAdd && !Def->bIsContainerItem)
	{
		int32 Existing = INDEX_NONE;
		// Prefer matching by per-instance stack key when present
		if (NormalizedNewItem.Item.CustomStackKey != 0)
		{
			Existing = FindExistingStackIndexForItem(NormalizedNewItem);
		}
		// Fallback to legacy definition-only match if no key match
		if (Existing == INDEX_NONE)
		{
			Existing = FindExistingStackIndex(Def);
		}
		if (Existing != INDEX_NONE && Def->IsRuntimeStackingAllowed())
		{
			int32 Room = Def->MaxStackCount - Items[Existing].Item.Count;
			if (Room > 0)
			{
				Items[Existing].Item.Count = FMath::Clamp(Items[Existing].Item.Count + FMath::Max(1, NewItem.Item.Count), 1, Def->MaxStackCount);
				if (ShouldMarkDirty()) { MarkPackageDirty(); } OnChanged.Broadcast();				return Existing;
			}
			// If stack is full, fall through to creating a new stack of the same item
		}
	}
	// Place into grid (first-fit if needed)
	FYIBagItem Copy = NormalizedNewItem;
	if (!CanPlaceAt(Copy.Pos, Copy.Size))
	{
		FIntPoint Fit;
		if (!FindFirstFit(Copy.Size, Fit)) { return INDEX_NONE; }
		Copy.Pos = Fit;
	}
	// Clamp count to MaxStackCount if stacking
	if (Def->IsRuntimeStackingAllowed())
	{
		Copy.Item.Count = FMath::Clamp(Copy.Item.Count, 1, Def->MaxStackCount);
	}
	int32 OutIndex = Items.Add(Copy);
	if (ShouldMarkDirty()) { MarkPackageDirty(); } OnChanged.Broadcast();
	OnItemAdded.Broadcast(OutIndex, Items[OutIndex]);
	return OutIndex;
}

bool UYIInventoryBag::CombineStacks(int32 IndexA, int32 IndexB)
{
	if (!Items.IsValidIndex(IndexA) || !Items.IsValidIndex(IndexB) || IndexA == IndexB) return false;
	FYIBagItem& A = Items[IndexA];
	FYIBagItem& B = Items[IndexB];
	UYIItemDefinition* DefA = A.Item.Definition.IsValid()?A.Item.Definition.Get():A.Item.Definition.LoadSynchronous();
	UYIItemDefinition* DefB = B.Item.Definition.IsValid()?B.Item.Definition.Get():B.Item.Definition.LoadSynchronous();
	if (!DefA || !DefB) return false;
	if (DefA != DefB) return false;
	if (!DefA->IsRuntimeStackingAllowed()) return false;
	int32 Room = DefA->MaxStackCount - A.Item.Count;
	if (Room <= 0) return false;
	int32 Moved = FMath::Min(Room, B.Item.Count);
	A.Item.Count += Moved;
	B.Item.Count -= Moved;
	if (B.Item.Count <= 0)
	{
		FYIBagItem RemovedB = B;
		Items.RemoveAt(IndexB);
		if (ShouldMarkDirty()) { MarkPackageDirty(); } OnChanged.Broadcast();
		OnItemRemoved.Broadcast(IndexB, RemovedB);
	}
	else
	{
		if (ShouldMarkDirty()) { MarkPackageDirty(); } OnChanged.Broadcast();
	}
	return Moved > 0;
}

int32 UYIInventoryBag::SplitStack(int32 Index, int32 Amount, const FIntPoint Position)
{
	if (!Items.IsValidIndex(Index) || Amount <= 0) return INDEX_NONE;
	FYIBagItem& Src = Items[Index];
	UYIItemDefinition* Def = Src.Item.Definition.IsValid()?Src.Item.Definition.Get():Src.Item.Definition.LoadSynchronous();
	if (!Def || !Def->IsRuntimeStackingAllowed()) return INDEX_NONE;
	if (Src.Item.Count <= Amount) return INDEX_NONE;
	FYIBagItem New = Src;
	New.Item.Count = Amount;
	New.Pos = Position;
	if (!CanPlaceAt(Position, New.Size))
	{
		FIntPoint Fit; if (!FindFirstFit(New.Size, Fit)) return INDEX_NONE; New.Pos = Fit;
	}
	Src.Item.Count -= Amount;
	int32 OutIdx = Items.Add(New);
	if (ShouldMarkDirty()) { MarkPackageDirty(); } OnChanged.Broadcast();
	return OutIdx;
}

bool UYIInventoryBag::CanPlaceAtWithScale(const FIntPoint Pos, const FIntPoint Size) const
{
	return CanPlaceAt(Pos, GetEffectiveSize(Size));
}

bool UYIInventoryBag::ShouldMarkDirty() const
{
	// Skip dirtying in PIE/runtime instances (duplicates) and transient objects
	if (HasAnyFlags(RF_Transient | RF_DuplicateTransient))
	{
		return false;
	}
	if (const UPackage* Package = GetOutermost())
	{
		if (Package->HasAnyPackageFlags(PKG_PlayInEditor | PKG_ContainsMapData))
		{
			return false;
		}
	}
	return true;
}

bool UYIInventoryBag::CanPlaceAtIgnoring(const FIntPoint& Pos, const FIntPoint& Size, int32 IgnoreIndex) const
{
	if (GridSize.X <= 0 || GridSize.Y <= 0)
	{
		return true;
	}
	const FIntPoint EffSize = GetEffectiveSize(Size);
	if (Pos.X < 0 || Pos.Y < 0 || Pos.X + EffSize.X > GridSize.X || Pos.Y + EffSize.Y > GridSize.Y)
	{
		return false;
	}
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (i == IgnoreIndex) continue;
		const FYIBagItem& It = Items[i];
		const FIntPoint OtherEff = GetEffectiveSize(It.Size);
		if (RectsOverlap(Pos, EffSize, It.Pos, OtherEff))
		{
			return false;
		}
	}
	return true;
}

bool UYIInventoryBag::RotateItem(int32 Index)
{
	if (!bAllowRotation || !Items.IsValidIndex(Index)) return false;
	// Respect per-item rotation rule if available on the asset
	const FYIBagItem& Cur = Items[Index];
	UYIItemDefinition* Def = Cur.Item.Definition.IsValid() ? Cur.Item.Definition.Get() : Cur.Item.Definition.LoadSynchronous();
	if (Def && !Def->IsEffectiveRotationAllowed())
	{
		return false;
	}

	FYIBagItem It = Cur;
	FIntPoint Rot(It.Size.Y, It.Size.X);
	// remove temporarily
	Items.RemoveAt(Index);
	bool bOk = CanPlaceAt(It.Pos, Rot);
	Items.Insert(It, Index);
	if (!bOk) return false;
	Items[Index].Size = Rot;
	// Update stack key to reflect rotation change
	UYIInventoryBlueprintLibrary::UpdateCustomStackKey(Items[Index].Item);
	MarkPackageDirty();
	OnChanged.Broadcast();
	return true;
}

void UYIInventoryBag::ApplyMinifyScale(float NewScale, TArray<FYIBagItem>& DroppedItems)
{
	DroppedItems.Reset();
	MinifyScale = FMath::Clamp(NewScale, 0.1f, 1.0f);
	if (GridSize.X <= 0 || GridSize.Y <= 0)
	{
		// list mode: nothing to drop
		return;
	}
	TArray<FYIBagItem> Kept;
	Kept.Reserve(Items.Num());
	for (const FYIBagItem& It : Items)
	{
		FIntPoint EffSize = GetEffectiveSize(It.Size);
		// test overlap against Kept
		bool bFits = (It.Pos.X >= 0 && It.Pos.Y >= 0 && It.Pos.X + EffSize.X <= GridSize.X && It.Pos.Y + EffSize.Y <= GridSize.Y);
		if (bFits)
		{
			for (const FYIBagItem& K : Kept)
			{
				if (RectsOverlap(It.Pos, EffSize, K.Pos, GetEffectiveSize(K.Size))) { bFits = false; break; }
			}
		}
		if (bFits) { Kept.Add(It); }
		else { DroppedItems.Add(It); }
	}
	if (DroppedItems.Num() > 0)
	{
		Items = Kept;
		MarkPackageDirty(); OnChanged.Broadcast();
	}
}

bool UYIInventoryBag::RemoveItem(int32 Index)
{
	if (!Items.IsValidIndex(Index)) return false;
	FYIBagItem Removed = Items[Index];
	Items.RemoveAt(Index);
	MarkPackageDirty();
	OnChanged.Broadcast();
	OnItemRemoved.Broadcast(Index, Removed);
	return true;
}

bool UYIInventoryBag::SwapItems(int32 IndexA, int32 IndexB)
{
	if (!Items.IsValidIndex(IndexA) || !Items.IsValidIndex(IndexB) || IndexA == IndexB) return false;
	Swap(Items[IndexA], Items[IndexB]);
	MarkPackageDirty();
	OnChanged.Broadcast();
	OnItemMoved.Broadcast(IndexA, Items[IndexA].Pos);
	OnItemMoved.Broadcast(IndexB, Items[IndexB].Pos);
	return true;
}


bool UYIInventoryBag::FindFirstFit(const FIntPoint Size, FIntPoint& OutPos) const
{
	FIntPoint Eff = GetEffectiveSize(Size);
	if (GridSize.X <= 0 || GridSize.Y <= 0)
	{
		OutPos = FIntPoint(0, Items.Num());
		return true;
	}
	for (int y=0; y<=GridSize.Y - Eff.Y; ++y)
	{
		for (int x=0; x<=GridSize.X - Eff.X; ++x)
		{
			// Use original Size when querying CanPlaceAt since it applies effective scale internally
			if (CanPlaceAt(FIntPoint(x,y), Size)) { OutPos = FIntPoint(x,y); return true; }
		}
	}
	return false;
}

void UYIInventoryBag::AutoPack()
{
	if (GridSize.X <= 0 || GridSize.Y <= 0) return;
	// Sort by footprint area descending so larger items get placed first (reduces fragmentation).
	TArray<FYIBagItem> Sorted = Items;
	Sorted.Sort([this](const FYIBagItem& A, const FYIBagItem& B)
	{
		const FIntPoint EffA = GetEffectiveSize(A.Size);
		const FIntPoint EffB = GetEffectiveSize(B.Size);
		const int32 AreaA = EffA.X * EffA.Y;
		const int32 AreaB = EffB.X * EffB.Y;
		if (AreaA != AreaB) return AreaA > AreaB;
		// Tie-breaker: wider first, then original ordering remains stable
		return EffA.X > EffB.X;
	});

	// Place into the bag one-by-one using current occupancy as we go
	Items.Reset();
	Items.Reserve(Sorted.Num());
	for (const FYIBagItem& It : Sorted)
	{
		FYIBagItem Tmp = It;
		FIntPoint Pos;
		if (FindFirstFit(Tmp.Size, Pos))
		{
			Tmp.Pos = Pos;
		}
		Items.Add(Tmp);
	}
	MarkPackageDirty();
	OnChanged.Broadcast();
}
