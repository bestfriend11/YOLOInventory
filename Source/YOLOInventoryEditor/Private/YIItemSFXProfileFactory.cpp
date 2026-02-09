#include "YIItemSFXProfileFactory.h"
#include "YIItemSFXLibrary.h"
#include "YIInventoryEditorModule.h"

UYIItemSFXProfileFactory::UYIItemSFXProfileFactory()
{
	SupportedClass = UYIItemSFXProfile::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

uint32 UYIItemSFXProfileFactory::GetMenuCategories() const
{
	return GetYoLoAssetCategoryBit();
}

UObject* UYIItemSFXProfileFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIItemSFXProfile>(InParent, Class, Name, Flags);
}
