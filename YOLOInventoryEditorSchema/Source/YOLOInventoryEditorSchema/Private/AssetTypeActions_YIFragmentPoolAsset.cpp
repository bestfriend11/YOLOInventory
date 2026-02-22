#include "AssetTypeActions_YIFragmentPoolAsset.h"
#include "YIFragmentPoolAsset.h"
#include "YIEditorSchemaCategory.h"

UClass* FAssetTypeActions_YIFragmentPoolAsset::GetSupportedClass() const
{
	return UYIFragmentPoolAsset::StaticClass();
}

uint32 FAssetTypeActions_YIFragmentPoolAsset::GetCategories()
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}

