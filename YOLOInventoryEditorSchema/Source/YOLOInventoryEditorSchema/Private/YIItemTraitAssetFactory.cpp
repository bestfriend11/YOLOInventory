#include "YIItemTraitAssetFactory.h"
#include "YIItemTraitAsset.h"
#include "YIEditorSchemaCategory.h"

UYIItemTraitAssetFactory::UYIItemTraitAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UYIItemTraitAsset::StaticClass();
}

UObject* UYIItemTraitAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIItemTraitAsset>(InParent, Class, Name, Flags);
}

uint32 UYIItemTraitAssetFactory::GetMenuCategories() const
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}

