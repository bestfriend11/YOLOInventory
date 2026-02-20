#include "AssetTypeActions_YIAffix.h"

#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "SYIFragmentDashboard.h"
#include "YIAffixAsset.h"
#include "YIEditorSchemaCategory.h"

namespace
{
	static void YIEditorSchema_OpenFragmentDashboard_FromAffix(UObject* Asset)
	{
		if (!Asset)
		{
			return;
		}

		const TSharedRef<SYIFragmentDashboard> Dashboard = SNew(SYIFragmentDashboard).LayoutMode(EYIFragmentDashboardLayout::Full);
		Dashboard->OpenAsset(Asset);

		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(NSLOCTEXT("YOLOInventory", "AffixDashboardWindowTitle", "Fragment Editor"))
			.ClientSize(FVector2D(1320.f, 860.f))
			.SupportsMaximize(true)
			.SupportsMinimize(true)
			[
				Dashboard
			];
		FSlateApplication::Get().AddWindow(Window);
	}
}

UClass* FAssetTypeActions_YIAffix::GetSupportedClass() const
{
	return UYIAffixAsset::StaticClass();
}

uint32 FAssetTypeActions_YIAffix::GetCategories()
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}

void FAssetTypeActions_YIAffix::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Object : InObjects)
	{
		YIEditorSchema_OpenFragmentDashboard_FromAffix(Object);
	}
}


