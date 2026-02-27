#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "YIItemSchemaResolver.h"
#include "YIInventoryBlueprintLibrary.h"
#include "InventoryUtils.h"
#include "UObject/Package.h"

void UYIInventoryBag::PostLoad()
{
	Super::PostLoad();
	EnsureBagId();
	MarkRuntimeLookupCacheDirty();
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

void UYIInventoryBag::MarkBagChanged()
{
	MarkRuntimeLookupCacheDirty();
	++RuntimeRevision;
	if (ShouldMarkDirty())
	{
		MarkPackageDirty();
	}
	OnChanged.Broadcast();
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
		const FGameplayTag ItemType = YIItemSchema::GetItemType(Definition);
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

	FGameplayTagContainer EffectiveTags;
	YIItemSchema::GetTags(Definition, EffectiveTags);
	const FGameplayTag ItemTypeTag = YIItemSchema::GetItemType(Definition);
	if (ItemTypeTag.IsValid())
	{
		EffectiveTags.AddTag(ItemTypeTag);
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
	BuildRuntimeLookupCache();
	for (int32 Y = 0; Y < EffSize.Y; ++Y)
	{
		const int32 GridY = Pos.Y + Y;
		const int32 RowBase = GridY * GridSize.X;
		for (int32 X = 0; X < EffSize.X; ++X)
		{
			const int32 FlatIdx = RowBase + Pos.X + X;
			if (RuntimeCellToItemIndex.IsValidIndex(FlatIdx) && RuntimeCellToItemIndex[FlatIdx] != INDEX_NONE)
			{
				return false;
			}
		}
	}
	return true;
}

bool UYIInventoryBag::MoveItem(int32 Index, const FIntPoint NewPos)
{
	if (!Items.IsValidIndex(Index)) return false;
	const FYIBagItem& Tmp = Items[Index];
	if (!CanPlaceAtIgnoring(NewPos, Tmp.Size, Index)) return false;
	Items[Index].Pos = NewPos;
	MarkBagChanged();
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
	if (YIItemSchema::IsContainerItem(Def))
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

	// Enforce uniqueness-per-type: only one stack per ItemType in this bag when flagged on the incoming item
	if (YIItemSchema::IsUniquePerType(Def))
	{
		const FGameplayTag NewItemType = YIItemSchema::GetItemType(Def);
		for (const FYIBagItem& E : Items)
		{
			if (UYIItemDefinition* EDef = (E.Item.Definition.IsValid() ? E.Item.Definition.Get() : E.Item.Definition.LoadSynchronous()))
			{
				if (YIItemSchema::GetItemType(EDef) == NewItemType)
				{
					// Another item of the same ItemType already exists; reject placement
					return INDEX_NONE;
				}
			}
		}
	}

	// Stacking logic: if enabled and there is an existing stack and stacking allowed, try to merge; otherwise create another stack
	if (bAutoMergeOnAdd && !YIItemSchema::IsContainerItem(Def))
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
			const int32 MaxStackCount = YIItemSchema::GetMaxStackCount(Def);
			int32 Room = MaxStackCount - Items[Existing].Item.Count;
			if (Room > 0)
			{
				Items[Existing].Item.Count = FMath::Clamp(Items[Existing].Item.Count + FMath::Max(1, NewItem.Item.Count), 1, MaxStackCount);
				MarkBagChanged();
				return Existing;
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
		Copy.Item.Count = FMath::Clamp(Copy.Item.Count, 1, YIItemSchema::GetMaxStackCount(Def));
	}
	int32 OutIndex = Items.Add(Copy);
	MarkBagChanged();
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
	int32 Room = YIItemSchema::GetMaxStackCount(DefA) - A.Item.Count;
	if (Room <= 0) return false;
	int32 Moved = FMath::Min(Room, B.Item.Count);
	A.Item.Count += Moved;
	B.Item.Count -= Moved;
	if (B.Item.Count <= 0)
	{
		FYIBagItem RemovedB = B;
		Items.RemoveAt(IndexB);
		MarkBagChanged();
		OnItemRemoved.Broadcast(IndexB, RemovedB);
	}
	else
	{
		MarkBagChanged();
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
	MarkBagChanged();
	return OutIdx;
}

bool UYIInventoryBag::CanPlaceAtWithScale(const FIntPoint Pos, const FIntPoint Size) const
{
	return CanPlaceAt(Pos, Size);
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
	BuildRuntimeLookupCache();
	for (int32 Y = 0; Y < EffSize.Y; ++Y)
	{
		const int32 GridY = Pos.Y + Y;
		const int32 RowBase = GridY * GridSize.X;
		for (int32 X = 0; X < EffSize.X; ++X)
		{
			const int32 FlatIdx = RowBase + Pos.X + X;
			if (!RuntimeCellToItemIndex.IsValidIndex(FlatIdx))
			{
				continue;
			}

			const int32 ItemIdx = RuntimeCellToItemIndex[FlatIdx];
			if (ItemIdx != INDEX_NONE && ItemIdx != IgnoreIndex)
			{
				return false;
			}
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
	if (Def && !YIItemSchema::IsRotationAllowed(Def))
	{
		return false;
	}

	const FIntPoint Rot(Cur.Size.Y, Cur.Size.X);
	const bool bOk = CanPlaceAtIgnoring(Cur.Pos, Rot, Index);
	if (!bOk) return false;
	Items[Index].Size = Rot;
	// Update stack key to reflect rotation change
	UYIInventoryBlueprintLibrary::UpdateCustomStackKey(Items[Index].Item);
	MarkBagChanged();
	return true;
}

void UYIInventoryBag::ApplyMinifyScale(float NewScale, TArray<FYIBagItem>& DroppedItems)
{
	DroppedItems.Reset();
	MinifyScale = FMath::Clamp(NewScale, 0.1f, 1.0f);
	MarkRuntimeLookupCacheDirty();
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
		MarkBagChanged();
	}
}

bool UYIInventoryBag::RemoveItem(int32 Index)
{
	if (!Items.IsValidIndex(Index)) return false;
	FYIBagItem Removed = Items[Index];
	Items.RemoveAt(Index);
	MarkBagChanged();
	OnItemRemoved.Broadcast(Index, Removed);
	return true;
}

bool UYIInventoryBag::SwapItems(int32 IndexA, int32 IndexB)
{
	if (!Items.IsValidIndex(IndexA) || !Items.IsValidIndex(IndexB) || IndexA == IndexB) return false;
	Swap(Items[IndexA], Items[IndexB]);
	MarkBagChanged();
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
	MarkBagChanged();
}

void UYIInventoryBag::MarkRuntimeLookupCacheDirty() const
{
	bRuntimeLookupCacheDirty = true;
}

bool UYIInventoryBag::IsRuntimeLookupCacheDirty() const
{
	if (bRuntimeLookupCacheDirty)
	{
		return true;
	}

	if (RuntimeCachedGridSize != GridSize)
	{
		return true;
	}

	return !FMath::IsNearlyEqual(RuntimeCachedMinifyScale, MinifyScale);
}

void UYIInventoryBag::BuildRuntimeLookupCache() const
{
	if (!IsRuntimeLookupCacheDirty())
	{
		return;
	}

	RuntimeCellToItemIndex.Reset();
	RuntimeInstanceToItemIndex.Reset();
	RuntimeCachedGridSize = GridSize;
	RuntimeCachedMinifyScale = MinifyScale;

	if (GridSize.X > 0 && GridSize.Y > 0)
	{
		RuntimeCellToItemIndex.Init(INDEX_NONE, GridSize.X * GridSize.Y);
	}

	for (int32 ItemIdx = 0; ItemIdx < Items.Num(); ++ItemIdx)
	{
		const FYIBagItem& Item = Items[ItemIdx];
		if (Item.Item.InstanceId.IsValid())
		{
			RuntimeInstanceToItemIndex.Add(Item.Item.InstanceId, ItemIdx);
		}

		if (GridSize.X <= 0 || GridSize.Y <= 0)
		{
			continue;
		}

		const FIntPoint Eff = GetEffectiveSize(Item.Size);
		for (int32 Y = 0; Y < Eff.Y; ++Y)
		{
			const int32 GridY = Item.Pos.Y + Y;
			if (GridY < 0 || GridY >= GridSize.Y)
			{
				continue;
			}

			const int32 RowBase = GridY * GridSize.X;
			for (int32 X = 0; X < Eff.X; ++X)
			{
				const int32 GridX = Item.Pos.X + X;
				if (GridX < 0 || GridX >= GridSize.X)
				{
					continue;
				}

				const int32 FlatIdx = RowBase + GridX;
				if (RuntimeCellToItemIndex.IsValidIndex(FlatIdx))
				{
					RuntimeCellToItemIndex[FlatIdx] = ItemIdx;
				}
			}
		}
	}

	bRuntimeLookupCacheDirty = false;
}

int32 UYIInventoryBag::GetItemIndexAtCellFast(const FIntPoint& Cell) const
{
	if (GridSize.X <= 0 || GridSize.Y <= 0)
	{
		return INDEX_NONE;
	}
	if (Cell.X < 0 || Cell.Y < 0 || Cell.X >= GridSize.X || Cell.Y >= GridSize.Y)
	{
		return INDEX_NONE;
	}

	BuildRuntimeLookupCache();
	const int32 FlatIdx = Cell.Y * GridSize.X + Cell.X;
	if (!RuntimeCellToItemIndex.IsValidIndex(FlatIdx))
	{
		return INDEX_NONE;
	}

	int32 ItemIdx = RuntimeCellToItemIndex[FlatIdx];
	if (ItemIdx != INDEX_NONE && !Items.IsValidIndex(ItemIdx))
	{
		MarkRuntimeLookupCacheDirty();
		BuildRuntimeLookupCache();
		ItemIdx = RuntimeCellToItemIndex.IsValidIndex(FlatIdx) ? RuntimeCellToItemIndex[FlatIdx] : INDEX_NONE;
		if (ItemIdx != INDEX_NONE && !Items.IsValidIndex(ItemIdx))
		{
			return INDEX_NONE;
		}
	}
	return ItemIdx;
}

bool UYIInventoryBag::FindItemIndexByInstanceIdFast(const FGuid& InstanceId, int32& OutIndex) const
{
	OutIndex = INDEX_NONE;
	if (!InstanceId.IsValid())
	{
		return false;
	}

	BuildRuntimeLookupCache();
	if (const int32* FoundIdx = RuntimeInstanceToItemIndex.Find(InstanceId))
	{
		OutIndex = *FoundIdx;
		return Items.IsValidIndex(OutIndex);
	}
	return false;
}

bool UYIInventoryBag::FindSingleOverlapAt(const FIntPoint& Pos, const FIntPoint& Size, int32 IgnoreIndex, int32& OutOverlapIdx) const
{
	OutOverlapIdx = INDEX_NONE;
	if (GridSize.X <= 0 || GridSize.Y <= 0)
	{
		return true;
	}

	const FIntPoint EffSize = GetEffectiveSize(Size);
	if (Pos.X < 0 || Pos.Y < 0 || Pos.X + EffSize.X > GridSize.X || Pos.Y + EffSize.Y > GridSize.Y)
	{
		return false;
	}

	BuildRuntimeLookupCache();
	for (int32 Y = 0; Y < EffSize.Y; ++Y)
	{
		const int32 GridY = Pos.Y + Y;
		const int32 RowBase = GridY * GridSize.X;
		for (int32 X = 0; X < EffSize.X; ++X)
		{
			const int32 FlatIdx = RowBase + Pos.X + X;
			if (!RuntimeCellToItemIndex.IsValidIndex(FlatIdx))
			{
				continue;
			}

			const int32 ItemIdx = RuntimeCellToItemIndex[FlatIdx];
			if (ItemIdx == INDEX_NONE || ItemIdx == IgnoreIndex || !Items.IsValidIndex(ItemIdx))
			{
				continue;
			}
			if (OutOverlapIdx == INDEX_NONE)
			{
				OutOverlapIdx = ItemIdx;
			}
			else if (OutOverlapIdx != ItemIdx)
			{
				OutOverlapIdx = INDEX_NONE;
				return false;
			}
		}
	}

	return true;
}

void UYIInventoryBag::GetOverlappingItemIndicesAt(const FIntPoint& Pos, const FIntPoint& Size, int32 IgnoreIndex, TArray<int32>& OutIndices) const
{
	OutIndices.Reset();
	if (GridSize.X <= 0 || GridSize.Y <= 0)
	{
		return;
	}

	const FIntPoint EffSize = GetEffectiveSize(Size);
	if (Pos.X < 0 || Pos.Y < 0 || Pos.X + EffSize.X > GridSize.X || Pos.Y + EffSize.Y > GridSize.Y)
	{
		return;
	}

	BuildRuntimeLookupCache();
	TArray<uint8> Seen;
	Seen.Init(0, Items.Num());
	for (int32 Y = 0; Y < EffSize.Y; ++Y)
	{
		const int32 GridY = Pos.Y + Y;
		const int32 RowBase = GridY * GridSize.X;
		for (int32 X = 0; X < EffSize.X; ++X)
		{
			const int32 FlatIdx = RowBase + Pos.X + X;
			if (!RuntimeCellToItemIndex.IsValidIndex(FlatIdx))
			{
				continue;
			}

			const int32 ItemIdx = RuntimeCellToItemIndex[FlatIdx];
			if (ItemIdx == INDEX_NONE || ItemIdx == IgnoreIndex || !Items.IsValidIndex(ItemIdx))
			{
				continue;
			}
			if (!Seen.IsValidIndex(ItemIdx) || Seen[ItemIdx] != 0)
			{
				continue;
			}
			Seen[ItemIdx] = 1;
			OutIndices.Add(ItemIdx);
		}
	}
}
