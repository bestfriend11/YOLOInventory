#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YIDataTableItemSourceFactory.generated.h"

UCLASS()
class UYIDataTableItemSourceFactory : public UFactory
{
	GENERATED_BODY()
public:
	UYIDataTableItemSourceFactory();
	virtual uint32 GetMenuCategories() const override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override { return true; }
	virtual FText GetDisplayName() const override { return NSLOCTEXT("YOLODataTableItemSourceFactory", "FactoryDisplayName", "YOLO Data Table Item Source"); }
};
