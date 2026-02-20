#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YIFragmentAssetFactory.generated.h"

class UYIFragmentAsset;

UCLASS()
class YOLOINVENTORYEDITORSCHEMA_API UYIFragmentAssetFactory : public UFactory
{
	GENERATED_BODY()
public:
	UYIFragmentAssetFactory();
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override { return true; }
	virtual FText GetDisplayName() const override { return NSLOCTEXT("YOLOInventory", "FragmentAssetFactory", "Fragment Asset"); }
	virtual uint32 GetMenuCategories() const override;
};

