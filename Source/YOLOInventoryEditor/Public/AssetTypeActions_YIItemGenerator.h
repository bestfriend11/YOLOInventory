#pragma once
#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_YIItemGenerator : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override { return NSLOCTEXT("YOLOInventory", "ItemGeneratorAssetTypeName", "Item Generator"); }
	virtual FColor GetTypeColor() const override { return FColor(150, 220, 240); }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
};
