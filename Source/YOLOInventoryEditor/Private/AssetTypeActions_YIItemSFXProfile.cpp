#include "AssetTypeActions_YIItemSFXProfile.h"
#include "YIItemSFXLibrary.h"
#include "YIInventoryEditorModule.h"

UClass* FAssetTypeActions_YIItemSFXProfile::GetSupportedClass() const
{
	return UYIItemSFXProfile::StaticClass();
}

uint32 FAssetTypeActions_YIItemSFXProfile::GetCategories()
{
	return GYOLOInventoryAssetCategory;
}
