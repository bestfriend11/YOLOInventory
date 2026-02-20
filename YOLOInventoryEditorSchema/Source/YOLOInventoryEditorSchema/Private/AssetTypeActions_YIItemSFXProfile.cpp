#include "AssetTypeActions_YIItemSFXProfile.h"
#include "YIItemSFXLibrary.h"
#include "YIEditorSchemaCategory.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"

UClass* FAssetTypeActions_YIItemSFXProfile::GetSupportedClass() const
{
	return UYIItemSFXProfile::StaticClass();
}

uint32 FAssetTypeActions_YIItemSFXProfile::GetCategories()
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}


