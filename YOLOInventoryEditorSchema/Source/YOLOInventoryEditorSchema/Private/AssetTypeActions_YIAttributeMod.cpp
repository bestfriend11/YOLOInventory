#include "AssetTypeActions_YIAttributeMod.h"
#include "YIAttributeModAsset.h"
#include "YIEditorSchemaCategory.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"

UClass* FAssetTypeActions_YIAttributeMod::GetSupportedClass() const
{
	return UYIAttributeModAsset::StaticClass();
}

uint32 FAssetTypeActions_YIAttributeMod::GetCategories()
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}


