#include "AssetTypeActions_YIItemVariant.h"
#include "YIItemVariant.h"
#include "YIEditorSchemaCategory.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"

UClass* FAssetTypeActions_YIItemVariant::GetSupportedClass() const
{
	return UYIItemVariantAsset::StaticClass();
}

uint32 FAssetTypeActions_YIItemVariant::GetCategories()
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}


