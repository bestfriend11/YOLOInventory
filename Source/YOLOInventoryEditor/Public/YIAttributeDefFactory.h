#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YIAttributeDefFactory.generated.h"

UCLASS()
class UYIAttributeDefFactory : public UFactory
{
	GENERATED_BODY()
public:
	UYIAttributeDefFactory();
	virtual uint32 GetMenuCategories() const override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override { return true; }
	virtual FText GetDisplayName() const override { return NSLOCTEXT("YOLOAttributeDefFactory", "FactoryDisplayName", "YOLO Attribute Def Factory"); }
};
