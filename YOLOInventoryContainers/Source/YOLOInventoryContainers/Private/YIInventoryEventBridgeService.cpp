#include "YIInventoryEventBridgeService.h"

#include "YIInventoryBag.h"
#include "YIInventoryComponent.h"
#include "YIDebugLibrary.h"

void FYIInventoryEventBridgeService::HandleBagItemAdded(UYIInventoryComponent& Inventory, int32 Index, FYIBagItem Item)
{
	Inventory.OnInventoryItemAdded.Broadcast(Inventory.EquippedBag, Index, Item);
	UYIDebugLibrary::EmitDebugMessage(
		&Inventory,
		EYIDebugChannel::Inventory,
		FString::Printf(TEXT("Added idx=%d count=%d"), Index, Item.Item.Count),
		FLinearColor(FColor::Green),
		Inventory.bDebugInventoryActions,
		Inventory.bDebugInventoryActions,
		2.0f,
		false,
		false,
		TEXT("InventoryComponent"));
}

void FYIInventoryEventBridgeService::HandleBagItemRemoved(UYIInventoryComponent& Inventory, int32 Index, FYIBagItem Item)
{
	if (Inventory.EquippedBag && Inventory.EquippedBag->BagId.IsValid())
	{
		const int32 Removed = Inventory.LockedBagItems.RemoveAllSwap([&Inventory, &Item](const FYIInventoryLockRef& Entry)
		{
			if (!Inventory.EquippedBag || Entry.ItemRef.Bag.BagId != Inventory.EquippedBag->BagId)
			{
				return false;
			}
			if (Entry.ItemRef.Item.ItemInstanceId.IsValid() && Item.Item.InstanceId.IsValid())
			{
				return Entry.ItemRef.Item.ItemInstanceId == Item.Item.InstanceId;
			}
			return Entry.ItemRef.Item.LegacyStackKey != 0 && Entry.ItemRef.Item.LegacyStackKey == Item.Item.CustomStackKey;
		}, EAllowShrinking::No);
		if (Removed > 0 && Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority())
		{
			Inventory.SyncNetState();
		}
	}

	Inventory.OnInventoryItemRemoved.Broadcast(Inventory.EquippedBag, Index, Item);
	UYIDebugLibrary::EmitDebugMessage(
		&Inventory,
		EYIDebugChannel::Inventory,
		FString::Printf(TEXT("Removed idx=%d"), Index),
		FLinearColor(FColor::Orange),
		Inventory.bDebugInventoryActions,
		Inventory.bDebugInventoryActions,
		2.0f,
		false,
		false,
		TEXT("InventoryComponent"));
}

void FYIInventoryEventBridgeService::HandleBagItemMoved(UYIInventoryComponent& Inventory, int32 Index, FIntPoint NewPos)
{
	Inventory.OnInventoryItemMoved.Broadcast(Inventory.EquippedBag, Index, NewPos);
	UYIDebugLibrary::EmitDebugMessage(
		&Inventory,
		EYIDebugChannel::Inventory,
		FString::Printf(TEXT("Moved idx=%d to (%d,%d)"), Index, NewPos.X, NewPos.Y),
		FLinearColor(FColor::Cyan),
		Inventory.bDebugInventoryActions,
		Inventory.bDebugInventoryActions,
		2.0f,
		false,
		false,
		TEXT("InventoryComponent"));
}

void FYIInventoryEventBridgeService::HandleBagItemRotated(UYIInventoryComponent& Inventory, int32 Index)
{
	Inventory.OnInventoryItemRotated.Broadcast(Inventory.EquippedBag, Index);
	UYIDebugLibrary::EmitDebugMessage(
		&Inventory,
		EYIDebugChannel::Inventory,
		FString::Printf(TEXT("Rotated idx=%d"), Index),
		FLinearColor(FColor::Yellow),
		Inventory.bDebugInventoryActions,
		Inventory.bDebugInventoryActions,
		2.0f,
		false,
		false,
		TEXT("InventoryComponent"));
}

void FYIInventoryEventBridgeService::HandleBagItemTransferred(UYIInventoryComponent& Inventory, UYIInventoryBag* Src, UYIInventoryBag* Dest, int32 SrcIdx, int32 DestIdx)
{
	Inventory.OnInventoryItemTransferred.Broadcast(Src, Dest, SrcIdx, DestIdx);
	UYIDebugLibrary::EmitDebugMessage(
		&Inventory,
		EYIDebugChannel::Inventory,
		FString::Printf(TEXT("Transfer %p:%d -> %p:%d"), Src, SrcIdx, Dest, DestIdx),
		FLinearColor(FColor::White),
		Inventory.bDebugInventoryActions,
		Inventory.bDebugInventoryActions,
		2.0f,
		false,
		false,
		TEXT("InventoryComponent"));
}

