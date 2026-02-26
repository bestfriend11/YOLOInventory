#pragma once

#include "CoreMinimal.h"

struct FYINetBagDescriptor;
class UYIInventoryBag;
class UYIInventoryComponent;

struct FYIInventoryBagContextService
{
	static UYIInventoryBag* CreateBag(UYIInventoryComponent& Inventory, FName BagName, FIntPoint GridSize);
	static void OpenBag(UYIInventoryComponent& Inventory, UYIInventoryBag* Bag);
	static void CloseBag(UYIInventoryComponent& Inventory, UYIInventoryBag* Bag);

	static UYIInventoryBag* GetBag(const UYIInventoryComponent& Inventory);
	static FGuid GetActiveContextBagId(const UYIInventoryComponent& Inventory, FGameplayTag ContextTag);
	static UYIInventoryBag* GetActiveContextBag(const UYIInventoryComponent& Inventory, FGameplayTag ContextTag);
	static UYIInventoryBag* GetBagById(const UYIInventoryComponent& Inventory, const FGuid& BagId);
	static UYIInventoryBag* GetBagByRoleTag(const UYIInventoryComponent& Inventory, FGameplayTag BagRoleTag);
	static UYIInventoryBag* GetBagByDisplayName(const UYIInventoryComponent& Inventory, FName BagName);
	static int32 FindActiveContextIndex(const UYIInventoryComponent& Inventory, FGameplayTag ContextTag);

	static bool SetActiveBagById(UYIInventoryComponent& Inventory, const FGuid& InBagId);
	static bool SetActiveBagByRoleTag(UYIInventoryComponent& Inventory, FGameplayTag InBagRoleTag);
	static bool OpenContainedBagAtIndex(UYIInventoryComponent& Inventory, int32 ItemIndex);
	static UYIInventoryBag* EnsureContainedBagAtIndex(UYIInventoryComponent& Inventory, int32 ItemIndex);
	static bool OpenParentBag(UYIInventoryComponent& Inventory);

	static bool SetActiveContextBagById(UYIInventoryComponent& Inventory, FGameplayTag ContextTag, const FGuid& InBagId);
	static bool SetActiveContextBagByRoleTag(UYIInventoryComponent& Inventory, FGameplayTag ContextTag, FGameplayTag InBagRoleTag);
	static bool ClearActiveContextBag(UYIInventoryComponent& Inventory, FGameplayTag ContextTag);
	static int32 ClearActiveContextsForBagId(UYIInventoryComponent& Inventory, const FGuid& InBagId);

	static void GetReplicatedBagDescriptors(const UYIInventoryComponent& Inventory, TArray<FYINetBagDescriptor>& OutDescriptors);
	static bool RemoveBag(UYIInventoryComponent& Inventory, UYIInventoryBag* Bag);
};

