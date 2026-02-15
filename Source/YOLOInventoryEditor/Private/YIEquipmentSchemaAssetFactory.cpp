#include "YIEquipmentSchemaAssetFactory.h"
#include "YIEquipmentSchemaAsset.h"
#include "YIInventoryEditorModule.h"

UYIEquipmentSchemaAssetFactory::UYIEquipmentSchemaAssetFactory()
{
	SupportedClass = UYIEquipmentSchemaAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

uint32 UYIEquipmentSchemaAssetFactory::GetMenuCategories() const
{
	return GetYoLoAssetCategoryBit();
}

UObject* UYIEquipmentSchemaAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIEquipmentSchemaAsset>(InParent, Class, Name, Flags);
}

