#include "YIAffixPoolFactory.h"
#include "YIAffixPoolAsset.h"

UYIAffixPoolFactory::UYIAffixPoolFactory()
{
    bCreateNew = true;
    bEditAfterNew = true;
    SupportedClass = UYIAffixPoolAsset::StaticClass();
}

UObject* UYIAffixPoolFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
    return NewObject<UYIAffixPoolAsset>(InParent, Class ? Class : UYIAffixPoolAsset::StaticClass(), Name, Flags | RF_Transactional);
}