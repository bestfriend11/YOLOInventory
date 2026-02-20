#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "AssetTypeActions_Base.h"
#include "IAssetTools.h"
#include "AssetTypeActions_YIInventoryBag.h"
#include "AssetTypeActions_YIEquipmentSchema.h"
#include "YIEditorGridCategory.h"
#include "IYOLOInventoryEditorCoreModule.h"
#include "SYIBagDashboard.h"

class FYIBagDashboardBridge final : public IYIBagDashboardBridge
{
public:
	FYIBagDashboardBridge()
		: BagDashboardWidget(SNew(SYIBagDashboard))
	{
	}

	virtual TSharedRef<SWidget> GetRootWidget() override
	{
		return BagDashboardWidget.ToSharedRef();
	}

	virtual TSharedRef<SWidget> GetDetailsPanelWidget() const override
	{
		return BagDashboardWidget->GetDetailsPanelWidget();
	}

	virtual TSharedRef<SWidget> GetEquipmentLayoutPanelWidget() const override
	{
		return BagDashboardWidget->GetEquipmentLayoutPanelWidget();
	}

	virtual void OpenAsset(UObject* Asset) override
	{
		BagDashboardWidget->OpenAsset(Asset);
	}

	virtual void SaveCurrentBagFromToolbar() override
	{
		BagDashboardWidget->SaveCurrentBagFromToolbar();
	}

	virtual void SaveCurrentEquipmentLayoutFromToolbar() override
	{
		BagDashboardWidget->SaveCurrentEquipmentLayoutFromToolbar();
	}

	virtual void SetSelectedBag(UYIInventoryBag* InBag) override
	{
		BagDashboardWidget->SetSelectedBag(InBag);
	}

	virtual UYIInventoryBag* GetSelectedBag() const override
	{
		return BagDashboardWidget->GetSelectedBag();
	}

private:
	TSharedPtr<SYIBagDashboard> BagDashboardWidget;
};

class FYOLOInventoryEditorGridModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
        const uint32 CategoryBit = AssetTools.RegisterAdvancedAssetCategory(
            FName("YOLOInventory"),
            NSLOCTEXT("YOLOInventory", "AssetCategory", "YOLO Inventory"));
        YIEditorGridCategory::Set(CategoryBit);

        TSharedRef<FAssetTypeActions_YIInventoryBag> BagAction = MakeShared<FAssetTypeActions_YIInventoryBag>();
        AssetTools.RegisterAssetTypeActions(BagAction);
        RegisteredAssetTypeActions.Add(BagAction);

		TSharedRef<FAssetTypeActions_YIEquipmentSchema> EquipmentSchemaAction = MakeShared<FAssetTypeActions_YIEquipmentSchema>();
		AssetTools.RegisterAssetTypeActions(EquipmentSchemaAction);
		RegisteredAssetTypeActions.Add(EquipmentSchemaAction);

		IYOLOInventoryEditorCoreModule& EditorCoreModule = FModuleManager::LoadModuleChecked<IYOLOInventoryEditorCoreModule>("YOLOInventoryEditorCore");
		EditorCoreModule.RegisterBagDashboardFactory(
			FYICreateBagDashboardBridge::CreateStatic(&FYOLOInventoryEditorGridModule::CreateBagDashboardBridge));
    }

    virtual void ShutdownModule() override
    {
		if (IYOLOInventoryEditorCoreModule::IsAvailable())
		{
			IYOLOInventoryEditorCoreModule::Get().ClearBagDashboardFactory();
		}

        if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
        {
            IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
            for (const TSharedPtr<FAssetTypeActions_Base>& Action : RegisteredAssetTypeActions)
            {
                if (Action.IsValid())
                {
                    AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
                }
            }
        }
        RegisteredAssetTypeActions.Reset();
    }

private:
	static TSharedRef<IYIBagDashboardBridge> CreateBagDashboardBridge()
	{
		return MakeShared<FYIBagDashboardBridge>();
	}

    TArray<TSharedPtr<FAssetTypeActions_Base>> RegisteredAssetTypeActions;
};

IMPLEMENT_MODULE(FYOLOInventoryEditorGridModule, YOLOInventoryEditorGrid)
