#include "YIRarityPaletteFactory.h"
#include "YIRarityPalette.h"
#include "YIInventoryEditorModule.h"

UYIRarityPaletteFactory::UYIRarityPaletteFactory()
{
	SupportedClass = UYIRarityPalette::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

uint32 UYIRarityPaletteFactory::GetMenuCategories() const
{
	return GetYoLoAssetCategoryBit();
}

UObject* UYIRarityPaletteFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UYIRarityPalette>(InParent, Class, Name, Flags);
}
