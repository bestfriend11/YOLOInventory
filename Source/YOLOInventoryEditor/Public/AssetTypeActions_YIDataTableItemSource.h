#pragma once
#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_YIDataTableItemSource : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override { return NSLOCTEXT("YOLOInventory", "DataTableItemSourceAssetTypeName", "Data Table Item Source"); }
	virtual FColor GetTypeColor() const override { return FColor(200, 200, 120); }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
};
