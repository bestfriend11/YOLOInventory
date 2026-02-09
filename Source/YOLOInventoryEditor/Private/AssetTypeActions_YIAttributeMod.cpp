#include "AssetTypeActions_YIAttributeMod.h"
#include "YIAttributeModAsset.h"
#include "YIInventoryEditorModule.h"

UClass* FAssetTypeActions_YIAttributeMod::GetSupportedClass() const
{
	return UYIAttributeModAsset::StaticClass();
}

uint32 FAssetTypeActions_YIAttributeMod::GetCategories()
{
	return GYOLOInventoryAssetCategory;
}
