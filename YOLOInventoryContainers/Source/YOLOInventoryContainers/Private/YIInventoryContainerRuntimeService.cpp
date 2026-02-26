#include "YIInventoryContainerRuntimeService.h"

#include "YIInventoryBag.h"
#include "YIInventoryComponent.h"
#include "YIItemDefinition.h"
#include "YIItemSchemaResolver.h"

namespace
{
	static void YIContainers_ReassignRuntimeItemIdentitiesForClonedBag(UYIInventoryBag* Bag)
	{
		if (!Bag)
		{
			return;
		}

		for (FYIBagItem& Item : Bag->Items)
		{
			Item.Item.InstanceId = FGuid::NewGuid();
			Item.Item.StackId = FGuid::NewGuid();
			Item.Item.ContainedBagId.Invalidate();

			// Container runtime instances should not stack.
			if (UYIItemDefinition* Def = Item.Item.Definition.IsValid()
				? Item.Item.Definition.Get()
				: Item.Item.Definition.LoadSynchronous())
			{
				if (YIItemSchema::IsContainerItem(Def))
				{
					Item.Item.Count = 1;
				}
			}
		}
	}
}

UYIInventoryBag* FYIInventoryContainerRuntimeService::EnsureContainedBagForItem(UYIInventoryComponent& Inventory, FYIBagItem& InOutItem, const UYIInventoryBag* ParentBag)
{
	(void)ParentBag;
	UYIItemDefinition* Definition = InOutItem.Item.Definition.IsValid()
		? InOutItem.Item.Definition.Get()
		: InOutItem.Item.Definition.LoadSynchronous();
	if (!Definition || !YIItemSchema::IsContainerItem(Definition))
	{
		return nullptr;
	}

	if (InOutItem.Item.ContainedBagId.IsValid())
	{
		if (UYIInventoryBag* Existing = Inventory.GetBagById(InOutItem.Item.ContainedBagId))
		{
			return Existing;
		}
	}

	UYIInventoryBag* ChildBag = nullptr;
	if (const UYIInventoryBag* TemplateBag = Cast<UYIInventoryBag>(YIItemSchema::GetContainerTemplateBag(Definition).LoadSynchronous()))
	{
		ChildBag = CloneBagTemplate(Inventory, TemplateBag);
	}
	else
	{
		ChildBag = NewObject<UYIInventoryBag>(&Inventory);
		if (ChildBag)
		{
			ChildBag->EnsureBagId();
			const FText EffectiveName = YIItemSchema::GetDisplayName(Definition);
			ChildBag->DisplayName = EffectiveName.IsEmpty()
				? FText::FromString(TEXT("Container"))
				: EffectiveName;
			const FIntPoint DefaultGrid = YIItemSchema::GetContainerDefaultGridSize(Definition);
			ChildBag->GridSize = FIntPoint(
				FMath::Max(1, DefaultGrid.X),
				FMath::Max(1, DefaultGrid.Y));
			ChildBag->bAllowRotation = true;
		}
	}

	if (!ChildBag)
	{
		return nullptr;
	}

	ChildBag->EnsureBagId();
	if (!Inventory.Bags.Contains(ChildBag))
	{
		Inventory.Bags.Add(ChildBag);
	}
	InOutItem.Item.ContainedBagId = ChildBag->BagId;
	InOutItem.Item.Count = 1;
	return ChildBag;
}

bool FYIInventoryContainerRuntimeService::TryOpenContainedBagInternal(UYIInventoryComponent& Inventory, UYIInventoryBag* ParentBag, int32 ItemIndex)
{
	if (!ParentBag || !ParentBag->Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	FYIBagItem& Item = ParentBag->Items[ItemIndex];
	UYIInventoryBag* ChildBag = EnsureContainedBagForItem(Inventory, Item, ParentBag);
	if (!ChildBag)
	{
		return false;
	}

	if (!ChildBag->BagId.IsValid())
	{
		ChildBag->EnsureBagId();
	}

	if (ParentBag->BagId.IsValid() && Inventory.IsBagDescendantOf(ParentBag->BagId, ChildBag->BagId))
	{
		return false;
	}

	Inventory.OpenBag(ChildBag);
	return true;
}

UYIInventoryBag* FYIInventoryContainerRuntimeService::CloneBagTemplate(UYIInventoryComponent& Inventory, const UYIInventoryBag* TemplateBag)
{
	if (!TemplateBag)
	{
		return nullptr;
	}

	UYIInventoryBag* NewBag = NewObject<UYIInventoryBag>(&Inventory);
	if (!NewBag)
	{
		return nullptr;
	}
	NewBag->EnsureBagId();

	NewBag->DisplayName = TemplateBag->DisplayName;
	NewBag->BagRoleTag = TemplateBag->BagRoleTag;
	NewBag->GridSize = TemplateBag->GridSize;
	NewBag->CellPixelSize = TemplateBag->CellPixelSize;
	NewBag->bAllowRotation = TemplateBag->bAllowRotation;
	NewBag->MinifyScale = TemplateBag->MinifyScale;
	NewBag->GridStyleAsset = TemplateBag->GridStyleAsset;
	NewBag->GridLineColor = TemplateBag->GridLineColor;
	NewBag->OuterLineColor = TemplateBag->OuterLineColor;
	NewBag->CellBgColor = TemplateBag->CellBgColor;
	NewBag->GridThickness = TemplateBag->GridThickness;
	NewBag->bShowCellTooltips = TemplateBag->bShowCellTooltips;
	NewBag->bShowSortingHeaders = TemplateBag->bShowSortingHeaders;
	NewBag->bEnableThumbnails = TemplateBag->bEnableThumbnails;
	NewBag->bEnableHoverHighlight = TemplateBag->bEnableHoverHighlight;
	NewBag->bUseTagFilter = TemplateBag->bUseTagFilter;
	NewBag->TagFilters = TemplateBag->TagFilters;
	NewBag->bUseFolderFilter = TemplateBag->bUseFolderFilter;
	NewBag->FolderFilters = TemplateBag->FolderFilters;
	NewBag->bAutoMergeOnAdd = TemplateBag->bAutoMergeOnAdd;

	NewBag->Items = TemplateBag->Items;
	YIContainers_ReassignRuntimeItemIdentitiesForClonedBag(NewBag);
	return NewBag;
}

