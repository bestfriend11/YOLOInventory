#include "YIInventoryComponent.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"

UYIInventoryComponent::UYIInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UYIInventoryBag* UYIInventoryComponent::CreateBag(FName BagName, FIntPoint GridSize)
{
	UYIInventoryBag* NewBag = NewObject<UYIInventoryBag>(this);
	if (NewBag)
	{
		NewBag->GridSize = GridSize;
		NewBag->DisplayName = FText::FromName(BagName);
		Bags.Add(NewBag);
		return NewBag;
	}
	return nullptr;
}

void UYIInventoryComponent::OpenBag(UYIInventoryBag* Bag)
{
	if (!Bag) return;
	EquippedBag = Bag;
	OnBagOpened.Broadcast(Bag);
}

void UYIInventoryComponent::CloseBag(UYIInventoryBag* Bag)
{
	if (!Bag) return;
	if (EquippedBag == Bag) { EquippedBag = nullptr; }
	OnBagClosed.Broadcast(Bag);
}

bool UYIInventoryComponent::RemoveBag(UYIInventoryBag* Bag)
{
	if (!Bag) return false;
	int32 Index = Bags.IndexOfByKey(Bag);
	if (Index != INDEX_NONE)
	{
		Bags.RemoveAt(Index);
		if (EquippedBag == Bag) { EquippedBag = nullptr; }
		return true;
	}
	return false;
}

bool UYIInventoryComponent::AddItemToBag(UYIInventoryBag* Bag, TSoftObjectPtr<UYIItemDefinition> ItemDef, int32 Count)
{
	if (!Bag || !ItemDef.IsValid()) { if (!ItemDef.IsValid()) ItemDef.LoadSynchronous(); if (!ItemDef.IsValid()) return false; }
	UYIItemDefinition* Def = ItemDef.Get();
	if (!Def) { Def = ItemDef.LoadSynchronous(); if (!Def) return false; }

	FYIBagItem New;
	New.Item.Definition = ItemDef;
	New.Item.Count = FMath::Max(1, Count);
	New.Size = Def->DefaultSize;
	int32 Idx = Bag->AddBagItem(New);
	return Idx != INDEX_NONE;
}
