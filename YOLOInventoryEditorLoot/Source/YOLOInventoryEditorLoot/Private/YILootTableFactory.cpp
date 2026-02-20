#include "YILootTableFactory.h"
#include "YILootTable.h"
#include "YIEditorLootCategory.h"

UYILootTableFactory::UYILootTableFactory()
{
	SupportedClass = UYILootTable::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

uint32 UYILootTableFactory::GetMenuCategories() const
{
	return YIEditorLootCategory::Get();
}

UObject* UYILootTableFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYILootTable>(InParent, Class, Name, Flags);
}
