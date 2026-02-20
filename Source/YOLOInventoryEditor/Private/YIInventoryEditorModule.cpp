#include "YIInventoryEditorModule.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
// #include "YIInventoryAssetEditor.h" // legacy editor removed

#include "GraphPanelNodeFactory_YOLO.h"
#include "EdGraphUtilities.h"
#include "Styling/AppStyle.h"
#include "SYIUnifiedDashboard.h"
#include "YIUnifiedDashboardEditor.h"
#include "YIAutoPickupDropActorDetails.h"
#include "YIAutoPickupDropActor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"
#include "PropertyEditorModule.h"

TSharedPtr<FGraphPanelNodeFactory> GYOLONodeFactory;

static const FName YOLOInventoryDashboardTabName(TEXT("YOLOInventory_Dashboard"));
static const FName YOLOInventoryHelpTabName(TEXT("YOLOInventory_Help"));

namespace
{
static EYIUnifiedDashboardTab YIHelpIndexToTab(int32 Index)
{
	switch (Index)
	{
	case 1: return EYIUnifiedDashboardTab::Affixes;
	case 2: return EYIUnifiedDashboardTab::Generators;
	default: return EYIUnifiedDashboardTab::Items;
	}
}
}

FYOLOInventoryEditorModule& FYOLOInventoryEditorModule::Get()
{
	return FModuleManager::LoadModuleChecked<FYOLOInventoryEditorModule>("YOLOInventoryEditor");
}

void FYOLOInventoryEditorModule::OpenDashboardForAsset(UObject* Asset)
{
	TSharedPtr<FYIUnifiedDashboardEditor> Editor = DashboardEditor.Pin();
	if (!Editor.IsValid())
	{
		Editor = MakeShared<FYIUnifiedDashboardEditor>();
		DashboardEditor = Editor;
		Editor->InitEditor(EToolkitMode::Standalone, nullptr, Asset);
		return;
	}

	Editor->BringToolkitToFront();
	if (Asset)
	{
		Editor->OpenAsset(Asset);
	}
	else
	{
		Editor->SetActiveTab(EYIUnifiedDashboardTab::Items);
	}
}

void FYOLOInventoryEditorModule::OpenDashboard()
{
	OpenDashboardForAsset(nullptr);
}

void FYOLOInventoryEditorModule::OpenDashboardHelp()
{
	TSharedPtr<FYIUnifiedDashboardEditor> Editor = DashboardEditor.Pin();
	if (!Editor.IsValid())
	{
		Editor = MakeShared<FYIUnifiedDashboardEditor>();
		DashboardEditor = Editor;
		Editor->InitEditor(EToolkitMode::Standalone, nullptr, nullptr);
	}
	Editor->BringToolkitToFront();
	Editor->OpenHelpTab();
}

void FYOLOInventoryEditorModule::RegisterHelpWidget(const TSharedPtr<SYIUnifiedHelpPanel>& Widget)
{
	if (!Widget.IsValid())
	{
		return;
	}
	HelpWidgets.Add(Widget);
	const int32 Clamped = FMath::Clamp(LastHelpTabIndex, 0, 2);
	Widget->SetActiveTab(YIHelpIndexToTab(Clamped));
}

void FYOLOInventoryEditorModule::UpdateHelpTabIndex(int32 Index)
{
	LastHelpTabIndex = FMath::Clamp(Index, 0, 2);
	for (int32 i = HelpWidgets.Num() - 1; i >= 0; --i)
	{
		if (TSharedPtr<SYIUnifiedHelpPanel> Widget = HelpWidgets[i].Pin())
		{
			Widget->SetActiveTab(YIHelpIndexToTab(LastHelpTabIndex));
		}
		else
		{
			HelpWidgets.RemoveAt(i, 1, EAllowShrinking::No);
		}
	}
}
void FYOLOInventoryEditorModule::StartupModule()
{
	GYOLONodeFactory = MakeShareable(new FGraphPanelNodeFactory_YOLO());

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

	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditorModule.RegisterCustomClassLayout(
			AYIAutoPickupDropActor::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FYIAutoPickupDropActorDetails::MakeInstance));
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}

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
					if (TabId == YOLOInventoryDashboardTabName)
					{
						FYOLOInventoryEditorModule::Get().OpenDashboard();
					}
					else if (TabId == YOLOInventoryHelpTabName)
					{
						FYOLOInventoryEditorModule::Get().OpenDashboardHelp();
					}
				}))
			);
		};
		AddMenuEntry(YOLOInventoryDashboardTabName, NSLOCTEXT("YOLOInventory","MenuDashboard","Dashboard"));
		AddMenuEntry(YOLOInventoryHelpTabName, NSLOCTEXT("YOLOInventory","MenuHelp","Help"));

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
						if (TabId == YOLOInventoryDashboardTabName)
						{
							FYOLOInventoryEditorModule::Get().OpenDashboard();
						}
						else if (TabId == YOLOInventoryHelpTabName)
						{
							FYOLOInventoryEditorModule::Get().OpenDashboardHelp();
						}
					})),
					Label,
					FText(),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), IconName)
				));
			};
			AddTool(YOLOInventoryDashboardTabName, NSLOCTEXT("YOLOInventory","ToolDashboard","Dashboard"), "ContentBrowser.TabIcon");
			AddTool(YOLOInventoryHelpTabName, NSLOCTEXT("YOLOInventory","ToolHelp","Help"), "Icons.Help");
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
	DashboardEditor.Reset();
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditorModule.UnregisterCustomClassLayout(AYIAutoPickupDropActor::StaticClass()->GetFName());
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}
	if (GYOLONodeFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(GYOLONodeFactory);
		GYOLONodeFactory.Reset();
	}
}

IMPLEMENT_MODULE(FYOLOInventoryEditorModule, YOLOInventoryEditor)
