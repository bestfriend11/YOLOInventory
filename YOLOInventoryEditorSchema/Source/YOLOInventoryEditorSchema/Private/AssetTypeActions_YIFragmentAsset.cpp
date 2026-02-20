#include "AssetTypeActions_YIFragmentAsset.h"

#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "SYIFragmentDashboard.h"
#include "YIFragmentAsset.h"
#include "YIEditorSchemaCategory.h"

namespace
{
	static void YIEditorSchema_OpenFragmentDashboard_FromFragmentAsset(UObject* Asset)
	{
		if (!Asset)
		{
			return;
		}

		const TSharedRef<SYIFragmentDashboard> Dashboard = SNew(SYIFragmentDashboard).LayoutMode(EYIFragmentDashboardLayout::Full);
		Dashboard->OpenAsset(Asset);

		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(NSLOCTEXT("YOLOInventory", "FragmentDashboardWindowTitle", "Fragment Editor"))
			.ClientSize(FVector2D(1320.f, 860.f))
			.SupportsMaximize(true)
			.SupportsMinimize(true)
			[
				Dashboard
			];
		FSlateApplication::Get().AddWindow(Window);
	}
}

UClass* FAssetTypeActions_YIFragmentAsset::GetSupportedClass() const
{
	return UYIFragmentAsset::StaticClass();
}

uint32 FAssetTypeActions_YIFragmentAsset::GetCategories()
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}

void FAssetTypeActions_YIFragmentAsset::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Object : InObjects)
	{
		YIEditorSchema_OpenFragmentDashboard_FromFragmentAsset(Object);
	}
}
