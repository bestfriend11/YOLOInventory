#include "YIInventoryEditorModule.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
// #include "YIInventoryAssetEditor.h" // legacy editor removed

#include "Styling/AppStyle.h"
#include "SYIUnifiedDashboard.h"
#include "YIUnifiedDashboardEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/Docking/TabManager.h"

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
	return FModuleManager::LoadModuleChecked<FYOLOInventoryEditorModule>("YOLOInventoryEditorCore");
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
			Sec2.AddMenuEntry("YOLO_OpenUnifiedDashboard",
				NSLOCTEXT("YOLOInventory","OpenUnifiedDashboard","Open Unified Dashboard"),
				NSLOCTEXT("YOLOInventory","OpenUnifiedDashboard_TT","Open YOLO Inventory unified dashboard window."),
				FSlateIcon(),
				FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext&)
				{
					FYOLOInventoryEditorModule::Get().OpenDashboard();
				})
			);
			Sec2.AddMenuEntry("YOLO_OpenUnifiedDashboardHelp",
				NSLOCTEXT("YOLOInventory","OpenUnifiedDashboardHelp","Open Dashboard Help"),
				NSLOCTEXT("YOLOInventory","OpenUnifiedDashboardHelp_TT","Open Help panel inside YOLO Inventory unified dashboard."),
				FSlateIcon(),
				FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext&)
				{
					FYOLOInventoryEditorModule::Get().OpenDashboardHelp();
				})
			);
			Sec2.AddMenuEntry("YOLO_ValidateUniqueCodes",
				NSLOCTEXT("YOLOInventory","ValidateCodes","Validate Unique Item Codes"),
				NSLOCTEXT("YOLOInventory","ValidateCodes_TT","Scan all UYIItemDefinition assets and ensure UniqueCode is non-zero and unique. Auto-fix assigns new codes where needed."),
				FSlateIcon(),
				FToolMenuExecuteAction::CreateLambda([](const FToolMenuContext&)
				{
					// old editor removed
					UClass* LibClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/Script/YOLOInventoryEditorCore.YIItemEditorLibrary"));
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
	BagDashboardFactory.Unbind();
	GeneratorDashboardFactory.Unbind();
	SchemaDashboardFactory.Unbind();
	DashboardEditor.Reset();
}

void FYOLOInventoryEditorModule::RegisterBagDashboardFactory(FYICreateBagDashboardBridge InFactory)
{
	BagDashboardFactory = MoveTemp(InFactory);
}

void FYOLOInventoryEditorModule::ClearBagDashboardFactory()
{
	BagDashboardFactory.Unbind();
}

bool FYOLOInventoryEditorModule::HasBagDashboardFactory() const
{
	return BagDashboardFactory.IsBound();
}

TSharedRef<IYIBagDashboardBridge> FYOLOInventoryEditorModule::CreateBagDashboardBridge()
{
	checkf(BagDashboardFactory.IsBound(), TEXT("YOLOInventoryEditorCore: Bag dashboard factory is not registered."));
	return BagDashboardFactory.Execute();
}

void FYOLOInventoryEditorModule::RegisterGeneratorDashboardFactory(FYICreateGeneratorDashboardBridge InFactory)
{
	GeneratorDashboardFactory = MoveTemp(InFactory);
}

void FYOLOInventoryEditorModule::ClearGeneratorDashboardFactory()
{
	GeneratorDashboardFactory.Unbind();
}

bool FYOLOInventoryEditorModule::HasGeneratorDashboardFactory() const
{
	return GeneratorDashboardFactory.IsBound();
}

TSharedRef<IYIGeneratorDashboardBridge> FYOLOInventoryEditorModule::CreateGeneratorDashboardBridge()
{
	checkf(GeneratorDashboardFactory.IsBound(), TEXT("YOLOInventoryEditorCore: Generator dashboard factory is not registered."));
	return GeneratorDashboardFactory.Execute();
}

void FYOLOInventoryEditorModule::RegisterSchemaDashboardFactory(FYICreateSchemaDashboardBridge InFactory)
{
	SchemaDashboardFactory = MoveTemp(InFactory);
}

void FYOLOInventoryEditorModule::ClearSchemaDashboardFactory()
{
	SchemaDashboardFactory.Unbind();
}

bool FYOLOInventoryEditorModule::HasSchemaDashboardFactory() const
{
	return SchemaDashboardFactory.IsBound();
}

TSharedRef<IYISchemaDashboardBridge> FYOLOInventoryEditorModule::CreateSchemaDashboardBridge()
{
	checkf(SchemaDashboardFactory.IsBound(), TEXT("YOLOInventoryEditorCore: Schema dashboard factory is not registered."));
	return SchemaDashboardFactory.Execute();
}

IMPLEMENT_MODULE(FYOLOInventoryEditorModule, YOLOInventoryEditorCore)
