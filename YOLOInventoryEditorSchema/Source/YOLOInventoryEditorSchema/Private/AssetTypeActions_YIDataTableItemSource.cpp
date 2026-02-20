#include "AssetTypeActions_YIDataTableItemSource.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "SYIItemDashboard.h"
#include "Data/YIDataTableItemSource.h"
#include "YIEditorSchemaCategory.h"

namespace
{
	static void YIEditorSchema_OpenItemDashboard_FromDataSource(UObject* Asset)
	{
		if (!Asset)
		{
			return;
		}

		const TSharedRef<SYIItemDashboard> Dashboard = SNew(SYIItemDashboard).LayoutMode(EYIItemDashboardLayout::Full);
		Dashboard->OpenAsset(Asset);

		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(NSLOCTEXT("YOLOInventory", "ItemSourceDashboardWindowTitle", "Item Source Editor"))
			.ClientSize(FVector2D(1500.f, 920.f))
			.SupportsMaximize(true)
			.SupportsMinimize(true)
			[
				Dashboard
			];
		FSlateApplication::Get().AddWindow(Window);
	}
}

UClass* FAssetTypeActions_YIDataTableItemSource::GetSupportedClass() const
{
	return UYIDataTableItemSource::StaticClass();
}

uint32 FAssetTypeActions_YIDataTableItemSource::GetCategories()
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}

void FAssetTypeActions_YIDataTableItemSource::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Object : InObjects)
	{
		YIEditorSchema_OpenItemDashboard_FromDataSource(Object);
	}
}


