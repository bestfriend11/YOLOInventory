#include "YIInventoryCommandLibrary.h"

#include "YIInventoryComponent.h"

bool UYIInventoryCommandLibrary::BuildItemRef(UYIInventoryComponent* Inventory, UYIInventoryBag* Bag, int32 ItemIndex, FYIInventoryItemRef& OutItemRef)
{
	return Inventory ? Inventory->GetBagItemCoreRef(Bag, ItemIndex, OutItemRef) : false;
}

bool UYIInventoryCommandLibrary::BuildActiveBagItemRef(UYIInventoryComponent* Inventory, int32 ItemIndex, FYIInventoryItemRef& OutItemRef)
{
	return Inventory ? Inventory->GetBagItemCoreRef(Inventory->GetBag(), ItemIndex, OutItemRef) : false;
}

bool UYIInventoryCommandLibrary::BuildContextBagItemRef(UYIInventoryComponent* Inventory, FGameplayTag ContextTag, int32 ItemIndex, FYIInventoryItemRef& OutItemRef)
{
	return Inventory ? Inventory->GetBagItemCoreRef(Inventory->GetActiveContextBag(ContextTag), ItemIndex, OutItemRef) : false;
}

bool UYIInventoryCommandLibrary::MoveItemByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef, FIntPoint NewPos)
{
	return Inventory && ItemRef.Bag.BagId.IsValid() && ItemRef.Item.ItemInstanceId.IsValid()
		? Inventory->MoveItemInBag(ItemRef.Bag.BagId, ItemRef.Item.ItemInstanceId, NewPos)
		: false;
}

bool UYIInventoryCommandLibrary::MoveItemAtCellByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef, FIntPoint DestCell, bool bAllowSingleOverlapSwap)
{
	return Inventory && ItemRef.Bag.BagId.IsValid() && ItemRef.Item.ItemInstanceId.IsValid()
		? Inventory->MoveItemInBagAtCell(ItemRef.Bag.BagId, ItemRef.Item.ItemInstanceId, DestCell, bAllowSingleOverlapSwap)
		: false;
}

bool UYIInventoryCommandLibrary::RotateItemByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef)
{
	return Inventory && ItemRef.Bag.BagId.IsValid() && ItemRef.Item.ItemInstanceId.IsValid()
		? Inventory->RotateItemInBag(ItemRef.Bag.BagId, ItemRef.Item.ItemInstanceId)
		: false;
}

bool UYIInventoryCommandLibrary::RemoveItemByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef)
{
	return Inventory && ItemRef.Bag.BagId.IsValid() && ItemRef.Item.ItemInstanceId.IsValid()
		? Inventory->RemoveItemFromBag(ItemRef.Bag.BagId, ItemRef.Item.ItemInstanceId)
		: false;
}

bool UYIInventoryCommandLibrary::CombineItemByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef)
{
	return Inventory && ItemRef.Bag.BagId.IsValid() && ItemRef.Item.ItemInstanceId.IsValid()
		? Inventory->CombineItemInBag(ItemRef.Bag.BagId, ItemRef.Item.ItemInstanceId)
		: false;
}

bool UYIInventoryCommandLibrary::SplitStackByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef, int32 Amount, FIntPoint DesiredPos)
{
	return Inventory && ItemRef.Bag.BagId.IsValid() && ItemRef.Item.ItemInstanceId.IsValid()
		? Inventory->SplitStackInBag(ItemRef.Bag.BagId, ItemRef.Item.ItemInstanceId, Amount, DesiredPos)
		: false;
}

bool UYIInventoryCommandLibrary::TransferItemToBagByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef, const FGuid& DestBagId, int32 Count, int32& OutDestIndex)
{
	if (!Inventory || !ItemRef.Bag.BagId.IsValid() || !ItemRef.Item.ItemInstanceId.IsValid() || !DestBagId.IsValid())
	{
		OutDestIndex = INDEX_NONE;
		return false;
	}
	return Inventory->TransferItemBetweenBagsById(ItemRef.Bag.BagId, ItemRef.Item.ItemInstanceId, DestBagId, Count, OutDestIndex);
}

bool UYIInventoryCommandLibrary::TransferItemToBagCellByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef, const FGuid& DestBagId, FIntPoint DestCell, int32 Count, bool bAllowSingleOverlapSwap)
{
	return Inventory && ItemRef.Bag.BagId.IsValid() && ItemRef.Item.ItemInstanceId.IsValid() && DestBagId.IsValid()
		? Inventory->TransferItemBetweenBagsAtCellById(ItemRef.Bag.BagId, ItemRef.Item.ItemInstanceId, DestBagId, DestCell, Count, bAllowSingleOverlapSwap)
		: false;
}

bool UYIInventoryCommandLibrary::SwapItemToBagCellByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef, const FGuid& DestBagId, FIntPoint DestCell)
{
	return Inventory && ItemRef.Bag.BagId.IsValid() && ItemRef.Item.ItemInstanceId.IsValid() && DestBagId.IsValid()
		? Inventory->SwapItemIntoBagCellById(ItemRef.Bag.BagId, ItemRef.Item.ItemInstanceId, DestBagId, DestCell)
		: false;
}

bool UYIInventoryCommandLibrary::SetItemLockedByRef(UYIInventoryComponent* Inventory, const FYIInventoryItemRef& ItemRef, bool bLocked)
{
	return Inventory ? Inventory->SetBagItemLockedByCoreRef(ItemRef, bLocked) : false;
}
