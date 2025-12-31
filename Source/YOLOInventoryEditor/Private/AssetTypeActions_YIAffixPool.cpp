#include "AssetTypeActions_YIAffixPool.h"
#include "YIAffixPoolAsset.h"

UClass* FAssetTypeActions_YIAffixPool::GetSupportedClass() const
{
    return UYIAffixPoolAsset::StaticClass();
}

uint32 FAssetTypeActions_YIAffixPool::GetCategories()
{
    extern uint32 GYOLOInventoryAssetCategory;
    return GYOLOInventoryAssetCategory;
}