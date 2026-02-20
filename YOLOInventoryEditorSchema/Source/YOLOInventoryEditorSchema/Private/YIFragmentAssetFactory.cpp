#include "YIFragmentAssetFactory.h"
#include "YIFragmentAsset.h"
#include "YIEditorSchemaCategory.h"

UYIFragmentAssetFactory::UYIFragmentAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UYIFragmentAsset::StaticClass();
}

UObject* UYIFragmentAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIFragmentAsset>(InParent, Class, Name, Flags);
}

uint32 UYIFragmentAssetFactory::GetMenuCategories() const
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}

