#include "YIInventoryMirrorService.h"

#include "YIInventoryBag.h"
#include "YIInventoryBlueprintLibrary.h"
#include "YIInventoryComponent.h"
#include "YIItemDefinition.h"
#include "YIItemRegistrySubsystem.h"
#include "YIItemSchemaResolver.h"
#include "Engine/Engine.h"

namespace
{
	static FYIItemInstance YIMirror_MakeItemInstanceByCode(int64 Code, int32 Count)
	{
		FYIItemInstance Out;
		Out.Count = Count;
		if (Code == 0 || !GEngine)
		{
			return Out;
		}

		if (UYIItemRegistrySubsystem* Registry = GEngine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
		{
			Out.Definition = Registry->GetByCode(Code);
		}
		return Out;
	}

	static void YIMirror_AppendNetBagItems(const UYIInventoryBag* Bag, TArray<FYINetBagItem>& OutItems)
	{
		if (!Bag)
		{
			return;
		}

		for (const FYIBagItem& It : Bag->Items)
		{
			if (It.Item.Count <= 0)
			{
				continue;
			}

			FYINetBagItem Net;
			Net.Code = It.Item.Definition.IsValid() ? It.Item.Definition.Get()->UniqueCode : 0;
			if (Net.Code == 0 && It.Item.Definition.ToSoftObjectPath().IsValid())
			{
				if (UYIItemDefinition* Def = Cast<UYIItemDefinition>(It.Item.Definition.LoadSynchronous()))
				{
					Net.Code = Def->UniqueCode;
				}
			}
			Net.Count = It.Item.Count;
			Net.InstanceId = It.Item.InstanceId;
			Net.StackId = It.Item.StackId;
			Net.Pos = It.Pos;
			Net.Size = It.Size;
			Net.CustomStackKey = It.Item.CustomStackKey;
			Net.ContainedBagId = It.Item.ContainedBagId;
			OutItems.Add(Net);
		}
	}
}

UYIInventoryBag* FYIInventoryMirrorService::FindClientContextPreviewBagById(const UYIInventoryComponent& Inventory, const FGuid& BagId)
{
	if (!BagId.IsValid())
	{
		return nullptr;
	}
	for (UYIInventoryBag* Bag : Inventory.ClientContextPreviewBags)
	{
		if (Bag && Bag->BagId == BagId)
		{
			return Bag;
		}
	}
	return nullptr;
}

UYIInventoryBag* FYIInventoryMirrorService::FindOrCreateClientContextPreviewBagById(UYIInventoryComponent& Inventory, const FGuid& BagId)
{
	if (!BagId.IsValid())
	{
		return nullptr;
	}
	if (UYIInventoryBag* Existing = FindClientContextPreviewBagById(Inventory, BagId))
	{
		return Existing;
	}
	UYIInventoryBag* NewPreview = NewObject<UYIInventoryBag>(&Inventory);
	if (!NewPreview)
	{
		return nullptr;
	}
	NewPreview->BagId = BagId;
	Inventory.ClientContextPreviewBags.Add(NewPreview);
	return NewPreview;
}

void FYIInventoryMirrorService::RebuildClientPreviewBagFromNet(
	UYIInventoryComponent& Inventory,
	UYIInventoryBag* TargetBag,
	const TArray<FYINetBagItem>& InItems,
	const FIntPoint& InGridSize,
	const FGuid& InBagId)
{
	(void)Inventory;
	if (!TargetBag)
	{
		return;
	}

	TargetBag->GridSize = InGridSize;
	TargetBag->BagId = InBagId;
	TargetBag->Items.Reset();

	for (const FYINetBagItem& Net : InItems)
	{
		if (Net.Code == 0 || Net.Count <= 0)
		{
			continue;
		}

		FYIBagItem Item;
		Item.Item = YIMirror_MakeItemInstanceByCode(Net.Code, Net.Count);
		if (Net.InstanceId.IsValid())
		{
			Item.Item.InstanceId = Net.InstanceId;
		}
		if (Net.StackId.IsValid())
		{
			Item.Item.StackId = Net.StackId;
		}
		Item.Item.CustomStackKey = Net.CustomStackKey;
		Item.Item.ContainedBagId = Net.ContainedBagId;
		Item.Pos = Net.Pos;
		Item.Size = Net.Size;
		TargetBag->Items.Add(Item);
	}
}

void FYIInventoryMirrorService::SyncNetState(UYIInventoryComponent& Inventory)
{
	if (!Inventory.GetOwner() || Inventory.GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (!Inventory.ActiveBagId.IsValid() && Inventory.EquippedBag)
	{
		Inventory.EquippedBag->EnsureBagId();
		Inventory.ActiveBagId = Inventory.EquippedBag->BagId;
	}
	if (!Inventory.ActiveBagId.IsValid())
	{
		for (UYIInventoryBag* Bag : Inventory.Bags)
		{
			if (Bag)
			{
				Bag->EnsureBagId();
				Inventory.ActiveBagId = Bag->BagId;
				break;
			}
		}
	}

	Inventory.NetBagItems.Reset();
	Inventory.NetBagDescriptors.Reset();
	Inventory.NetContextBagMirrors.Reset();
	Inventory.NetBagDescriptors.Reserve(Inventory.Bags.Num());

	for (UYIInventoryBag* Bag : Inventory.Bags)
	{
		if (!Bag)
		{
			continue;
		}
		Bag->EnsureBagId();
		FYINetBagDescriptor Desc;
		Desc.BagId = Bag->BagId;
		Desc.DisplayName = Bag->DisplayName;
		Desc.BagRoleTag = Bag->BagRoleTag;
		Desc.GridSize = Bag->GridSize;
		Desc.ItemCount = Bag->Items.Num();
		Desc.ParentBagId.Invalidate();
		Desc.ParentItemInstanceId.Invalidate();
		Desc.bIsNestedContainer = Inventory.FindContainerParentForBag(Bag->BagId, Desc.ParentBagId, Desc.ParentItemInstanceId);
		Desc.bIsActive = (Bag->BagId == Inventory.ActiveBagId);
		Inventory.NetBagDescriptors.Add(Desc);
	}

	if (Inventory.EquippedBag)
	{
		Inventory.EquippedBag->EnsureBagId();
		Inventory.NetBagGridSize = Inventory.EquippedBag->GridSize;
		YIMirror_AppendNetBagItems(Inventory.EquippedBag, Inventory.NetBagItems);
	}

	TSet<FGuid> MirroredContextBagIds;
	for (const FYIActiveBagContextEntry& ContextEntry : Inventory.ActiveBagContexts)
	{
		if (!ContextEntry.BagId.IsValid() || ContextEntry.BagId == Inventory.ActiveBagId || MirroredContextBagIds.Contains(ContextEntry.BagId))
		{
			continue;
		}

		UYIInventoryBag* ContextBag = Inventory.GetBagById(ContextEntry.BagId);
		if (!ContextBag)
		{
			continue;
		}

		ContextBag->EnsureBagId();
		MirroredContextBagIds.Add(ContextEntry.BagId);

		FYINetBagMirrorView& Mirror = Inventory.NetContextBagMirrors.AddDefaulted_GetRef();
		Mirror.BagId = ContextBag->BagId;
		Mirror.GridSize = ContextBag->GridSize;
		Mirror.Items.Reserve(ContextBag->Items.Num());
		YIMirror_AppendNetBagItems(ContextBag, Mirror.Items);
	}

	if (AActor* OwnerActor = Inventory.GetOwner())
	{
		OwnerActor->ForceNetUpdate();
	}
}

void FYIInventoryMirrorService::OnRep_NetBag(UYIInventoryComponent& Inventory)
{
	if (!Inventory.ClientPreviewBag)
	{
		Inventory.ClientPreviewBag = NewObject<UYIInventoryBag>(&Inventory);
		if (!Inventory.ClientPreviewBag)
		{
			return;
		}
	}

	RebuildClientPreviewBagFromNet(Inventory, Inventory.ClientPreviewBag, Inventory.NetBagItems, Inventory.NetBagGridSize, Inventory.ActiveBagId);
	Inventory.ClientPreviewBag->OnChanged.Broadcast();
	Inventory.OnBagOpened.Broadcast(Inventory.ClientPreviewBag);
}

void FYIInventoryMirrorService::OnRep_NetBagDescriptors(UYIInventoryComponent& Inventory)
{
	(void)Inventory;
}

void FYIInventoryMirrorService::OnRep_ActiveBagContexts(UYIInventoryComponent& Inventory)
{
	if (UYIInventoryBag* ActiveBag = Inventory.GetBagById(Inventory.ActiveBagId))
	{
		Inventory.OnBagOpened.Broadcast(ActiveBag);
	}
	else if (Inventory.ClientPreviewBag && Inventory.ActiveBagId.IsValid())
	{
		Inventory.OnBagOpened.Broadcast(Inventory.ClientPreviewBag);
	}

	for (const FYIActiveBagContextEntry& ContextEntry : Inventory.ActiveBagContexts)
	{
		if (UYIInventoryBag* ContextBag = Inventory.GetBagById(ContextEntry.BagId))
		{
			Inventory.OnBagOpened.Broadcast(ContextBag);
		}
	}
}

void FYIInventoryMirrorService::OnRep_NetContextBagMirrors(UYIInventoryComponent& Inventory)
{
	TSet<FGuid> IncomingBagIds;
	for (const FYINetBagMirrorView& Mirror : Inventory.NetContextBagMirrors)
	{
		if (!Mirror.BagId.IsValid())
		{
			continue;
		}

		IncomingBagIds.Add(Mirror.BagId);
		if (UYIInventoryBag* PreviewBag = FindOrCreateClientContextPreviewBagById(Inventory, Mirror.BagId))
		{
			RebuildClientPreviewBagFromNet(Inventory, PreviewBag, Mirror.Items, Mirror.GridSize, Mirror.BagId);
			PreviewBag->OnChanged.Broadcast();
			Inventory.OnBagOpened.Broadcast(PreviewBag);
		}
	}

	for (int32 Index = Inventory.ClientContextPreviewBags.Num() - 1; Index >= 0; --Index)
	{
		UYIInventoryBag* PreviewBag = Inventory.ClientContextPreviewBags[Index];
		if (!PreviewBag || !PreviewBag->BagId.IsValid() || IncomingBagIds.Contains(PreviewBag->BagId))
		{
			continue;
		}

		Inventory.OnBagClosed.Broadcast(PreviewBag);
		Inventory.ClientContextPreviewBags.RemoveAtSwap(Index, 1, EAllowShrinking::No);
	}
}

void FYIInventoryMirrorService::OnRep_LockedBagItems(UYIInventoryComponent& Inventory)
{
	if (Inventory.ClientPreviewBag)
	{
		Inventory.ClientPreviewBag->OnChanged.Broadcast();
	}
	for (UYIInventoryBag* ContextPreviewBag : Inventory.ClientContextPreviewBags)
	{
		if (ContextPreviewBag)
		{
			ContextPreviewBag->OnChanged.Broadcast();
		}
	}
	if (Inventory.EquippedBag)
	{
		Inventory.EquippedBag->OnChanged.Broadcast();
	}
}

