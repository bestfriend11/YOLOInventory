#include "AssetTypeActions_YIEquipmentLayout.h"
#include "YIEquipmentLayoutAsset.h"
#include "YIInventoryEditorModule.h"

UClass* FAssetTypeActions_YIEquipmentLayout::GetSupportedClass() const
{
	return UYIEquipmentLayoutAsset::StaticClass();
}

uint32 FAssetTypeActions_YIEquipmentLayout::GetCategories()
{
	return GYOLOInventoryAssetCategory;
}

void FAssetTypeActions_YIEquipmentLayout::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Obj : InObjects)
	{
		FYOLOInventoryEditorModule::Get().OpenDashboardForAsset(Obj);
	}
}

