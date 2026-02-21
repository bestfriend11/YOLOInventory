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
#include "Styling/SlateIconFinder.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STileView.h"
#include "Widgets/Images/SImage.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "YIItemRegistrySubsystem.h"
#include "Data/YIDataTableItemSource.h"
#include "CSVBPFunctionLibrary.h"
#include "RowData.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "PackageTools.h"
#include "ObjectTools.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "UObject/UnrealType.h"
#include "YIItemSchemaResolver.h"

TWeakPtr<FYIInventoryBagEditor> FYIInventoryBagEditor::ActiveEditor;

static const FName Tab_BagGrid("YOLOInventory_Bag_Grid");
static const FName Tab_BagDetails("YOLOInventory_Bag_Details");
static const FName Tab_BagPalette("YOLOInventory_Bag_Palette");
static const FName Tab_BagInfo("YOLOInventory_Bag_Info");

static FString YIEditorGrid_GetRowStringFromStruct(const UScriptStruct* Struct, const uint8* RowData, FName Field)
{
	if (!Struct || !RowData || Field.IsNone())
	{
		return FString();
	}

	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		const FProperty* Prop = *It;
		if (!Prop || !Prop->GetAuthoredName().Equals(Field.ToString(), ESearchCase::IgnoreCase))
		{
			continue;
		}

		if (const FStrProperty* StrProp = CastField<FStrProperty>(Prop))
		{
			return StrProp->GetPropertyValue_InContainer(RowData);
		}
		if (const FNameProperty* NameProp = CastField<FNameProperty>(Prop))
		{
			return NameProp->GetPropertyValue_InContainer(RowData).ToString();
		}
		if (const FTextProperty* TextProp = CastField<FTextProperty>(Prop))
		{
			return TextProp->GetPropertyValue_InContainer(RowData).ToString();
		}
	}
	return FString();
}

static bool YIEditorGrid_MatchesFieldByAuthoredName(const FProperty* Property, const FName FieldName)
{
	if (!Property || FieldName.IsNone())
	{
		return false;
	}
	return Property->GetAuthoredName().Equals(FieldName.ToString(), ESearchCase::IgnoreCase);
}

static bool YIEditorGrid_TryReadInt64FromProperty(const FProperty* Property, const uint8* RowData, int64& OutValue)
{
	if (!Property || !RowData)
	{
		return false;
	}

	if (const FNumericProperty* Num = CastField<FNumericProperty>(Property))
	{
		const bool bTreatAsSigned = Num->CanHoldValue<int64>(-1);
		if (Num->IsInteger())
		{
			OutValue = bTreatAsSigned
				? Num->GetSignedIntPropertyValue(Property->ContainerPtrToValuePtr<uint8>(RowData))
				: (int64)Num->GetUnsignedIntPropertyValue(Property->ContainerPtrToValuePtr<uint8>(RowData));
			return true;
		}
		OutValue = (int64)Num->GetFloatingPointPropertyValue(Property->ContainerPtrToValuePtr<uint8>(RowData));
		return true;
	}

	FString AsString;
	if (const FStrProperty* Str = CastField<FStrProperty>(Property))
	{
		AsString = Str->GetPropertyValue_InContainer(RowData);
	}
	else if (const FNameProperty* NameProp = CastField<FNameProperty>(Property))
	{
		AsString = NameProp->GetPropertyValue_InContainer(RowData).ToString();
	}
	else if (const FTextProperty* TextProp = CastField<FTextProperty>(Property))
	{
		AsString = TextProp->GetPropertyValue_InContainer(RowData).ToString();
	}
	else
	{
		return false;
	}

	int64 Parsed = 0;
	if (LexTryParseString(Parsed, *AsString))
	{
		OutValue = Parsed;
		return true;
	}
	return false;
}

static bool YIEditorGrid_TryExtractRowCode(const UScriptStruct* RowStruct, const uint8* RowData, const FName ConfiguredField, int64& OutCode)
{
	if (!RowStruct || !RowData)
	{
		return false;
	}

	auto TryField = [&](const FName FieldName) -> bool
	{
		if (FieldName.IsNone())
		{
			return false;
		}
		for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
		{
			const FProperty* Prop = *It;
			if (YIEditorGrid_MatchesFieldByAuthoredName(Prop, FieldName) && YIEditorGrid_TryReadInt64FromProperty(Prop, RowData, OutCode))
			{
				return true;
			}
		}
		return false;
	};

	if (TryField(ConfiguredField))
	{
		return true;
	}

	static const FName FallbackNames[] = { TEXT("UniqueCode"), TEXT("Code"), TEXT("ItemCode"), TEXT("ID") };
	for (const FName Fallback : FallbackNames)
	{
		if (TryField(Fallback))
		{
			return true;
		}
	}
	return false;
}

static bool YIEditorGrid_TryExtractRowText(const UScriptStruct* RowStruct, const uint8* RowData, const FName FieldName, FString& OutValue)
{
	if (!RowStruct || !RowData || FieldName.IsNone())
	{
		return false;
	}

	for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
	{
		const FProperty* Prop = *It;
		if (!YIEditorGrid_MatchesFieldByAuthoredName(Prop, FieldName))
		{
			continue;
		}
		if (const FStrProperty* Str = CastField<FStrProperty>(Prop))
		{
			OutValue = Str->GetPropertyValue_InContainer(RowData);
			return true;
		}
		if (const FNameProperty* NameProp = CastField<FNameProperty>(Prop))
		{
			OutValue = NameProp->GetPropertyValue_InContainer(RowData).ToString();
			return true;
		}
		if (const FTextProperty* TextProp = CastField<FTextProperty>(Prop))
		{
			OutValue = TextProp->GetPropertyValue_InContainer(RowData).ToString();
			return true;
		}
		if (const FNumericProperty* Num = CastField<FNumericProperty>(Prop))
		{
			const bool bTreatAsSigned = Num->CanHoldValue<int64>(-1);
			OutValue = Num->IsInteger()
				? (bTreatAsSigned
					? LexToString(Num->GetSignedIntPropertyValue(Prop->ContainerPtrToValuePtr<uint8>(RowData)))
					: LexToString(Num->GetUnsignedIntPropertyValue(Prop->ContainerPtrToValuePtr<uint8>(RowData))))
				: LexToString(Num->GetFloatingPointPropertyValue(Prop->ContainerPtrToValuePtr<uint8>(RowData)));
			return true;
		}
		return false;
	}
	return false;
}

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
	RefreshRuntimeEntries();

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
					for (const TSharedPtr<FYIBagDataRowEntry>& Entry : DataRowEntries)
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
			SAssignNew(DataRowListView, SListView<TSharedPtr<FYIBagDataRowEntry>>)
			.ListItemsSource(&DataRowEntries)
			.OnGenerateRow(this, &FYIInventoryBagEditor::MakeDataRowWidget)
			.OnSelectionChanged_Lambda([this](TSharedPtr<FYIBagDataRowEntry> Entry, ESelectInfo::Type)
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
			.OnMouseButtonDoubleClick_Lambda([this](TSharedPtr<FYIBagDataRowEntry> Entry)
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
				TArray<TSharedPtr<FYIBagDataRowEntry>> Selected = DataRowListView->GetSelectedItems();
				if (Selected.Num() == 1)
				{
					return BuildDataRowContextMenu(Selected[0]);
				}
				return TSharedPtr<SWidget>();
			})
		];

	TSharedRef<SWidget> RuntimeWidget =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(4)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(2)
			[
				SNew(SButton)
				.Text(NSLOCTEXT("YOLOInventory","BagPaletteRuntimeRefresh","Refresh Runtime View"))
				.OnClicked_Lambda([this]()
				{
					RefreshRuntimeEntries();
					if (RuntimeItemTileView.IsValid())
					{
						RuntimeItemTileView->RequestListRefresh();
					}
					return FReply::Handled();
				})
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(4, 0)
			[
				SNew(STextBlock)
				.Text_Lambda([this]()
				{
					const int32 Count = Bag ? Bag->Items.Num() : 0;
					return FText::Format(NSLOCTEXT("YOLOInventory","BagPaletteRuntimeCount","Items in bag: {0}"), FText::AsNumber(Count));
				})
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(4)
		[
			SAssignNew(RuntimeItemTileView, STileView<TSharedPtr<int32>>)
			.ListItemsSource(&RuntimeEntries)
			.ItemWidth(220.f)
			.ItemHeight(64.f)
			.OnGenerateTile(this, &FYIInventoryBagEditor::MakeRuntimeItemTile)
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
			+ SHorizontalBox::Slot().AutoWidth().Padding(2)
			[
				SNew(SCheckBox)
				.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
				.IsChecked_Lambda([this](){ return PaletteMode == EYIBagPaletteMode::Runtime ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
				{
					if (State == ECheckBoxState::Checked)
					{
						PaletteMode = EYIBagPaletteMode::Runtime;
						RefreshRuntimeEntries();
						if (RuntimeItemTileView.IsValid())
						{
							RuntimeItemTileView->RequestListRefresh();
						}
					}
				})
				.Content()
				[
					SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","BagPaletteRuntime","Runtime"))
				]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex_Lambda([this]()
			{
				if (PaletteMode == EYIBagPaletteMode::Assets) return 0;
				if (PaletteMode == EYIBagPaletteMode::DataRows) return 1;
				return 2;
			})
			+ SWidgetSwitcher::Slot()[ AssetPickerWidget ]
			+ SWidgetSwitcher::Slot()[ DataRowWidget ]
			+ SWidgetSwitcher::Slot()[ RuntimeWidget ]
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

					TSharedPtr<FYIBagDataRowEntry> Entry = MakeShared<FYIBagDataRowEntry>();
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
								const FString RowDisplay = YIEditorGrid_GetRowStringFromStruct(Table->RowStruct, *Found, PreviewField);
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
				FString TemplateIdValue;

				const bool bFoundCode = YIEditorGrid_TryExtractRowCode(Table->RowStruct, RowData, CodeField, CodeValue);
				if (!bFoundCode)
				{
					continue;
				}
				if (TemplateField != NAME_None)
				{
					YIEditorGrid_TryExtractRowText(Table->RowStruct, RowData, TemplateField, TemplateIdValue);
				}

				const FString RowKey = FString::Printf(TEXT("%lld|%s"), CodeValue, *RowName.ToString());
				if (ExistingRowKeys.Contains(RowKey))
				{
					continue;
				}
				TSharedPtr<FYIBagDataRowEntry> Entry = MakeShared<FYIBagDataRowEntry>();
				Entry->Code = CodeValue;
				Entry->RowName = RowName;
				Entry->bIsDataTable = true;
				Entry->DataSource = Source;
				Entry->DataTable = Table;
				Entry->TemplateId = TemplateIdValue;
				Entry->Source = SourcePath.ToString();
				Entry->Name = RowName.ToString();

				const FString PreviewName = YIEditorGrid_GetRowStringFromStruct(Table->RowStruct, RowData, PreviewField);
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

void FYIInventoryBagEditor::RefreshRuntimeEntries()
{
	RuntimeEntries.Reset();
	if (!Bag)
	{
		return;
	}

	for (int32 Index = 0; Index < Bag->Items.Num(); ++Index)
	{
		RuntimeEntries.Add(MakeShared<int32>(Index));
	}
}

const FSlateBrush* FYIInventoryBagEditor::ResolveRuntimeItemIcon(UYIItemDefinition* Def)
{
	if (Def)
	{
		const TSoftObjectPtr<UTexture2D> EffectiveIcon = YIItemSchema::GetIcon(Def);
		UTexture2D* IconTex = EffectiveIcon.IsValid() ? EffectiveIcon.Get() : nullptr;
		if (!IconTex)
		{
			IconTex = EffectiveIcon.LoadSynchronous();
		}
		if (IconTex)
		{
			TSharedPtr<FSlateDynamicImageBrush>& Brush = RuntimeIconBrushCache.FindOrAdd(IconTex);
			if (!Brush.IsValid())
			{
				Brush = MakeShared<FSlateDynamicImageBrush>(IconTex->GetFName(), FVector2D(IconTex->GetSizeX(), IconTex->GetSizeY()));
				Brush->SetResourceObject(IconTex);
			}
			return Brush.Get();
		}
	}
	return FSlateIconFinder::FindIconBrushForClass(UYIItemDefinition::StaticClass());
}

FText FYIInventoryBagEditor::BuildRuntimeItemTooltip(const UYIItemDefinition* Def, int32 Count) const
{
	if (!Def)
	{
		return NSLOCTEXT("YOLOInventory","BagRuntimeTooltipMissing","Missing item definition.");
	}

	const FString DisplayName = YIItemSchema::GetDisplayName(Def).ToString();
	const FText EffectiveDescription = YIItemSchema::GetDescription(Def);
	const FString Description = EffectiveDescription.IsEmpty() ? TEXT("-") : EffectiveDescription.ToString();
	const FGameplayTag EffectiveItemType = YIItemSchema::GetItemType(Def);
	const FGameplayTag EffectiveRarity = YIItemSchema::GetRarityTag(Def);
	const FString Type = EffectiveItemType.IsValid() ? EffectiveItemType.ToString() : TEXT("-");
	const FString Rarity = EffectiveRarity.IsValid() ? EffectiveRarity.ToString() : TEXT("-");

	const FString Tooltip = FString::Printf(
		TEXT("%s\nCount: %d\nType: %s\nRarity: %s\nCode: %lld\nTemplate: %s\n\n%s"),
		*DisplayName,
		Count,
		*Type,
		*Rarity,
		(long long)Def->UniqueCode,
		*Def->TemplateId,
		*Description);
	return FText::FromString(Tooltip);
}

TSharedRef<ITableRow> FYIInventoryBagEditor::MakeRuntimeItemTile(TSharedPtr<int32> ItemIndex, const TSharedRef<STableViewBase>& Owner)
{
	const FYIBagItem* BagItem = nullptr;
	UYIItemDefinition* Def = nullptr;
	int32 Count = 0;

	if (Bag && ItemIndex.IsValid() && Bag->Items.IsValidIndex(*ItemIndex))
	{
		BagItem = &Bag->Items[*ItemIndex];
		Def = BagItem->Item.Definition.IsValid() ? BagItem->Item.Definition.Get() : BagItem->Item.Definition.LoadSynchronous();
		Count = BagItem->Item.Count;
	}

	const FText NameText = Def
		? YIItemSchema::GetDisplayName(Def)
		: NSLOCTEXT("YOLOInventory","BagRuntimeUnknown","Unknown");
	const FText MetaText = Def
		? FText::FromString(FString::Printf(TEXT("x%d  |  %s"), Count, YIItemSchema::GetRarityTag(Def).IsValid() ? *YIItemSchema::GetRarityTag(Def).ToString() : TEXT("No Rarity")))
		: FText::FromString(TEXT("x0"));
	const FSlateBrush* IconBrush = ResolveRuntimeItemIcon(Def);

	return SNew(STableRow<TSharedPtr<int32>>, Owner)
	[
		SNew(SBorder)
		.Padding(6)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.ToolTipText(BuildRuntimeItemTooltip(Def, Count))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
			[
				SNew(SBox)
				.WidthOverride(44.f)
				.HeightOverride(44.f)
				[
					SNew(SImage).Image(IconBrush)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(NameText)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(MetaText)
					.ColorAndOpacity(FLinearColor(0.75f, 0.75f, 0.75f))
				]
			]
		]
	];
}

TSharedRef<ITableRow> FYIInventoryBagEditor::MakeDataRowWidget(TSharedPtr<FYIBagDataRowEntry> Entry, const TSharedRef<STableViewBase>& Owner)
{
	return SNew(STableRow<TSharedPtr<FYIBagDataRowEntry>>, Owner)
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
						UE_LOG(LogTemp, Error, TEXT("Bag palette create failed for source '%s'."), Entry.IsValid() ? *Entry->Source : TEXT("<invalid>"));
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
						UE_LOG(LogTemp, Warning, TEXT("Bag palette add failed for source '%s'."), Entry.IsValid() ? *Entry->Source : TEXT("<invalid>"));
					}
				}
				return FReply::Handled();
			})
		]
	];
}

TSharedPtr<SWidget> FYIInventoryBagEditor::BuildDataRowContextMenu(const TSharedPtr<FYIBagDataRowEntry>& Entry) const
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

bool FYIInventoryBagEditor::CreateAssetFromEntry(const FYIBagDataRowEntry& Entry) const
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

	FString AssetName = YIEditorGrid_GetRowStringFromStruct(Table->RowStruct, RowPtr, AssetNameField);
	FString PackagePath = YIEditorGrid_GetRowStringFromStruct(Table->RowStruct, RowPtr, PackagePathField);

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

bool FYIInventoryBagEditor::AddEntryToBag(const FYIBagDataRowEntry& Entry)
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
	NewItem.Size = YIItemSchema::GetDefaultSize(Def);
	NewItem.Pos = FIntPoint::ZeroValue;
	Bag->AddBagItem(NewItem);
	RefreshRuntimeEntries();
	if (RuntimeItemTileView.IsValid())
	{
		RuntimeItemTileView->RequestListRefresh();
	}
	return true;
}
