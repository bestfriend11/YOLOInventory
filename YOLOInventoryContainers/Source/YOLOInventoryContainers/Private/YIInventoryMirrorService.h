#pragma once

#include "CoreMinimal.h"

struct FYINetBagItem;
class UYIInventoryBag;
class UYIInventoryComponent;

struct FYIInventoryMirrorService
{
	static void SyncNetState(UYIInventoryComponent& Inventory);
	static void OnRep_NetBag(UYIInventoryComponent& Inventory);
	static void OnRep_NetBagDescriptors(UYIInventoryComponent& Inventory);
	static void OnRep_ActiveBagContexts(UYIInventoryComponent& Inventory);
	static void OnRep_NetContextBagMirrors(UYIInventoryComponent& Inventory);
	static void OnRep_LockedBagItems(UYIInventoryComponent& Inventory);

	static UYIInventoryBag* FindClientContextPreviewBagById(const UYIInventoryComponent& Inventory, const FGuid& BagId);
	static UYIInventoryBag* FindOrCreateClientContextPreviewBagById(UYIInventoryComponent& Inventory, const FGuid& BagId);
	static void RebuildClientPreviewBagFromNet(
		UYIInventoryComponent& Inventory,
		UYIInventoryBag* TargetBag,
		const TArray<FYINetBagItem>& InItems,
		const FIntPoint& InGridSize,
		const FGuid& InBagId);
};

