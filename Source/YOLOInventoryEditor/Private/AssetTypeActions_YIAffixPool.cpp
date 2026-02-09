#include "AssetTypeActions_YIAffixPool.h"
#include "YIAffixPoolAsset.h"
#include "YIInventoryEditorModule.h"

UClass* FAssetTypeActions_YIAffixPool::GetSupportedClass() const
{
    return UYIAffixPoolAsset::StaticClass();
}

uint32 FAssetTypeActions_YIAffixPool::GetCategories()
{
    extern uint32 GYOLOInventoryAssetCategory;
    return GYOLOInventoryAssetCategory;
}

void FAssetTypeActions_YIAffixPool::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Obj : InObjects)
	{
		FYOLOInventoryEditorModule::Get().OpenDashboardForAsset(Obj);
	}
}
