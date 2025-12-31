#pragma once
#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_YIAffix : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override { return NSLOCTEXT("YOLOInventory", "AffixAssetTypeName", "Affix"); }
	virtual FColor GetTypeColor() const override { return FColor(255, 200, 100); }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
};
