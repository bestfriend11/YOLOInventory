#include "YIAttributeModFactory.h"
#include "YIAttributeModAsset.h"
#include "YIInventoryEditorModule.h"

UYIAttributeModFactory::UYIAttributeModFactory()
{
	SupportedClass = UYIAttributeModAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

uint32 UYIAttributeModFactory::GetMenuCategories() const
{
	// Use YOLO Inventory category if available
	return GetYoLoAssetCategoryBit();
}

UObject* UYIAttributeModFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIAttributeModAsset>(InParent, Class, Name, Flags);
}
