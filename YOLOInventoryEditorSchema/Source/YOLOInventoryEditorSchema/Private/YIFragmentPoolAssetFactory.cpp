#include "YIFragmentPoolAssetFactory.h"
#include "YIFragmentPoolAsset.h"
#include "YIEditorSchemaCategory.h"

UYIFragmentPoolAssetFactory::UYIFragmentPoolAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UYIFragmentPoolAsset::StaticClass();
}

UObject* UYIFragmentPoolAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIFragmentPoolAsset>(InParent, Class, Name, Flags);
}

uint32 UYIFragmentPoolAssetFactory::GetMenuCategories() const
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}

