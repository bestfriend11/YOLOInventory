#include "YIBagItemDetailsProxy.h"

void UYIBagItemDetailsProxy::LoadFromBag(UYIInventoryBag* InBag, int32 InItemIndex)
{
	SourceBag = InBag;
	ItemIndex = InItemIndex;
	ItemInstance = FYIBagItem();

	if (!InBag || !InBag->Items.IsValidIndex(InItemIndex))
	{
		return;
	}

	ItemInstance = InBag->Items[InItemIndex];
}

