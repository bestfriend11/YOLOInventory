#include "AssetTypeActions_YIItemDefinition.h"

#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "SYIItemDashboard.h"
#include "YIItemDefinition.h"
#include "YIEditorSchemaCategory.h"

namespace
{
	static void YIEditorSchema_OpenItemDashboard_FromItemDefinition(UObject* Asset)
	{
		if (!Asset)
		{
			return;
		}

		const TSharedRef<SYIItemDashboard> Dashboard = SNew(SYIItemDashboard).LayoutMode(EYIItemDashboardLayout::Full);
		Dashboard->OpenAsset(Asset);

		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(NSLOCTEXT("YOLOInventory", "ItemDashboardWindowTitle", "Item Editor"))
			.ClientSize(FVector2D(1500.f, 920.f))
			.SupportsMaximize(true)
			.SupportsMinimize(true)
			[
				Dashboard
			];
		FSlateApplication::Get().AddWindow(Window);
	}
}

UClass* FAssetTypeActions_YIItemDefinition::GetSupportedClass() const
{
	return UYIItemDefinition::StaticClass();
}

uint32 FAssetTypeActions_YIItemDefinition::GetCategories()
{
	return GetYOLOInventoryEditorSchemaAssetCategory();
}

void FAssetTypeActions_YIItemDefinition::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	for (UObject* Object : InObjects)
	{
		YIEditorSchema_OpenItemDashboard_FromItemDefinition(Object);
	}
}


