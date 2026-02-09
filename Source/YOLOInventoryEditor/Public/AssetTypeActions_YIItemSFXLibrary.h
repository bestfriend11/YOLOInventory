#pragma once
#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_YIItemSFXLibrary : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override { return NSLOCTEXT("YOLOInventory", "ItemSFXLibraryAssetTypeName", "Item SFX Library"); }
	virtual FColor GetTypeColor() const override { return FColor(80, 180, 220); }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
};
