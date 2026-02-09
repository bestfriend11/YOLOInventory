#include "YIItemSFXLibraryFactory.h"
#include "YIItemSFXLibrary.h"
#include "YIInventoryEditorModule.h"

UYIItemSFXLibraryFactory::UYIItemSFXLibraryFactory()
{
	SupportedClass = UYIItemSFXLibrary::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

uint32 UYIItemSFXLibraryFactory::GetMenuCategories() const
{
	return GetYoLoAssetCategoryBit();
}

UObject* UYIItemSFXLibraryFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIItemSFXLibrary>(InParent, Class, Name, Flags);
}
