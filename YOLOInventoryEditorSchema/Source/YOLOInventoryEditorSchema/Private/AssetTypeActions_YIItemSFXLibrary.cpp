#include "AssetTypeActions_YIItemSFXLibrary.h"
#include "YIItemSFXLibrary.h"
#include "YIEditorSchemaCategory.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"

UClass* FAssetTypeActions_YIItemSFXLibrary::GetSupportedClass() const
{
	return UYIItemSFXLibrary::StaticClass();
}

uint32 FAssetTypeActions_YIItemSFXLibrary::GetCategories()
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}


