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
#include "Subsystems/AssetEditorSubsystem.h"
#include "DataTableEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBox.h"
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

static FString GetRowStringFromStruct(const UScriptStruct* Struct, const uint8* RowData, FName Field)
{
	if (!Struct || !RowData || Field.IsNone())
	{
		return FString();
	}

	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		const FProperty* Prop = *It;
		if (!Prop)
		{
			continue;
		}
		const FString Authored = Prop->GetAuthoredName();
		if (!Authored.Equals(Field.ToString(), ESearchCase::IgnoreCase))
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

void SYIItemDashboard::Construct(const FArguments& InArgs)
{
	FPropertyEditorModule& PropModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailArgs;
	DetailArgs.bAllowSearch = true;
	DetailArgs.bHideSelectionTip = true;
	DetailArgs.bAllowMultipleTopLevelObjects = false;
	DetailArgs.bShowOptions = true;
	DetailsView = PropModule.CreateDetailView(DetailArgs);

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
					+ SHorizontalBox::Slot().FillWidth(1.0f)
					[
						SNew(SSearchBox)
						.OnTextChanged(this, &SYIItemDashboard::OnSearchTextChanged)
					]
				]
				+ SVerticalBox::Slot().FillHeight(1.f).Padding(8)
				[
					SAssignNew(ListView, SListView<TSharedPtr<FYIItemDashboardEntry>>)
					.ListItemsSource(&FilteredItems)
					.SelectionMode(ESelectionMode::Single)
					.OnContextMenuOpening(FOnContextMenuOpening::CreateSP(this, &SYIItemDashboard::BuildListContextMenu))
					.OnSelectionChanged_Lambda([this](TSharedPtr<FYIItemDashboardEntry> Entry, ESelectInfo::Type)
					{
						ShowDetailsForEntry(Entry);
					})
					.OnGenerateRow(this, &SYIItemDashboard::MakeRowWidget)
					.OnMouseButtonDoubleClick_Lambda([this](TSharedPtr<FYIItemDashboardEntry> Entry)
					{
						ShowDetailsForEntry(Entry);
					})
					.HeaderRow
					(
						SNew(SHeaderRow)
						+ SHeaderRow::Column("Code").DefaultLabel(NSLOCTEXT("YOLOInventory","Dash_Code","Code")).FillWidth(0.15f)
						+ SHeaderRow::Column("Name").DefaultLabel(NSLOCTEXT("YOLOInventory","Dash_Name","Name")).FillWidth(0.25f)
						+ SHeaderRow::Column("Template").DefaultLabel(NSLOCTEXT("YOLOInventory","Dash_Template","TemplateId")).FillWidth(0.2f)
						+ SHeaderRow::Column("Type").DefaultLabel(NSLOCTEXT("YOLOInventory","Dash_Type","Type")).FillWidth(0.1f)
						+ SHeaderRow::Column("Source").DefaultLabel(NSLOCTEXT("YOLOInventory","Dash_Source","Source")).FillWidth(0.45f)
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
		]
	];

	Refresh();
}

TSharedRef<ITableRow> SYIItemDashboard::MakeRowWidget(TSharedPtr<FYIItemDashboardEntry> Entry, const TSharedRef<STableViewBase>& Owner) const
{
	return SNew(STableRow<TSharedPtr<FYIItemDashboardEntry>>, Owner)
		.ToolTipText(BuildPreviewText(Entry))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.15f)
		[
			SNew(STextBlock)
			.Text(FText::AsNumber(Entry->Code))
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
		]
		+ SHorizontalBox::Slot().FillWidth(0.3f)
		[
			SNew(STextBlock).Text(FText::FromString(Entry->Source))
		]
	];
}

void SYIItemDashboard::Refresh()
{
	Items.Reset();
	FilteredItems.Reset();

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
							const FString RowDisplay = GetRowStringFromStruct(Table->RowStruct, *Found, PreviewField);
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

	const FString Filter = SearchText.ToString();
	for (const TSharedPtr<FYIItemDashboardEntry>& Entry : Items)
	{
		const bool bPass = Filter.IsEmpty() ||
			Entry->Name.Contains(Filter) ||
			Entry->TemplateId.Contains(Filter) ||
			Entry->Source.Contains(Filter) ||
			(FString::FromInt((int32)Entry->Code).Contains(Filter));
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

		if (UDataTable* Table = Entry->DataTable.LoadSynchronous())
		{
			if (UAssetEditorSubsystem* AssetEditor = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr)
			{
				AssetEditor->OpenEditorForAsset(Table);
			}
		}
	}
}

void SYIItemDashboard::OpenDataSource(const TSharedPtr<FYIItemDashboardEntry>& Entry) const
{
	if (!Entry.IsValid() || !Entry->DataSource.IsValid())
	{
		return;
	}

	if (UObject* Obj = Entry->DataSource.LoadSynchronous())
	{
		if (UAssetEditorSubsystem* AssetEditor = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr)
		{
			AssetEditor->OpenEditorForAsset(Obj);
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
			return false;
		}
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

FString SYIItemDashboard::GetRowString(const UScriptStruct* Struct, const uint8* RowData, FName Field) const
{
	return GetRowStringFromStruct(Struct, RowData, Field);
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
			NSLOCTEXT("YOLOInventory", "Dash_Context_OpenItemAsset", "Open Item Asset"),
			NSLOCTEXT("YOLOInventory", "Dash_Context_OpenItemAsset_Tip", "Open this item asset in the editor."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([ItemDef]()
			{
				if (UAssetEditorSubsystem* AssetEditor = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr)
				{
					AssetEditor->OpenEditorForAsset(ItemDef);
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
			NSLOCTEXT("YOLOInventory", "Dash_Context_OpenDataTable", "Open Data Table"),
			NSLOCTEXT("YOLOInventory", "Dash_Context_OpenDataTable_Tip", "Open the underlying data table for this row."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([Entry]()
			{
				if (UDataTable* Table = Entry->DataTable.LoadSynchronous())
				{
					if (UAssetEditorSubsystem* AssetEditor = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr)
					{
						AssetEditor->OpenEditorForAsset(Table);
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

	return BuildContextMenuForEntry(Selected[0]);
}

void SYIItemDashboard::RefreshInlineMappingEditor(UYIDataTableItemSource* Source)
{
	MappingRows.Reset();
	SourceFieldOptions.Reset();
	TargetPropertyOptions.Reset();

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
				SourceFieldOptions.Add(MakeShared<FString>((*It)->GetAuthoredName()));
			}
		}
	}
	for (TFieldIterator<FProperty> It(UYIItemDefinition::StaticClass()); It; ++It)
	{
		TargetPropertyOptions.Add(MakeShared<FString>((*It)->GetAuthoredName()));
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

TSharedRef<ITableRow> SYIItemDashboard::MakeMappingRow(TSharedPtr<FYIFieldMapping> Mapping, const TSharedRef<STableViewBase>& OwnerTable)
{
	auto DropdownText = [](const TSharedPtr<FString>& Str)->FText
	{
		return Str.IsValid() ? FText::FromString(*Str) : FText::GetEmpty();
	};

	return SNew(STableRow<TSharedPtr<FYIFieldMapping>>, OwnerTable)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(2)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&const_cast<SYIItemDashboard*>(this)->SourceFieldOptions)
			.OnGenerateWidget_Lambda([DropdownText](TSharedPtr<FString> InItem)
			{
				return SNew(STextBlock).Text(DropdownText(InItem));
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
		+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(2)
		[
			SNew(SComboBox<TSharedPtr<FString>>)
			.OptionsSource(&const_cast<SYIItemDashboard*>(this)->TargetPropertyOptions)
			.OnGenerateWidget_Lambda([DropdownText](TSharedPtr<FString> InItem)
			{
				return SNew(STextBlock).Text(DropdownText(InItem));
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
		return FText();
	}

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
				const FString Preview = FString::Printf(TEXT("%s\n%s"), *Def->DisplayName.ToString(), *Def->Description.ToString());
				return FText::FromString(Preview);
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
						const FString Preview = Desc.IsEmpty() ? Name : FString::Printf(TEXT("%s\n%s"), *Name, *Desc);
						return FText::FromString(Preview);
					}
				}
			}
		}
	}

	return FText::GetEmpty();
}
