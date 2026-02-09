#include "AssetTypeActions_YIRarityProfile.h"
#include "YIRarityProfile.h"
#include "YIInventoryEditorModule.h"

UClass* FAssetTypeActions_YIRarityProfile::GetSupportedClass() const
{
	return UYIRarityProfile::StaticClass();
}

uint32 FAssetTypeActions_YIRarityProfile::GetCategories()
{
	return GYOLOInventoryAssetCategory;
}
