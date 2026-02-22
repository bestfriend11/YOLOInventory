#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YIFragmentPoolRollStrategyFactory.generated.h"

UCLASS()
class YOLOINVENTORYEDITORLOOT_API UYIFragmentPoolRollStrategyFactory : public UFactory
{
	GENERATED_BODY()
public:
	UYIFragmentPoolRollStrategyFactory();
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override { return true; }
	virtual FText GetDisplayName() const override { return NSLOCTEXT("YOLOInventory", "FragmentPoolRollStrategyFactory", "Fragment Pool Roll Strategy"); }
	virtual uint32 GetMenuCategories() const override;
};

