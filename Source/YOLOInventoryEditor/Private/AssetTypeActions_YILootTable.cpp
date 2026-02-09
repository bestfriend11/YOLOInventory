#include "AssetTypeActions_YILootTable.h"
#include "YILootTable.h"
#include "YIInventoryEditorModule.h"

UClass* FAssetTypeActions_YILootTable::GetSupportedClass() const
{
	return UYILootTable::StaticClass();
}

uint32 FAssetTypeActions_YILootTable::GetCategories()
{
	return GYOLOInventoryAssetCategory;
}

void FAssetTypeActions_YILootTable::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Obj : InObjects)
	{
		FYOLOInventoryEditorModule::Get().OpenDashboardForAsset(Obj);
	}
}
