#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YIFragmentPoolAssetFactory.generated.h"

class UYIFragmentPoolAsset;

UCLASS()
class YOLOINVENTORYEDITORSCHEMA_API UYIFragmentPoolAssetFactory : public UFactory
{
	GENERATED_BODY()
public:
	UYIFragmentPoolAssetFactory();
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override { return true; }
	virtual FText GetDisplayName() const override { return NSLOCTEXT("YOLOInventory", "FragmentPoolAssetFactory", "Fragment Pool Asset"); }
	virtual uint32 GetMenuCategories() const override;
};

