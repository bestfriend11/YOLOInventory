#include "AssetTypeActions_YIAffixPool.h"
#include "YIAffixPoolAsset.h"
#include "YIEditorSchemaCategory.h"

UClass* FAssetTypeActions_YIAffixPool::GetSupportedClass() const
{
    return UYIAffixPoolAsset::StaticClass();
}

uint32 FAssetTypeActions_YIAffixPool::GetCategories()
{
        return GetYOLOInventoryEditorSchemaAssetCategory();
}

void FAssetTypeActions_YIAffixPool::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	FAssetTypeActions_Base::OpenAssetEditor(InObjects, EditWithinLevelEditor);
}


