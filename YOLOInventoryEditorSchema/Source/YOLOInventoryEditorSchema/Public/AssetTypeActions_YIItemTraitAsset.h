#pragma once

#include "CoreMinimal.h"
#include "AssetTypeActions_Base.h"

class FAssetTypeActions_YIItemTraitAsset : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override { return NSLOCTEXT("YOLOInventory", "ItemTraitAssetTypeName", "Item Trait Asset"); }
	virtual FColor GetTypeColor() const override { return FColor(160, 220, 255); }
	virtual UClass* GetSupportedClass() const override;
	virtual uint32 GetCategories() override;
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return false; }
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<class IToolkitHost> EditWithinLevelEditor) override;
};

