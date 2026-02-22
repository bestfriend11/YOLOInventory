#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "AssetTypeActions_Base.h"
#include "IAssetTools.h"
#include "IYOLOInventoryEditorCoreModule.h"
#include "SYIGeneratorDashboard.h"
#include "YIEditorLootCategory.h"
#include "YILootTable.h"
#include "YIRarityProfile.h"
#include "YIItemGenerator.h"
#include "YIFragmentPoolRollStrategy.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"

namespace
{
	static void YIEditorLoot_OpenAssetInDashboardOrFallback(UObject* Asset)
	{
		if (!Asset)
		{
			return;
		}

		const TSharedRef<SYIGeneratorDashboard> Dashboard = SNew(SYIGeneratorDashboard);
		Dashboard->OpenAsset(Asset);

		TSharedRef<SWindow> Window = SNew(SWindow)
			.Title(NSLOCTEXT("YOLOInventory", "GeneratorDashboardWindowTitle", "Loot & Generator Editor"))
			.ClientSize(FVector2D(1460.f, 900.f))
			.SupportsMaximize(true)
			.SupportsMinimize(true)
			[
				Dashboard
			];
		FSlateApplication::Get().AddWindow(Window);
	}
}

class FAssetTypeActions_YILootTable_EditorLoot final : public FAssetTypeActions_Base
{
public:
	explicit FAssetTypeActions_YILootTable_EditorLoot(uint32 InCategoryBit)
		: CategoryBit(InCategoryBit)
	{
	}

	virtual FText GetName() const override { return NSLOCTEXT("YOLOInventory", "LootTableAssetTypeName", "Loot Table"); }
	virtual FColor GetTypeColor() const override { return FColor(180, 220, 140); }
	virtual UClass* GetSupportedClass() const override { return UYILootTable::StaticClass(); }
	virtual uint32 GetCategories() override { return CategoryBit; }
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost>) override
	{
		for (UObject* Obj : InObjects)
		{
			YIEditorLoot_OpenAssetInDashboardOrFallback(Obj);
		}
	}

private:
	uint32 CategoryBit = EAssetTypeCategories::Misc;
};

class FAssetTypeActions_YIRarityProfile_EditorLoot final : public FAssetTypeActions_Base
{
public:
	explicit FAssetTypeActions_YIRarityProfile_EditorLoot(uint32 InCategoryBit)
		: CategoryBit(InCategoryBit)
	{
	}

	virtual FText GetName() const override { return NSLOCTEXT("YOLOInventory", "RarityProfileAssetTypeName", "Rarity Profile"); }
	virtual FColor GetTypeColor() const override { return FColor(200, 190, 255); }
	virtual UClass* GetSupportedClass() const override { return UYIRarityProfile::StaticClass(); }
	virtual uint32 GetCategories() override { return CategoryBit; }
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost>) override
	{
		for (UObject* Obj : InObjects)
		{
			YIEditorLoot_OpenAssetInDashboardOrFallback(Obj);
		}
	}

private:
	uint32 CategoryBit = EAssetTypeCategories::Misc;
};

class FAssetTypeActions_YIItemGenerator_EditorLoot final : public FAssetTypeActions_Base
{
public:
	explicit FAssetTypeActions_YIItemGenerator_EditorLoot(uint32 InCategoryBit)
		: CategoryBit(InCategoryBit)
	{
	}

	virtual FText GetName() const override { return NSLOCTEXT("YOLOInventory", "ItemGeneratorAssetTypeName", "Item Generator"); }
	virtual FColor GetTypeColor() const override { return FColor(150, 220, 240); }
	virtual UClass* GetSupportedClass() const override { return UYIItemGenerator::StaticClass(); }
	virtual uint32 GetCategories() override { return CategoryBit; }
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost>) override
	{
		for (UObject* Obj : InObjects)
		{
			YIEditorLoot_OpenAssetInDashboardOrFallback(Obj);
		}
	}

private:
	uint32 CategoryBit = EAssetTypeCategories::Misc;
};

class FAssetTypeActions_YIFragmentPoolRollStrategy_EditorLoot final : public FAssetTypeActions_Base
{
public:
	explicit FAssetTypeActions_YIFragmentPoolRollStrategy_EditorLoot(uint32 InCategoryBit)
		: CategoryBit(InCategoryBit)
	{
	}

	virtual FText GetName() const override { return NSLOCTEXT("YOLOInventory", "FragmentPoolRollStrategyAssetTypeName", "Fragment Pool Roll Strategy"); }
	virtual FColor GetTypeColor() const override { return FColor(120, 200, 255); }
	virtual UClass* GetSupportedClass() const override { return UYIFragmentPoolRollStrategy::StaticClass(); }
	virtual uint32 GetCategories() override { return CategoryBit; }
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost>) override
	{
		for (UObject* Obj : InObjects)
		{
			YIEditorLoot_OpenAssetInDashboardOrFallback(Obj);
		}
	}

private:
	uint32 CategoryBit = EAssetTypeCategories::Misc;
};

class FYIGeneratorDashboardBridge final : public IYIGeneratorDashboardBridge
{
public:
	FYIGeneratorDashboardBridge()
		: GeneratorDashboardWidget(SNew(SYIGeneratorDashboard).LayoutMode(EYIGeneratorDashboardLayout::AssetListOnly))
	{
	}

	virtual TSharedRef<SWidget> GetRootWidget() override
	{
		return GeneratorDashboardWidget.ToSharedRef();
	}

	virtual TSharedRef<SWidget> GetDetailsPanelWidget() const override
	{
		return GeneratorDashboardWidget->GetDetailsPanelWidget();
	}

	virtual TSharedRef<SWidget> GetTestPanelWidget() const override
	{
		return GeneratorDashboardWidget->GetTestPanelWidget();
	}

	virtual void OpenAsset(UObject* Asset) override
	{
		GeneratorDashboardWidget->OpenAsset(Asset);
	}

	virtual void SaveCurrentAssetFromToolbar() override
	{
		GeneratorDashboardWidget->SaveCurrentAssetFromToolbar();
	}

private:
	TSharedPtr<SYIGeneratorDashboard> GeneratorDashboardWidget;
};

class FYOLOInventoryEditorLootModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		const uint32 CategoryBit = AssetTools.RegisterAdvancedAssetCategory(
			FName("YOLOInventory"),
			NSLOCTEXT("YOLOInventory", "AssetCategory", "YOLO Inventory"));
		YIEditorLootCategory::Set(CategoryBit);

		TSharedRef<FAssetTypeActions_YILootTable_EditorLoot> LootAction = MakeShared<FAssetTypeActions_YILootTable_EditorLoot>(CategoryBit);
		AssetTools.RegisterAssetTypeActions(LootAction);
		RegisteredAssetTypeActions.Add(LootAction);

		TSharedRef<FAssetTypeActions_YIRarityProfile_EditorLoot> RarityAction = MakeShared<FAssetTypeActions_YIRarityProfile_EditorLoot>(CategoryBit);
		AssetTools.RegisterAssetTypeActions(RarityAction);
		RegisteredAssetTypeActions.Add(RarityAction);

		TSharedRef<FAssetTypeActions_YIItemGenerator_EditorLoot> GeneratorAction = MakeShared<FAssetTypeActions_YIItemGenerator_EditorLoot>(CategoryBit);
		AssetTools.RegisterAssetTypeActions(GeneratorAction);
		RegisteredAssetTypeActions.Add(GeneratorAction);

		TSharedRef<FAssetTypeActions_YIFragmentPoolRollStrategy_EditorLoot> FragmentStrategyAction = MakeShared<FAssetTypeActions_YIFragmentPoolRollStrategy_EditorLoot>(CategoryBit);
		AssetTools.RegisterAssetTypeActions(FragmentStrategyAction);
		RegisteredAssetTypeActions.Add(FragmentStrategyAction);

		IYOLOInventoryEditorCoreModule& EditorCoreModule = FModuleManager::LoadModuleChecked<IYOLOInventoryEditorCoreModule>("YOLOInventoryEditorCore");
		EditorCoreModule.RegisterGeneratorDashboardFactory(
			FYICreateGeneratorDashboardBridge::CreateStatic(&FYOLOInventoryEditorLootModule::CreateGeneratorDashboardBridge));
    }

    virtual void ShutdownModule() override
    {
		if (IYOLOInventoryEditorCoreModule::IsAvailable())
		{
			IYOLOInventoryEditorCoreModule::Get().ClearGeneratorDashboardFactory();
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
	static TSharedRef<IYIGeneratorDashboardBridge> CreateGeneratorDashboardBridge()
	{
		return MakeShared<FYIGeneratorDashboardBridge>();
	}

	TArray<TSharedPtr<FAssetTypeActions_Base>> RegisteredAssetTypeActions;
};

IMPLEMENT_MODULE(FYOLOInventoryEditorLootModule, YOLOInventoryEditorLoot)
