#include "YIAffixFactory.h"
#include "YIAffixAsset.h"

UYIAffixFactory::UYIAffixFactory()
{
    bCreateNew = true;
    bEditAfterNew = true;
    SupportedClass = UYIAffixAsset::StaticClass();
}

UObject* UYIAffixFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    return NewObject<UYIAffixAsset>(InParent, Class ? Class : UYIAffixAsset::StaticClass(), Name, Flags | RF_Transactional);
}

