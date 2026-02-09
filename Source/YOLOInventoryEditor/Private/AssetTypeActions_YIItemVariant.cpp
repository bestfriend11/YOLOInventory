#include "AssetTypeActions_YIItemVariant.h"
#include "YIItemVariant.h"
#include "YIInventoryEditorModule.h"

UClass* FAssetTypeActions_YIItemVariant::GetSupportedClass() const
{
	return UYIItemVariantAsset::StaticClass();
}

uint32 FAssetTypeActions_YIItemVariant::GetCategories()
{
	return GYOLOInventoryAssetCategory;
}
