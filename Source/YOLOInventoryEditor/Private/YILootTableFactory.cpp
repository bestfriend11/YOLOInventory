#include "YILootTableFactory.h"
#include "YILootTable.h"
#include "YIInventoryEditorModule.h"

UYILootTableFactory::UYILootTableFactory()
{
	SupportedClass = UYILootTable::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

uint32 UYILootTableFactory::GetMenuCategories() const
{
	return GetYoLoAssetCategoryBit();
}

UObject* UYILootTableFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYILootTable>(InParent, Class, Name, Flags);
}
