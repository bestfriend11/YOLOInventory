#pragma once
#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_YIItemVariant : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override { return NSLOCTEXT("YOLOInventory", "ItemVariantAssetTypeName", "Item Variant"); }
	virtual FColor GetTypeColor() const override { return FColor(200, 160, 255); }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
};
