#include "AssetTypeActions_YIEvolutionPath.h"
#include "YIEvolutionPath.h"
#include "YIInventoryEditorModule.h"

UClass* FAssetTypeActions_YIEvolutionPath::GetSupportedClass() const
{
	return UYIEvolutionPath::StaticClass();
}

uint32 FAssetTypeActions_YIEvolutionPath::GetCategories()
{
	return GYOLOInventoryAssetCategory;
}
