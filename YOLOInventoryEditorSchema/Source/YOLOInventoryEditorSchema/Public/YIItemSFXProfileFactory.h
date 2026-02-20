#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YIItemSFXProfileFactory.generated.h"

UCLASS()
class UYIItemSFXProfileFactory : public UFactory
{
	GENERATED_BODY()
public:
	UYIItemSFXProfileFactory();
	virtual uint32 GetMenuCategories() const override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override { return true; }
	virtual FText GetDisplayName() const override { return NSLOCTEXT("YOLOItemSFXProfileFactory", "FactoryDisplayName", "YOLO Item SFX Profile"); }
};
