#include "SYIItemDashboard.h"
#include "YIItemRegistrySubsystem.h"
#include "Data/YIDataTableItemSource.h"
#include "YIItemDefinition.h"
#include "CSVBPFunctionLibrary.h"
#include "RowData.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "AssetToolsModule.h"
#include "Factories/DataAssetFactory.h"
#include "IAssetTools.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "DataTableEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Styling/AppStyle.h"
#include "Misc/PackageName.h"
#include "PackageTools.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/MessageDialog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Factories/BlueprintFactory.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "InputCoreTypes.h"
#include "ObjectTools.h"
#include "Algo/Sort.h"
#include "YIEditorRowHelpers.h"
#include "YIEditorMessageLog.h"

void SYIItemDashboard::Construct(const FArguments& InArgs)
{
	FPropertyEditorModule& PropModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailArgs;
	DetailArgs.bAllowSearch = true;
	DetailArgs.bHideSelectionTip = true;
	DetailArgs.bAllowMultipleTopLevelObjects = false;
	DetailArgs.bShowOptions = true;
	DetailsView = PropModule.CreateDetailView(DetailArgs);
	LogChangedHandle = FYIEditorMessageLog::OnLogChanged().AddSP(this, &SYIItemDashboard::RefreshLogEntries);
	RefreshLogEntries();

	ChildSlot
	[
		SNew(SSplitter)
		+ SSplitter::Slot().Value(0.45f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(8)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0,0,8,0))
					[
						SNew(SButton)
						.Text(NSLOCTEXT("YOLOInventory","DashboardRefresh","Refresh"))
						.OnClicked_Lambda([this]()
						{
							Refresh();
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0,0,8,0))
					[
						SNew(SButton)
						.Text(NSLOCTEXT("YOLOInventory","DashboardCreateSource","New Data Table Source"))
						.OnClicked_Lambda([this]()
						{
							CreateDataTableSourceAsset();
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0,0,8,0))
					[
						SNew(SButton)
						.Text(NSLOCTEXT("YOLOInventory","DashboardValidateCodes","Validate Unique Codes"))
						.OnClicked_Lambda([this]()
						{
							ValidateUniqueCodes();
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0,0,8,0))
					[
						SNew(SButton)
						.Text(NSLOCTEXT("YOLOInventory","DashboardBulkCreate","Create Assets (Selected)"))
						.ToolTipText(NSLOCTEXT("YOLOInventory","DashboardBulkCreate_Tip","Generate assets for all selected data-table rows"))
						.OnClicked_Lambda([this]()
						{
							if (!ListView.IsValid())
							{
								return FReply::Handled();
							}
							const TArray<TSharedPtr<FYIItemDashboardEntry>> Selected = ListView->GetSelectedItems();
							bool bChanged = false;
							for (const TSharedPtr<FYIItemDashboardEntry>& Entry : Selected)
							{
								if (Entry.IsValid() && Entry->bIsDataTable)
								{
									bChanged |= CreateAssetFromEntry(*Entry);
								}
							}
							if (bChanged)
							{
								Refresh();
							}
							return FReply::Handled();
						})
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SNew(SSearchBox)
						.OnTextChanged(this, &SYIItemDashboard::OnSearchTextChanged)
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(8,0,0,0))
					[
						SNew(SComboBox<TSharedPtr<FString>>)
						.OptionsSource(&const_cast<SYIItemDashboard*>(this)->TargetPropertyOptions) // reuse buffer temporarily
						.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
						{
							return SNew(STextBlock).Text(InItem.IsValid() ? FText::FromString(*InItem) : FText::GetEmpty());
						})
						.OnComboBoxOpening_Lambda([this]()
						{
							TargetPropertyOptions.Reset();
							TargetPropertyOptions.Add(MakeShared<FString>(TEXT("Type: All")));
							TargetPropertyOptions.Add(MakeShared<FString>(TEXT("Type: Data Rows")));
							TargetPropertyOptions.Add(MakeShared<FString>(TEXT("Type: Assets Only")));
						})
						.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewItem, ESelectInfo::Type)
						{
							if (!NewItem.IsValid()) return;
							if (*NewItem == TEXT("Type: Data Rows")) TypeFilter = EDashTypeFilter::DataTableRows;
							else if (*NewItem == TEXT("Type: Assets Only")) TypeFilter = EDashTypeFilter::AssetsOnly;
							else TypeFilter = EDashTypeFilter::All;
							Refresh();
						})
						.InitiallySelectedItem(nullptr)
						.Content()
						[
							SNew(STextBlock).Text_Lambda([this]()
							{
								switch (TypeFilter)
								{
								case EDashTypeFilter::DataTableRows: return FText::FromString(TEXT("Type: Data Rows"));
								case EDashTypeFilter::AssetsOnly: return FText::FromString(TEXT("Type: Assets Only"));
								default: return FText::FromString(TEXT("Type: All"));
								}
							})
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(8,0,0,0))
					[
						SNew(SComboBox<TSharedPtr<FString>>)
						.OptionsSource(&const_cast<SYIItemDashboard*>(this)->ConverterOptions) // reuse buffer temporarily
						.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
						{
							return SNew(STextBlock).Text(InItem.IsValid() ? FText::FromString(*InItem) : FText::GetEmpty());
						})
						.OnComboBoxOpening_Lambda([this]()
						{
							ConverterOptions.Reset();
							ConverterOptions.Add(MakeShared<FString>(TEXT("Status: All")));
							ConverterOptions.Add(MakeShared<FString>(TEXT("Status: Needs Asset")));
							ConverterOptions.Add(MakeShared<FString>(TEXT("Status: Has Asset")));
							ConverterOptions.Add(MakeShared<FString>(TEXT("Status: Asset Only")));
						})
						.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewItem, ESelectInfo::Type)
						{
							if (!NewItem.IsValid()) return;
							if (*NewItem == TEXT("Status: Needs Asset")) StatusFilter = EDashStatusFilter::NeedsAsset;
							else if (*NewItem == TEXT("Status: Has Asset")) StatusFilter = EDashStatusFilter::HasAsset;
							else if (*NewItem == TEXT("Status: Asset Only")) StatusFilter = EDashStatusFilter::AssetOnly;
							else StatusFilter = EDashStatusFilter::All;
							Refresh();
						})
						.InitiallySelectedItem(nullptr)
						.Content()
						[
							SNew(STextBlock).Text_Lambda([this]()
							{
								switch (StatusFilter)
								{
								case EDashStatusFilter::NeedsAsset: return FText::FromString(TEXT("Status: Needs Asset"));
								case EDashStatusFilter::HasAsset: return FText::FromString(TEXT("Status: Has Asset"));
								case EDashStatusFilter::AssetOnly: return FText::FromString(TEXT("Status: Asset Only"));
								default: return FText::FromString(TEXT("Status: All"));
								}
							})
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(8,0,0,0))
					[
						SNew(SCheckBox)
						.Style(&FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("ToggleButtonCheckBox"))
						.Padding(FMargin(6,2))
						.IsChecked_Lambda([this](){ return bGroupBySource ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
						{
							bGroupBySource = (State == ECheckBoxState::Checked);
							Refresh();
						})
						[
							SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","Dash_GroupBySource","Group by Source"))
						]
					]
				]
				+ SVerticalBox::Slot().FillHeight(1.f).Padding(8)
				[
					SAssignNew(ListView, SListView<TSharedPtr<FYIItemDashboardEntry>>)
					.ListItemsSource(&FilteredItems)
					.SelectionMode(ESelectionMode::Multi)
					.OnContextMenuOpening(FOnContextMenuOpening::CreateSP(this, &SYIItemDashboard::BuildListContextMenu))
					.OnSelectionChanged_Lambda([this](TSharedPtr<FYIItemDashboardEntry> Entry, ESelectInfo::Type)
					{
						ShowDetailsForEntry(Entry);
					})
					.OnGenerateRow(this, &SYIItemDashboard::MakeRowWidget)
					.OnMouseButtonDoubleClick_Lambda([this](TSharedPtr<FYIItemDashboardEntry> Entry)
					{
						OpenEntry(Entry);
					})
					.OnKeyDownHandler(FOnKeyDown::CreateSP(this, &SYIItemDashboard::HandleListKeyDown))
					.HeaderRow
					(
				SNew(SHeaderRow)
				+ SHeaderRow::Column("Code").DefaultLabel(NSLOCTEXT("YOLOInventory","Dash_Code","Code")).FillWidth(0.15f)
				+ SHeaderRow::Column("Name").DefaultLabel(NSLOCTEXT("YOLOInventory","Dash_Name","Name")).FillWidth(0.25f)
				+ SHeaderRow::Column("Template").DefaultLabel(NSLOCTEXT("YOLOInventory","Dash_Template","TemplateId")).FillWidth(0.2f)
				+ SHeaderRow::Column("Type").DefaultLabel(NSLOCTEXT("YOLOInventory","Dash_Type","Type")).FillWidth(0.1f)
				+ SHeaderRow::Column("Source").DefaultLabel(NSLOCTEXT("YOLOInventory","Dash_Source","Source")).FillWidth(0.3f)
				+ SHeaderRow::Column("Asset").DefaultLabel(NSLOCTEXT("YOLOInventory","Dash_Asset","Asset")).FillWidth(0.1f)
			)
				]
			]
		]
		+ SSplitter::Slot().Value(0.55f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(0.6f)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				[
					DetailsView.IsValid()
					? StaticCastSharedRef<SWidget>(DetailsView.ToSharedRef())
					: StaticCastSharedRef<SWidget>(SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","Dash_NoDetails","Details panel unavailable")))
				]
			]
			+ SVerticalBox::Slot().FillHeight(0.4f).Padding(4)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
				.Visibility_Lambda([this]()
				{
					return CurrentMappingSource.IsValid() ? EVisibility::Visible : EVisibility::Collapsed;
				})
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(4)
					[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","Dash_InlineMappings","Inline Mappings"))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(8,0)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
				.OptionsSource(&const_cast<SYIItemDashboard*>(this)->TargetPropertyOptions) // reuse buffer; repopulated below
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
				{
					return SNew(STextBlock).Text(InItem.IsValid() ? FText::FromString(*InItem) : FText::GetEmpty());
				})
				.OnComboBoxOpening_Lambda([this]()
				{
					TargetPropertyOptions.Reset();
					TargetPropertyOptions.Add(MakeShared<FString>(TEXT("Inline Only")));
					TargetPropertyOptions.Add(MakeShared<FString>(TEXT("Transformer Only")));
					TargetPropertyOptions.Add(MakeShared<FString>(TEXT("Inline then Transformer")));
				})
				.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewItem, ESelectInfo::Type)
				{
					if (!CurrentMappingSource.IsValid() || !NewItem.IsValid()) return;
					CurrentMappingSource->Modify();
					if (*NewItem == TEXT("Inline Only")) CurrentMappingSource->TransformMode = EYITransformMode::InlineOnly;
					else if (*NewItem == TEXT("Transformer Only")) CurrentMappingSource->TransformMode = EYITransformMode::TransformerOnly;
					else CurrentMappingSource->TransformMode = EYITransformMode::HybridInlineThenTransformer;
				})
				.InitiallySelectedItem(nullptr)
				.Content()
				[
					SNew(STextBlock).Text_Lambda([this]()
					{
						if (!CurrentMappingSource.IsValid()) return FText::FromString(TEXT("Mode"));
						switch (CurrentMappingSource->TransformMode)
						{
						case EYITransformMode::InlineOnly: return FText::FromString(TEXT("Inline Only"));
						case EYITransformMode::TransformerOnly: return FText::FromString(TEXT("Transformer Only"));
						default: return FText::FromString(TEXT("Inline then Transformer"));
						}
					})
				]
			]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8,0)
						[
							SNew(SCheckBox)
							.IsChecked_Lambda([this]()
							{
								return (CurrentMappingSource.IsValid() && CurrentMappingSource->bUseInlineMappings) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
							})
							.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
							{
								if (CurrentMappingSource.IsValid())
								{
									CurrentMappingSource->Modify();
									CurrentMappingSource->bUseInlineMappings = (State == ECheckBoxState::Checked);
								}
							})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8,0)
						[
							SNew(SButton)
							.Text(NSLOCTEXT("YOLOInventory","Dash_AddMapping","Add Mapping"))
							.OnClicked_Lambda([this]()
							{
								if (CurrentMappingSource.IsValid())
								{
									CurrentMappingSource->Modify();
									FYIFieldMapping NewMap;
									CurrentMappingSource->InlineMappings.Add(NewMap);
									RefreshInlineMappingEditor(CurrentMappingSource.Get());
								}
								return FReply::Handled();
							})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8,0)
						[
							SNew(SButton)
							.Text(NSLOCTEXT("YOLOInventory","Dash_AutoMatch","Auto Match"))
							.ToolTipText(NSLOCTEXT("YOLOInventory","Dash_AutoMatch_TT","Match fields by name and update missing source fields."))
							.OnClicked_Lambda([this]()
							{
								AutoMatchInlineMappings(false);
								return FReply::Handled();
							})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8,0)
						[
							SNew(SButton)
							.Text(NSLOCTEXT("YOLOInventory","Dash_AddAllFields","Add All Fields"))
							.ToolTipText(NSLOCTEXT("YOLOInventory","Dash_AddAllFields_TT","Add mapping rows for every item definition field, and auto-match where possible."))
							.OnClicked_Lambda([this]()
							{
								AutoMatchInlineMappings(true);
								return FReply::Handled();
							})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8,0)
						[
							SNew(SButton)
							.Text(NSLOCTEXT("YOLOInventory","Dash_ClearMapping","Clear"))
							.OnClicked_Lambda([this]()
							{
								if (CurrentMappingSource.IsValid())
								{
									CurrentMappingSource->Modify();
									CurrentMappingSource->InlineMappings.Reset();
									RefreshInlineMappingEditor(CurrentMappingSource.Get());
								}
								return FReply::Handled();
							})
						]
					]
					+ SVerticalBox::Slot().FillHeight(1.f).Padding(4)
					[
						SAssignNew(MappingListView, SListView<TSharedPtr<FYIFieldMapping>>)
						.ListItemsSource(&MappingRows)
						.OnGenerateRow(this, &SYIItemDashboard::MakeMappingRow)
						.SelectionMode(ESelectionMode::Single)
					]
				]
			]
			+ SVerticalBox::Slot().FillHeight(0.25f).Padding(4)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(6, 4)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(NSLOCTEXT("YOLOInventory","Dash_ErrorsTitle","Errors & Notifications"))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(6,0)
						[
							SNew(SButton)
							.Text(NSLOCTEXT("YOLOInventory","Dash_ErrorsClear","Clear"))
							.OnClicked_Lambda([this]()
							{
								FYIEditorMessageLog::Clear();
								return FReply::Handled();
							})
						]
					]
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SAssignNew(LogListView, SListView<TSharedPtr<FYIEditorLogEntry>>)
						.ListItemsSource(&LogEntries)
						.OnGenerateRow(this, &SYIItemDashboard::MakeLogRow)
					]
				]
			]
		]
	];

	Refresh();
}

SYIItemDashboard::~SYIItemDashboard()
{
	if (LogChangedHandle.IsValid())
	{
		FYIEditorMessageLog::OnLogChanged().Remove(LogChangedHandle);
		LogChangedHandle.Reset();
	}
}

void SYIItemDashboard::RefreshLogEntries()
{
	LogEntries.Reset();
	for (const FYIEditorLogEntry& Entry : FYIEditorMessageLog::GetEntries())
	{
		LogEntries.Add(MakeShared<FYIEditorLogEntry>(Entry));
	}
	if (LogListView.IsValid())
	{
		LogListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SYIItemDashboard::MakeLogRow(TSharedPtr<FYIEditorLogEntry> Entry, const TSharedRef<STableViewBase>& Owner)
{
	const FLinearColor SeverityColor = Entry.IsValid() && Entry->Severity == EYIEditorLogSeverity::Error
		? FLinearColor(1.f, 0.25f, 0.2f)
		: Entry.IsValid() && Entry->Severity == EYIEditorLogSeverity::Warning
			? FLinearColor(1.f, 0.75f, 0.2f)
			: FLinearColor(0.6f, 0.9f, 0.8f);

	const FText TimeText = Entry.IsValid() ? FText::FromString(Entry->TimeUtc.ToString(TEXT("%H:%M:%S"))) : FText::GetEmpty();
	const FText ContextText = (Entry.IsValid() && !Entry->Context.IsEmpty()) ? Entry->Context : FText::GetEmpty();

	return SNew(STableRow<TSharedPtr<FYIEditorLogEntry>>, Owner)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(2,0)
		[
			SNew(STextBlock).Text(TimeText).ColorAndOpacity(FSlateColor(FLinearColor(0.6f,0.6f,0.6f)))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(6,0)
		[
			SNew(STextBlock)
			.Text(Entry.IsValid() ? Entry->Message : FText::GetEmpty())
			.ColorAndOpacity(FSlateColor(SeverityColor))
		]
		+ SHorizontalBox::Slot().FillWidth(1.f).Padding(6,0)
		[
			SNew(STextBlock)
			.Text(ContextText)
			.ColorAndOpacity(FSlateColor(FLinearColor(0.7f,0.7f,0.7f)))
			.WrapTextAt(420.f)
		]
	];
}

TSharedRef<ITableRow> SYIItemDashboard::MakeRowWidget(TSharedPtr<FYIItemDashboardEntry> Entry, const TSharedRef<STableViewBase>& Owner)
{
	auto StatusColor = [Entry]() -> FLinearColor
	{
		if (!Entry.IsValid())
		{
			return FLinearColor::Gray;
		}
		if (Entry->bIsDataTable)
		{
			return Entry->bHasAsset
				? FLinearColor(0.18f, 0.65f, 0.32f, 0.9f)    // green tint for rows with generated asset
				: FLinearColor(0.95f, 0.55f, 0.20f, 0.9f);   // orange for rows needing generation
		}
		return FLinearColor(0.20f, 0.45f, 0.90f, 0.9f); // blue for pure assets
	};
	auto StatusText = [Entry]() -> FText
	{
		if (!Entry.IsValid())
		{
			return FText::FromString(TEXT("Unknown"));
		}
		if (Entry->bIsDataTable)
		{
			return Entry->bHasAsset
				? NSLOCTEXT("YOLOInventory","Dash_Status_HasAsset","Generated asset exists")
				: NSLOCTEXT("YOLOInventory","Dash_Status_NeedsAsset","Needs asset generation");
		}
		return NSLOCTEXT("YOLOInventory","Dash_Status_AssetOnly","Item asset");
	};

	return SNew(STableRow<TSharedPtr<FYIItemDashboardEntry>>, Owner)
		.ToolTipText(BuildPreviewText(Entry))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.Padding(FMargin(2, 0))
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(StatusColor())
			.ToolTipText(StatusText())
		]
		+ SHorizontalBox::Slot().FillWidth(0.15f)
		[
			SNew(STextBlock)
			.Text(FText::AsNumber(Entry->Code))
			.ColorAndOpacity(StatusColor())
		]
		+ SHorizontalBox::Slot().FillWidth(0.25f)
		[
			SNew(STextBlock)
			.Text(FText::FromString(Entry->Name))
		]
		+ SHorizontalBox::Slot().FillWidth(0.2f)
		[
			SNew(STextBlock).Text(FText::FromString(Entry->TemplateId))
		]
		+ SHorizontalBox::Slot().FillWidth(0.1f)
		[
			SNew(STextBlock).Text(Entry->bIsDataTable
				? NSLOCTEXT("YOLOInventory","Dash_Type_DataTable","Data Table")
				: NSLOCTEXT("YOLOInventory","Dash_Type_Asset","Asset"))
			.ColorAndOpacity(StatusColor())
		]
		+ SHorizontalBox::Slot().FillWidth(0.3f)
		[
			SNew(STextBlock).Text(FText::FromString(Entry->Source))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(2)
		[
			Entry->bIsDataTable && Entry->bHasAsset
			? StaticCastSharedRef<SWidget>(
				SNew(SButton)
				.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
				.ToolTipText(NSLOCTEXT("YOLOInventory","Dash_OpenAsset","Select/Open generated item asset"))
				.OnClicked_Lambda([this, Entry]()
				{
					if (!ListView.IsValid())
					{
						return FReply::Handled();
					}

					TSharedPtr<FYIItemDashboardEntry> AssetEntry;
					for (const TSharedPtr<FYIItemDashboardEntry>& It : Items)
					{
						if (It.IsValid() && !It->bIsDataTable && It->Code == Entry->Code)
						{
							AssetEntry = It;
							break;
						}
					}

					if (AssetEntry.IsValid())
					{
						ListView->SetSelection(AssetEntry);
						ShowDetailsForEntry(AssetEntry);
					}
					else if (Entry->ItemAsset.IsValid())
					{
						if (UObject* AssetObj = Entry->ItemAsset.LoadSynchronous())
						{
							LastDetailObject = AssetObj;
							if (DetailsView.IsValid())
							{
								DetailsView->SetObject(AssetObj);
							}
						}
					}
					return FReply::Handled();
				})
				.Content()
				[
					SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","Dash_AssetBadge","Asset"))
				])
			: StaticCastSharedRef<SWidget>(SNew(SSpacer).Size(FVector2D(40.f, 1.f)))
		]
	];
}

void SYIItemDashboard::Refresh()
{
	Items.Reset();
	FilteredItems.Reset();
	TMap<int64, TSoftObjectPtr<UYIItemDefinition>> ExistingAssets;
	TSet<FString> ExistingRowKeys;

	if (GEngine)
	{
		if (UYIItemRegistrySubsystem* Registry = GEngine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
		{
			TArray<FYIItemRegistryView> ViewItems;
			Registry->GetAllItems(ViewItems, true);

			for (const FYIItemRegistryView& View : ViewItems)
			{
				TSharedPtr<FYIItemDashboardEntry> Entry = MakeShared<FYIItemDashboardEntry>();
				Entry->Code = View.UniqueCode;
				Entry->TemplateId = View.TemplateId;
				Entry->Source = View.SourcePath;
				Entry->bIsDataTable = View.bIsDataTable;
				Entry->RowName = View.RowName;
				Entry->Object = View.Object;
				Entry->DataSource = View.DataSource;

				if (!Entry->bIsDataTable)
				{
					Entry->ItemAsset = TSoftObjectPtr<UYIItemDefinition>(View.Object.ToSoftObjectPath());
					ExistingAssets.Add(Entry->Code, Entry->ItemAsset);
					if (UYIItemDefinition* Def = Cast<UYIItemDefinition>(View.Object.LoadSynchronous()))
					{
						Entry->Name = Def->DisplayName.IsEmpty() ? Def->GetName() : Def->DisplayName.ToString();
					}
					else
					{
						Entry->Name = View.Object.ToSoftObjectPath().GetAssetName();
					}
				}
				else
				{
					Entry->DataTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(View.SourcePath));
					Entry->Name = View.RowName.IsNone() ? TEXT("Row") : View.RowName.ToString();
					ExistingRowKeys.Add(FString::Printf(TEXT("%lld|%s"), Entry->Code, *Entry->RowName.ToString()));

					// Try to read a preview name field from the row (configurable on the data source)
					if (UDataTable* Table = Entry->DataTable.LoadSynchronous())
					{
						FName PreviewField = TEXT("DisplayName");
						if (UYIDataTableItemSource* Source = Entry->DataSource.LoadSynchronous())
						{
							PreviewField = Source->PreviewNameFieldName.IsNone() ? PreviewField : Source->PreviewNameFieldName;
						}

						if (const uint8* const* Found = Table->GetRowMap().Find(Entry->RowName))
						{
							const FString RowDisplay = YIEditor_GetRowStringFromStruct(Table->RowStruct, *Found, PreviewField);
							if (!RowDisplay.IsEmpty())
							{
								Entry->Name = RowDisplay;
							}
						}
					}
				}

				Items.Add(Entry);
			}
		}
	}

	// Ensure all data-table rows are represented even when a generated asset already exists
	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FARFilter Filter;
	Filter.ClassPaths.Add(UYIDataTableItemSource::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	TArray<FAssetData> Sources;
	AssetRegistry.Get().GetAssets(Filter, Sources);

	for (const FAssetData& SourceData : Sources)
	{
		if (UYIDataTableItemSource* Source = Cast<UYIDataTableItemSource>(SourceData.GetAsset()))
		{
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
					Entry->Source = SourceData.GetSoftObjectPath().ToString();
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

					Items.Add(Entry);
					ExistingRowKeys.Add(RowKey);
				}
			}
		}
	}

	// Optional grouping by source path to keep related rows together
	if (bGroupBySource)
	{
		Algo::Sort(Items, [](const TSharedPtr<FYIItemDashboardEntry>& A, const TSharedPtr<FYIItemDashboardEntry>& B)
		{
			if (!A.IsValid() || !B.IsValid()) return A.IsValid();
			int32 PathCompare = A->Source.Compare(B->Source);
			if (PathCompare == 0)
			{
				return A->Name < B->Name;
			}
			return PathCompare < 0;
		});
	}

	const FString SearchFilter = SearchText.ToString();
	for (const TSharedPtr<FYIItemDashboardEntry>& Entry : Items)
	{
		// Type filter
		if (TypeFilter == EDashTypeFilter::DataTableRows && !Entry->bIsDataTable)
		{
			continue;
		}
		if (TypeFilter == EDashTypeFilter::AssetsOnly && Entry->bIsDataTable)
		{
			continue;
		}

		// Status filter
		if (StatusFilter == EDashStatusFilter::NeedsAsset && !(Entry->bIsDataTable && !Entry->bHasAsset))
		{
			continue;
		}
		if (StatusFilter == EDashStatusFilter::HasAsset && !(Entry->bIsDataTable && Entry->bHasAsset))
		{
			continue;
		}
		if (StatusFilter == EDashStatusFilter::AssetOnly && Entry->bIsDataTable)
		{
			continue;
		}

		const bool bPass = SearchFilter.IsEmpty() ||
			Entry->Name.Contains(SearchFilter) ||
			Entry->TemplateId.Contains(SearchFilter) ||
			Entry->Source.Contains(SearchFilter) ||
			(FString::FromInt((int32)Entry->Code).Contains(SearchFilter));
		if (bPass)
		{
			FilteredItems.Add(Entry);
		}
	}

	if (ListView.IsValid())
	{
		ListView->RequestListRefresh();
	}
}

void SYIItemDashboard::OnSearchTextChanged(const FText& NewText)
{
	SearchText = NewText;
	Refresh();
}

void SYIItemDashboard::OpenEntry(const TSharedPtr<FYIItemDashboardEntry>& Entry)
{
	if (!Entry.IsValid())
	{
		return;
	}

	// Prefer inline details view to keep designers inside the dashboard
	ShowDetailsForEntry(Entry);

	if (Entry->bIsDataTable)
	{
		if (Entry->DataSource.IsValid())
		{
			const EAppReturnType::Type Result = FMessageDialog::Open(EAppMsgType::YesNo, FText::Format(
				NSLOCTEXT("YOLOInventory","DashboardCreateAssetConfirm","Create an item asset for row '{0}'?"),
				FText::FromString(Entry->Name)));
			if (Result == EAppReturnType::Yes)
			{
				if (CreateAssetFromEntry(*Entry))
				{
					Refresh();
				}
			}
			return;
		}
	}
}

void SYIItemDashboard::OpenDataSource(const TSharedPtr<FYIItemDashboardEntry>& Entry)
{
	if (!Entry.IsValid() || !Entry->DataSource.IsValid())
	{
		return;
	}

	if (UObject* Obj = Entry->DataSource.LoadSynchronous())
	{
		if (DetailsView.IsValid())
		{
			DetailsView->SetObject(Obj);
		}
	}
}

void SYIItemDashboard::CreateDataTableSourceAsset() const
{
	IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
	Factory->DataAssetClass = UYIDataTableItemSource::StaticClass();

	const FString TargetPath = TEXT("/Game/YOLOInventory/ItemSources");
	const FString BaseName = TEXT("ItemSource");
	FString PackageName, AssetName;
	Tools.CreateUniqueAssetName(TargetPath / BaseName, TEXT(""), PackageName, AssetName);
	Tools.CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), Factory->DataAssetClass, Factory);
}

void SYIItemDashboard::ValidateUniqueCodes() const
{
	if (GEngine)
	{
		if (UYIItemRegistrySubsystem* Registry = GEngine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
		{
			Registry->EnsureUniqueCodes(true);
		}
	}
}

bool SYIItemDashboard::CreateAssetFromEntry(const FYIItemDashboardEntry& Entry) const
{
	if (!Entry.bIsDataTable || !Entry.DataSource.IsValid())
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error,
			NSLOCTEXT("YOLOInventory","Dash_CreateFail_NoDataSource","Create failed: entry has no data source."),
			FText::FromString(Entry.Source));
		return false;
	}

	UYIDataTableItemSource* Source = Entry.DataSource.LoadSynchronous();
	if (!Source)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error,
			NSLOCTEXT("YOLOInventory","Dash_CreateFail_SourceLoad","Create failed: could not load data source."),
			FText::FromString(Entry.Source));
		return false;
	}

	const bool bHasInline = Source->bUseInlineMappings && Source->InlineMappings.Num() > 0;

	if (!Source->TransformerClass && !bHasInline)
	{
		const EAppReturnType::Type Res = FMessageDialog::Open(EAppMsgType::YesNo, FText::Format(
			NSLOCTEXT("YOLOInventory","DashboardMissingTransformer","Data Source '{0}' has no TransformerClass. Create a transformer Blueprint next to it?"),
			FText::FromString(Source->GetName())));
		if (Res == EAppReturnType::Yes)
		{
			const FString SourcePath = Source->GetPathName();
			const FString FolderPath = FPackageName::GetLongPackagePath(SourcePath);
			const FString BaseName = Source->GetName() + TEXT("_Transformer");

			UBlueprintFactory* Factory = NewObject<UBlueprintFactory>();
			Factory->ParentClass = UCSVDataTransformer::StaticClass();

			IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
			FString Pkg, Name;
			Tools.CreateUniqueAssetName(FolderPath / BaseName, TEXT(""), Pkg, Name);
			UObject* NewAsset = Tools.CreateAsset(Name, FolderPath, UBlueprint::StaticClass(), Factory);
			if (UBlueprint* BP = Cast<UBlueprint>(NewAsset))
			{
				BP->MarkPackageDirty();
				Source->Modify();
				Source->TransformerClass = BP->GeneratedClass;
			}
		}

		if (!Source->TransformerClass)
		{
			FYIEditorMessageLog::Add(EYIEditorLogSeverity::Warning,
				NSLOCTEXT("YOLOInventory","Dash_CreateFail_NoTransformer","Create failed: transformer missing (and inline mapping disabled)."),
				FText::FromString(Source->GetPathName()));
			return false;
		}
	}

	UDataTable* Table = Source->DataTable.LoadSynchronous();
	if (!Table || !Table->RowStruct)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error,
			NSLOCTEXT("YOLOInventory","Dash_CreateFail_NoTable","Create failed: data table missing or has no row struct."),
			FText::FromString(Source->GetPathName()));
		return false;
	}

	const uint8* const* Found = Table->GetRowMap().Find(Entry.RowName);
	const uint8* RowPtr = Found ? *Found : nullptr;
	if (!RowPtr)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error,
			NSLOCTEXT("YOLOInventory","Dash_CreateFail_NoRow","Create failed: row not found in data table."),
			FText::FromString(Entry.RowName.ToString()));
		return false;
	}

	const FName AssetNameField = Source->AssetNameFieldName.IsNone() ? TEXT("AssetName") : Source->AssetNameFieldName;
	const FName PackagePathField = Source->PackagePathFieldName.IsNone() ? TEXT("PackagePath") : Source->PackagePathFieldName;

	FString AssetName = GetRowString(Table->RowStruct, RowPtr, AssetNameField);
	FString PackagePath = GetRowString(Table->RowStruct, RowPtr, PackagePathField);

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

	UObject* Transformed = nullptr;
	if (TSubclassOf<UCSVDataTransformer> Effective = Source->GetEffectiveTransformerClass())
	{
		if (UCSVDataTransformer* Transformer = NewObject<UCSVDataTransformer>(Source, Effective))
		{
			Transformed = Transformer->TransformObject(RowWrapper);
		}
	}
	else if (GEngine)
	{
		if (UYIItemRegistrySubsystem* Registry = GEngine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
		{
			Transformed = Registry->GetByCode(Entry.Code);
		}
	}
	UYIItemDefinition* Def = Cast<UYIItemDefinition>(Transformed);
	if (!Def)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error,
			NSLOCTEXT("YOLOInventory","Dash_CreateFail_Transform","Create failed: transformer/inline mapping returned null."),
			FText::FromString(Entry.RowName.ToString()));
		return false;
	}

	const FString AssetLongPath = PackagePath / AssetName;
	const FString SanitizedPackage = UPackageTools::SanitizePackageName(AssetLongPath);
	const FString ObjectPath = SanitizedPackage + TEXT(".") + AssetName;

	if (UYIItemDefinition* Existing = Cast<UYIItemDefinition>(StaticLoadObject(UYIItemDefinition::StaticClass(), nullptr, *ObjectPath)))
	{
		// Update existing asset from table row
		if (UCSVBPFunctionLibrary::AutoAssignProperties(RowWrapper, Existing, {}))
		{
			Existing->MarkPackageDirty();
			return true;
		}
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Warning,
			NSLOCTEXT("YOLOInventory","Dash_CreateFail_Update","Create failed: unable to update existing asset from row."),
			FText::FromString(ObjectPath));
		return false;
	}

	UPackage* Pkg = CreatePackage(*SanitizedPackage);
	UObject* NewAsset = NewObject<UObject>(Pkg, Def->GetClass(), *AssetName, RF_Public | RF_Standalone | RF_Transactional, Def);
	if (!NewAsset)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error,
			NSLOCTEXT("YOLOInventory","Dash_CreateFail_NewAsset","Create failed: could not create asset object."),
			FText::FromString(ObjectPath));
		return false;
	}

	FAssetRegistryModule::AssetCreated(NewAsset);
	Pkg->MarkPackageDirty();
	return true;
}

FString SYIItemDashboard::GetRowString(const UScriptStruct* Struct, const uint8* RowData, FName Field) const
{
	return YIEditor_GetRowStringFromStruct(Struct, RowData, Field);
}

TSharedPtr<SWidget> SYIItemDashboard::BuildContextMenuForEntry(const TSharedPtr<FYIItemDashboardEntry>& Entry) const
{
	if (!Entry.IsValid())
	{
		return nullptr;
	}

	SYIItemDashboard* Self = const_cast<SYIItemDashboard*>(this);
	FMenuBuilder MenuBuilder(true, nullptr);

	UYIItemDefinition* ItemDef = nullptr;
	if (Entry->Object.ToSoftObjectPath().IsValid())
	{
		ItemDef = Cast<UYIItemDefinition>(Entry->Object.LoadSynchronous());
	}
	if (!ItemDef && GEngine)
	{
		if (UYIItemRegistrySubsystem* Registry = GEngine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
		{
			ItemDef = Registry->GetByCode(Entry->Code);
		}
	}

	if (IsValid(ItemDef))
	{
		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("YOLOInventory", "Dash_Context_OpenItemAsset", "Focus Item Asset"),
			NSLOCTEXT("YOLOInventory", "Dash_Context_OpenItemAsset_Tip", "Focus this item asset inside the dashboard."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([Self, ItemDef]()
			{
				if (Self && IsValid(ItemDef))
				{
					Self->OpenAsset(ItemDef);
				}
			})));

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("YOLOInventory", "Dash_Context_ShowInBrowser", "Show in Content Browser"),
			NSLOCTEXT("YOLOInventory", "Dash_Context_ShowInBrowser_Tip", "Highlight this item asset in the Content Browser."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([ItemDef]()
			{
				if (GEditor && IsValid(ItemDef))
				{
					TArray<UObject*> Objects;
					Objects.Add(ItemDef);
					GEditor->SyncBrowserToObjects(Objects);
				}
			})));

		MenuBuilder.AddSeparator();
	}

	if (!Entry->bIsDataTable)
	{
	}
	else
	{
		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("YOLOInventory", "Dash_Context_CreateAsset", "Create Item Asset from Row"),
			NSLOCTEXT("YOLOInventory", "Dash_Context_CreateAsset_Tip", "Run the transformer on this row and create/update the item asset."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([Self, Entry]()
			{
				if (Self->CreateAssetFromEntry(*Entry))
				{
					Self->Refresh();
				}
			})));

		if (Entry->DataSource.IsValid())
		{
			MenuBuilder.AddMenuEntry(
				NSLOCTEXT("YOLOInventory", "Dash_Context_OpenSourceAsset", "Open Data Source Asset"),
				NSLOCTEXT("YOLOInventory", "Dash_Context_OpenSourceAsset_Tip", "Open the UYIDataTableItemSource that owns this row."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([Self, Entry]()
				{
					Self->OpenDataSource(Entry);
				})));
		}

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("YOLOInventory", "Dash_Context_OpenDataTable", "Focus Data Table"),
			NSLOCTEXT("YOLOInventory", "Dash_Context_OpenDataTable_Tip", "Focus the data table for this row inside the dashboard."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([Self, Entry]()
			{
				if (Self && Entry.IsValid())
				{
					if (UDataTable* Table = Entry->DataTable.LoadSynchronous())
					{
						Self->OpenAsset(Table);
					}
				}
			})));
	}

	return MenuBuilder.MakeWidget();
}

void SYIItemDashboard::ShowDetailsForEntry(const TSharedPtr<FYIItemDashboardEntry>& Entry)
{
	if (!DetailsView.IsValid())
	{
		return;
	}

	UObject* Target = Entry.IsValid() ? ResolveDetailObject(*Entry) : nullptr;
	if (Target && LastDetailObject.Get() == Target)
	{
		return;
	}

	LastDetailObject = Target;
	DetailKeepAlive.Reset();
	if (Target)
	{
		DetailKeepAlive = TStrongObjectPtr<UObject>(Target);
	}
	TArray<UObject*> Objects;
	if (Target)
	{
		Objects.Add(Target);
	}
	DetailsView->SetObjects(Objects);

	CurrentMappingSource.Reset();
	if (Entry.IsValid())
	{
		if (UYIDataTableItemSource* Source = Cast<UYIDataTableItemSource>(Target))
		{
			CurrentMappingSource = Source;
			RefreshInlineMappingEditor(Source);
		}
		else if (Entry->bIsDataTable)
		{
			if (Entry->DataSource.IsValid())
			{
				CurrentMappingSource = Entry->DataSource.LoadSynchronous();
				RefreshInlineMappingEditor(CurrentMappingSource.Get());
			}
		}
	}
	else
	{
		MappingRows.Reset();
		if (MappingListView.IsValid())
		{
			MappingListView->RequestListRefresh();
		}
	}
}

void SYIItemDashboard::OpenAsset(UObject* Asset)
{
	if (!Asset)
	{
		return;
	}

	if (DetailsView.IsValid())
	{
		DetailsView->SetObject(Asset);
	}

	if (Items.Num() == 0)
	{
		Refresh();
	}

	TSharedPtr<FYIItemDashboardEntry> Match;
	for (const TSharedPtr<FYIItemDashboardEntry>& Entry : Items)
	{
		if (!Entry.IsValid())
		{
			continue;
		}
		if (Entry->ItemAsset.IsValid() && Entry->ItemAsset.Get() == Asset)
		{
			Match = Entry;
			break;
		}
		if (Entry->Object.IsValid() && Entry->Object.Get() == Asset)
		{
			Match = Entry;
			break;
		}
		if (Entry->DataSource.IsValid() && Entry->DataSource.Get() == Asset)
		{
			Match = Entry;
			break;
		}
		if (Entry->DataTable.IsValid() && Entry->DataTable.Get() == Asset)
		{
			Match = Entry;
			break;
		}
	}

	if (Match.IsValid() && ListView.IsValid())
	{
		ListView->SetSelection(Match);
		ListView->RequestScrollIntoView(Match);
		ShowDetailsForEntry(Match);
	}
}

UObject* SYIItemDashboard::ResolveDetailObject(const FYIItemDashboardEntry& Entry) const
{
	// Prefer a real asset if it exists
	if (!Entry.bIsDataTable && Entry.Object.ToSoftObjectPath().IsValid())
	{
		if (UObject* Obj = Entry.Object.LoadSynchronous())
		{
			return Obj;
		}
	}

	// Try registry lookup (will transform rows if needed)
	if (GEngine)
	{
		if (UYIItemRegistrySubsystem* Registry = GEngine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
		{
			if (UYIItemDefinition* Def = Registry->GetByCode(Entry.Code))
			{
				return Def;
			}
		}
	}

	// Fallback to data source asset, then data table itself
	if (Entry.DataSource.IsValid())
	{
		if (UObject* Source = Entry.DataSource.LoadSynchronous())
		{
			return Source;
		}
	}

	if (Entry.DataTable.IsValid())
	{
		return Entry.DataTable.LoadSynchronous();
	}

	return nullptr;
}

TSharedPtr<SWidget> SYIItemDashboard::BuildListContextMenu()
{
	if (!ListView.IsValid())
	{
		return nullptr;
	}

	const TArray<TSharedPtr<FYIItemDashboardEntry>> Selected = ListView->GetSelectedItems();
	if (Selected.Num() == 0 || !Selected[0].IsValid())
	{
		return nullptr;
	}

	// Multi-select support: bulk create assets for rows
	bool bAnyRows = false;
	for (const TSharedPtr<FYIItemDashboardEntry>& E : Selected)
	{
		if (E.IsValid() && E->bIsDataTable)
		{
			bAnyRows = true;
			break;
		}
	}

	if (Selected.Num() > 1)
	{
		SYIItemDashboard* Self = this;
		FMenuBuilder MenuBuilder(true, nullptr);

		if (bAnyRows)
		{
			MenuBuilder.AddMenuEntry(
				NSLOCTEXT("YOLOInventory", "Dash_Context_BulkCreate", "Create Assets for Selected Rows"),
				NSLOCTEXT("YOLOInventory", "Dash_Context_BulkCreate_Tip", "Run transformer/inline mappings for all selected data table rows."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([Self, Selected]()
				{
					bool bChanged = false;
					for (const TSharedPtr<FYIItemDashboardEntry>& E : Selected)
					{
						if (E.IsValid() && E->bIsDataTable)
						{
							bChanged |= Self->CreateAssetFromEntry(*E);
						}
					}
					if (bChanged)
					{
						Self->Refresh();
					}
				})));
		}

		// Provide single-entry actions for the first selected item as a convenience
		if (Selected[0].IsValid())
		{
			MenuBuilder.AddSeparator();
			if (TSharedPtr<SWidget> Single = BuildContextMenuForEntry(Selected[0]))
			{
				MenuBuilder.AddWidget(Single.ToSharedRef(), FText::GetEmpty(), true);
			}
		}

		return MenuBuilder.MakeWidget();
	}

	return BuildContextMenuForEntry(Selected[0]);
}

FReply SYIItemDashboard::HandleListKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (!ListView.IsValid())
	{
		return FReply::Unhandled();
	}

	const TArray<TSharedPtr<FYIItemDashboardEntry>> Selected = ListView->GetSelectedItems();
	if (Selected.Num() == 0 || !Selected[0].IsValid())
	{
		return FReply::Unhandled();
	}

	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
	{
		// Confirm bulk create
		int32 RowsNeedingAsset = 0;
		for (const TSharedPtr<FYIItemDashboardEntry>& E : Selected)
		{
			if (E.IsValid() && E->bIsDataTable)
			{
				RowsNeedingAsset++;
			}
		}

		if (RowsNeedingAsset > 0)
		{
			const FText Msg = FText::Format(
				NSLOCTEXT("YOLOInventory","Dash_ConfirmCreate","Create/update item assets for {0} selected row(s)?"),
				FText::AsNumber(RowsNeedingAsset));
			if (FMessageDialog::Open(EAppMsgType::YesNo, Msg) == EAppReturnType::Yes)
			{
				bool bChanged = false;
				for (const TSharedPtr<FYIItemDashboardEntry>& E : Selected)
				{
					if (E.IsValid() && E->bIsDataTable)
					{
						bChanged |= CreateAssetFromEntry(*E);
					}
				}
				if (bChanged)
				{
					Refresh();
				}
			}
			return FReply::Handled();
		}

		OpenEntry(Selected[0]);
		return FReply::Handled();
	}

	if (Key == EKeys::Delete)
	{
		TArray<UObject*> ToDelete;
		for (const TSharedPtr<FYIItemDashboardEntry>& E : Selected)
		{
			if (!E.IsValid())
			{
				continue;
			}
			if (!E->bIsDataTable && E->Object.ToSoftObjectPath().IsValid())
			{
				if (UObject* Obj = E->Object.LoadSynchronous())
				{
					ToDelete.Add(Obj);
				}
			}
			else if (E->bIsDataTable && E->ItemAsset.IsValid())
			{
				if (UObject* Obj = E->ItemAsset.LoadSynchronous())
				{
					ToDelete.Add(Obj);
				}
			}
		}

		if (ToDelete.Num() == 0)
		{
			return FReply::Handled();
		}

		const FText Msg = FText::Format(
			NSLOCTEXT("YOLOInventory","Dash_ConfirmDelete","Delete {0} asset(s)? This cannot be undone."),
			FText::AsNumber(ToDelete.Num()));
		if (FMessageDialog::Open(EAppMsgType::YesNo, Msg) == EAppReturnType::Yes)
		{
			ObjectTools::DeleteObjects(ToDelete, /*bShowConfirmation=*/false);
			Refresh();
		}
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

void SYIItemDashboard::RefreshInlineMappingEditor(UYIDataTableItemSource* Source)
{
	MappingRows.Reset();
	SourceFieldOptions.Reset();
	TargetPropertyOptions.Reset();
	ConverterOptions.Reset();
	SourceFieldPropCache.Reset();
	TargetFieldPropCache.Reset();

	if (!Source)
	{
		if (MappingListView.IsValid())
		{
			MappingListView->RequestListRefresh();
		}
		return;
	}

	// Build options from row struct and item definition properties
	if (UDataTable* Table = Source->DataTable.LoadSynchronous())
	{
		if (UScriptStruct* RowStruct = Table->RowStruct)
		{
			for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
			{
				const FName FieldName((*It)->GetAuthoredName());
				SourceFieldOptions.Add(MakeShared<FString>(FieldName.ToString()));
				SourceFieldPropCache.Add(FieldName, *It);
			}
		}
	}
	for (TFieldIterator<FProperty> It(UYIItemDefinition::StaticClass()); It; ++It)
	{
		const FName FieldName((*It)->GetAuthoredName());
		TargetPropertyOptions.Add(MakeShared<FString>(FieldName.ToString()));
		TargetFieldPropCache.Add(FieldName, *It);
	}

	for (const FYIFieldMapping& M : Source->InlineMappings)
	{
		MappingRows.Add(MakeShared<FYIFieldMapping>(M));
	}

	if (MappingListView.IsValid())
	{
		MappingListView->RequestListRefresh();
	}
}

void SYIItemDashboard::AutoMatchInlineMappings(bool bAddAllFields)
{
	if (!CurrentMappingSource.IsValid())
	{
		return;
	}

	UYIDataTableItemSource* Source = CurrentMappingSource.Get();
	if (!Source)
	{
		return;
	}

	UDataTable* Table = Source->DataTable.LoadSynchronous();
	if (!Table || !Table->RowStruct)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error,
			NSLOCTEXT("YOLOInventory","Dash_AutoMatch_NoTable","Auto-match failed: data table missing or no row struct."),
			FText::FromString(Source->GetPathName()));
		return;
	}

	auto NormalizeField = [](const FString& In)->FString
	{
		FString Out;
		Out.Reserve(In.Len());
		for (TCHAR C : In)
		{
			if (FChar::IsAlnum(C))
			{
				Out.AppendChar(FChar::ToLower(C));
			}
		}
		return Out;
	};

	TMap<FString, FName> SourceByNorm;
	TMap<FString, FName> SourceByLower;
	for (TFieldIterator<FProperty> It(Table->RowStruct); It; ++It)
	{
		const FName FieldName((*It)->GetAuthoredName());
		const FString NameStr = FieldName.ToString();
		SourceByNorm.Add(NormalizeField(NameStr), FieldName);
		SourceByLower.Add(NameStr.ToLower(), FieldName);
	}

	TSet<FName> ExistingTargets;
	for (const FYIFieldMapping& M : Source->InlineMappings)
	{
		if (!M.TargetProperty.IsNone())
		{
			ExistingTargets.Add(M.TargetProperty);
		}
	}

	bool bChanged = false;

	auto GuessConversion = [&](const FProperty* SourceProp, const FProperty* TargetProp)->EYIFieldMappingConversion
	{
		if (!SourceProp || !TargetProp)
		{
			return EYIFieldMappingConversion::None;
		}
		if (CastField<FBoolProperty>(TargetProp))
		{
			if (CastField<FNumericProperty>(SourceProp))
			{
				return EYIFieldMappingConversion::BoolFromInt;
			}
			if (CastField<FStrProperty>(SourceProp) || CastField<FTextProperty>(SourceProp) || CastField<FNameProperty>(SourceProp))
			{
				return EYIFieldMappingConversion::BoolFromText;
			}
		}
		if (CastField<FNameProperty>(TargetProp)) return EYIFieldMappingConversion::ToName;
		if (CastField<FTextProperty>(TargetProp)) return EYIFieldMappingConversion::ToText;
		if (const FNumericProperty* NumTarget = CastField<FNumericProperty>(TargetProp))
		{
			if (NumTarget->IsInteger()) return EYIFieldMappingConversion::ToInt;
			return EYIFieldMappingConversion::ToFloat;
		}
		return EYIFieldMappingConversion::None;
	};

	// First, patch existing mappings that have missing source fields
	for (FYIFieldMapping& M : Source->InlineMappings)
	{
		if (M.TargetProperty.IsNone() || !M.SourceField.IsNone())
		{
			continue;
		}
		const FString TargetLower = M.TargetProperty.ToString().ToLower();
		const FString TargetNorm = NormalizeField(M.TargetProperty.ToString());
		FName Match = NAME_None;
		if (const FName* Found = SourceByLower.Find(TargetLower)) Match = *Found;
		else if (const FName* FoundNorm = SourceByNorm.Find(TargetNorm)) Match = *FoundNorm;
		if (!Match.IsNone())
		{
			M.SourceField = Match;
			const FProperty* SourceProp = SourceFieldPropCache.FindRef(Match);
			const FProperty* TargetProp = TargetFieldPropCache.FindRef(M.TargetProperty);
			M.Conversion = GuessConversion(SourceProp, TargetProp);
			bChanged = true;
		}
	}

	for (TFieldIterator<FProperty> It(UYIItemDefinition::StaticClass()); It; ++It)
	{
		const FName TargetName((*It)->GetAuthoredName());
		if (!bAddAllFields && ExistingTargets.Contains(TargetName))
		{
			continue;
		}

		const FString TargetLower = TargetName.ToString().ToLower();
		const FString TargetNorm = NormalizeField(TargetName.ToString());
		FName Match = NAME_None;
		if (const FName* Found = SourceByLower.Find(TargetLower)) Match = *Found;
		else if (const FName* FoundNorm = SourceByNorm.Find(TargetNorm)) Match = *FoundNorm;

		if (!bAddAllFields && Match.IsNone())
		{
			continue;
		}

		FYIFieldMapping NewMap;
		NewMap.TargetProperty = TargetName;
		NewMap.SourceField = Match;
		NewMap.Conversion = GuessConversion(SourceFieldPropCache.FindRef(Match), *It);
		Source->InlineMappings.Add(NewMap);
		bChanged = true;
	}

	if (bChanged)
	{
		Source->Modify();
		RefreshInlineMappingEditor(Source);
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Info,
			NSLOCTEXT("YOLOInventory","Dash_AutoMatch_Done","Auto-match updated inline mappings."),
			FText::FromString(Source->GetPathName()));
	}
}

TSharedRef<ITableRow> SYIItemDashboard::MakeMappingRow(TSharedPtr<FYIFieldMapping> Mapping, const TSharedRef<STableViewBase>& OwnerTable)
{
	auto DropdownText = [](const TSharedPtr<FString>& Str)->FText
	{
		return Str.IsValid() ? FText::FromString(*Str) : FText::GetEmpty();
	};

	auto GetTypeInfo = [](const FProperty* Prop, FString& OutLabel, FLinearColor& OutColor)
	{
		OutLabel = TEXT("?");
		OutColor = FLinearColor(0.5f, 0.5f, 0.5f);
		if (!Prop) return;
		if (const FNumericProperty* Num = CastField<FNumericProperty>(Prop))
		{
			OutLabel = Num->IsInteger() ? TEXT("Int") : TEXT("Float");
			OutColor = Num->IsInteger() ? FLinearColor(0.3f, 0.7f, 1.f) : FLinearColor(0.3f, 1.f, 0.6f);
			return;
		}
		if (CastField<FBoolProperty>(Prop)) { OutLabel = TEXT("Bool"); OutColor = FLinearColor(1.f, 0.85f, 0.3f); return; }
		if (CastField<FNameProperty>(Prop)) { OutLabel = TEXT("Name"); OutColor = FLinearColor(0.4f, 0.7f, 1.f); return; }
		if (CastField<FStrProperty>(Prop)) { OutLabel = TEXT("String"); OutColor = FLinearColor(0.7f, 0.5f, 1.f); return; }
		if (CastField<FTextProperty>(Prop)) { OutLabel = TEXT("Text"); OutColor = FLinearColor(0.3f, 0.9f, 0.9f); return; }
		if (CastField<FEnumProperty>(Prop)) { OutLabel = TEXT("Enum"); OutColor = FLinearColor(1.f, 0.6f, 0.2f); return; }
		if (CastField<FStructProperty>(Prop)) { OutLabel = TEXT("Struct"); OutColor = FLinearColor(0.6f, 0.6f, 0.6f); return; }
		if (CastField<FObjectPropertyBase>(Prop)) { OutLabel = TEXT("Object"); OutColor = FLinearColor(0.8f, 0.6f, 0.3f); return; }
		if (CastField<FSoftObjectProperty>(Prop)) { OutLabel = TEXT("SoftObj"); OutColor = FLinearColor(0.8f, 0.6f, 0.3f); return; }
		if (CastField<FArrayProperty>(Prop)) { OutLabel = TEXT("Array"); OutColor = FLinearColor(0.6f, 0.8f, 0.4f); return; }
		if (CastField<FMapProperty>(Prop)) { OutLabel = TEXT("Map"); OutColor = FLinearColor(0.6f, 0.8f, 0.4f); return; }
		if (CastField<FSetProperty>(Prop)) { OutLabel = TEXT("Set"); OutColor = FLinearColor(0.6f, 0.8f, 0.4f); return; }
	};

	auto MakeTypeBadge = [&](const FProperty* Prop)->TSharedRef<SWidget>
	{
		FString Label;
		FLinearColor Color;
		GetTypeInfo(Prop, Label, Color);
		return SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(FMargin(4,1))
			.BorderBackgroundColor(Color)
			[
				SNew(STextBlock).Text(FText::FromString(Label)).ColorAndOpacity(FSlateColor(FLinearColor::Black))
			];
	};

	auto GetSourceProp = [this, Mapping]() -> FProperty*
	{
		if (!Mapping.IsValid() || Mapping->SourceField.IsNone()) return nullptr;
		if (FProperty** Found = SourceFieldPropCache.Find(Mapping->SourceField)) return *Found;
		return nullptr;
	};

	auto GetTargetProp = [this, Mapping]() -> FProperty*
	{
		if (!Mapping.IsValid() || Mapping->TargetProperty.IsNone()) return nullptr;
		if (FProperty** Found = TargetFieldPropCache.Find(Mapping->TargetProperty)) return *Found;
		return nullptr;
	};

	auto BuildStatusWidget = [this, Mapping, GetSourceProp, GetTargetProp]() -> TSharedRef<SWidget>
	{
		if (!Mapping.IsValid())
		{
			return SNew(SSpacer);
		}
		const FProperty* SourceProp = GetSourceProp();
		const FProperty* TargetProp = GetTargetProp();
		if (!SourceProp || !TargetProp)
		{
			return SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.Error"))
				.ToolTipText(NSLOCTEXT("YOLOInventory","Dash_MappingMissing","Missing source or target field."));
		}
		const bool bTypesMatch = SourceProp->GetClass() == TargetProp->GetClass();
		const bool bBothNumeric = CastField<FNumericProperty>(SourceProp) && CastField<FNumericProperty>(TargetProp);
		const bool bBothTextish =
			(CastField<FStrProperty>(SourceProp) || CastField<FNameProperty>(SourceProp) || CastField<FTextProperty>(SourceProp)) &&
			(CastField<FStrProperty>(TargetProp) || CastField<FNameProperty>(TargetProp) || CastField<FTextProperty>(TargetProp));
		const bool bCompatible = bTypesMatch || bBothNumeric || bBothTextish;
		if (!bCompatible && Mapping->Conversion == EYIFieldMappingConversion::None)
		{
			return SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
				.ToolTipText(NSLOCTEXT("YOLOInventory","Dash_MappingTypeMismatch","Type mismatch. Add a conversion."));
		}
		return SNew(SImage)
			.Image(FAppStyle::Get().GetBrush("Icons.Check"))
			.ToolTipText(NSLOCTEXT("YOLOInventory","Dash_MappingOk","Mapping looks OK."));
	};

	return SNew(STableRow<TSharedPtr<FYIFieldMapping>>, OwnerTable)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(2).VAlign(VAlign_Center)
		[
			BuildStatusWidget()
		]
		+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(2)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&const_cast<SYIItemDashboard*>(this)->SourceFieldOptions)
			.OnGenerateWidget_Lambda([this, DropdownText, GetTypeInfo](TSharedPtr<FString> InItem)
			{
				const FName FieldName = InItem.IsValid() ? FName(**InItem) : NAME_None;
				FString Label;
				FLinearColor Color;
				FProperty* Prop = nullptr;
				if (FieldName != NAME_None)
				{
					if (FProperty** Found = const_cast<SYIItemDashboard*>(this)->SourceFieldPropCache.Find(FieldName))
					{
						Prop = *Found;
					}
				}
				GetTypeInfo(Prop, Label, Color);
				return SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(2,0)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						.Padding(FMargin(3,1))
						.BorderBackgroundColor(Color)
						[
							SNew(STextBlock).Text(FText::FromString(Label)).ColorAndOpacity(FSlateColor(FLinearColor::Black))
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4,0)
					[
						SNew(STextBlock).Text(DropdownText(InItem))
					];
			})
			.OnSelectionChanged_Lambda([this, Mapping](TSharedPtr<FString> NewItem, ESelectInfo::Type)
			{
				if (CurrentMappingSource.IsValid() && Mapping.IsValid() && NewItem.IsValid())
				{
					CurrentMappingSource->Modify();
					Mapping->SourceField = FName(**NewItem);
					const int32 Index = MappingRows.Find(Mapping);
					if (Index != INDEX_NONE)
					{
						CurrentMappingSource->InlineMappings[Index].SourceField = Mapping->SourceField;
					}
				}
			})
			.InitiallySelectedItem([this, Mapping]()
			{
				if (!Mapping.IsValid()) return TSharedPtr<FString>();
				if (const TSharedPtr<FString>* FoundPtr = SourceFieldOptions.FindByPredicate([Mapping](const TSharedPtr<FString>& Opt)
				{
					return Opt.IsValid() && FName(**Opt).IsEqual(Mapping->SourceField);
				}))
				{
					return *FoundPtr;
				}
				return TSharedPtr<FString>();
			}())
			.Content()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2,0)
				[
					MakeTypeBadge(GetSourceProp())
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4,0)
				[
					SNew(STextBlock).Text_Lambda([DropdownText, Mapping, this]()
					{
						const TSharedPtr<FString>* Found = SourceFieldOptions.FindByPredicate([Mapping](const TSharedPtr<FString>& Opt)
						{
							return Mapping.IsValid() && Opt.IsValid() && FName(**Opt).IsEqual(Mapping->SourceField);
						});
						return Found ? DropdownText(*Found) : FText::FromString(Mapping.IsValid() ? Mapping->SourceField.ToString() : TEXT(""));
					})
				]
			]
		]
		+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(2)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&const_cast<SYIItemDashboard*>(this)->TargetPropertyOptions)
			.OnGenerateWidget_Lambda([this, DropdownText, GetTypeInfo](TSharedPtr<FString> InItem)
			{
				const FName FieldName = InItem.IsValid() ? FName(**InItem) : NAME_None;
				FString Label;
				FLinearColor Color;
				FProperty* Prop = nullptr;
				if (FieldName != NAME_None)
				{
					if (FProperty** Found = const_cast<SYIItemDashboard*>(this)->TargetFieldPropCache.Find(FieldName))
					{
						Prop = *Found;
					}
				}
				GetTypeInfo(Prop, Label, Color);
				return SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(2,0)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
						.Padding(FMargin(3,1))
						.BorderBackgroundColor(Color)
						[
							SNew(STextBlock).Text(FText::FromString(Label)).ColorAndOpacity(FSlateColor(FLinearColor::Black))
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4,0)
					[
						SNew(STextBlock).Text(DropdownText(InItem))
					];
			})
			.OnSelectionChanged_Lambda([this, Mapping](TSharedPtr<FString> NewItem, ESelectInfo::Type)
			{
				if (CurrentMappingSource.IsValid() && Mapping.IsValid() && NewItem.IsValid())
				{
					CurrentMappingSource->Modify();
					Mapping->TargetProperty = FName(**NewItem);
					const int32 Index = MappingRows.Find(Mapping);
					if (Index != INDEX_NONE)
					{
						CurrentMappingSource->InlineMappings[Index].TargetProperty = Mapping->TargetProperty;
					}
				}
			})
			.InitiallySelectedItem([this, Mapping]()
			{
				if (!Mapping.IsValid()) return TSharedPtr<FString>();
				if (const TSharedPtr<FString>* FoundPtr = TargetPropertyOptions.FindByPredicate([Mapping](const TSharedPtr<FString>& Opt)
				{
					return Opt.IsValid() && FName(**Opt).IsEqual(Mapping->TargetProperty);
				}))
				{
					return *FoundPtr;
				}
				return TSharedPtr<FString>();
			}())
			.Content()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2,0)
				[
					MakeTypeBadge(GetTargetProp())
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4,0)
				[
					SNew(STextBlock).Text_Lambda([DropdownText, Mapping, this]()
					{
						const TSharedPtr<FString>* Found = TargetPropertyOptions.FindByPredicate([Mapping](const TSharedPtr<FString>& Opt)
						{
							return Mapping.IsValid() && Opt.IsValid() && FName(**Opt).IsEqual(Mapping->TargetProperty);
						});
						return Found ? DropdownText(*Found) : FText::FromString(Mapping.IsValid() ? Mapping->TargetProperty.ToString() : TEXT(""));
					})
				]
			]
		]
	+ SHorizontalBox::Slot().FillWidth(0.2f).Padding(2)
	[
		SNew(SComboBox<TSharedPtr<FString>>)
		.OptionsSource(&const_cast<SYIItemDashboard*>(this)->ConverterOptions)
		.OnGenerateWidget_Lambda([DropdownText](TSharedPtr<FString> InItem)
		{
			return SNew(STextBlock).Text(DropdownText(InItem));
		})
		.OnSelectionChanged_Lambda([this, Mapping](TSharedPtr<FString> NewItem, ESelectInfo::Type)
		{
			if (CurrentMappingSource.IsValid() && Mapping.IsValid() && NewItem.IsValid())
			{
				CurrentMappingSource->Modify();
				EYIFieldMappingConversion NewConv = EYIFieldMappingConversion::None;
				if (*NewItem == TEXT("To Name")) NewConv = EYIFieldMappingConversion::ToName;
				else if (*NewItem == TEXT("To Text")) NewConv = EYIFieldMappingConversion::ToText;
				else if (*NewItem == TEXT("To Int")) NewConv = EYIFieldMappingConversion::ToInt;
				else if (*NewItem == TEXT("To Float")) NewConv = EYIFieldMappingConversion::ToFloat;
				else if (*NewItem == TEXT("Bool from Int>0")) NewConv = EYIFieldMappingConversion::BoolFromInt;
				else if (*NewItem == TEXT("Bool from Text (non-empty)")) NewConv = EYIFieldMappingConversion::BoolFromText;
				Mapping->Conversion = NewConv;
				const int32 Index = MappingRows.Find(Mapping);
				if (Index != INDEX_NONE)
				{
					CurrentMappingSource->InlineMappings[Index].Conversion = Mapping->Conversion;
				}
			}
		})
		.OnComboBoxOpening_Lambda([this]()
		{
			ConverterOptions.Reset();
			ConverterOptions.Add(MakeShared<FString>(TEXT("None")));
			ConverterOptions.Add(MakeShared<FString>(TEXT("To Name")));
			ConverterOptions.Add(MakeShared<FString>(TEXT("To Text")));
			ConverterOptions.Add(MakeShared<FString>(TEXT("To Int")));
			ConverterOptions.Add(MakeShared<FString>(TEXT("To Float")));
			ConverterOptions.Add(MakeShared<FString>(TEXT("Bool from Int>0")));
			ConverterOptions.Add(MakeShared<FString>(TEXT("Bool from Text (non-empty)")));
		})
		.InitiallySelectedItem([this, Mapping]()
		{
			if (!Mapping.IsValid()) return TSharedPtr<FString>();
			const TCHAR* Label = TEXT("None");
			switch (Mapping->Conversion)
			{
			case EYIFieldMappingConversion::ToName: Label = TEXT("To Name"); break;
			case EYIFieldMappingConversion::ToText: Label = TEXT("To Text"); break;
			case EYIFieldMappingConversion::ToInt: Label = TEXT("To Int"); break;
			case EYIFieldMappingConversion::ToFloat: Label = TEXT("To Float"); break;
			case EYIFieldMappingConversion::BoolFromInt: Label = TEXT("Bool from Int>0"); break;
			case EYIFieldMappingConversion::BoolFromText: Label = TEXT("Bool from Text (non-empty)"); break;
			default: break;
			}
			for (const TSharedPtr<FString>& Opt : ConverterOptions)
			{
				if (Opt.IsValid() && *Opt == Label)
				{
					return Opt;
				}
			}
			return TSharedPtr<FString>();
		}())
		.Content()
		[
			SNew(STextBlock).Text_Lambda([Mapping]()
			{
				const TCHAR* Label = TEXT("None");
				if (Mapping.IsValid())
				{
					switch (Mapping->Conversion)
					{
					case EYIFieldMappingConversion::ToName: Label = TEXT("To Name"); break;
					case EYIFieldMappingConversion::ToText: Label = TEXT("To Text"); break;
					case EYIFieldMappingConversion::ToInt: Label = TEXT("To Int"); break;
					case EYIFieldMappingConversion::ToFloat: Label = TEXT("To Float"); break;
					case EYIFieldMappingConversion::BoolFromInt: Label = TEXT("Bool from Int>0"); break;
					case EYIFieldMappingConversion::BoolFromText: Label = TEXT("Bool from Text (non-empty)"); break;
					default: break;
					}
				}
				return FText::FromString(Label);
			})
		]
	]
	+ SHorizontalBox::Slot().AutoWidth().Padding(2)
	[
		SNew(SButton)
		.Text(NSLOCTEXT("YOLOInventory","Dash_RemoveMapping","X"))
		.OnClicked_Lambda([this, Mapping]()
			{
				if (CurrentMappingSource.IsValid() && Mapping.IsValid())
				{
					const int32 Index = MappingRows.Find(Mapping);
					if (Index != INDEX_NONE)
					{
						CurrentMappingSource->Modify();
						MappingRows.RemoveAt(Index, 1, EAllowShrinking::No);
						CurrentMappingSource->InlineMappings.RemoveAt(Index, 1, EAllowShrinking::No);
						if (MappingListView.IsValid())
						{
							MappingListView->RequestListRefresh();
						}
					}
				}
				return FReply::Handled();
			})
		]
	];
}

FText SYIItemDashboard::BuildPreviewText(const TSharedPtr<FYIItemDashboardEntry>& Entry) const
{
	if (!Entry.IsValid())
	{
		return FText::FromString(TEXT("No entry selected."));
	}

	FString Status;
	if (Entry->bIsDataTable)
	{
		Status = Entry->bHasAsset ? TEXT("Data Row (generated asset exists)") : TEXT("Data Row (needs asset)");
	}
	else
	{
		Status = TEXT("Item Asset");
	}

	FString Summary = FString::Printf(TEXT("%s\nType: %s\nCode: %lld\nTemplate: %s\nSource: %s"),
		*Entry->Name,
		*Status,
		(long long)Entry->Code,
		*Entry->TemplateId,
		*Entry->Source);

	if (GEngine)
	{
		if (UYIItemRegistrySubsystem* Registry = GEngine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
		{
			if (UYIItemDefinition* Def = Registry->GetByCode(Entry->Code))
			{
				if (!IsValid(Def))
				{
					return FText();
				}
				if (!Def->Description.IsEmpty())
				{
					Summary += FString::Printf(TEXT("\n\n%s"), *Def->Description.ToString());
				}
				return FText::FromString(Summary);
			}
		}
	}

	// Fallback preview for data table rows when no definition is available
	if (Entry->bIsDataTable && Entry->DataSource.IsValid())
	{
		if (UYIDataTableItemSource* Source = Entry->DataSource.LoadSynchronous())
		{
			if (UDataTable* Table = Source->DataTable.LoadSynchronous())
			{
				if (const uint8* const* Found = Table->GetRowMap().Find(Entry->RowName))
				{
					const FName NameField = Source->PreviewNameFieldName.IsNone() ? TEXT("DisplayName") : Source->PreviewNameFieldName;
					const FName DescField = Source->PreviewDescriptionFieldName.IsNone() ? TEXT("Description") : Source->PreviewDescriptionFieldName;

					const FString Name = GetRowString(Table->RowStruct, *Found, NameField);
					const FString Desc = GetRowString(Table->RowStruct, *Found, DescField);

					if (!Name.IsEmpty() || !Desc.IsEmpty())
					{
						if (!Name.IsEmpty())
						{
							Summary += FString::Printf(TEXT("\nPreview Name: %s"), *Name);
						}
						if (!Desc.IsEmpty())
						{
							Summary += FString::Printf(TEXT("\nPreview Desc: %s"), *Desc);
						}
						return FText::FromString(Summary);
					}
				}
			}
		}
	}

	return FText::FromString(Summary);
}
