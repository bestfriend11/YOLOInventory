#include "AssetTypeActions_YIAttributeDef.h"
#include "YIAttributeDef.h"
#include "YIEditorSchemaCategory.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"

UClass* FAssetTypeActions_YIAttributeDef::GetSupportedClass() const
{
	return UYIAttributeDef::StaticClass();
}

uint32 FAssetTypeActions_YIAttributeDef::GetCategories()
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}


