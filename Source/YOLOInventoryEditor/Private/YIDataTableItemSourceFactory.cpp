#include "YIDataTableItemSourceFactory.h"
#include "Data/YIDataTableItemSource.h"
#include "YIInventoryEditorModule.h"

UYIDataTableItemSourceFactory::UYIDataTableItemSourceFactory()
{
	SupportedClass = UYIDataTableItemSource::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

uint32 UYIDataTableItemSourceFactory::GetMenuCategories() const
{
	return GetYoLoAssetCategoryBit();
}

UObject* UYIDataTableItemSourceFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIDataTableItemSource>(InParent, Class, Name, Flags);
}
