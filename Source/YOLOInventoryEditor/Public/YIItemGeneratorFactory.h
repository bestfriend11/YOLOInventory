#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YIItemGeneratorFactory.generated.h"

UCLASS()
class YOLOINVENTORYEDITOR_API UYIItemGeneratorFactory : public UFactory
{
	GENERATED_BODY()
public:
	UYIItemGeneratorFactory();
	virtual uint32 GetMenuCategories() const override;
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
