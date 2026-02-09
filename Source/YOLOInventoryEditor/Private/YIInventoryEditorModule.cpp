#include "YIInventoryEditorModule.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "IAssetTools.h"
#include "YIItemDefinition.h"

#include "AssetTypeActions_Base.h"
#include "YIItemDefinitionFactory.h"
#include "AssetTypeActions_YIInventoryBag.h"
#include "AssetTypeActions_YIAffix.h"
#include "AssetTypeActions_YIAttributeDef.h"
#include "AssetTypeActions_YIAttributeMod.h"
#include "AssetTypeActions_YIItemVariant.h"
#include "AssetTypeActions_YIRarityPalette.h"
#include "AssetTypeActions_YIRarityProfile.h"
#include "AssetTypeActions_YIEvolutionPath.h"
#include "AssetTypeActions_YIItemSFXProfile.h"
#include "AssetTypeActions_YIItemSFXLibrary.h"
#include "AssetTypeActions_YIDataTableItemSource.h"
#include "AssetTypeActions_YILootTable.h"
#include "AssetTypeActions_YIItemGenerator.h"
#include "YIInventoryBagFactory.h"
#include "YILootTableFactory.h"
#include "YIRarityProfileFactory.h"
#include "YIItemGeneratorFactory.h"
#include "ToolMenus.h"
// #include "YIInventoryAssetEditor.h" // legacy editor removed

#include "GraphPanelNodeFactory_YOLO.h"
#include "EdGraphUtilities.h"
#include "Styling/AppStyle.h"
#include "YIItemDefinition.h"
#include "YIAffixAsset.h"
#include "YIAffixFactory.h"
#include "AssetTypeActions_YIAffixPool.h"
#include "YIAffixPoolFactory.h"
#include "YIAffixPoolAsset.h"
#include "SYIUnifiedDashboard.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"

TSharedPtr<FGraphPanelNodeFactory> GYOLONodeFactory;

uint32 GYOLOInventoryAssetCategory = EAssetTypeCategories::Misc;
static const FName YOLOInventoryDashboardTabName(TEXT("YOLOInventory_Dashboard"));

#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/IToolkitHost.h"

class FAssetTypeActions_YIItemDefinition : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override { return NSLOCTEXT("YOLOInventory", "ItemDefAssetTypeName", "Item Definition"); }
	virtual FColor GetTypeColor() const override { return FColor(120, 170, 255); }
	virtual UClass* GetSupportedClass() const override { return UYIItemDefinition::StaticClass(); }
	virtual uint32 GetCategories() override { return GYOLOInventoryAssetCategory; }
	virtual bool HasActions(const TArray<UObject*>& InObjects) const override { return false; }
	virtual void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor) override
	{
		for (UObject* Obj : InObjects)
		{
			FYOLOInventoryEditorModule::Get().OpenDashboardForAsset(Obj);
		}
	}
};

FYOLOInventoryEditorModule& FYOLOInventoryEditorModule::Get()
{
	return FModuleManager::LoadModuleChecked<FYOLOInventoryEditorModule>("YOLOInventoryEditor");
}

void FYOLOInventoryEditorModule::OpenDashboardForAsset(UObject* Asset)
{
	if (!Asset)
	{
		return;
	}

	FGlobalTabmanager::Get()->TryInvokeTab(YOLOInventoryDashboardTabName);
	if (TSharedPtr<SYIUnifiedDashboard> Dashboard = DashboardWidget.Pin())
	{
		Dashboard->OpenAsset(Asset);
	}
	else
	{
		PendingAsset = Asset;
	}
}

void FYOLOInventoryEditorModule::RegisterDashboardWidget(const TSharedPtr<SYIUnifiedDashboard>& Widget)
{
	DashboardWidget = Widget;
	if (PendingAsset.IsValid())
	{
		if (TSharedPtr<SYIUnifiedDashboard> Dashboard = DashboardWidget.Pin())
		{
			Dashboard->OpenAsset(PendingAsset.Get());
			PendingAsset.Reset();
		}
	}
}
void FYOLOInventoryEditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	GYOLOInventoryAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName("YOLOInventory"), NSLOCTEXT("YOLOInventory", "AssetCategory", "YOLO Inventory"));
	RegisterAssetTypeActions();
	GYOLONodeFactory = MakeShareable(new FGraphPanelNodeFactory_YOLO());

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(YOLOInventoryDashboardTabName, FOnSpawnTab::CreateLambda([](const FSpawnTabArgs& Args)
	{
		TSharedRef<SYIUnifiedDashboard> Dashboard = SNew(SYIUnifiedDashboard);
		FYOLOInventoryEditorModule::Get().RegisterDashboardWidget(Dashboard);
		return SNew(SDockTab)
			.TabRole(ETabRole::NomadTab)
			.Label(NSLOCTEXT("YOLOInventory","DashboardTab","YOLO Inventory Dashboard"))
			[
				Dashboard
			];
	}))
	.SetDisplayName(NSLOCTEXT("YOLOInventory","DashboardTabName","YOLO Inventory Dashboard"))
	.SetTooltipText(NSLOCTEXT("YOLOInventory","DashboardTabTooltip","View all YOLO Inventory items (assets + data table rows)"))
	.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "ContentBrowser.TabIcon"));

	// // Ensure an example Affix asset exists for authoring docs/demo
	// {
	// 	FString PackageName = TEXT("/Game/YOLOInventory/Affixes/Affix_Example");
	// 	FAssetRegistryModule& Arm = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	// 	FAssetData Existing = Arm.Get().GetAssetByObjectPath(*FString::Printf(TEXT("%s.%s"), *PackageName, TEXT("Affix_Example")));
	// 	if (!Existing.IsValid())
	// 	{
	// 		IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	// 		FString Name, Path; Tools.CreateUniqueAssetName(PackageName, TEXT(""), Name, Path);
	// 		UYIAffixFactory* Factory = NewObject<UYIAffixFactory>();
	// 		UClass* Class = UYIAffixAsset::StaticClass();
	// 		UObject* NewAsset = Tools.CreateAsset(FPackageName::GetLongPackageAssetName(Name), FPackageName::GetLongPackagePath(Path), Class, Factory);
	// 		if (UYIAffixAsset* Affix = Cast<UYIAffixAsset>(NewAsset))
	// 		{
	// 			Affix->DisplayName = NSLOCTEXT("YOLOInventory", "AffixExampleName", "Example Affix");
	// 			Affix->Description = NSLOCTEXT("YOLOInventory", "AffixExampleDesc", "Example modifier used by YOLOInventory. Edit Min/Max and TooltipFormat.");
	// 			Affix->TooltipFormat = NSLOCTEXT("YOLOInventory", "AffixExampleFmt", "+{0}% Damage");
	// 			Affix->MinValue = 5.f; Affix->MaxValue = 10.f;
	// 			Affix->Modify();
	// 		}
	// 	}
	// }

	// // Ensure an example Affix Pool asset exists for authoring/demo
	// {
	// 	FString PoolPkg = TEXT("/Game/YOLOInventory/Affixes/Pool_Example");
	// 	FAssetRegistryModule& Arm2 = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	// 	FAssetData Existing = Arm2.Get().GetAssetByObjectPath(*FString::Printf(TEXT("%s.%s"), *PoolPkg, TEXT("Pool_Example")));
	// 	if (!Existing.IsValid())
	// 	{
	// 		IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	// 		FString Name, Path; Tools.CreateUniqueAssetName(PoolPkg, TEXT(""), Name, Path);
	// 		UYIAffixPoolFactory* Factory = NewObject<UYIAffixPoolFactory>();
	// 		UClass* Class = UYIAffixPoolAsset::StaticClass();
	// 		UObject* NewAsset = Tools.CreateAsset(FPackageName::GetLongPackageAssetName(Name), FPackageName::GetLongPackagePath(Path), Class, Factory);
	// 		if (UYIAffixPoolAsset* Pool = Cast<UYIAffixPoolAsset>(NewAsset))
	// 		{
	// 			// stub: leave empty to edit in project
	// 			Pool->Modify();
	// 		}
	// 	}
	// }

	FEdGraphUtilities::RegisterVisualNodeFactory(GYOLONodeFactory);

	// Register Window -> YOLO Inventory menu and toolbar buttons
	static bool bYOLOMenusExtended = false;
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
	{
		static bool bExtended = false; if (bExtended) return; bExtended = true;
		UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
		FToolMenuSection& Sec = WindowMenu->AddSection("YOLOInventory", NSLOCTEXT("YOLOInventory","WindowMenu","YOLO Inventory"));
		auto AddMenuEntry = [&Sec](const FName& TabId, const FText& Label)
		{
			const FName EntryName = FName(*FString::Printf(TEXT("YOLOInventory_Open_%s"), *TabId.ToString()));
			Sec.AddMenuEntry(EntryName, Label, FText(), FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([TabId]()
				{
					FGlobalTabmanager::Get()->TryInvokeTab(TabId);
				}))
			);
		};
		AddMenuEntry(YOLOInventoryDashboardTabName, NSLOCTEXT("YOLOInventory","MenuDashboard","Dashboard"));

		if (UToolMenu* Toolbar = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar"))
		{
			FToolMenuSection& TSec = Toolbar->AddSection("YOLOInventoryToolbar", NSLOCTEXT("YOLOInventory","Toolbar","YOLO Inventory"));
			auto AddTool = [&TSec](const FName& TabId, const FText& Label, const FName& IconName)
			{
				const FName EntryName = FName(*FString::Printf(TEXT("YOLOInventory_Tool_%s"), *TabId.ToString()));
				TSec.AddEntry(FToolMenuEntry::InitToolBarButton(
					EntryName,
					FUIAction(FExecuteAction::CreateLambda([TabId]()
					{
						FGlobalTabmanager::Get()->TryInvokeTab(TabId);
					})),
					Label,
					FText(),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), IconName)
				));
			};
			AddTool(YOLOInventoryDashboardTabName, NSLOCTEXT("YOLOInventory","ToolDashboard","Dashboard"), "ContentBrowser.TabIcon");
		}

		// Add Tools menu action to validate unique item codes
		if (UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools"))
		{
			FToolMenuSection& Sec2 = ToolsMenu->AddSection("YOLOInventoryTools", NSLOCTEXT("YOLOInventory","Tools","YOLO Inventory"));
			Sec2.AddMenuEntry("YOLO_ValidateUniqueCodes",
				NSLOCTEXT("YOLOInventory","ValidateCodes","Validate Unique Item Codes"),
				NSLOCTEXT("YOLOInventory","ValidateCodes_TT","Scan all UYIItemDefinition assets and ensure UniqueCode is non-zero and unique. Auto-fix assigns new codes where needed."),
				FSlateIcon(),
				FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext&)
				{
					// old editor removed
					UClass* LibClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/Script/YOLOInventoryEditor.YIItemEditorLibrary"));
					if (LibClass)
					{
						UFunction* Fn = LibClass->FindFunctionByName(TEXT("EnsureUniqueCodes"));
						if (Fn)
						{
							UObject* CDO = LibClass->GetDefaultObject();
							struct { bool bAutoFix; bool ReturnValue; } Parms{ true, false };
							CDO->ProcessEvent(Fn, &Parms);
						}
					}
				})
			);
		}
	}));
}

void FYOLOInventoryEditorModule::ShutdownModule()
{
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(YOLOInventoryDashboardTabName);
	UnregisterAssetTypeActions();
	if (GYOLONodeFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(GYOLONodeFactory);
		GYOLONodeFactory.Reset();
	}
}

void FYOLOInventoryEditorModule::RegisterAssetTypeActions()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	{
		TSharedRef<FAssetTypeActions_YIInventoryBag> Action = MakeShared<FAssetTypeActions_YIInventoryBag>();
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}
	{
		TSharedRef<FAssetTypeActions_YIItemDefinition> Action = MakeShared<FAssetTypeActions_YIItemDefinition>();
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}
	{
		TSharedRef<FAssetTypeActions_YIAffix> Action = MakeShared<FAssetTypeActions_YIAffix>();
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}
	{
		TSharedRef<FAssetTypeActions_YIAffixPool> Action = MakeShared<FAssetTypeActions_YIAffixPool>();
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}
	{
		TSharedRef<FAssetTypeActions_YIAttributeDef> Action = MakeShared<FAssetTypeActions_YIAttributeDef>();
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}
	{
		TSharedRef<FAssetTypeActions_YIAttributeMod> Action = MakeShared<FAssetTypeActions_YIAttributeMod>();
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}
	{
		TSharedRef<FAssetTypeActions_YIItemVariant> Action = MakeShared<FAssetTypeActions_YIItemVariant>();
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}
	{
		TSharedRef<FAssetTypeActions_YIRarityPalette> Action = MakeShared<FAssetTypeActions_YIRarityPalette>();
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}
	{
		TSharedRef<FAssetTypeActions_YIRarityProfile> Action = MakeShared<FAssetTypeActions_YIRarityProfile>();
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}
	{
		TSharedRef<FAssetTypeActions_YILootTable> Action = MakeShared<FAssetTypeActions_YILootTable>();
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}
	{
		TSharedRef<FAssetTypeActions_YIItemGenerator> Action = MakeShared<FAssetTypeActions_YIItemGenerator>();
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}
	{
		TSharedRef<FAssetTypeActions_YIEvolutionPath> Action = MakeShared<FAssetTypeActions_YIEvolutionPath>();
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}
	{
		TSharedRef<FAssetTypeActions_YIItemSFXProfile> Action = MakeShared<FAssetTypeActions_YIItemSFXProfile>();
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}
	{
		TSharedRef<FAssetTypeActions_YIItemSFXLibrary> Action = MakeShared<FAssetTypeActions_YIItemSFXLibrary>();
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}
	{
		TSharedRef<FAssetTypeActions_YIDataTableItemSource> Action = MakeShared<FAssetTypeActions_YIDataTableItemSource>();
		AssetTools.RegisterAssetTypeActions(Action);
		RegisteredAssetTypeActions.Add(Action);
	}
}

void FYOLOInventoryEditorModule::UnregisterAssetTypeActions()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		for (auto& Action : RegisteredAssetTypeActions)
		{
			AssetTools.UnregisterAssetTypeActions(Action.ToSharedRef());
		}
	}
	RegisteredAssetTypeActions.Empty();
}

IMPLEMENT_MODULE(FYOLOInventoryEditorModule, YOLOInventoryEditor)
