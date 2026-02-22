#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_YIFragmentPoolAsset : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override { return NSLOCTEXT("YOLOInventory", "FragmentPoolAssetTypeName", "Fragment Pool Asset"); }
	virtual FColor GetTypeColor() const override { return FColor(108, 198, 255); }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return false; }
};

