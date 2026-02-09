#pragma once
#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_YIRarityPalette : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override { return NSLOCTEXT("YOLOInventory", "RarityPaletteAssetTypeName", "Rarity Palette"); }
	virtual FColor GetTypeColor() const override { return FColor(255, 120, 180); }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
};
