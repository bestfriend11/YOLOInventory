#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "PropertyEditorModule.h"
#include "AssetTypeActions_Base.h"
#include "YIEditorSchemaCategory.h"
#include "AssetTypeActions_YIItemDefinition.h"
#include "AssetTypeActions_YIFragmentAsset.h"
#include "AssetTypeActions_YIFragmentPoolAsset.h"
#include "AssetTypeActions_YIItemTraitAsset.h"
#include "AssetTypeActions_YIAffix.h"
#include "AssetTypeActions_YIAffixPool.h"
#include "AssetTypeActions_YIAttributeDef.h"
#include "AssetTypeActions_YIAttributeMod.h"
#include "AssetTypeActions_YIDataTableItemSource.h"
#include "AssetTypeActions_YIEvolutionPath.h"
#include "AssetTypeActions_YIItemSFXLibrary.h"
#include "AssetTypeActions_YIItemSFXProfile.h"
#include "AssetTypeActions_YIItemVariant.h"
#include "AssetTypeActions_YIRarityPalette.h"
#include "IYOLOInventoryEditorCoreModule.h"
#include "SYIItemDashboard.h"
#include "SYIFragmentDashboard.h"
#include "SYICraftingDashboard.h"
#include "YIItemDefinition.h"
#include "YIItemDefinitionDetails.h"
#include "YIDataTableItemSourceDetails.h"
#include "YIFragmentStructCustomization.h"
#include "Data/YIDataTableItemSource.h"
#include "YIItemFragments.h"

class FYISchemaDashboardBridge final : public IYISchemaDashboardBridge
{
public:
	FYISchemaDashboardBridge()
		: ItemDashboardWidget(SNew(SYIItemDashboard).LayoutMode(EYIItemDashboardLayout::ItemListOnly))
		, FragmentDashboardWidget(SNew(SYIFragmentDashboard).LayoutMode(EYIFragmentDashboardLayout::AssetListOnly))
		, CraftingDashboardWidget(SNew(SYICraftingDashboard))
	{
	}

	virtual TSharedRef<SWidget> GetItemsRootWidget() override { return ItemDashboardWidget.ToSharedRef(); }
	virtual TSharedRef<SWidget> GetFragmentsRootWidget() override { return FragmentDashboardWidget.ToSharedRef(); }
	virtual TSharedRef<SWidget> GetCraftingRootWidget() override { return CraftingDashboardWidget.ToSharedRef(); }

	virtual TSharedRef<SWidget> GetItemDetailsPanelWidget() const override { return ItemDashboardWidget->GetDetailsPanelWidget(); }
	virtual TSharedRef<SWidget> GetItemMappingsPanelWidget() const override { return ItemDashboardWidget->GetMappingPanelWidget(); }
	virtual TSharedRef<SWidget> GetItemPreviewPanelWidget() const override { return ItemDashboardWidget->GetPreviewPanelWidget(); }
	virtual TSharedRef<SWidget> GetItemPreflightPanelWidget() const override { return ItemDashboardWidget->GetPreflightPanelWidget(); }
	virtual TSharedRef<SWidget> GetItemDiffPanelWidget() const override { return ItemDashboardWidget->GetDiffPanelWidget(); }
	virtual TSharedRef<SWidget> GetItemLogsPanelWidget() const override { return ItemDashboardWidget->GetLogsPanelWidget(); }

	virtual TSharedRef<SWidget> GetFragmentDetailsPanelWidget() const override { return FragmentDashboardWidget->GetDetailsPanelWidget(); }
	virtual TSharedRef<SWidget> GetFragmentMappingsPanelWidget() const override { return FragmentDashboardWidget->GetMappingPanelWidget(); }
	virtual TSharedRef<SWidget> GetFragmentPreviewPanelWidget() const override { return FragmentDashboardWidget->GetPreviewPanelWidget(); }

	virtual void OpenAssetInItems(UObject* Asset) override { ItemDashboardWidget->OpenAsset(Asset); }
	virtual void OpenAssetInFragments(UObject* Asset) override { FragmentDashboardWidget->OpenAsset(Asset); }
	virtual void OpenAssetInCrafting(UObject* Asset) override { CraftingDashboardWidget->OpenAsset(Asset); }

	virtual void SaveItemsFromToolbar() override { ItemDashboardWidget->SaveCurrentAssetFromToolbar(); }
	virtual void SaveFragmentsFromToolbar() override { FragmentDashboardWidget->SaveCurrentAssetFromToolbar(); }
	virtual void SaveCraftingFromToolbar() override { CraftingDashboardWidget->SaveTargetBagFromToolbar(); }
	virtual void CreateItemDataSourceFromToolbar() override { ItemDashboardWidget->CreateDataSourceFromToolbar(); }

	virtual void SetCraftingTargetBag(UYIInventoryBag* InBag) override { CraftingDashboardWidget->SetTargetBag(InBag); }
	virtual UYIInventoryBag* GetCraftingTargetBag() const override { return CraftingDashboardWidget->GetTargetBag(); }

private:
	TSharedPtr<SYIItemDashboard> ItemDashboardWidget;
	TSharedPtr<SYIFragmentDashboard> FragmentDashboardWidget;
	TSharedPtr<SYICraftingDashboard> CraftingDashboardWidget;
};

uint32 GYOLOInventoryEditorSchemaAssetCategory = EAssetTypeCategories::Misc;

class FYOLOInventoryEditorSchemaModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		GYOLOInventoryEditorSchemaAssetCategory = AssetTools.RegisterAdvancedAssetCategory(
			FName("YOLOInventory"),
			NSLOCTEXT("YOLOInventory", "AssetCategory", "YOLO Inventory"));

		RegisterAction<FAssetTypeActions_YIItemDefinition>(AssetTools);
		RegisterAction<FAssetTypeActions_YIFragmentAsset>(AssetTools);
		RegisterAction<FAssetTypeActions_YIFragmentPoolAsset>(AssetTools);
		RegisterAction<FAssetTypeActions_YIItemTraitAsset>(AssetTools);
		RegisterAction<FAssetTypeActions_YIAffix>(AssetTools);
		RegisterAction<FAssetTypeActions_YIAffixPool>(AssetTools);
		RegisterAction<FAssetTypeActions_YIAttributeDef>(AssetTools);
		RegisterAction<FAssetTypeActions_YIAttributeMod>(AssetTools);
		RegisterAction<FAssetTypeActions_YIDataTableItemSource>(AssetTools);
		RegisterAction<FAssetTypeActions_YIEvolutionPath>(AssetTools);
		RegisterAction<FAssetTypeActions_YIItemSFXLibrary>(AssetTools);
		RegisterAction<FAssetTypeActions_YIItemSFXProfile>(AssetTools);
		RegisterAction<FAssetTypeActions_YIItemVariant>(AssetTools);
		RegisterAction<FAssetTypeActions_YIRarityPalette>(AssetTools);

		FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditorModule.RegisterCustomClassLayout(
			UYIItemDefinition::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FYIItemDefinitionDetails::MakeInstance));
		PropertyEditorModule.RegisterCustomClassLayout(
			UYIDataTableItemSource::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FYIDataTableItemSourceDetails::MakeInstance));
		RegisterFragmentPropertyTypeCustomizations(PropertyEditorModule);
		PropertyEditorModule.NotifyCustomizationModuleChanged();

		IYOLOInventoryEditorCoreModule& EditorCoreModule = FModuleManager::LoadModuleChecked<IYOLOInventoryEditorCoreModule>("YOLOInventoryEditorCore");
		EditorCoreModule.RegisterSchemaDashboardFactory(
			FYICreateSchemaDashboardBridge::CreateStatic(&FYOLOInventoryEditorSchemaModule::CreateSchemaDashboardBridge));
	}

	virtual void ShutdownModule() override
	{
		if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
		{
			FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
			PropertyEditorModule.UnregisterCustomClassLayout(UYIItemDefinition::StaticClass()->GetFName());
			PropertyEditorModule.UnregisterCustomClassLayout(UYIDataTableItemSource::StaticClass()->GetFName());
			for (const FName& TypeName : RegisteredFragmentPropertyTypeCustomizations)
			{
				PropertyEditorModule.UnregisterCustomPropertyTypeLayout(TypeName);
			}
			RegisteredFragmentPropertyTypeCustomizations.Empty();
			PropertyEditorModule.NotifyCustomizationModuleChanged();
		}

		if (IYOLOInventoryEditorCoreModule::IsAvailable())
		{
			IYOLOInventoryEditorCoreModule::Get().ClearSchemaDashboardFactory();
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
		RegisteredAssetTypeActions.Empty();
	}

private:
	template <typename TAction>
	void RegisterAction(IAssetTools& AssetTools)
	{
		TSharedRef<TAction> Action = MakeShared<TAction>();
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}

	static TSharedRef<IYISchemaDashboardBridge> CreateSchemaDashboardBridge()
	{
		return MakeShared<FYISchemaDashboardBridge>();
	}

	void RegisterFragmentPropertyTypeCustomizations(FPropertyEditorModule& PropertyEditorModule)
	{
		auto Register = [this, &PropertyEditorModule](const UScriptStruct* StructType)
		{
			if (!StructType)
			{
				return;
			}

			const FName TypeName = StructType->GetFName();
			PropertyEditorModule.RegisterCustomPropertyTypeLayout(
				TypeName,
				FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FYIFragmentStructCustomization::MakeInstance));
			RegisteredFragmentPropertyTypeCustomizations.AddUnique(TypeName);
		};

		Register(FYIItemCustomRuntimeFragment::StaticStruct());
		Register(FYIItemCustomDefinitionFragment::StaticStruct());
		Register(FYIItemUIDefinitionFragment::StaticStruct());
		Register(FYIItemClassificationDefinitionFragment::StaticStruct());
		Register(FYIItemAudioDefinitionFragment::StaticStruct());
		Register(FYIItemLayoutDefinitionFragment::StaticStruct());
		Register(FYIItemStackingDefinitionFragment::StaticStruct());
		Register(FYIItemRulesDefinitionFragment::StaticStruct());
		Register(FYIItemContainerDefinitionFragment::StaticStruct());
		Register(FYIItemAttributeModsDefinitionFragment::StaticStruct());
		Register(FYIItemPickupDefinitionFragment::StaticStruct());
		Register(FYIItemWeightDefinitionFragment::StaticStruct());
		Register(FYIItemEquipmentDefinitionFragment::StaticStruct());
		Register(FYIItemAffixDefinitionFragment::StaticStruct());
		Register(FYIItemAttributesFragment::StaticStruct());
		Register(FYIItemAffixesFragment::StaticStruct());
		Register(FYIItemDurabilityFragment::StaticStruct());
	}

private:
	TArray<TSharedPtr<FAssetTypeActions_Base>> RegisteredAssetTypeActions;
	TArray<FName> RegisteredFragmentPropertyTypeCustomizations;
};

IMPLEMENT_MODULE(FYOLOInventoryEditorSchemaModule, YOLOInventoryEditorSchema)
