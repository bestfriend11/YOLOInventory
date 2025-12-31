#include "YIItemVariantFactory.h"
#include "YIItemVariant.h"
#include "YIInventoryEditorModule.h"

UYIItemVariantFactory::UYIItemVariantFactory()
{
	SupportedClass = UYIItemVariantAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

uint32 UYIItemVariantFactory::GetMenuCategories() const
{
	return GetYoLoAssetCategoryBit();
}

UObject* UYIItemVariantFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIItemVariantAsset>(InParent, Class, Name, Flags);
}
