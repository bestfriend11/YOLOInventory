#include "YIEvolutionPathFactory.h"
#include "YIEvolutionPath.h"
#include "YIEditorSchemaCategory.h"

UYIEvolutionPathFactory::UYIEvolutionPathFactory()
{
	SupportedClass = UYIEvolutionPath::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

uint32 UYIEvolutionPathFactory::GetMenuCategories() const
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}

UObject* UYIEvolutionPathFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIEvolutionPath>(InParent, Class, Name, Flags);
}

