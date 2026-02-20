#include "AssetTypeActions_YIItemTraitAsset.h"

#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "SYIFragmentDashboard.h"
#include "YIItemTraitAsset.h"
#include "YIEditorSchemaCategory.h"

namespace
{
	static void YIEditorSchema_OpenFragmentDashboard_FromTraitAsset(UObject* Asset)
	{
		if (!Asset)
		{
			return;
		}

		const TSharedRef<SYIFragmentDashboard> Dashboard = SNew(SYIFragmentDashboard).LayoutMode(EYIFragmentDashboardLayout::Full);
		Dashboard->OpenAsset(Asset);

		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(NSLOCTEXT("YOLOInventory", "ItemTraitDashboardWindowTitle", "Item Trait Editor"))
			.ClientSize(FVector2D(1320.f, 860.f))
			.SupportsMaximize(true)
			.SupportsMinimize(true)
			[
				Dashboard
			];
		FSlateApplication::Get().AddWindow(Window);
	}
}

UClass* FAssetTypeActions_YIItemTraitAsset::GetSupportedClass() const
{
	return UYIItemTraitAsset::StaticClass();
}

uint32 FAssetTypeActions_YIItemTraitAsset::GetCategories()
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}

void FAssetTypeActions_YIItemTraitAsset::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Object : InObjects)
	{
		YIEditorSchema_OpenFragmentDashboard_FromTraitAsset(Object);
	}
}

