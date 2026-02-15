#include "AssetTypeActions_YIEquipmentSchema.h"
#include "YIEquipmentSchemaAsset.h"
#include "YIInventoryEditorModule.h"

UClass* FAssetTypeActions_YIEquipmentSchema::GetSupportedClass() const
{
	return UYIEquipmentSchemaAsset::StaticClass();
}

uint32 FAssetTypeActions_YIEquipmentSchema::GetCategories()
{
	return GYOLOInventoryAssetCategory;
}

void FAssetTypeActions_YIEquipmentSchema::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Obj : InObjects)
	{
		FYOLOInventoryEditorModule::Get().OpenDashboardForAsset(Obj);
	}
}

