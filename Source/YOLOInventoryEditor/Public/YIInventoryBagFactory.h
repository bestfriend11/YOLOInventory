#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YIInventoryBagFactory.generated.h"

UCLASS()
class YOLOINVENTORYEDITOR_API UYIInventoryBagFactory : public UFactory
{
	GENERATED_BODY()
public:
	UYIInventoryBagFactory();
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override { return true; }
	virtual uint32 GetMenuCategories() const override;
};
