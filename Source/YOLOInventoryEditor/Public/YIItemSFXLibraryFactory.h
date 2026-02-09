#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YIItemSFXLibraryFactory.generated.h"

UCLASS()
class UYIItemSFXLibraryFactory : public UFactory
{
	GENERATED_BODY()
public:
	UYIItemSFXLibraryFactory();
	virtual uint32 GetMenuCategories() const override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override { return true; }
	virtual FText GetDisplayName() const override { return NSLOCTEXT("YOLOItemSFXLibraryFactory", "FactoryDisplayName", "YOLO Item SFX Library"); }
};
