#pragma once
#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_YIEvolutionPath : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override { return NSLOCTEXT("YOLOInventory", "EvolutionPathAssetTypeName", "Evolution Path"); }
	virtual FColor GetTypeColor() const override { return FColor(150, 220, 120); }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
};
