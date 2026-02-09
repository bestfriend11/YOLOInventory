#pragma once
#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_YIAttributeDef : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override { return NSLOCTEXT("YOLOInventory", "AttributeDefAssetTypeName", "Attribute Definition"); }
	virtual FColor GetTypeColor() const override { return FColor(140, 200, 255); }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
};
