#include "AssetTypeActions_YIDataTableItemSource.h"
#include "Data/YIDataTableItemSource.h"
#include "YIInventoryEditorModule.h"

UClass* FAssetTypeActions_YIDataTableItemSource::GetSupportedClass() const
{
	return UYIDataTableItemSource::StaticClass();
}

uint32 FAssetTypeActions_YIDataTableItemSource::GetCategories()
{
	return GYOLOInventoryAssetCategory;
}

void FAssetTypeActions_YIDataTableItemSource::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Obj : InObjects)
	{
		FYOLOInventoryEditorModule::Get().OpenDashboardForAsset(Obj);
	}
}
