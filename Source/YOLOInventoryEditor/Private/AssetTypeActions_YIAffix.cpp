#include "AssetTypeActions_YIAffix.h"
#include "YIAffixAsset.h"
#include "YIInventoryEditorModule.h"

UClass* FAssetTypeActions_YIAffix::GetSupportedClass() const
{
	return UYIAffixAsset::StaticClass();
}

uint32 FAssetTypeActions_YIAffix::GetCategories()
{
	return GetYoLoAssetCategoryBit();
}
