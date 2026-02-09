#include "YIEvolutionPathFactory.h"
#include "YIEvolutionPath.h"
#include "YIInventoryEditorModule.h"

UYIEvolutionPathFactory::UYIEvolutionPathFactory()
{
	SupportedClass = UYIEvolutionPath::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

uint32 UYIEvolutionPathFactory::GetMenuCategories() const
{
	return GetYoLoAssetCategoryBit();
}

UObject* UYIEvolutionPathFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIEvolutionPath>(InParent, Class, Name, Flags);
}
