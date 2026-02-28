#include "YIItemDescriptionBlueprintLibrary.h"

#include "YIInventoryBag.h"
#include "YIItemDescriptionResolver.h"
#include "YIShopComponent.h"

bool UYIItemDescriptionBlueprintLibrary::BuildRichTooltipForBagItem(
	const UYIInventoryBag* Bag,
	int32 ItemIndex,
	FYITooltipData& OutTooltipData,
	APlayerState* ViewerPlayerState,
	UYIShopComponent* Shop,
	bool bShopBuyContext,
	int32 Count)
{
	OutTooltipData = FYITooltipData();
	if (!Bag || !Bag->Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	if (!UYIInventoryBlueprintLibrary::GetItemTooltipData(Bag, ItemIndex, OutTooltipData))
	{
		return false;
	}

	FYIItemDescriptionContext DescriptionContext;
	DescriptionContext.Item = &Bag->Items[ItemIndex].Item;
	DescriptionContext.Bag = Bag;
	DescriptionContext.Shop = Shop;
	DescriptionContext.ViewerPlayerState = ViewerPlayerState;
	DescriptionContext.bShopBuyContext = (Shop != nullptr) && bShopBuyContext;
	DescriptionContext.Count = FMath::Max(1, Count);
	FYIItemDescriptionResolver::AugmentTooltip(DescriptionContext, OutTooltipData);
	return true;
}
