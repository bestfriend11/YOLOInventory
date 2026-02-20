#include "AssetTypeActions_YIEvolutionPath.h"
#include "YIEvolutionPath.h"
#include "YIEditorSchemaCategory.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"

UClass* FAssetTypeActions_YIEvolutionPath::GetSupportedClass() const
{
	return UYIEvolutionPath::StaticClass();
}

uint32 FAssetTypeActions_YIEvolutionPath::GetCategories()
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}


