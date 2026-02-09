#include "YIRarityProfileFactory.h"
#include "YIRarityProfile.h"
#include "YIInventoryEditorModule.h"

UYIRarityProfileFactory::UYIRarityProfileFactory()
{
	SupportedClass = UYIRarityProfile::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

uint32 UYIRarityProfileFactory::GetMenuCategories() const
{
	return GetYoLoAssetCategoryBit();
}

UObject* UYIRarityProfileFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIRarityProfile>(InParent, Class, Name, Flags);
}
