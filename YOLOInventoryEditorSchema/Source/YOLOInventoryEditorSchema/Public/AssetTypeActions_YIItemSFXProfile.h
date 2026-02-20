#pragma once
#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_YIItemSFXProfile : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override { return NSLOCTEXT("YOLOInventory", "ItemSFXProfileAssetTypeName", "Item SFX Profile"); }
	virtual FColor GetTypeColor() const override { return FColor(100, 200, 200); }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
};
