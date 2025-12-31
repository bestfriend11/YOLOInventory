#include "AssetTypeActions_YIInventoryBag.h"
#include "YIInventoryBag.h"
#include "YIInventoryBagEditor.h"
#include "YIInventoryEditorModule.h"
#include "Toolkits/IToolkitHost.h"

UClass* FAssetTypeActions_YIInventoryBag::GetSupportedClass() const { return UYIInventoryBag::StaticClass(); }
uint32 FAssetTypeActions_YIInventoryBag::GetCategories() { return GYOLOInventoryAssetCategory; }

void FAssetTypeActions_YIInventoryBag::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (auto* Obj : InObjects)
	{
		if (UYIInventoryBag* Bag = Cast<UYIInventoryBag>(Obj))
		{
			TSharedRef<FYIInventoryBagEditor> Editor = MakeShareable(new FYIInventoryBagEditor());
			Editor->InitEditor(EToolkitMode::Standalone, EditWithinLevelEditor, Bag);
		}
	}
}
