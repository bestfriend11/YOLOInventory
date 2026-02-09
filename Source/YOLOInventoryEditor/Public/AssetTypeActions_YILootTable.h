#pragma once
#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_YILootTable : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override { return NSLOCTEXT("YOLOInventory", "LootTableAssetTypeName", "Loot Table"); }
	virtual FColor GetTypeColor() const override { return FColor(180, 220, 140); }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
};
