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
	ChildSlot
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
				.OnGenerateRow(this, &SYIItemDashboard::MakeRowWidget)
				.OnMouseButtonDoubleClick_Lambda([this](TSharedPtr<FYIItemDashboardEntry> Entry)
				{
					OpenEntry(Entry);
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

	if (!Entry->bIsDataTable)
	{
		if (UObject* Obj = Entry->Object.LoadSynchronous())
		{
			if (UAssetEditorSubsystem* AssetEditor = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr)
			{
				AssetEditor->OpenEditorForAsset(Obj);
			}
		}
		else
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::Format(
				NSLOCTEXT("YOLOInventory","DashboardMissingAsset","Asset not found for code {0}. It may have been moved or deleted."),
				FText::AsNumber(Entry->Code)));
		}
	}
	else
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
			FDataTableEditorModule& DataTableEditorModule = FModuleManager::LoadModuleChecked<FDataTableEditorModule>("DataTableEditor");
			const EToolkitMode::Type Mode = EToolkitMode::WorldCentric;
			DataTableEditorModule.CreateDataTableEditor(Mode, nullptr, Table);
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

	if (!Source->TransformerClass)
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

	UCSVDataTransformer* Transformer = NewObject<UCSVDataTransformer>(Source, Source->TransformerClass);
	UObject* Transformed = Transformer ? Transformer->TransformObject(RowWrapper) : nullptr;
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
