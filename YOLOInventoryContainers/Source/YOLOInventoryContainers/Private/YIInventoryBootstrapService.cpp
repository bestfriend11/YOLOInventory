#include "YIInventoryBootstrapService.h"

#include "YIInventoryBag.h"
#include "YIInventoryComponent.h"

void FYIInventoryBootstrapService::BeginPlay(UYIInventoryComponent& Inventory)
{
	if (!(Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority()))
	{
		return;
	}

	TMap<const UYIInventoryBag*, UYIInventoryBag*> RuntimeCloneMap;
	for (int32 Index = 0; Index < Inventory.Bags.Num(); ++Index)
	{
		UYIInventoryBag* Bag = Inventory.Bags[Index];
		if (!Bag)
		{
			continue;
		}
		if (Inventory.IsTemplateBag(Bag))
		{
			if (UYIInventoryBag* RuntimeBag = Inventory.CloneBagTemplate(Bag))
			{
				RuntimeCloneMap.Add(Bag, RuntimeBag);
				Inventory.Bags[Index] = RuntimeBag;
			}
		}
	}

	Inventory.Bags.RemoveAllSwap([](UYIInventoryBag* Bag) { return Bag == nullptr; }, EAllowShrinking::No);

	if (Inventory.EquippedBag)
	{
		if (UYIInventoryBag** RuntimeFromList = RuntimeCloneMap.Find(Inventory.EquippedBag))
		{
			Inventory.EquippedBag = *RuntimeFromList;
		}
		else if (Inventory.IsTemplateBag(Inventory.EquippedBag))
		{
			Inventory.EquippedBag = Inventory.CloneBagTemplate(Inventory.EquippedBag);
		}
	}

	if (!Inventory.EquippedBag)
	{
		Inventory.EquippedBag = Inventory.GetBag();
	}

	if (!Inventory.EquippedBag)
	{
		return;
	}

	Inventory.EquippedBag->EnsureBagId();
	Inventory.ActiveBagId = Inventory.EquippedBag->BagId;
	if (!Inventory.Bags.Contains(Inventory.EquippedBag))
	{
		Inventory.Bags.Insert(Inventory.EquippedBag, 0);
	}

	// Materialize nested runtime bags for pre-authored container items copied from bag templates.
	for (int32 BagIndex = 0; BagIndex < Inventory.Bags.Num(); ++BagIndex)
	{
		UYIInventoryBag* RuntimeBag = Inventory.Bags[BagIndex];
		if (!RuntimeBag)
		{
			continue;
		}
		for (int32 ItemIndex = 0; ItemIndex < RuntimeBag->Items.Num(); ++ItemIndex)
		{
			Inventory.EnsureContainedBagForItem(RuntimeBag->Items[ItemIndex], RuntimeBag);
		}
	}

	Inventory.OpenBag(Inventory.EquippedBag);
}

