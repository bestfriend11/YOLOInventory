#include "YIEquipmentSchemaAssetFactory.h"
#include "YIEquipmentSchemaAsset.h"
#include "YIEditorGridCategory.h"

UYIEquipmentSchemaAssetFactory::UYIEquipmentSchemaAssetFactory()
{
	SupportedClass = UYIEquipmentSchemaAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

uint32 UYIEquipmentSchemaAssetFactory::GetMenuCategories() const
{
	return YIEditorGridCategory::Get();
}

UObject* UYIEquipmentSchemaAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIEquipmentSchemaAsset>(InParent, Class, Name, Flags);
}
