#include "AssetTypeActions_YIAffix.h"
#include "YIAffixAsset.h"
#include "YIInventoryEditorModule.h"

UClass* FAssetTypeActions_YIAffix::GetSupportedClass() const
{
	return UYIAffixAsset::StaticClass();
}

uint32 FAssetTypeActions_YIAffix::GetCategories()
{
	return GetYoLoAssetCategoryBit();
}

void FAssetTypeActions_YIAffix::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Obj : InObjects)
	{
		FYOLOInventoryEditorModule::Get().OpenDashboardForAsset(Obj);
	}
}
