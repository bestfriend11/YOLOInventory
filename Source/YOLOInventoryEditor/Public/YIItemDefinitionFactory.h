#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YIItemDefinitionFactory.generated.h"

class UYIItemDefinition;

UCLASS()
class YOLOINVENTORYEDITOR_API UYIItemDefinitionFactory : public UFactory
{
	GENERATED_BODY()
public:
	UYIItemDefinitionFactory();
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override { return true; }
	virtual FText GetDisplayName() const override { return NSLOCTEXT("YOLOInventory", "ItemDefinitionFactory", "Item Definition"); }
	virtual uint32 GetMenuCategories() const override;
};
