#include "AssetTypeActions_YIItemGenerator.h"
#include "YIItemGenerator.h"
#include "YIInventoryEditorModule.h"

UClass* FAssetTypeActions_YIItemGenerator::GetSupportedClass() const
{
	return UYIItemGenerator::StaticClass();
}

uint32 FAssetTypeActions_YIItemGenerator::GetCategories()
{
	return GYOLOInventoryAssetCategory;
}
