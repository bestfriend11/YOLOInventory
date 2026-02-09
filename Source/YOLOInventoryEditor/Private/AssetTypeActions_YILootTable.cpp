#include "AssetTypeActions_YILootTable.h"
#include "YILootTable.h"
#include "YIInventoryEditorModule.h"

UClass* FAssetTypeActions_YILootTable::GetSupportedClass() const
{
	return UYILootTable::StaticClass();
}

uint32 FAssetTypeActions_YILootTable::GetCategories()
{
	return GYOLOInventoryAssetCategory;
}
