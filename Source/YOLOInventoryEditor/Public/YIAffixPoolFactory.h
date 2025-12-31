#pragma once
#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YIAffixPoolFactory.generated.h"

UCLASS()
class UYIAffixPoolFactory : public UFactory
{
    GENERATED_BODY()
public:
    UYIAffixPoolFactory();
    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};