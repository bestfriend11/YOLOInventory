#pragma once

#include "CoreMinimal.h"

struct FYIBagItem;
struct FYIItemInstanceNet;
class UYIInventoryBag;
class UYIInventoryComponent;
class UYIItemDefinition;

struct FYIInventoryItemIngressService
{
	static bool AddItemToBag(UYIInventoryComponent& Inventory, UYIInventoryBag* Bag, TSoftObjectPtr<UYIItemDefinition> ItemDef, int32 Count);
	static int32 AddBagItem(UYIInventoryComponent& Inventory, const FYIBagItem& Item);
	static void ServerAddBagItem(UYIInventoryComponent& Inventory, const FYIItemInstanceNet& NetItem, FIntPoint Pos, FIntPoint Size);
};

