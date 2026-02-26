#include "YIInventoryItemIngressService.h"

#include "YIInventoryBag.h"
#include "YIInventoryComponent.h"
#include "YIItemDefinition.h"
#include "YIItemInstanceFragmentAccess.h"
#include "YIItemSchemaResolver.h"

namespace
{
	static FYIItemInstance YIIngress_NetToFull(const FYIItemInstanceNet& Net)
	{
		FYIItemInstance Out;
		Out.Definition = Net.Definition;
		Out.Count = Net.Count;
		Out.InstanceId = Net.InstanceId.IsValid() ? Net.InstanceId : FGuid::NewGuid();
		Out.StackId = Net.StackId.IsValid() ? Net.StackId : FGuid::NewGuid();
		Out.CustomStackKey = Net.CustomStackKey;
		Out.ContainedBagId = Net.ContainedBagId;
		Out.bRotated = Net.bRotated;
		YIItemInstanceFragments::ImportNetFragmentPayload(Out, Net.Fragments);
		return Out;
	}
}

bool FYIInventoryItemIngressService::AddItemToBag(UYIInventoryComponent& Inventory, UYIInventoryBag* Bag, TSoftObjectPtr<UYIItemDefinition> ItemDef, int32 Count)
{
	if (!Bag)
	{
		return false;
	}

	if (Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority() && Inventory.IsTemplateBag(Bag))
	{
		Bag = Inventory.CloneBagTemplate(Bag);
		Inventory.OpenBag(Bag);
	}

	if (!ItemDef.IsValid())
	{
		ItemDef.LoadSynchronous();
		if (!ItemDef.IsValid())
		{
			return false;
		}
	}

	UYIItemDefinition* Def = ItemDef.Get();
	if (!Def)
	{
		Def = ItemDef.LoadSynchronous();
		if (!Def)
		{
			return false;
		}
	}

	FYIBagItem NewItem;
	NewItem.Item.Definition = ItemDef;
	NewItem.Item.Count = FMath::Max(1, Count);
	NewItem.Size = YIItemSchema::GetDefaultSize(Def);
	if (YIItemSchema::IsContainerItem(Def))
	{
		NewItem.Item.Count = 1;
	}

	const int32 Idx = Bag->AddBagItem(NewItem);
	if (Idx != INDEX_NONE && Bag->Items.IsValidIndex(Idx))
	{
		Inventory.EnsureContainedBagForItem(Bag->Items[Idx], Bag);
	}
	if (Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority())
	{
		Inventory.SyncNetState();
	}
	return Idx != INDEX_NONE;
}

int32 FYIInventoryItemIngressService::AddBagItem(UYIInventoryComponent& Inventory, const FYIBagItem& Item)
{
	if (Inventory.GetOwner() && Inventory.GetOwner()->HasAuthority())
	{
		if (!Inventory.EquippedBag)
		{
			return INDEX_NONE;
		}

		FYIBagItem MutableItem = Item;
		if (UYIItemDefinition* Def = MutableItem.Item.Definition.IsValid()
			? MutableItem.Item.Definition.Get()
			: MutableItem.Item.Definition.LoadSynchronous())
		{
			if (YIItemSchema::IsContainerItem(Def))
			{
				MutableItem.Item.Count = 1;
			}
		}

		if (MutableItem.Item.ContainedBagId.IsValid() &&
			Inventory.EquippedBag->BagId.IsValid() &&
			Inventory.IsBagDescendantOf(Inventory.EquippedBag->BagId, MutableItem.Item.ContainedBagId))
		{
			return INDEX_NONE;
		}

		const int32 Idx = Inventory.EquippedBag->AddBagItem(MutableItem);
		if (Idx != INDEX_NONE && Inventory.EquippedBag->Items.IsValidIndex(Idx))
		{
			Inventory.EnsureContainedBagForItem(Inventory.EquippedBag->Items[Idx], Inventory.EquippedBag);
			Inventory.SyncNetState();
		}
		return Idx;
	}

	FYIItemInstance RuntimeItem = Item.Item;
	FYIItemInstanceNet Net;
	Net.Definition = RuntimeItem.Definition;
	Net.Count = RuntimeItem.Count;
	Net.InstanceId = RuntimeItem.InstanceId;
	Net.StackId = RuntimeItem.StackId;
	Net.CustomStackKey = RuntimeItem.CustomStackKey;
	Net.ContainedBagId = RuntimeItem.ContainedBagId;
	Net.bRotated = RuntimeItem.bRotated;
	YIItemInstanceFragments::ExportNetFragmentPayload(RuntimeItem, Net.Fragments);
	Inventory.ServerAddBagItem(Net, Item.Pos, Item.Size);
	return 0;
}

void FYIInventoryItemIngressService::ServerAddBagItem(UYIInventoryComponent& Inventory, const FYIItemInstanceNet& NetItem, FIntPoint Pos, FIntPoint Size)
{
	FYIBagItem Item;
	Item.Item = YIIngress_NetToFull(NetItem);
	Item.Pos = Pos;
	Item.Size = Size;
	AddBagItem(Inventory, Item);
}

