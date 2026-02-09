#include "YIItemGeneratorFactory.h"
#include "YIItemGenerator.h"
#include "YIInventoryEditorModule.h"

UYIItemGeneratorFactory::UYIItemGeneratorFactory()
{
	SupportedClass = UYIItemGenerator::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

uint32 UYIItemGeneratorFactory::GetMenuCategories() const
{
	return GetYoLoAssetCategoryBit();
}

UObject* UYIItemGeneratorFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIItemGenerator>(InParent, Class, Name, Flags);
}
