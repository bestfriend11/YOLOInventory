#include "YIItemDefinitionFactory.h"
#include "YIItemDefinition.h"
#include "AssetTypeCategories.h"
#include "YIEditorSchemaCategory.h"

UYIItemDefinitionFactory::UYIItemDefinitionFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UYIItemDefinition::StaticClass();
}

UObject* UYIItemDefinitionFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIItemDefinition>(InParent, Class, Name, Flags);
}

uint32 UYIItemDefinitionFactory::GetMenuCategories() const
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}

