#include "YIFragmentPoolRollStrategyFactory.h"
#include "YIFragmentPoolRollStrategy.h"
#include "YIEditorLootCategory.h"

UYIFragmentPoolRollStrategyFactory::UYIFragmentPoolRollStrategyFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UYIFragmentPoolRollStrategy::StaticClass();
}

UObject* UYIFragmentPoolRollStrategyFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIFragmentPoolRollStrategy>(InParent, Class, Name, Flags);
}

uint32 UYIFragmentPoolRollStrategyFactory::GetMenuCategories() const
{
	return YIEditorLootCategory::Get();
}

