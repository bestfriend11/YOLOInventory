#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "AssetTypeCategories.h"
#include "YIInventoryFactory.generated.h"

UCLASS()
class YOLOINVENTORYEDITOR_API UYIInventoryFactory : public UFactory
{
	GENERATED_BODY()
public:
	UYIInventoryFactory();

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override { return false; }
	virtual FText GetDisplayName() const override { return NSLOCTEXT("YOLOInventory", "FactoryDisplayName", "YOLO Inventory"); }
	virtual uint32 GetMenuCategories() const override;
};
