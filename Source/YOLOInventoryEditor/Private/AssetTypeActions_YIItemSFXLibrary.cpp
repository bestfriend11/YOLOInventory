#include "AssetTypeActions_YIItemSFXLibrary.h"
#include "YIItemSFXLibrary.h"
#include "YIInventoryEditorModule.h"

UClass* FAssetTypeActions_YIItemSFXLibrary::GetSupportedClass() const
{
	return UYIItemSFXLibrary::StaticClass();
}

uint32 FAssetTypeActions_YIItemSFXLibrary::GetCategories()
{
	return GYOLOInventoryAssetCategory;
}
