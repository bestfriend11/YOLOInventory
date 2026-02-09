#include "AssetTypeActions_YIItemGenerator.h"
#include "YIItemGenerator.h"
#include "YIInventoryEditorModule.h"

UClass* FAssetTypeActions_YIItemGenerator::GetSupportedClass() const
{
	return UYIItemGenerator::StaticClass();
}

uint32 FAssetTypeActions_YIItemGenerator::GetCategories()
{
	return GYOLOInventoryAssetCategory;
}

void FAssetTypeActions_YIItemGenerator::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Obj : InObjects)
	{
		FYOLOInventoryEditorModule::Get().OpenDashboardForAsset(Obj);
	}
}
