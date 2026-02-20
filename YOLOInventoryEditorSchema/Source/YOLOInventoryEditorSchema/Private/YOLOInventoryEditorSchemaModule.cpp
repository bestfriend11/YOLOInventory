#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetTypeActions_Base.h"
#include "YIEditorSchemaCategory.h"
#include "AssetTypeActions_YIItemDefinition.h"
#include "AssetTypeActions_YIFragmentAsset.h"
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
	}

	virtual void ShutdownModule() override
	{
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

private:
	TArray<TSharedPtr<FAssetTypeActions_Base>> RegisteredAssetTypeActions;
};

IMPLEMENT_MODULE(FYOLOInventoryEditorSchemaModule, YOLOInventoryEditorSchema)
