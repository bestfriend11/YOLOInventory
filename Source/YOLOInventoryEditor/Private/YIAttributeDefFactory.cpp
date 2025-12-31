#include "YIAttributeDefFactory.h"
#include "YIAttributeDef.h"
#include "YIInventoryEditorModule.h"

UYIAttributeDefFactory::UYIAttributeDefFactory()
{
	SupportedClass = UYIAttributeDef::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

uint32 UYIAttributeDefFactory::GetMenuCategories() const
{
	return GetYoLoAssetCategoryBit();
}

UObject* UYIAttributeDefFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIAttributeDef>(InParent, Class, Name, Flags);
}
