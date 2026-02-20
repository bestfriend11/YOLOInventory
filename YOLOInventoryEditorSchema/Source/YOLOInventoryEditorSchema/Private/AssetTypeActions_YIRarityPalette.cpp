#include "AssetTypeActions_YIRarityPalette.h"
#include "YIRarityPalette.h"
#include "YIEditorSchemaCategory.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"

UClass* FAssetTypeActions_YIRarityPalette::GetSupportedClass() const
{
	return UYIRarityPalette::StaticClass();
}

uint32 FAssetTypeActions_YIRarityPalette::GetCategories()
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}


