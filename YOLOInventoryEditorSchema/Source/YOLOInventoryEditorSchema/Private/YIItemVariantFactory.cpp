#include "YIItemVariantFactory.h"
#include "YIItemVariant.h"
#include "YIEditorSchemaCategory.h"

UYIItemVariantFactory::UYIItemVariantFactory()
{
	SupportedClass = UYIItemVariantAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

uint32 UYIItemVariantFactory::GetMenuCategories() const
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}

UObject* UYIItemVariantFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIItemVariantAsset>(InParent, Class, Name, Flags);
}

