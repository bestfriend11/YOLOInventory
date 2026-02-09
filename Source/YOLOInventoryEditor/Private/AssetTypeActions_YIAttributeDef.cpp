#include "AssetTypeActions_YIAttributeDef.h"
#include "YIAttributeDef.h"
#include "YIInventoryEditorModule.h"

UClass* FAssetTypeActions_YIAttributeDef::GetSupportedClass() const
{
	return UYIAttributeDef::StaticClass();
}

uint32 FAssetTypeActions_YIAttributeDef::GetCategories()
{
	return GYOLOInventoryAssetCategory;
}
