#include "YIInventoryBagEditor.h"
#include "YIInventoryBag.h"
#include "SBagEditor.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "IContentBrowserSingleton.h"
#include "ContentBrowserModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "YIItemDefinition.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Styling/AppStyle.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

TWeakPtr<FYIInventoryBagEditor> FYIInventoryBagEditor::ActiveEditor;

static const FName Tab_BagGrid("YOLOInventory_Bag_Grid");
static const FName Tab_BagDetails("YOLOInventory_Bag_Details");
static const FName Tab_BagPalette("YOLOInventory_Bag_Palette");
static const FName Tab_BagInfo("YOLOInventory_Bag_Info");

void FYIInventoryBagEditor::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UYIInventoryBag* InBag)
{
	Bag = InBag;
	{
		TSharedRef<FYIInventoryBagEditor> SelfRef = StaticCastSharedRef<FYIInventoryBagEditor>(AsShared());
		ActiveEditor = SelfRef;
	}

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("YOLOInventory_Bag_Layout_v2")
		->AddArea(
			FTabManager::NewPrimaryArea()
			->Split(
				FTabManager::NewSplitter()
				->SetOrientation(Orient_Horizontal)
				->Split(FTabManager::NewStack()->SetSizeCoefficient(0.20f)->AddTab(Tab_BagPalette, ETabState::OpenedTab))
				->Split(FTabManager::NewStack()->SetSizeCoefficient(0.55f)->AddTab(Tab_BagGrid, ETabState::OpenedTab))
				->Split(FTabManager::NewStack()->SetSizeCoefficient(0.25f)->AddTab(Tab_BagDetails, ETabState::OpenedTab))
			)
			->Split(
				FTabManager::NewStack()->SetSizeCoefficient(0.20f)->AddTab(Tab_BagInfo, ETabState::OpenedTab)
			)
		);

	const bool bCreateMenu = true;
	const bool bCreateToolbar = true;
	InitAssetEditor(Mode, InitToolkitHost, GetToolkitFName(), Layout, bCreateMenu, bCreateToolbar, { InBag });

// Define a local workspace menu category so tabs show under Window for this editor
WorkspaceMenuCategory = TabManager->AddLocalWorkspaceMenuCategory(NSLOCTEXT("YOLOInventory","BagWorkspace","YOLO Inventory Bag"));

// AddApplicationMode is not required here for UE 5.7 minimal editor setup.
// AddApplicationMode("Default", MakeShareable(new FApplicationMode("Default")));

TabManager->RegisterTabSpawner(Tab_BagGrid, FOnSpawnTab::CreateSP(this, &FYIInventoryBagEditor::SpawnGridTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory","BagGrid","Grid"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
	TabManager->RegisterTabSpawner(Tab_BagDetails, FOnSpawnTab::CreateSP(this, &FYIInventoryBagEditor::SpawnDetailsTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory","BagDetails","Details"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
	TabManager->RegisterTabSpawner(Tab_BagPalette, FOnSpawnTab::CreateSP(this, &FYIInventoryBagEditor::SpawnPaletteTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory","BagPalette","Palette"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());
	TabManager->RegisterTabSpawner(Tab_BagInfo, FOnSpawnTab::CreateSP(this, &FYIInventoryBagEditor::SpawnInfoTab))
		.SetDisplayName(NSLOCTEXT("YOLOInventory","BagInfo","Info"))
		.SetGroup(WorkspaceMenuCategory.ToSharedRef());

	// Ensure tabs are opened on editor start
	TabManager->TryInvokeTab(Tab_BagPalette);
	TabManager->TryInvokeTab(Tab_BagGrid);
	TabManager->TryInvokeTab(Tab_BagDetails);
	TabManager->TryInvokeTab(Tab_BagInfo);
}

void FYIInventoryBagEditor::UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(Tab_BagGrid);
	InTabManager->UnregisterTabSpawner(Tab_BagDetails);
	InTabManager->UnregisterTabSpawner(Tab_BagPalette);
	InTabManager->UnregisterTabSpawner(Tab_BagInfo);
}

TSharedRef<SDockTab> FYIInventoryBagEditor::SpawnGridTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
	[
		SAssignNew(GridWidget, SBagEditor).Bag(Bag)
	];
}

TSharedRef<SDockTab> FYIInventoryBagEditor::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
	FPropertyEditorModule& PropModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DArgs;
	DArgs.bHideSelectionTip = true;
	TSharedRef<IDetailsView> Details = PropModule.CreateDetailView(DArgs);
	Details->SetObject(Bag);
	return SNew(SDockTab)[ Details ];
}

TSharedRef<SDockTab> FYIInventoryBagEditor::SpawnPaletteTab(const FSpawnTabArgs& Args)
{
	FAssetPickerConfig Picker;
	Picker.InitialAssetViewType = EAssetViewType::Tile;
	Picker.Filter.ClassPaths.Add(UYIItemDefinition::StaticClass()->GetClassPathName());
	// Apply bag filters if set
	if (Bag && Bag->bUseFolderFilter)
	{
		for (FDirectoryPath& Dir : Bag->FolderFilters)
		{
			if (!Dir.Path.IsEmpty())
			{
				Picker.Filter.PackagePaths.Add(*Dir.Path);
			}
		}
	}
	if (Bag && Bag->bUseTagFilter)
	{
		for (FName& Tag : Bag->TagFilters)
		{
			Picker.Filter.TagsAndValues.Add(Tag, TOptional<FString>(FString(TEXT("true"))));
		}
	}
	Picker.bAllowNullSelection = false;
	// Do not add items on selection; adding occurs via drag-and-drop onto the grid (SBagEditor::OnDrop)
	Picker.OnAssetSelected = FOnAssetSelected::CreateLambda([this](const FAssetData& AD)
	{
		// Intentionally no-op to avoid adding items prematurely when clicking in the palette.
	});
	// Open asset in editor when double-clicked
	Picker.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &FYIInventoryBagEditor::OnPaletteAssetDoubleClicked);

	FContentBrowserModule& CB = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	return SNew(SDockTab)[ CB.Get().CreateAssetPicker(Picker) ];
}

TSharedRef<SDockTab> FYIInventoryBagEditor::SpawnInfoTab(const FSpawnTabArgs& Args)
{
	return SNew(SDockTab)
	[
		SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("DetailsView.CategoryTop"))
			.Padding(FMargin(8))
			[
				SNew(SVerticalBox)
				// Header
				+ SVerticalBox::Slot().AutoHeight()[ SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","InfoHeader","Bag Info & Controls")) ]
				// Live status separator
				+ SVerticalBox::Slot().AutoHeight().Padding(0,4)[ SNew(SSeparator) ]
				// Live status
				+ SVerticalBox::Slot().AutoHeight()[
					SNew(STextBlock).Text_Lambda([this](){
						if (!Bag) return FText::FromString(TEXT("No Bag"));
						FString Line1 = FString::Printf(TEXT("Grid: %dx%d  Minify: %.2f  Cell: %.0f"), Bag->GridSize.X, Bag->GridSize.Y, Bag->MinifyScale, Bag->CellPixelSize);
						if (GridWidget.IsValid())
						{
							FIntPoint C = GridWidget->GetPreviewCell();
							int32 H = GridWidget->GetHoveredIndex();
							int32 S = GridWidget->GetSelectedIndex();
							Line1 += FString::Printf(TEXT("\nHovered Cell: (%d,%d)  HoverIdx: %d  SelectedIdx: %d"), C.X, C.Y, H, S);
						}
						return FText::FromString(Line1);
					})
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0,6)[ SNew(SSeparator) ]
				// Sorting controls
				+ SVerticalBox::Slot().AutoHeight()[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)[ SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","SortBy","Sort By:")) ]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2)[
						SAssignNew(SortCombo, SComboBox<TSharedPtr<FString>>)
							.OptionsSource(&SortOptionsCache)
							.OnComboBoxOpening_Lambda([this]()
							{
								SortOptionsCache.Reset();
								if (GridWidget.IsValid())
								{
									// Copy current options from grid
									SortOptionsCache = GridWidget->GetSortOptions();
								}
								if (SortCombo.IsValid())
								{
									SortCombo->RefreshOptions();
								}
							})
							.OnGenerateWidget_Lambda([](TSharedPtr<FString> In){ return SNew(STextBlock).Text(FText::FromString(*In)); })
							.OnSelectionChanged_Lambda([this](TSharedPtr<FString> In, ESelectInfo::Type){ if (GridWidget.IsValid()) GridWidget->SetSelectedSort(In); })
							[ SNew(STextBlock).Text_Lambda([this](){ return (GridWidget.IsValid() && GridWidget->GetSelectedSort().IsValid())? FText::FromString(*GridWidget->GetSelectedSort()) : NSLOCTEXT("YOLOInventory","Choose","Choose"); }) ]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(2).VAlign(VAlign_Center)[
						SNew(SCheckBox)
							.OnCheckStateChanged_Lambda([this](ECheckBoxState){ if (GridWidget.IsValid()) GridWidget->ToggleSortAscending(); })
							.Content()[ SNew(STextBlock).Text_Lambda([this](){ return (GridWidget.IsValid() && GridWidget->IsSortAscending())? NSLOCTEXT("YOLOInventory","Asc","Asc"): NSLOCTEXT("YOLOInventory","Desc","Desc"); }) ]
					]
				]
				// Cell size slider
				+ SVerticalBox::Slot().AutoHeight().Padding(0,2)[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","CellSize","Cell")) ]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4,0)[
						SNew(SSlider)
							.MinValue(8.f).MaxValue(128.f)
							.Value_Lambda([this](){ return GridWidget.IsValid()? GridWidget->GetCellPixelSize() : 32.f; })
							.OnValueChanged_Lambda([this](float V){ if (GridWidget.IsValid()) GridWidget->SetCellPixelSize(V); })
					]
				]
				// Minify slider
				+ SVerticalBox::Slot().AutoHeight().Padding(0,2)[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[ SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","Minify","Minify")) ]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4,0)[
						SNew(SSlider)
							.MinValue(0.1f).MaxValue(1.0f)
							.Value_Lambda([this](){ return GridWidget.IsValid()? GridWidget->GetMinifyScale() : 1.0f; })
							.OnValueChanged_Lambda([this](float V){ if (GridWidget.IsValid()) GridWidget->SetMinifyScale(V); })
					]
				]
			]
		]
	;
}

void FYIInventoryBagEditor::OnPaletteAssetDoubleClicked(const FAssetData& AssetData)
{
	TArray<UObject*> ToOpen;
	if (UObject* Asset = AssetData.GetAsset())
	{
		ToOpen.Add(Asset);
	}
	else
	{
		UObject* Loaded = AssetData.ToSoftObjectPath().TryLoad();
		if (Loaded) ToOpen.Add(Loaded);
	}
	if (ToOpen.Num() > 0)
	{
		if (GEditor)
		{
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(ToOpen);
		}
	}
}
