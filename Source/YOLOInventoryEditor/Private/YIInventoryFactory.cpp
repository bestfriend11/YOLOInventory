#include "YIInventoryFactory.h"
#include "YIInventoryEditorModule.h"

UYIInventoryFactory::UYIInventoryFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UYIItemDefinition::StaticClass();
}

uint32 UYIInventoryFactory::GetMenuCategories() const
{
	return GYOLOInventoryAssetCategory;
}

UObject* UYIInventoryFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UYIItemDefinition* NewAsset = NewObject<UYIItemDefinition>(InParent, Class, Name, Flags);
#if WITH_EDITORONLY_DATA
	// Legacy graph/script creation removed; UYIItemDefinition no longer has Graph/ScriptGraph members.
#endif
	return NewAsset;
}
