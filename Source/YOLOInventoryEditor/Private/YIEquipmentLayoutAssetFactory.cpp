#include "YIEquipmentLayoutAssetFactory.h"
#include "YIEquipmentLayoutAsset.h"
#include "YIInventoryEditorModule.h"

UYIEquipmentLayoutAssetFactory::UYIEquipmentLayoutAssetFactory()
{
	SupportedClass = UYIEquipmentLayoutAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

uint32 UYIEquipmentLayoutAssetFactory::GetMenuCategories() const
{
	return GetYoLoAssetCategoryBit();
}

UObject* UYIEquipmentLayoutAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIEquipmentLayoutAsset>(InParent, Class, Name, Flags);
}

