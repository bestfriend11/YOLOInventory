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
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Styling/AppStyle.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Views/SListView.h"
#include "SYIItemDashboard.h"
#include "YIItemRegistrySubsystem.h"
#include "Data/YIDataTableItemSource.h"
#include "CSVBPFunctionLibrary.h"
#include "RowData.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/BlueprintFactory.h"
#include "YIItemDefinitionFactory.h"
#include "PackageTools.h"
#include "ObjectTools.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "YIEditorRowHelpers.h"

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
		SAssignNew(GridWidget, SBagEditor)
			.Bag(Bag)
			.OnSelectionChanged(SBagEditor::FOnSelectionChanged::CreateSP(this, &FYIInventoryBagEditor::HandleGridSelectionChanged))
	];
}

TSharedRef<SDockTab> FYIInventoryBagEditor::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
	FPropertyEditorModule& PropModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DArgs;
	DArgs.bHideSelectionTip = true;
	DetailsView = PropModule.CreateDetailView(DArgs);
	DetailsView->SetObject(Bag);
	return SNew(SDockTab)[ DetailsView.ToSharedRef() ];
}

void FYIInventoryBagEditor::HandleGridSelectionChanged(int32 Index)
{
	if (!DetailsView.IsValid() || !Bag)
	{
		return;
	}

	if (Index == INDEX_NONE || !Bag->Items.IsValidIndex(Index))
	{
		DetailsView->SetObject(Bag);
		return;
	}

	FYIBagItem& Item = Bag->Items[Index];
	UYIItemDefinition* Def = Item.Item.Definition.IsValid() ? Item.Item.Definition.Get() : Item.Item.Definition.LoadSynchronous();
	if (Def)
	{
		DetailsView->SetObject(Def);
	}
	else
	{
		DetailsView->SetObject(Bag);
	}
}

TSharedRef<SDockTab> FYIInventoryBagEditor::SpawnPaletteTab(const FSpawnTabArgs& Args)
{
	RefreshDataRowEntries();

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
	TSharedRef<SWidget> AssetPickerWidget = CB.Get().CreateAssetPicker(Picker);

	TSharedRef<SWidget> DataRowWidget =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(2)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("YOLOInventory","BagPaletteRefreshRows","Refresh Rows"))
				.OnClicked_Lambda([this]()
				{
					RefreshDataRowEntries();
					if (DataRowListView.IsValid())
					{
						DataRowListView->RequestListRefresh();
					}
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("YOLOInventory","BagPaletteCreateAll","Create Missing Assets"))
				.ToolTipText(NSLOCTEXT("YOLOInventory","BagPaletteCreateAll_TT","Create assets for all rows that do not yet have an item asset."))
				.OnClicked_Lambda([this]()
				{
					for (const TSharedPtr<FYIItemDashboardEntry>& Entry : DataRowEntries)
					{
						if (Entry.IsValid() && Entry->bIsDataTable && !Entry->bHasAsset)
						{
							CreateAssetFromEntry(*Entry);
						}
					}
					RefreshDataRowEntries();
					if (DataRowListView.IsValid())
					{
						DataRowListView->RequestListRefresh();
					}
					return FReply::Handled();
				})
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(4)
		[
			SAssignNew(DataRowListView, SListView<TSharedPtr<FYIItemDashboardEntry>>)
			.ListItemsSource(&DataRowEntries)
			.OnGenerateRow(this, &FYIInventoryBagEditor::MakeDataRowWidget)
			.OnSelectionChanged_Lambda([this](TSharedPtr<FYIItemDashboardEntry> Entry, ESelectInfo::Type)
			{
				if (!DetailsView.IsValid())
				{
					return;
				}
				if (Entry.IsValid())
				{
					if (Entry->ItemAsset.IsValid())
					{
						DetailsView->SetObject(Entry->ItemAsset.Get());
					}
					else if (Entry->DataSource.IsValid())
					{
						DetailsView->SetObject(Entry->DataSource.Get());
					}
					else
					{
						DetailsView->SetObject(Bag);
					}
				}
				else
				{
					DetailsView->SetObject(Bag);
				}
			})
			.OnMouseButtonDoubleClick_Lambda([this](TSharedPtr<FYIItemDashboardEntry> Entry)
			{
				if (!Entry.IsValid())
				{
					return;
				}
				if (Entry->ItemAsset.IsValid())
				{
					OnPaletteAssetDoubleClicked(FAssetData(Entry->ItemAsset.Get()));
				}
				else if (CreateAssetFromEntry(*Entry))
				{
					RefreshDataRowEntries();
					if (Entry->ItemAsset.IsValid())
					{
						OnPaletteAssetDoubleClicked(FAssetData(Entry->ItemAsset.Get()));
					}
				}
			})
			.OnContextMenuOpening_Lambda([this]()
			{
				if (!DataRowListView.IsValid())
				{
					return TSharedPtr<SWidget>();
				}
				TArray<TSharedPtr<FYIItemDashboardEntry>> Selected = DataRowListView->GetSelectedItems();
				if (Selected.Num() == 1)
				{
					return BuildDataRowContextMenu(Selected[0]);
				}
				return TSharedPtr<SWidget>();
			})
		];

	return SNew(SDockTab)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(2)
			[
				SNew(SCheckBox)
				.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
				.IsChecked_Lambda([this](){ return PaletteMode == EYIBagPaletteMode::Assets ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
				{
					if (State == ECheckBoxState::Checked)
					{
						PaletteMode = EYIBagPaletteMode::Assets;
					}
				})
				.Content()
				[
					SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","BagPaletteAssets","Assets"))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2)
			[
				SNew(SCheckBox)
				.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
				.IsChecked_Lambda([this](){ return PaletteMode == EYIBagPaletteMode::DataRows ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
				{
					if (State == ECheckBoxState::Checked)
					{
						PaletteMode = EYIBagPaletteMode::DataRows;
					}
				})
				.Content()
				[
					SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","BagPaletteRows","Data Rows"))
				]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex_Lambda([this]() { return PaletteMode == EYIBagPaletteMode::Assets ? 0 : 1; })
			+ SWidgetSwitcher::Slot()[ AssetPickerWidget ]
			+ SWidgetSwitcher::Slot()[ DataRowWidget ]
		]
	];
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

void FYIInventoryBagEditor::RefreshDataRowEntries()
{
	DataRowEntries.Reset();
	TMap<int64, TSoftObjectPtr<UYIItemDefinition>> ExistingAssets;
	TSet<FString> ExistingRowKeys;
	TSet<FSoftObjectPath> RegistrySourcePaths;

	if (GEngine)
	{
		if (UYIItemRegistrySubsystem* Registry = GEngine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
		{
			TArray<FYIItemRegistryView> ViewItems;
			Registry->GetAllItems(ViewItems, true);

			for (const FYIItemRegistryView& View : ViewItems)
			{
				if (!View.bIsDataTable)
				{
					ExistingAssets.Add(View.UniqueCode, TSoftObjectPtr<UYIItemDefinition>(View.Object.ToSoftObjectPath()));
				}
				else
				{
					ExistingRowKeys.Add(FString::Printf(TEXT("%lld|%s"), View.UniqueCode, *View.RowName.ToString()));
					if (View.DataSource.IsValid())
					{
						RegistrySourcePaths.Add(View.DataSource.ToSoftObjectPath());
					}

					TSharedPtr<FYIItemDashboardEntry> Entry = MakeShared<FYIItemDashboardEntry>();
					Entry->Code = View.UniqueCode;
					Entry->RowName = View.RowName;
					Entry->bIsDataTable = true;
					Entry->DataSource = View.DataSource;
					Entry->TemplateId = View.TemplateId;
					Entry->Source = View.SourcePath;
					Entry->Name = View.RowName.IsNone() ? TEXT("Row") : View.RowName.ToString();

					if (UYIDataTableItemSource* Source = View.DataSource.LoadSynchronous())
					{
						if (UDataTable* Table = Source->ResolveDataTable())
						{
							Entry->DataTable = Table;
							const FName PreviewField = Source->PreviewNameFieldName.IsNone() ? TEXT("DisplayName") : Source->PreviewNameFieldName;
							if (const uint8* const* Found = Table->GetRowMap().Find(View.RowName))
							{
								const FString RowDisplay = YIEditor_GetRowStringFromStruct(Table->RowStruct, *Found, PreviewField);
								if (!RowDisplay.IsEmpty())
								{
									Entry->Name = RowDisplay;
								}
							}
						}
					}

					if (TSoftObjectPtr<UYIItemDefinition>* FoundAsset = ExistingAssets.Find(View.UniqueCode))
					{
						Entry->bHasAsset = true;
						Entry->ItemAsset = *FoundAsset;
					}

					DataRowEntries.Add(Entry);
				}
			}
		}
	}

	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FARFilter Filter;
	Filter.ClassPaths.Add(UYIDataTableItemSource::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	TArray<FAssetData> Sources;
	AssetRegistry.Get().GetAssets(Filter, Sources);

	TSet<FSoftObjectPath> SeenSourcePaths;
	auto ProcessSource = [&](UYIDataTableItemSource* Source, const FSoftObjectPath& SourcePath)
	{
		if (!Source)
		{
			return;
		}
		if (UDataTable* Table = Source->ResolveDataTable())
		{
			const FName CodeField = Source->UniqueCodeFieldName.IsNone() ? TEXT("UniqueCode") : Source->UniqueCodeFieldName;
			const FName PreviewField = Source->PreviewNameFieldName.IsNone() ? TEXT("DisplayName") : Source->PreviewNameFieldName;
			const FName TemplateField = Source->TemplateIdFieldName;

			for (const auto& RowPair : Table->GetRowMap())
			{
				const FName RowName = RowPair.Key;
				const uint8* RowData = RowPair.Value;

				int64 CodeValue = 0;
				bool bFoundCode = false;
				FString TemplateIdValue;

				for (TFieldIterator<FProperty> It(Table->RowStruct); It; ++It)
				{
					const FProperty* Prop = *It;
					if (!Prop) continue;

					if (Prop->GetFName() == CodeField)
					{
						if (const FNumericProperty* Num = CastField<FNumericProperty>(Prop))
						{
							if (Num->IsInteger())
							{
								CodeValue = Num->GetSignedIntPropertyValue(Prop->ContainerPtrToValuePtr<uint8>(RowData));
								bFoundCode = true;
							}
						}
					}
					else if (TemplateField != NAME_None && Prop->GetFName() == TemplateField)
					{
						if (const FStrProperty* Str = CastField<FStrProperty>(Prop))
						{
							TemplateIdValue = Str->GetPropertyValue_InContainer(RowData);
						}
						else if (const FNameProperty* NameProp = CastField<FNameProperty>(Prop))
						{
							TemplateIdValue = NameProp->GetPropertyValue_InContainer(RowData).ToString();
						}
						else if (const FTextProperty* TextProp = CastField<FTextProperty>(Prop))
						{
							TemplateIdValue = TextProp->GetPropertyValue_InContainer(RowData).ToString();
						}
					}
				}

				if (!bFoundCode)
				{
					continue;
				}

				const FString RowKey = FString::Printf(TEXT("%lld|%s"), CodeValue, *RowName.ToString());
				if (ExistingRowKeys.Contains(RowKey))
				{
					continue;
				}
				TSharedPtr<FYIItemDashboardEntry> Entry = MakeShared<FYIItemDashboardEntry>();
				Entry->Code = CodeValue;
				Entry->RowName = RowName;
				Entry->bIsDataTable = true;
				Entry->DataSource = Source;
				Entry->DataTable = Table;
				Entry->TemplateId = TemplateIdValue;
				Entry->Source = SourcePath.ToString();
				Entry->Name = RowName.ToString();

				const FString PreviewName = YIEditor_GetRowStringFromStruct(Table->RowStruct, RowData, PreviewField);
				if (!PreviewName.IsEmpty())
				{
					Entry->Name = PreviewName;
				}

				if (TSoftObjectPtr<UYIItemDefinition>* FoundAsset = ExistingAssets.Find(CodeValue))
				{
					Entry->bHasAsset = true;
					Entry->ItemAsset = *FoundAsset;
				}

				DataRowEntries.Add(Entry);
				ExistingRowKeys.Add(RowKey);
			}
		}
	};

	for (const FAssetData& SourceData : Sources)
	{
		UObject* SourceObj = SourceData.GetAsset();
		if (!SourceObj)
		{
			SourceObj = SourceData.ToSoftObjectPath().TryLoad();
		}
		if (UYIDataTableItemSource* Source = Cast<UYIDataTableItemSource>(SourceObj))
		{
			const FSoftObjectPath SourcePath = SourceData.ToSoftObjectPath();
			SeenSourcePaths.Add(SourcePath);
			ProcessSource(Source, SourcePath);
		}
	}

	for (const FSoftObjectPath& SourcePath : RegistrySourcePaths)
	{
		if (SeenSourcePaths.Contains(SourcePath))
		{
			continue;
		}
		if (UYIDataTableItemSource* Source = Cast<UYIDataTableItemSource>(SourcePath.TryLoad()))
		{
			ProcessSource(Source, SourcePath);
		}
	}
}

TSharedRef<ITableRow> FYIInventoryBagEditor::MakeDataRowWidget(TSharedPtr<FYIItemDashboardEntry> Entry, const TSharedRef<STableViewBase>& Owner)
{
	return SNew(STableRow<TSharedPtr<FYIItemDashboardEntry>>, Owner)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(2)
		[
			SNew(STextBlock).Text(Entry.IsValid() ? FText::FromString(Entry->Name) : FText::GetEmpty())
		]
		+ SHorizontalBox::Slot().FillWidth(0.35f).Padding(2)
		[
			SNew(STextBlock).Text(Entry.IsValid() ? FText::FromString(Entry->Source) : FText::GetEmpty())
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			Entry.IsValid() && Entry->bHasAsset
			? SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","BagPaletteHasAsset","Asset"))
			: SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","BagPaletteMissingAsset","Missing"))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			SNew(SButton)
			.Text(NSLOCTEXT("YOLOInventory","BagPaletteCreateAsset","Create"))
			.IsEnabled_Lambda([Entry](){ return Entry.IsValid() && Entry->bIsDataTable; })
			.OnClicked_Lambda([this, Entry]()
			{
				if (Entry.IsValid())
				{
					const bool bCreated = CreateAssetFromEntry(*Entry);
					if (!bCreated)
					{
						FNotificationInfo Info(NSLOCTEXT("YOLOInventory","BagPaletteCreateFailed","Create failed. Check DataTable source + transformer/inline mapping."));
						Info.ExpireDuration = 4.f;
						FSlateNotificationManager::Get().AddNotification(Info);
					}
					RefreshDataRowEntries();
					if (DataRowListView.IsValid())
					{
						DataRowListView->RequestListRefresh();
					}
				}
				return FReply::Handled();
			})
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			SNew(SButton)
			.Text(NSLOCTEXT("YOLOInventory","BagPaletteAddToBag","Add"))
			.IsEnabled_Lambda([Entry](){ return Entry.IsValid(); })
			.OnClicked_Lambda([this, Entry]()
			{
				if (Entry.IsValid())
				{
					if (!AddEntryToBag(*Entry))
					{
						FNotificationInfo Info(NSLOCTEXT("YOLOInventory","BagPaletteAddFailed","Add failed. No valid item definition found for this entry."));
						Info.ExpireDuration = 4.f;
						FSlateNotificationManager::Get().AddNotification(Info);
					}
				}
				return FReply::Handled();
			})
		]
	];
}

TSharedPtr<SWidget> FYIInventoryBagEditor::BuildDataRowContextMenu(const TSharedPtr<FYIItemDashboardEntry>& Entry) const
{
	if (!Entry.IsValid())
	{
		return nullptr;
	}

	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.AddMenuEntry(
		NSLOCTEXT("YOLOInventory","BagRow_CreateAsset","Create Item Asset from Row"),
		NSLOCTEXT("YOLOInventory","BagRow_CreateAsset_TT","Run the transformer on this row and create/update the item asset."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([this, Entry]()
		{
			if (Entry.IsValid())
			{
				CreateAssetFromEntry(*Entry);
			}
		}))
	);
	if (Entry->ItemAsset.IsValid())
	{
		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("YOLOInventory","BagRow_OpenAsset","Open Item Asset"),
			NSLOCTEXT("YOLOInventory","BagRow_OpenAsset_TT","Open this item asset in the editor."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([Entry]()
			{
				if (Entry.IsValid() && Entry->ItemAsset.IsValid())
				{
					if (UAssetEditorSubsystem* AssetEditor = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr)
					{
						AssetEditor->OpenEditorForAsset(Entry->ItemAsset.Get());
					}
				}
			}))
		);
	}
	return MenuBuilder.MakeWidget();
}

bool FYIInventoryBagEditor::CreateAssetFromEntry(const FYIItemDashboardEntry& Entry) const
{
	if (!Entry.bIsDataTable || !Entry.DataSource.IsValid())
	{
		return false;
	}

	UYIDataTableItemSource* Source = Entry.DataSource.LoadSynchronous();
	if (!Source)
	{
		return false;
	}

	const bool bHasInline = Source->bUseInlineMappings && Source->InlineMappings.Num() > 0;
	if (!Source->TransformerClass && !bHasInline)
	{
		return false;
	}

	UDataTable* Table = Source->DataTable.LoadSynchronous();
	if (!Table || !Table->RowStruct)
	{
		return false;
	}

	const uint8* const* Found = Table->GetRowMap().Find(Entry.RowName);
	const uint8* RowPtr = Found ? *Found : nullptr;
	if (!RowPtr)
	{
		return false;
	}

	const FName AssetNameField = Source->AssetNameFieldName.IsNone() ? TEXT("AssetName") : Source->AssetNameFieldName;
	const FName PackagePathField = Source->PackagePathFieldName.IsNone() ? TEXT("PackagePath") : Source->PackagePathFieldName;

	FString AssetName = YIEditor_GetRowStringFromStruct(Table->RowStruct, RowPtr, AssetNameField);
	FString PackagePath = YIEditor_GetRowStringFromStruct(Table->RowStruct, RowPtr, PackagePathField);

	if (AssetName.IsEmpty())
	{
		AssetName = Entry.RowName.IsNone() ? FString::Printf(TEXT("Item_%lld"), (long long)Entry.Code) : Entry.RowName.ToString();
	}
	if (PackagePath.IsEmpty())
	{
		PackagePath = TEXT("/Game/YOLOInventory/Generated");
	}

	URowData* RowWrapper = NewObject<URowData>();
	RowWrapper->Address = const_cast<uint8*>(RowPtr);
	RowWrapper->Struct = Table->RowStruct;

	UYIItemDefinition* Def = nullptr;
	if (TSubclassOf<UCSVDataTransformer> Effective = Source->GetEffectiveTransformerClass())
	{
		if (UCSVDataTransformer* Transformer = NewObject<UCSVDataTransformer>(Source, Effective))
		{
			Def = Cast<UYIItemDefinition>(Transformer->TransformObject(RowWrapper));
		}
	}
	if (!Def && GEngine)
	{
		// Inline mapping path (no transformer)
		if (UYIItemRegistrySubsystem* Registry = GEngine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
		{
			Def = Registry->GetByCode(Entry.Code);
		}
	}
	if (!Def)
	{
		return false;
	}

	const FString AssetLongPath = PackagePath / AssetName;
	const FString SanitizedPackage = UPackageTools::SanitizePackageName(AssetLongPath);
	const FString ObjectPath = SanitizedPackage + TEXT(".") + AssetName;

	if (UYIItemDefinition* Existing = Cast<UYIItemDefinition>(StaticLoadObject(UYIItemDefinition::StaticClass(), nullptr, *ObjectPath)))
	{
		if (UCSVBPFunctionLibrary::AutoAssignProperties(RowWrapper, Existing, {}))
		{
			Existing->MarkPackageDirty();
			return true;
		}
		return false;
	}

	UPackage* Pkg = CreatePackage(*SanitizedPackage);
	UObject* NewAsset = NewObject<UObject>(Pkg, Def->GetClass(), *AssetName, RF_Public | RF_Standalone | RF_Transactional, Def);
	if (!NewAsset)
	{
		return false;
	}

	FAssetRegistryModule::AssetCreated(NewAsset);
	Pkg->MarkPackageDirty();
	return true;
}

bool FYIInventoryBagEditor::AddEntryToBag(const FYIItemDashboardEntry& Entry)
{
	if (!Bag)
	{
		return false;
	}

	UYIItemDefinition* Def = nullptr;
	if (Entry.ItemAsset.IsValid())
	{
		Def = Entry.ItemAsset.Get();
	}
	if (!Def && GEngine)
	{
		if (UYIItemRegistrySubsystem* Registry = GEngine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
		{
			Def = Registry->GetByCode(Entry.Code);
		}
	}
	if (!Def && Entry.bIsDataTable)
	{
		if (CreateAssetFromEntry(Entry))
		{
			if (Entry.ItemAsset.IsValid())
			{
				Def = Entry.ItemAsset.Get();
			}
		}
	}
	if (!Def)
	{
		return false;
	 }

	FYIBagItem NewItem;
	NewItem.Item.Definition = Def;
	NewItem.Item.Count = 1;
	NewItem.Size = Def->DefaultSize;
	NewItem.Pos = FIntPoint::ZeroValue;
	Bag->AddBagItem(NewItem);
	return true;
}
