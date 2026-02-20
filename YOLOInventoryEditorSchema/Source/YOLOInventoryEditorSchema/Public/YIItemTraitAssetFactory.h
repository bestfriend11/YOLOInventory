#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YIItemTraitAssetFactory.generated.h"

class UYIItemTraitAsset;

UCLASS()
class YOLOINVENTORYEDITORSCHEMA_API UYIItemTraitAssetFactory : public UFactory
{
	GENERATED_BODY()
public:
	UYIItemTraitAssetFactory();
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override { return true; }
	virtual FText GetDisplayName() const override { return NSLOCTEXT("YOLOInventory", "ItemTraitAssetFactory", "Item Trait Asset"); }
	virtual uint32 GetMenuCategories() const override;
};

