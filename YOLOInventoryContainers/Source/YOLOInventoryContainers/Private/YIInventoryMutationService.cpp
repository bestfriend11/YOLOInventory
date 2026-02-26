#include "YIInventoryMutationService.h"

#include "YIInventoryBag.h"
#include "YIInventoryBlueprintLibrary.h"
#include "YIInventoryComponent.h"
#include "YIItemDefinition.h"

namespace
{
	static bool YIMutation_RectsOverlap(const FIntPoint& APos, const FIntPoint& ASize, const FIntPoint& BPos, const FIntPoint& BSize)
	{
		return !(APos.X + ASize.X <= BPos.X || BPos.X + BSize.X <= APos.X ||
				 APos.Y + ASize.Y <= BPos.Y || BPos.Y + BSize.Y <= APos.Y);
	}

	static bool YIMutation_FindSingleOverlapAtCell(const UYIInventoryBag* Bag, const FIntPoint& Cell, const FIntPoint& Size, int32& OutVictimIndex, int32 IgnoreIndex = INDEX_NONE)
	{
		OutVictimIndex = INDEX_NONE;
		if (!Bag)
		{
			return false;
		}

		const FIntPoint Footprint = Bag->GetEffectiveSize(Size);
		for (int32 Index = 0; Index < Bag->Items.Num(); ++Index)
		{
			if (Index == IgnoreIndex)
			{
				continue;
			}
			const FYIBagItem& Existing = Bag->Items[Index];
			const FIntPoint ExistingSize = Bag->GetEffectiveSize(Existing.Size);
			if (!YIMutation_RectsOverlap(Cell, Footprint, Existing.Pos, ExistingSize))
			{
				continue;
			}

			if (OutVictimIndex != INDEX_NONE)
			{
				OutVictimIndex = INDEX_NONE;
				return false;
			}
			OutVictimIndex = Index;
		}

		return OutVictimIndex != INDEX_NONE;
	}

	static int32 YIMutation_AddBagItemExact(UYIInventoryBag* Bag, const FYIBagItem& ItemAtExactPos)
	{
		if (!Bag)
		{
			return INDEX_NONE;
		}
		if (!Bag->CanPlaceAt(ItemAtExactPos.Pos, ItemAtExactPos.Size))
		{
			return INDEX_NONE;
		}

		const bool bSavedAutoMerge = Bag->bAutoMergeOnAdd;
		Bag->bAutoMergeOnAdd = false;
		const int32 NewIndex = Bag->AddBagItem(ItemAtExactPos);
		Bag->bAutoMergeOnAdd = bSavedAutoMerge;
		return NewIndex;
	}
}

bool FYIInventoryMutationService::MoveItemInBag(UYIInventoryComponent& Inventory, const FGuid& BagId, const FGuid& ItemInstanceId, FIntPoint NewPos)
{
	if (!BagId.IsValid() || !ItemInstanceId.IsValid())
	{
		return false;
	}

	if (Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority())
	{
		UYIInventoryBag* TargetBag = Inventory.GetBagById(BagId);
		if (!TargetBag)
		{
			return false;
		}

		int32 ItemIndex = INDEX_NONE;
		if (!Inventory.FindItemIndexByInstanceId(TargetBag, ItemInstanceId, ItemIndex))
		{
			return false;
		}
		if (Inventory.IsBagItemLocked(TargetBag, ItemIndex))
		{
			return false;
		}
		if (TargetBag->MoveItem(ItemIndex, NewPos))
		{
			Inventory.SyncNetState();
			return true;
		}
		return false;
	}

	Inventory.ServerMoveItemInBag(BagId, ItemInstanceId, NewPos);
	return true;
}

bool FYIInventoryMutationService::MoveItemInBagAtCell(UYIInventoryComponent& Inventory, const FGuid& BagId, const FGuid& ItemInstanceId, FIntPoint DestCell, bool bAllowSingleOverlapSwap)
{
	if (!BagId.IsValid() || !ItemInstanceId.IsValid())
	{
		return false;
	}

	if (Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority())
	{
		UYIInventoryBag* TargetBag = Inventory.GetBagById(BagId);
		if (!TargetBag)
		{
			return false;
		}

		int32 SourceIndex = INDEX_NONE;
		if (!Inventory.FindItemIndexByInstanceId(TargetBag, ItemInstanceId, SourceIndex) || !TargetBag->Items.IsValidIndex(SourceIndex))
		{
			return false;
		}
		if (Inventory.IsBagItemLocked(TargetBag, SourceIndex))
		{
			return false;
		}

		if (TargetBag->MoveItem(SourceIndex, DestCell))
		{
			Inventory.SyncNetState();
			return true;
		}
		if (!bAllowSingleOverlapSwap)
		{
			return false;
		}

		int32 VictimIndex = INDEX_NONE;
		const FYIBagItem SourceItemCopy = TargetBag->Items[SourceIndex];
		if (!YIMutation_FindSingleOverlapAtCell(TargetBag, DestCell, SourceItemCopy.Size, VictimIndex, SourceIndex) ||
			!TargetBag->Items.IsValidIndex(VictimIndex))
		{
			return false;
		}
		if (Inventory.IsBagItemLocked(TargetBag, VictimIndex))
		{
			return false;
		}

		const FYIBagItem VictimItemCopy = TargetBag->Items[VictimIndex];
		const FIntPoint SourceOriginalPos = SourceItemCopy.Pos;
		const FIntPoint VictimOriginalPos = VictimItemCopy.Pos;

		if (!TargetBag->RemoveItem(VictimIndex))
		{
			return false;
		}

		int32 SourceIndexAfterVictimRemove = INDEX_NONE;
		if (!Inventory.FindItemIndexByInstanceId(TargetBag, ItemInstanceId, SourceIndexAfterVictimRemove) ||
			!TargetBag->Items.IsValidIndex(SourceIndexAfterVictimRemove))
		{
			FYIBagItem RestoreVictim = VictimItemCopy;
			RestoreVictim.Pos = VictimOriginalPos;
			YIMutation_AddBagItemExact(TargetBag, RestoreVictim);
			return false;
		}

		if (!TargetBag->RemoveItem(SourceIndexAfterVictimRemove))
		{
			FYIBagItem RestoreVictim = VictimItemCopy;
			RestoreVictim.Pos = VictimOriginalPos;
			YIMutation_AddBagItemExact(TargetBag, RestoreVictim);
			return false;
		}

		FYIBagItem PlacedSource = SourceItemCopy;
		PlacedSource.Pos = DestCell;
		const int32 NewSourceIdx = YIMutation_AddBagItemExact(TargetBag, PlacedSource);
		if (NewSourceIdx == INDEX_NONE)
		{
			FYIBagItem RestoreSource = SourceItemCopy; RestoreSource.Pos = SourceOriginalPos;
			YIMutation_AddBagItemExact(TargetBag, RestoreSource);
			FYIBagItem RestoreVictim = VictimItemCopy; RestoreVictim.Pos = VictimOriginalPos;
			YIMutation_AddBagItemExact(TargetBag, RestoreVictim);
			return false;
		}

		FYIBagItem PlacedVictim = VictimItemCopy;
		PlacedVictim.Pos = SourceOriginalPos;
		const int32 NewVictimIdx = YIMutation_AddBagItemExact(TargetBag, PlacedVictim);
		if (NewVictimIdx == INDEX_NONE)
		{
			TargetBag->RemoveItem(NewSourceIdx);
			FYIBagItem RestoreSource = SourceItemCopy; RestoreSource.Pos = SourceOriginalPos;
			YIMutation_AddBagItemExact(TargetBag, RestoreSource);
			FYIBagItem RestoreVictim = VictimItemCopy; RestoreVictim.Pos = VictimOriginalPos;
			YIMutation_AddBagItemExact(TargetBag, RestoreVictim);
			return false;
		}

		Inventory.SyncNetState();
		return true;
	}

	Inventory.ServerMoveItemInBagAtCell(BagId, ItemInstanceId, DestCell, bAllowSingleOverlapSwap);
	return true;
}

bool FYIInventoryMutationService::RotateItemInBag(UYIInventoryComponent& Inventory, const FGuid& BagId, const FGuid& ItemInstanceId)
{
	if (!BagId.IsValid() || !ItemInstanceId.IsValid())
	{
		return false;
	}

	if (Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority())
	{
		UYIInventoryBag* TargetBag = Inventory.GetBagById(BagId);
		if (!TargetBag)
		{
			return false;
		}

		int32 ItemIndex = INDEX_NONE;
		if (!Inventory.FindItemIndexByInstanceId(TargetBag, ItemInstanceId, ItemIndex))
		{
			return false;
		}
		if (Inventory.IsBagItemLocked(TargetBag, ItemIndex))
		{
			return false;
		}
		if (TargetBag->RotateItem(ItemIndex))
		{
			Inventory.SyncNetState();
			return true;
		}
		return false;
	}

	Inventory.ServerRotateItemInBag(BagId, ItemInstanceId);
	return true;
}

bool FYIInventoryMutationService::RemoveItemFromBag(UYIInventoryComponent& Inventory, const FGuid& BagId, const FGuid& ItemInstanceId)
{
	if (!BagId.IsValid() || !ItemInstanceId.IsValid())
	{
		return false;
	}

	if (Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority())
	{
		UYIInventoryBag* TargetBag = Inventory.GetBagById(BagId);
		if (!TargetBag)
		{
			return false;
		}

		int32 ItemIndex = INDEX_NONE;
		if (!Inventory.FindItemIndexByInstanceId(TargetBag, ItemInstanceId, ItemIndex))
		{
			return false;
		}
		if (Inventory.IsBagItemLocked(TargetBag, ItemIndex))
		{
			return false;
		}
		if (TargetBag->RemoveItem(ItemIndex))
		{
			Inventory.SyncNetState();
			return true;
		}
		return false;
	}

	Inventory.ServerRemoveItemFromBag(BagId, ItemInstanceId);
	return true;
}

bool FYIInventoryMutationService::TransferItemBetweenBagsById(UYIInventoryComponent& Inventory, const FGuid& SourceBagId, const FGuid& ItemInstanceId, const FGuid& DestBagId, int32 Count, int32& OutDestIndex)
{
	OutDestIndex = INDEX_NONE;
	if (!SourceBagId.IsValid() || !DestBagId.IsValid() || !ItemInstanceId.IsValid())
	{
		return false;
	}
	if (SourceBagId == DestBagId)
	{
		return false;
	}

	if (Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority())
	{
		UYIInventoryBag* SourceBag = Inventory.GetBagById(SourceBagId);
		UYIInventoryBag* DestBag = Inventory.GetBagById(DestBagId);
		if (!SourceBag || !DestBag)
		{
			return false;
		}

		int32 SourceIndex = INDEX_NONE;
		if (!Inventory.FindItemIndexByInstanceId(SourceBag, ItemInstanceId, SourceIndex) || !SourceBag->Items.IsValidIndex(SourceIndex))
		{
			return false;
		}
		if (Inventory.IsBagItemLocked(SourceBag, SourceIndex))
		{
			return false;
		}

		const FYIBagItem& SourceItem = SourceBag->Items[SourceIndex];
		if (SourceItem.Item.ContainedBagId.IsValid() && DestBag->BagId.IsValid() &&
			Inventory.IsBagDescendantOf(DestBag->BagId, SourceItem.Item.ContainedBagId))
		{
			return false;
		}

		if (UYIItemDefinition* Def = SourceItem.Item.Definition.IsValid()
			? SourceItem.Item.Definition.Get()
			: SourceItem.Item.Definition.LoadSynchronous())
		{
			if (!DestBag->CanAcceptItemDefinition(Def))
			{
				return false;
			}
		}

		if (UYIInventoryBlueprintLibrary::TransferItemBetweenBags(SourceBag, DestBag, SourceIndex, Count, OutDestIndex))
		{
			Inventory.SyncNetState();
			return true;
		}
		return false;
	}

	Inventory.ServerTransferItemBetweenBagsById(SourceBagId, ItemInstanceId, DestBagId, Count);
	return true;
}

bool FYIInventoryMutationService::TransferItemBetweenBagsAtCellById(UYIInventoryComponent& Inventory, const FGuid& SourceBagId, const FGuid& ItemInstanceId, const FGuid& DestBagId, FIntPoint DestCell, int32 Count, bool bAllowSingleOverlapSwap)
{
	if (!SourceBagId.IsValid() || !DestBagId.IsValid() || !ItemInstanceId.IsValid())
	{
		return false;
	}
	if (SourceBagId == DestBagId)
	{
		return false;
	}

	if (Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority())
	{
		UYIInventoryBag* SourceBag = Inventory.GetBagById(SourceBagId);
		UYIInventoryBag* DestBag = Inventory.GetBagById(DestBagId);
		if (!SourceBag || !DestBag)
		{
			return false;
		}

		int32 SourceIndex = INDEX_NONE;
		if (!Inventory.FindItemIndexByInstanceId(SourceBag, ItemInstanceId, SourceIndex) || !SourceBag->Items.IsValidIndex(SourceIndex))
		{
			return false;
		}
		if (Inventory.IsBagItemLocked(SourceBag, SourceIndex))
		{
			return false;
		}

		const FYIBagItem SourceBagItem = SourceBag->Items[SourceIndex];
		FYIBagItem ToPlace = SourceBagItem;
		const FIntPoint SourceOriginalPos = SourceBagItem.Pos;

		UYIItemDefinition* Def = ToPlace.Item.Definition.IsValid()
			? ToPlace.Item.Definition.Get()
			: ToPlace.Item.Definition.LoadSynchronous();
		if (!Def || !DestBag->CanAcceptItemDefinition(Def))
		{
			return false;
		}

		if (SourceBagItem.Item.ContainedBagId.IsValid() && DestBag->BagId.IsValid() &&
			Inventory.IsBagDescendantOf(DestBag->BagId, SourceBagItem.Item.ContainedBagId))
		{
			return false;
		}

		const bool bStacking = Def->IsRuntimeStackingAllowed();
		const bool bPartialTransferRequested = (bStacking && Count > 0 && Count < SourceBagItem.Item.Count);
		if (Count > 0 && bStacking)
		{
			ToPlace.Item.Count = FMath::Clamp(Count, 1, SourceBagItem.Item.Count);
		}
		ToPlace.Pos = DestCell;

		if (DestBag->CanPlaceAt(DestCell, ToPlace.Size))
		{
			const int32 DestInsertIndex = YIMutation_AddBagItemExact(DestBag, ToPlace);
			if (DestInsertIndex == INDEX_NONE)
			{
				return false;
			}

			if (bPartialTransferRequested)
			{
				if (!SourceBag->Items.IsValidIndex(SourceIndex))
				{
					DestBag->RemoveItem(DestInsertIndex);
					return false;
				}

				SourceBag->Items[SourceIndex].Item.Count -= ToPlace.Item.Count;
				if (SourceBag->Items[SourceIndex].Item.Count <= 0)
				{
					if (!SourceBag->RemoveItem(SourceIndex))
					{
						DestBag->RemoveItem(DestInsertIndex);
						return false;
					}
				}
				else
				{
					SourceBag->MarkPackageDirty();
					SourceBag->OnChanged.Broadcast();
				}
			}
			else
			{
				if (!SourceBag->RemoveItem(SourceIndex))
				{
					DestBag->RemoveItem(DestInsertIndex);
					return false;
				}
			}

			SourceBag->OnItemTransferred.Broadcast(SourceBag, DestBag, SourceIndex, DestInsertIndex);
			DestBag->OnItemTransferred.Broadcast(SourceBag, DestBag, SourceIndex, DestInsertIndex);
			Inventory.SyncNetState();
			return true;
		}

		if (!bAllowSingleOverlapSwap)
		{
			return false;
		}

		if (bPartialTransferRequested || (Count > 0 && Count != SourceBagItem.Item.Count))
		{
			return false;
		}

		int32 VictimIndex = INDEX_NONE;
		if (!YIMutation_FindSingleOverlapAtCell(DestBag, DestCell, ToPlace.Size, VictimIndex) || !DestBag->Items.IsValidIndex(VictimIndex))
		{
			return false;
		}
		if (Inventory.IsBagItemLocked(DestBag, VictimIndex))
		{
			return false;
		}

		const FYIBagItem VictimItem = DestBag->Items[VictimIndex];
		if (VictimItem.Item.ContainedBagId.IsValid() && SourceBag->BagId.IsValid() &&
			Inventory.IsBagDescendantOf(SourceBag->BagId, VictimItem.Item.ContainedBagId))
		{
			return false;
		}

		UYIItemDefinition* VictimDef = VictimItem.Item.Definition.IsValid()
			? VictimItem.Item.Definition.Get()
			: VictimItem.Item.Definition.LoadSynchronous();
		if (!VictimDef || !SourceBag->CanAcceptItemDefinition(VictimDef))
		{
			return false;
		}

		if (!SourceBag->CanPlaceAtIgnoring(SourceOriginalPos, VictimItem.Size, SourceIndex))
		{
			return false;
		}

		if (!DestBag->RemoveItem(VictimIndex))
		{
			return false;
		}

		FYIBagItem PlacedSource = SourceBagItem;
		PlacedSource.Pos = DestCell;
		const int32 DestPlacedIndex = YIMutation_AddBagItemExact(DestBag, PlacedSource);
		if (DestPlacedIndex == INDEX_NONE)
		{
			FYIBagItem RestoreVictim = VictimItem;
			RestoreVictim.Pos = VictimItem.Pos;
			YIMutation_AddBagItemExact(DestBag, RestoreVictim);
			return false;
		}

		if (!SourceBag->RemoveItem(SourceIndex))
		{
			DestBag->RemoveItem(DestPlacedIndex);
			FYIBagItem RestoreVictim = VictimItem;
			RestoreVictim.Pos = VictimItem.Pos;
			YIMutation_AddBagItemExact(DestBag, RestoreVictim);
			return false;
		}

		FYIBagItem PlacedVictim = VictimItem;
		PlacedVictim.Pos = SourceOriginalPos;
		const int32 SourcePlacedIndex = YIMutation_AddBagItemExact(SourceBag, PlacedVictim);
		if (SourcePlacedIndex == INDEX_NONE)
		{
			DestBag->RemoveItem(DestPlacedIndex);
			FYIBagItem RestoreSource = SourceBagItem;
			RestoreSource.Pos = SourceOriginalPos;
			YIMutation_AddBagItemExact(SourceBag, RestoreSource);
			FYIBagItem RestoreVictim = VictimItem;
			RestoreVictim.Pos = VictimItem.Pos;
			YIMutation_AddBagItemExact(DestBag, RestoreVictim);
			return false;
		}

		SourceBag->OnItemTransferred.Broadcast(SourceBag, DestBag, SourceIndex, DestPlacedIndex);
		DestBag->OnItemTransferred.Broadcast(SourceBag, DestBag, SourceIndex, DestPlacedIndex);
		Inventory.SyncNetState();
		return true;
	}

	Inventory.ServerTransferItemBetweenBagsAtCellById(SourceBagId, ItemInstanceId, DestBagId, DestCell, Count, bAllowSingleOverlapSwap);
	return true;
}

bool FYIInventoryMutationService::CombineItemInBag(UYIInventoryComponent& Inventory, const FGuid& BagId, const FGuid& ItemInstanceId)
{
	if (!BagId.IsValid() || !ItemInstanceId.IsValid())
	{
		return false;
	}

	if (Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority())
	{
		UYIInventoryBag* TargetBag = Inventory.GetBagById(BagId);
		if (!TargetBag)
		{
			return false;
		}

		int32 SourceIndex = INDEX_NONE;
		if (!Inventory.FindItemIndexByInstanceId(TargetBag, ItemInstanceId, SourceIndex) || !TargetBag->Items.IsValidIndex(SourceIndex))
		{
			return false;
		}
		if (Inventory.IsBagItemLocked(TargetBag, SourceIndex))
		{
			return false;
		}

		int32 TargetIndex = TargetBag->FindExistingStackIndexForItem(TargetBag->Items[SourceIndex]);
		if (TargetIndex == INDEX_NONE || TargetIndex == SourceIndex)
		{
			return false;
		}
		if (Inventory.IsBagItemLocked(TargetBag, TargetIndex))
		{
			return false;
		}

		if (TargetBag->CombineStacks(TargetIndex, SourceIndex))
		{
			Inventory.SyncNetState();
			return true;
		}
		return false;
	}

	Inventory.ServerCombineItemInBag(BagId, ItemInstanceId);
	return true;
}

bool FYIInventoryMutationService::SplitStackInBag(UYIInventoryComponent& Inventory, const FGuid& BagId, const FGuid& ItemInstanceId, int32 Amount, FIntPoint DesiredPos)
{
	if (!BagId.IsValid() || !ItemInstanceId.IsValid() || Amount <= 0)
	{
		return false;
	}

	if (Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority())
	{
		UYIInventoryBag* TargetBag = Inventory.GetBagById(BagId);
		if (!TargetBag)
		{
			return false;
		}

		int32 SourceIndex = INDEX_NONE;
		if (!Inventory.FindItemIndexByInstanceId(TargetBag, ItemInstanceId, SourceIndex))
		{
			return false;
		}
		if (Inventory.IsBagItemLocked(TargetBag, SourceIndex))
		{
			return false;
		}

		if (TargetBag->SplitStack(SourceIndex, Amount, DesiredPos) != INDEX_NONE)
		{
			Inventory.SyncNetState();
			return true;
		}
		return false;
	}

	Inventory.ServerSplitStackInBag(BagId, ItemInstanceId, Amount, DesiredPos);
	return true;
}

