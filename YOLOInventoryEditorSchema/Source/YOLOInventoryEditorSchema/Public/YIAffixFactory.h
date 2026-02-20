#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "YIAffixFactory.generated.h"

UCLASS()
class UYIAffixFactory : public UFactory
{
    GENERATED_BODY()
public:
    UYIAffixFactory();

    // UFactory
    virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
    virtual bool ShouldShowInNewMenu() const override { return true; }
};
