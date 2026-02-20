#pragma once
#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_YIAttributeMod : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override { return NSLOCTEXT("YOLOInventory", "AttributeModAssetTypeName", "Attribute Mod"); }
	virtual FColor GetTypeColor() const override { return FColor(120, 210, 170); }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
};
