#include "AssetTypeActions_YIEquipmentSchema.h"
#include "YIEquipmentSchemaAsset.h"
#include "YIEditorGridCategory.h"

UClass* FAssetTypeActions_YIEquipmentSchema::GetSupportedClass() const
{
	return UYIEquipmentSchemaAsset::StaticClass();
}

uint32 FAssetTypeActions_YIEquipmentSchema::GetCategories()
{
	return YIEditorGridCategory::Get();
}

void FAssetTypeActions_YIEquipmentSchema::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	FAssetTypeActions_Base::OpenAssetEditor(InObjects, EditWithinLevelEditor);
}
