#include "AssetTypeActions_YIDataTableItemSource.h"
#include "Data/YIDataTableItemSource.h"
#include "YIInventoryEditorModule.h"

UClass* FAssetTypeActions_YIDataTableItemSource::GetSupportedClass() const
{
	return UYIDataTableItemSource::StaticClass();
}

uint32 FAssetTypeActions_YIDataTableItemSource::GetCategories()
{
	return GYOLOInventoryAssetCategory;
}
