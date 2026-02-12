#include "SYIAffixDashboard.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Modules/ModuleManager.h"
#include "YIAffixAsset.h"
#include "YIAffixPoolAsset.h"
#include "YIAffixFactory.h"
#include "YIAffixPoolFactory.h"
#include "Data/YIDataTableAffixSource.h"
#include "Data/YIDataTableItemSource.h"
#include "CSVDataTransformer.h"
#include "RowData.h"
#include "YIEditorRowHelpers.h"
#include "YIEditorMessageLog.h"
#include "SYIItemDashboard.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"
#include "Editor.h"
#include "Misc/PackageName.h"
#include "Factories/DataAssetFactory.h"
#include "PropertyCustomizationHelpers.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Engine/Texture.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
#include "UObject/SoftObjectPath.h"
#include "PackageTools.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Images/SImage.h"

struct FYIAffixDashboardEntry
{
	int64 Code = 0;
	FString Name;
	FString Source;
	bool bIsDataTable = false;
	FName RowName = NAME_None;
	TSoftObjectPtr<UObject> Object;
	bool bHasAsset = false;
	TSoftObjectPtr<UYIAffixAsset> AffixAsset;
	TSoftObjectPtr<UDataTable> DataTable;
	TSoftObjectPtr<UYIDataTableAffixSource> DataSource;
};

void SYIAffixDashboard::Construct(const FArguments& InArgs)
{
	FPropertyEditorModule& PropModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailArgs;
	DetailArgs.bAllowSearch = true;
	DetailArgs.bHideSelectionTip = true;
	DetailsView = PropModule.CreateDetailView(DetailArgs);

	ChildSlot
	[
		SNew(SSplitter)
		+ SSplitter::Slot().Value(0.40f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(6)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory","AffixDash_NewAffix","New Affix"))
					.OnClicked(this, &SYIAffixDashboard::CreateAffix)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory","AffixDash_NewPool","New Affix Pool"))
					.OnClicked(this, &SYIAffixDashboard::CreateAffixPool)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory","AffixDash_NewSource","New Affix Source"))
					.OnClicked(this, &SYIAffixDashboard::CreateAffixSource)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(6, 2)
			[
				BuildSourcePicker()
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(6, 2)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory","AffixDash_Import","Import Source Rows"))
					.OnClicked(this, &SYIAffixDashboard::ImportFromSource)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory","AffixDash_UpdateLinked","Update Selected Affix"))
					.OnClicked(this, &SYIAffixDashboard::UpdateSelectedAffix)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0,2)
			[
				SNew(SSeparator)
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				BuildAssetPicker()
			]
		]
		+ SSplitter::Slot().Value(0.60f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(0.55f).Padding(0, 0, 0, 4)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				[
					DetailsView.ToSharedRef()
				]
			]
			+ SVerticalBox::Slot().FillHeight(0.45f).Padding(0, 4, 0, 0)
			[
				BuildMappingPanelWidget()
			]
		]
	];

	RefreshList();
}

TSharedRef<SWidget> SYIAffixDashboard::BuildAssetPicker()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(6, 0, 6, 6)
		[
			SNew(SSearchBox)
				.OnTextChanged(this, &SYIAffixDashboard::OnSearchTextChanged)
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(6, 0)
		[
			SAssignNew(ListView, SListView<TSharedPtr<FYIAffixDashboardEntry>>)
				.ListItemsSource(&FilteredItems)
				.SelectionMode(ESelectionMode::Multi)
				.OnSelectionChanged_Lambda([this](TSharedPtr<FYIAffixDashboardEntry> Entry, ESelectInfo::Type)
					{
						ShowDetailsForEntry(Entry);
					})
				.OnGenerateRow(this, &SYIAffixDashboard::MakeRowWidget)
				.OnMouseButtonDoubleClick_Lambda([this](TSharedPtr<FYIAffixDashboardEntry> Entry)
					{
						OpenEntry(Entry);
					})
				.HeaderRow
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column("Code").DefaultLabel(NSLOCTEXT("YOLOInventory", "AffixDash_Code", "Code")).FillWidth(0.2f)
					+ SHeaderRow::Column("Name").DefaultLabel(NSLOCTEXT("YOLOInventory", "AffixDash_Name", "Name")).FillWidth(0.35f)
					+ SHeaderRow::Column("Type").DefaultLabel(NSLOCTEXT("YOLOInventory", "AffixDash_Type", "Type")).FillWidth(0.15f)
					+ SHeaderRow::Column("Source").DefaultLabel(NSLOCTEXT("YOLOInventory", "AffixDash_Source", "Source")).FillWidth(0.3f)
				)
		];
}

TSharedRef<SWidget> SYIAffixDashboard::BuildSourcePicker()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2, 0, 8, 0)
		[
			SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","AffixDash_SourceLabel","Affix Data Source:"))
		]
		+ SHorizontalBox::Slot().FillWidth(1.f)
		[
			SNew(SObjectPropertyEntryBox)
				.AllowedClass(UYIDataTableAffixSource::StaticClass())
				.ObjectPath_Lambda([this]()
					{
						return CurrentSource.IsValid() ? CurrentSource->GetPathName() : FString();
					})
				.OnObjectChanged_Lambda([this](const FAssetData& AssetData)
					{
						UObject* Obj = AssetData.GetAsset();
						if (UYIDataTableAffixSource* Source = Cast<UYIDataTableAffixSource>(Obj))
						{
							CurrentSource = Source;
							if (DetailsView.IsValid())
							{
								DetailsView->SetObject(Source);
							}
							RefreshInlineMappingEditor(Source);
						}
					})
		];
}

void SYIAffixDashboard::OnAssetSelected(const FAssetData& AssetData)
{
	if (!DetailsView.IsValid())
	{
		return;
	}
	if (UObject* Obj = AssetData.GetAsset())
	{
		LastSelectedAsset = Obj;
		DetailsView->SetObject(Obj);
		if (UYIDataTableAffixSource* Source = Cast<UYIDataTableAffixSource>(Obj))
		{
			CurrentSource = Source;
			RefreshInlineMappingEditor(Source);
		}
		else if (UYIAffixAsset* Affix = Cast<UYIAffixAsset>(Obj))
		{
			if (Affix->SourceDataSource.IsValid())
			{
				CurrentSource = Affix->SourceDataSource.LoadSynchronous();
				RefreshInlineMappingEditor(CurrentSource.Get());
			}
		}
	}
}

void SYIAffixDashboard::OnAssetDoubleClicked(const FAssetData& AssetData)
{
	OnAssetSelected(AssetData);
}

void SYIAffixDashboard::OpenAsset(UObject* Asset)
{
	if (DetailsView.IsValid() && Asset)
	{
		LastSelectedAsset = Asset;
		DetailsView->SetObject(Asset);
		if (UYIDataTableAffixSource* Source = Cast<UYIDataTableAffixSource>(Asset))
		{
			CurrentSource = Source;
			RefreshInlineMappingEditor(Source);
		}
		else if (UYIAffixAsset* Affix = Cast<UYIAffixAsset>(Asset))
		{
			if (Affix->SourceDataSource.IsValid())
			{
				CurrentSource = Affix->SourceDataSource.LoadSynchronous();
				RefreshInlineMappingEditor(CurrentSource.Get());
			}
		}
	}
}

void SYIAffixDashboard::RefreshList()
{
	Items.Reset();
	FilteredItems.Reset();

	TMap<int64, TSoftObjectPtr<UYIAffixAsset>> ExistingAssets;
	TSet<FString> ExistingRowKeys;

	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	TArray<FAssetData> AffixAssets;
	AssetRegistry.Get().GetAssetsByClass(UYIAffixAsset::StaticClass()->GetClassPathName(), AffixAssets, true);
	for (const FAssetData& AD : AffixAssets)
	{
		UYIAffixAsset* Affix = Cast<UYIAffixAsset>(AD.GetAsset());
		if (!Affix)
		{
			continue;
		}
		TSharedPtr<FYIAffixDashboardEntry> Entry = MakeShared<FYIAffixDashboardEntry>();
		Entry->bIsDataTable = false;
		Entry->Object = AD.ToSoftObjectPath();
		Entry->AffixAsset = TSoftObjectPtr<UYIAffixAsset>(AD.ToSoftObjectPath());
		Entry->Code = Affix->UniqueCode;
		Entry->Name = Affix->DisplayName.IsEmpty() ? AD.AssetName.ToString() : Affix->DisplayName.ToString();
		Entry->Source = AD.GetSoftObjectPath().ToString();
		Entry->DataSource = Affix->SourceDataSource;
		Entry->RowName = Affix->SourceRowName;

		if (Entry->Code != 0)
		{
			ExistingAssets.FindOrAdd(Entry->Code) = Entry->AffixAsset;
		}

		Items.Add(Entry);
	}

	TArray<FAssetData> Sources;
	AssetRegistry.Get().GetAssetsByClass(UYIDataTableAffixSource::StaticClass()->GetClassPathName(), Sources, true);
	for (const FAssetData& SourceData : Sources)
	{
		UYIDataTableAffixSource* Source = Cast<UYIDataTableAffixSource>(SourceData.GetAsset());
		if (!Source)
		{
			continue;
		}

		UDataTable* Table = Source->ResolveDataTable();
		if (!Table || !Table->RowStruct)
		{
			continue;
		}

		const FName CodeField = Source->UniqueCodeFieldName.IsNone() ? TEXT("UniqueCode") : Source->UniqueCodeFieldName;
		const FName PreviewField = Source->PreviewNameFieldName.IsNone() ? TEXT("DisplayName") : Source->PreviewNameFieldName;

		for (const auto& RowPair : Table->GetRowMap())
		{
			const FName RowName = RowPair.Key;
			const uint8* RowPtr = RowPair.Value;

			const int64 CodeValue = ExtractCodeFromRow(Table->RowStruct, RowPtr, CodeField);
			const FString RowKey = FString::Printf(TEXT("%lld|%s"), CodeValue, *RowName.ToString());
			if (ExistingRowKeys.Contains(RowKey))
			{
				continue;
			}

			TSharedPtr<FYIAffixDashboardEntry> Entry = MakeShared<FYIAffixDashboardEntry>();
			Entry->bIsDataTable = true;
			Entry->Code = CodeValue;
			Entry->RowName = RowName;
			Entry->DataSource = Source;
			Entry->DataTable = Table;
			Entry->Source = SourceData.GetSoftObjectPath().ToString();
			Entry->Name = RowName.ToString();

			const FString PreviewName = GetRowString(Table->RowStruct, RowPtr, PreviewField);
			if (!PreviewName.IsEmpty())
			{
				Entry->Name = PreviewName;
			}

			if (TSoftObjectPtr<UYIAffixAsset>* Found = ExistingAssets.Find(CodeValue))
			{
				Entry->bHasAsset = true;
				Entry->AffixAsset = *Found;
			}

			Items.Add(Entry);
			ExistingRowKeys.Add(RowKey);
		}
	}

	const FString SearchFilter = SearchText.ToString();
	for (const TSharedPtr<FYIAffixDashboardEntry>& Entry : Items)
	{
		if (!Entry.IsValid())
		{
			continue;
		}
		const bool bPass = SearchFilter.IsEmpty() ||
			Entry->Name.Contains(SearchFilter) ||
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

void SYIAffixDashboard::OnSearchTextChanged(const FText& NewText)
{
	SearchText = NewText;
	RefreshList();
}

void SYIAffixDashboard::ShowDetailsForEntry(const TSharedPtr<FYIAffixDashboardEntry>& Entry)
{
	if (!DetailsView.IsValid())
	{
		return;
	}

	if (!Entry.IsValid())
	{
		DetailsView->SetObject(nullptr);
		return;
	}

	if (!Entry->bIsDataTable)
	{
		if (UObject* Obj = Entry->Object.LoadSynchronous())
		{
			LastSelectedAsset = Obj;
			DetailsView->SetObject(Obj);
			if (UYIAffixAsset* Affix = Cast<UYIAffixAsset>(Obj))
			{
				if (Affix->SourceDataSource.IsValid())
				{
					CurrentSource = Affix->SourceDataSource.LoadSynchronous();
					RefreshInlineMappingEditor(CurrentSource.Get());
				}
			}
		}
		return;
	}

	if (Entry->DataSource.IsValid())
	{
		if (UObject* Obj = Entry->DataSource.LoadSynchronous())
		{
			LastSelectedAsset = Obj;
			DetailsView->SetObject(Obj);
			if (UYIDataTableAffixSource* Source = Cast<UYIDataTableAffixSource>(Obj))
			{
				CurrentSource = Source;
				RefreshInlineMappingEditor(Source);
			}
		}
	}
}

void SYIAffixDashboard::OpenEntry(const TSharedPtr<FYIAffixDashboardEntry>& Entry)
{
	if (!Entry.IsValid())
	{
		return;
	}
	if (Entry->bIsDataTable)
	{
		if (Entry->AffixAsset.IsValid())
		{
			if (UObject* Obj = Entry->AffixAsset.LoadSynchronous())
			{
				OpenAsset(Obj);
			}
		}
		else
		{
			ShowDetailsForEntry(Entry);
		}
		return;
	}
	if (Entry->Object.IsValid())
	{
		if (UObject* Obj = Entry->Object.LoadSynchronous())
		{
			OpenAsset(Obj);
		}
	}
}

TSharedRef<ITableRow> SYIAffixDashboard::MakeRowWidget(TSharedPtr<FYIAffixDashboardEntry> Entry, const TSharedRef<STableViewBase>& Owner)
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
				? FLinearColor(0.18f, 0.65f, 0.32f, 0.9f)
				: FLinearColor(0.95f, 0.55f, 0.20f, 0.9f);
		}
		return FLinearColor(0.20f, 0.45f, 0.90f, 0.9f);
	};

	return SNew(STableRow<TSharedPtr<FYIAffixDashboardEntry>>, Owner)
	[
		SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Fill)
			[
				SNew(SBorder)
					.Padding(FMargin(2, 0))
					.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(StatusColor())
			]
			+ SHorizontalBox::Slot().FillWidth(0.2f)
			[
				SNew(STextBlock)
					.Text(FText::AsNumber(Entry.IsValid() ? Entry->Code : 0))
					.ColorAndOpacity(StatusColor())
			]
			+ SHorizontalBox::Slot().FillWidth(0.35f)
			[
				SNew(STextBlock).Text(FText::FromString(Entry.IsValid() ? Entry->Name : TEXT("")))
			]
			+ SHorizontalBox::Slot().FillWidth(0.15f)
			[
				SNew(STextBlock).Text(Entry->bIsDataTable
					? NSLOCTEXT("YOLOInventory", "AffixDash_Type_DataRow", "Data Row")
					: NSLOCTEXT("YOLOInventory", "AffixDash_Type_Asset", "Asset"))
					.ColorAndOpacity(StatusColor())
			]
			+ SHorizontalBox::Slot().FillWidth(0.3f)
			[
				SNew(STextBlock).Text(FText::FromString(Entry.IsValid() ? Entry->Source : TEXT("")))
			]
	];
}

FReply SYIAffixDashboard::CreateAffix()
{
	IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UYIAffixFactory* Factory = NewObject<UYIAffixFactory>();
	const FString TargetPath = TEXT("/Game/YOLOInventory/Affixes");
	const FString BaseName = TEXT("Affix");
	FString PackageName, AssetName;
	Tools.CreateUniqueAssetName(TargetPath / BaseName, TEXT(""), PackageName, AssetName);
	Tools.CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), Factory->SupportedClass, Factory);
	return FReply::Handled();
}

FReply SYIAffixDashboard::CreateAffixPool()
{
	IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UYIAffixPoolFactory* Factory = NewObject<UYIAffixPoolFactory>();
	const FString TargetPath = TEXT("/Game/YOLOInventory/Affixes");
	const FString BaseName = TEXT("AffixPool");
	FString PackageName, AssetName;
	Tools.CreateUniqueAssetName(TargetPath / BaseName, TEXT(""), PackageName, AssetName);
	Tools.CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), Factory->SupportedClass, Factory);
	return FReply::Handled();
}

FReply SYIAffixDashboard::CreateAffixSource()
{
	IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
	Factory->DataAssetClass = UYIDataTableAffixSource::StaticClass();
	const FString TargetPath = TEXT("/Game/YOLOInventory/Affixes/Sources");
	const FString BaseName = TEXT("AffixSource");
	FString PackageName, AssetName;
	Tools.CreateUniqueAssetName(TargetPath / BaseName, TEXT(""), PackageName, AssetName);
	Tools.CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), Factory->DataAssetClass, Factory);
	return FReply::Handled();
}

UYIDataTableAffixSource* SYIAffixDashboard::ResolveCurrentSource() const
{
	if (CurrentSource.IsValid())
	{
		return CurrentSource.Get();
	}
	if (LastSelectedAsset.IsValid())
	{
		if (UYIDataTableAffixSource* Source = Cast<UYIDataTableAffixSource>(LastSelectedAsset.Get()))
		{
			return Source;
		}
	}
	return nullptr;
}

int64 SYIAffixDashboard::ExtractCodeFromRow(const UScriptStruct* Struct, const uint8* RowData, FName FieldName) const
{
	if (!Struct || !RowData || FieldName.IsNone())
	{
		return 0;
	}
	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		const FProperty* Prop = *It;
		if (!Prop || Prop->GetFName() != FieldName)
		{
			continue;
		}
		if (const FNumericProperty* Num = CastField<FNumericProperty>(Prop))
		{
			if (Num->IsInteger())
			{
				return Num->GetSignedIntPropertyValue(Prop->ContainerPtrToValuePtr<uint8>(RowData));
			}
		}
	}
	return 0;
}

FString SYIAffixDashboard::GetRowString(const UScriptStruct* Struct, const uint8* RowData, FName Field) const
{
	return YIEditor_GetRowStringFromStruct(Struct, RowData, Field);
}

static bool TryGetEnumValueFromStringAffix(const UEnum* Enum, const FString& Value, int64& OutValue)
{
	if (!Enum)
	{
		return false;
	}
	if (Value.IsEmpty())
	{
		return false;
	}
	if (Value.IsNumeric())
	{
		OutValue = FCString::Atoi64(*Value);
		return true;
	}
	const int64 NameValue = Enum->GetValueByNameString(Value);
	if (NameValue != INDEX_NONE)
	{
		OutValue = NameValue;
		return true;
	}
	const FName Name(*Value);
	const int64 ExactValue = Enum->GetValueByName(Name);
	if (ExactValue != INDEX_NONE)
	{
		OutValue = ExactValue;
		return true;
	}
	const FString Lower = Value.ToLower();
	for (int32 Index = 0; Index < Enum->NumEnums(); ++Index)
	{
		const FString EnumName = Enum->GetNameStringByIndex(Index);
		if (EnumName.ToLower() == Lower)
		{
			OutValue = Enum->GetValueByIndex(Index);
			return true;
		}
	}
	return false;
}

static bool SetEnumPropertyValueAffix(FProperty* DestProp, uint8* DestPtr, int64 Value)
{
	if (FEnumProperty* EnumProp = CastField<FEnumProperty>(DestProp))
	{
		if (FNumericProperty* Underlying = EnumProp->GetUnderlyingProperty())
		{
			Underlying->SetIntPropertyValue(DestPtr, Value);
			return true;
		}
	}
	if (FByteProperty* ByteProp = CastField<FByteProperty>(DestProp))
	{
		if (ByteProp->Enum)
		{
			ByteProp->SetPropertyValue(DestPtr, (uint8)Value);
			return true;
		}
	}
	return false;
}

static bool SetEnumPropertyValueAffix(FProperty* DestProp, uint8* DestPtr, const FString& Value)
{
	if (FEnumProperty* EnumProp = CastField<FEnumProperty>(DestProp))
	{
		if (UEnum* Enum = EnumProp->GetEnum())
		{
			int64 EnumValue = 0;
			if (TryGetEnumValueFromStringAffix(Enum, Value, EnumValue))
			{
				if (FNumericProperty* Underlying = EnumProp->GetUnderlyingProperty())
				{
					Underlying->SetIntPropertyValue(DestPtr, EnumValue);
					return true;
				}
			}
		}
	}
	if (FByteProperty* ByteProp = CastField<FByteProperty>(DestProp))
	{
		if (ByteProp->Enum)
		{
			int64 EnumValue = 0;
			if (TryGetEnumValueFromStringAffix(ByteProp->Enum, Value, EnumValue))
			{
				ByteProp->SetPropertyValue(DestPtr, (uint8)EnumValue);
				return true;
			}
		}
	}
	return false;
}

static bool SetPropertyValueFromStringAffix(const FString& Value, FProperty* DestProp, uint8* DestPtr, EYIFieldMappingConversion Conversion)
{
	if (!DestProp || !DestPtr)
	{
		return false;
	}

	if (Conversion == EYIFieldMappingConversion::ToEnum)
	{
		return SetEnumPropertyValueAffix(DestProp, DestPtr, Value);
	}
	if (Conversion == EYIFieldMappingConversion::None)
	{
		if (CastField<FEnumProperty>(DestProp) || (CastField<FByteProperty>(DestProp) && CastField<FByteProperty>(DestProp)->Enum))
		{
			return SetEnumPropertyValueAffix(DestProp, DestPtr, Value);
		}
	}

	if (FBoolProperty* DestBool = CastField<FBoolProperty>(DestProp))
	{
		if (Conversion == EYIFieldMappingConversion::BoolFromText)
		{
			DestBool->SetPropertyValue(DestPtr, !Value.IsEmpty());
			return true;
		}
		const FString Lower = Value.ToLower();
		const bool bVal = (Lower == TEXT("true") || Lower == TEXT("1") || Lower == TEXT("yes") || Lower == TEXT("on"));
		DestBool->SetPropertyValue(DestPtr, bVal);
		return true;
	}

	if (FNumericProperty* DestNum = CastField<FNumericProperty>(DestProp))
	{
		if (Conversion == EYIFieldMappingConversion::ToInt || DestNum->IsInteger())
		{
			const int64 IntVal = FCString::Atoi64(*Value);
			DestNum->SetIntPropertyValue(DestPtr, IntVal);
			return true;
		}
		const double FloatVal = FCString::Atod(*Value);
		DestNum->SetFloatingPointPropertyValue(DestPtr, FloatVal);
		return true;
	}

	if (FNameProperty* DestName = CastField<FNameProperty>(DestProp))
	{
		DestName->SetPropertyValue(DestPtr, FName(*Value));
		return true;
	}
	if (FStrProperty* DestStr = CastField<FStrProperty>(DestProp))
	{
		DestStr->SetPropertyValue(DestPtr, Value);
		return true;
	}
	if (FTextProperty* DestText = CastField<FTextProperty>(DestProp))
	{
		DestText->SetPropertyValue(DestPtr, FText::FromString(Value));
		return true;
	}

	if (Conversion == EYIFieldMappingConversion::ToGameplayTag)
	{
		if (const FStructProperty* DestStruct = CastField<FStructProperty>(DestProp))
		{
			if (DestStruct->Struct == FGameplayTag::StaticStruct())
			{
				FGameplayTag* TagPtr = reinterpret_cast<FGameplayTag*>(DestPtr);
				*TagPtr = FGameplayTag::RequestGameplayTag(FName(*Value), false);
				return true;
			}
		}
	}

	if (Conversion == EYIFieldMappingConversion::ToSoftTexture)
	{
		if (FSoftObjectProperty* DestSoftObj = CastField<FSoftObjectProperty>(DestProp))
		{
			if (DestSoftObj->PropertyClass && DestSoftObj->PropertyClass->IsChildOf(UTexture::StaticClass()))
			{
				const FSoftObjectPtr SoftPtr = FSoftObjectPtr(FSoftObjectPath(Value));
				DestSoftObj->SetPropertyValue(DestPtr, SoftPtr);
				return true;
			}
		}
	}

	return false;
}

static bool CopyValueBetweenPropertiesAffix(const FProperty* SourceProp, const uint8* SourcePtr, FProperty* DestProp, uint8* DestPtr, EYIFieldMappingConversion Conversion)
{
	if (!SourceProp || !DestProp || !SourcePtr || !DestPtr)
	{
		return false;
	}
	if (SourceProp->SameType(DestProp))
	{
		SourceProp->CopyCompleteValue(DestPtr, SourcePtr);
		return true;
	}

	if (Conversion == EYIFieldMappingConversion::ToEnum)
	{
		if (const FEnumProperty* SrcEnum = CastField<FEnumProperty>(SourceProp))
		{
			if (UEnum* Enum = SrcEnum->GetEnum())
			{
				const int64 Val = SrcEnum->GetUnderlyingProperty()->GetSignedIntPropertyValue(SourcePtr);
				return SetEnumPropertyValueAffix(DestProp, DestPtr, Enum->GetNameStringByValue(Val));
			}
		}
		if (const FByteProperty* SrcByte = CastField<FByteProperty>(SourceProp))
		{
			if (SrcByte->Enum)
			{
				const int64 Val = SrcByte->GetPropertyValue(SourcePtr);
				return SetEnumPropertyValueAffix(DestProp, DestPtr, SrcByte->Enum->GetNameStringByValue(Val));
			}
		}
		if (const FNumericProperty* SrcNum = CastField<FNumericProperty>(SourceProp))
		{
			const int64 Val = SrcNum->GetSignedIntPropertyValue(SourcePtr);
			return SetEnumPropertyValueAffix(DestProp, DestPtr, Val);
		}
		if (const FNameProperty* SrcName = CastField<FNameProperty>(SourceProp))
		{
			return SetEnumPropertyValueAffix(DestProp, DestPtr, SrcName->GetPropertyValue(SourcePtr).ToString());
		}
		if (const FStrProperty* SrcStr = CastField<FStrProperty>(SourceProp))
		{
			return SetEnumPropertyValueAffix(DestProp, DestPtr, SrcStr->GetPropertyValue(SourcePtr));
		}
		if (const FTextProperty* SrcText = CastField<FTextProperty>(SourceProp))
		{
			return SetEnumPropertyValueAffix(DestProp, DestPtr, SrcText->GetPropertyValue(SourcePtr).ToString());
		}
	}

	if (Conversion == EYIFieldMappingConversion::ToSoftTexture)
	{
		if (FSoftObjectProperty* DestSoftObj = CastField<FSoftObjectProperty>(DestProp))
		{
			if (DestSoftObj->PropertyClass && DestSoftObj->PropertyClass->IsChildOf(UTexture::StaticClass()))
			{
				if (const FSoftObjectProperty* SrcSoftObj = CastField<FSoftObjectProperty>(SourceProp))
				{
					if (!SrcSoftObj->PropertyClass || SrcSoftObj->PropertyClass->IsChildOf(UTexture::StaticClass()))
					{
						DestSoftObj->SetPropertyValue(DestPtr, SrcSoftObj->GetPropertyValue(SourcePtr));
						return true;
					}
				}
				if (const FObjectPropertyBase* SrcObj = CastField<FObjectPropertyBase>(SourceProp))
				{
					if (UObject* Obj = SrcObj->GetObjectPropertyValue(SourcePtr))
					{
						if (Obj->IsA(UTexture::StaticClass()))
						{
							DestSoftObj->SetPropertyValue(DestPtr, FSoftObjectPtr(Obj));
							return true;
						}
					}
				}
			}
		}
	}

	if (const FStrProperty* SrcStr = CastField<FStrProperty>(SourceProp))
	{
		const FString Value = SrcStr->GetPropertyValue(SourcePtr);
		if (Conversion == EYIFieldMappingConversion::BoolFromText)
		{
			if (FBoolProperty* DestBool = CastField<FBoolProperty>(DestProp))
			{
				DestBool->SetPropertyValue(DestPtr, !Value.IsEmpty());
				return true;
			}
		}
		if (Conversion == EYIFieldMappingConversion::ToName)
		{
			if (FNameProperty* DestName = CastField<FNameProperty>(DestProp))
			{
				DestName->SetPropertyValue(DestPtr, FName(*Value));
				return true;
			}
		}
		if (Conversion == EYIFieldMappingConversion::ToText)
		{
			if (FTextProperty* DestText = CastField<FTextProperty>(DestProp))
			{
				DestText->SetPropertyValue(DestPtr, FText::FromString(Value));
				return true;
			}
		}
		if (FStrProperty* DestStr = CastField<FStrProperty>(DestProp))
		{
			DestStr->SetPropertyValue(DestPtr, Value);
			return true;
		}
		if (FNameProperty* DestName = CastField<FNameProperty>(DestProp))
		{
			DestName->SetPropertyValue(DestPtr, FName(*Value));
			return true;
		}
		if (FTextProperty* DestText = CastField<FTextProperty>(DestProp))
		{
			DestText->SetPropertyValue(DestPtr, FText::FromString(Value));
			return true;
		}
		if (Conversion == EYIFieldMappingConversion::ToGameplayTag)
		{
			if (const FStructProperty* DestStruct = CastField<FStructProperty>(DestProp))
			{
				if (DestStruct->Struct == FGameplayTag::StaticStruct())
				{
					FGameplayTag* TagPtr = reinterpret_cast<FGameplayTag*>(DestPtr);
					*TagPtr = FGameplayTag::RequestGameplayTag(FName(*Value), false);
					return true;
				}
			}
		}
		if (Conversion == EYIFieldMappingConversion::ToSoftTexture)
		{
			if (FSoftObjectProperty* DestSoftObj = CastField<FSoftObjectProperty>(DestProp))
			{
				if (DestSoftObj->PropertyClass && DestSoftObj->PropertyClass->IsChildOf(UTexture::StaticClass()))
				{
					const FSoftObjectPtr SoftPtr = FSoftObjectPtr(FSoftObjectPath(Value));
					DestSoftObj->SetPropertyValue(DestPtr, SoftPtr);
					return true;
				}
			}
		}
	}
	if (const FNameProperty* SrcName = CastField<FNameProperty>(SourceProp))
	{
		const FName Value = SrcName->GetPropertyValue(SourcePtr);
		if (Conversion == EYIFieldMappingConversion::ToText)
		{
			if (FTextProperty* DestText = CastField<FTextProperty>(DestProp))
			{
				DestText->SetPropertyValue(DestPtr, FText::FromName(Value));
				return true;
			}
		}
		if (FStrProperty* DestStr = CastField<FStrProperty>(DestProp))
		{
			DestStr->SetPropertyValue(DestPtr, Value.ToString());
			return true;
		}
		if (FTextProperty* DestText = CastField<FTextProperty>(DestProp))
		{
			DestText->SetPropertyValue(DestPtr, FText::FromName(Value));
			return true;
		}
		if (Conversion == EYIFieldMappingConversion::ToGameplayTag)
		{
			if (const FStructProperty* DestStruct = CastField<FStructProperty>(DestProp))
			{
				if (DestStruct->Struct == FGameplayTag::StaticStruct())
				{
					FGameplayTag* TagPtr = reinterpret_cast<FGameplayTag*>(DestPtr);
					*TagPtr = FGameplayTag::RequestGameplayTag(Value, false);
					return true;
				}
			}
		}
		if (Conversion == EYIFieldMappingConversion::ToSoftTexture)
		{
			if (FSoftObjectProperty* DestSoftObj = CastField<FSoftObjectProperty>(DestProp))
			{
				if (DestSoftObj->PropertyClass && DestSoftObj->PropertyClass->IsChildOf(UTexture::StaticClass()))
				{
					const FSoftObjectPtr SoftPtr = FSoftObjectPtr(FSoftObjectPath(Value.ToString()));
					DestSoftObj->SetPropertyValue(DestPtr, SoftPtr);
					return true;
				}
			}
		}
	}
	if (const FTextProperty* SrcText = CastField<FTextProperty>(SourceProp))
	{
		const FText Value = SrcText->GetPropertyValue(SourcePtr);
		if (Conversion == EYIFieldMappingConversion::ToName)
		{
			if (FNameProperty* DestName = CastField<FNameProperty>(DestProp))
			{
				DestName->SetPropertyValue(DestPtr, FName(*Value.ToString()));
				return true;
			}
		}
		if (FStrProperty* DestStr = CastField<FStrProperty>(DestProp))
		{
			DestStr->SetPropertyValue(DestPtr, Value.ToString());
			return true;
		}
		if (FNameProperty* DestName = CastField<FNameProperty>(DestProp))
		{
			DestName->SetPropertyValue(DestPtr, FName(*Value.ToString()));
			return true;
		}
		if (FTextProperty* DestText = CastField<FTextProperty>(DestProp))
		{
			DestText->SetPropertyValue(DestPtr, Value);
			return true;
		}
		if (Conversion == EYIFieldMappingConversion::ToGameplayTag)
		{
			if (const FStructProperty* DestStruct = CastField<FStructProperty>(DestProp))
			{
				if (DestStruct->Struct == FGameplayTag::StaticStruct())
				{
					FGameplayTag* TagPtr = reinterpret_cast<FGameplayTag*>(DestPtr);
					*TagPtr = FGameplayTag::RequestGameplayTag(FName(*Value.ToString()), false);
					return true;
				}
			}
		}
		if (Conversion == EYIFieldMappingConversion::ToSoftTexture)
		{
			if (FSoftObjectProperty* DestSoftObj = CastField<FSoftObjectProperty>(DestProp))
			{
				if (DestSoftObj->PropertyClass && DestSoftObj->PropertyClass->IsChildOf(UTexture::StaticClass()))
				{
					const FSoftObjectPtr SoftPtr = FSoftObjectPtr(FSoftObjectPath(Value.ToString()));
					DestSoftObj->SetPropertyValue(DestPtr, SoftPtr);
					return true;
				}
			}
		}
	}

	if (const FNumericProperty* SrcNum = CastField<FNumericProperty>(SourceProp))
	{
		if (const FNumericProperty* DestNum = CastField<FNumericProperty>(DestProp))
		{
			double Value = 0.0;
			if (SrcNum->IsFloatingPoint())
			{
				Value = SrcNum->GetFloatingPointPropertyValue(SourcePtr);
			}
			else
			{
				Value = (double)SrcNum->GetSignedIntPropertyValue(SourcePtr);
			}
			if (DestNum->IsInteger())
			{
				DestNum->SetIntPropertyValue(DestPtr, (int64)Value);
			}
			else
			{
				DestNum->SetFloatingPointPropertyValue(DestPtr, Value);
			}
			return true;
		}
	}

	if (const FBoolProperty* SrcBool = CastField<FBoolProperty>(SourceProp))
	{
		const bool bVal = SrcBool->GetPropertyValue(SourcePtr);
		if (FBoolProperty* DestBool = CastField<FBoolProperty>(DestProp))
		{
			DestBool->SetPropertyValue(DestPtr, bVal);
			return true;
		}
		if (FNumericProperty* DestNum = CastField<FNumericProperty>(DestProp))
		{
			if (DestNum->IsInteger())
			{
				DestNum->SetIntPropertyValue(DestPtr, bVal ? (int64)1 : (int64)0);
			}
			else
			{
				DestNum->SetFloatingPointPropertyValue(DestPtr, bVal ? 1.0 : 0.0);
			}
			return true;
		}
	}

	if (Conversion == EYIFieldMappingConversion::ToGameplayTag)
	{
		if (const FStructProperty* DestStruct = CastField<FStructProperty>(DestProp))
		{
			if (DestStruct->Struct == FGameplayTag::StaticStruct())
			{
				if (const FStructProperty* SrcStruct = CastField<FStructProperty>(SourceProp))
				{
					if (SrcStruct->Struct == FGameplayTag::StaticStruct())
					{
						DestStruct->CopyCompleteValue(DestPtr, SourcePtr);
						return true;
					}
				}
			}
		}
	}

	return false;
}

static EYIFieldMappingConversion GuessConversionForPropsAffix(const FProperty* SourceProp, const FProperty* TargetProp)
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
	if (CastField<FEnumProperty>(TargetProp)) return EYIFieldMappingConversion::ToEnum;
	if (const FByteProperty* ByteProp = CastField<FByteProperty>(TargetProp))
	{
		if (ByteProp->Enum)
		{
			return EYIFieldMappingConversion::ToEnum;
		}
	}
	if (const FStructProperty* DestStruct = CastField<FStructProperty>(TargetProp))
	{
		if (DestStruct->Struct == FGameplayTag::StaticStruct())
		{
			return EYIFieldMappingConversion::ToGameplayTag;
		}
	}
	if (const FSoftObjectProperty* DestSoftObj = CastField<FSoftObjectProperty>(TargetProp))
	{
		if (DestSoftObj->PropertyClass && DestSoftObj->PropertyClass->IsChildOf(UTexture::StaticClass()))
		{
			return EYIFieldMappingConversion::ToSoftTexture;
		}
	}
	if (const FNumericProperty* NumTarget = CastField<FNumericProperty>(TargetProp))
	{
		if (NumTarget->IsInteger()) return EYIFieldMappingConversion::ToInt;
		return EYIFieldMappingConversion::ToFloat;
	}
	return EYIFieldMappingConversion::None;
}

static bool GetTransformFunctionPropsAffix(UFunction* Function, FProperty*& OutInput, FProperty*& OutOutput)
{
	OutInput = nullptr;
	OutOutput = nullptr;
	if (!Function)
	{
		return false;
	}

	for (TFieldIterator<FProperty> It(Function); It; ++It)
	{
		if (!It->HasAnyPropertyFlags(CPF_Parm))
		{
			continue;
		}
		if (It->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			if (OutOutput)
			{
				return false;
			}
			OutOutput = *It;
			continue;
		}
		if (It->HasAnyPropertyFlags(CPF_OutParm))
		{
			if (OutOutput)
			{
				return false;
			}
			OutOutput = *It;
			continue;
		}

		if (OutInput)
		{
			return false;
		}
		OutInput = *It;
	}

	return OutInput && OutOutput;
}

static bool ApplyTransformFunctionAffix(const FYIFieldMapping& Mapping, FProperty* DestProp, uint8* DestPtr, FString* OutError = nullptr)
{
	if (!DestProp || !DestPtr)
	{
		return false;
	}

	if (Mapping.TransformFunction.IsNone())
	{
		return false;
	}

	UClass* LibraryClass = Mapping.TransformLibrary.LoadSynchronous();
	if (!LibraryClass)
	{
		if (OutError) { *OutError = TEXT("Transform library missing."); }
		return false;
	}

	UFunction* Function = LibraryClass->FindFunctionByName(Mapping.TransformFunction);
	if (!Function)
	{
		if (OutError) { *OutError = TEXT("Transform function not found."); }
		return false;
	}

	FProperty* InputProp = nullptr;
	FProperty* OutputProp = nullptr;
	if (!GetTransformFunctionPropsAffix(Function, InputProp, OutputProp))
	{
		if (OutError) { *OutError = TEXT("Transform function signature invalid."); }
		return false;
	}

	FStructOnScope Params(Function);
	uint8* ParamsMem = Params.GetStructMemory();
	uint8* InputPtr = InputProp->ContainerPtrToValuePtr<uint8>(ParamsMem);
	uint8* OutputPtr = OutputProp->ContainerPtrToValuePtr<uint8>(ParamsMem);

	if (!CopyValueBetweenPropertiesAffix(DestProp, DestPtr, InputProp, InputPtr, EYIFieldMappingConversion::None))
	{
		if (OutError) { *OutError = TEXT("Transform input copy failed."); }
		return false;
	}

	UObject* CDO = LibraryClass->GetDefaultObject();
	if (!CDO)
	{
		if (OutError) { *OutError = TEXT("Transform library CDO missing."); }
		return false;
	}

	CDO->ProcessEvent(Function, ParamsMem);

	if (!CopyValueBetweenPropertiesAffix(OutputProp, OutputPtr, DestProp, DestPtr, EYIFieldMappingConversion::None))
	{
		if (OutError) { *OutError = TEXT("Transform output copy failed."); }
		return false;
	}

	return true;
}

static FString ExportPropertyValueToStringAffix(const FProperty* Prop, const uint8* ValuePtr)
{
	if (!Prop || !ValuePtr)
	{
		return FString();
	}
	FString Out;
	Prop->ExportTextItem_Direct(Out, ValuePtr, nullptr, nullptr, PPF_None);
	return Out;
}

static bool ApplyInlineMappingsAffix(const UYIDataTableAffixSource* Source, const UDataTable* DataTable, FName RowName, UYIAffixAsset*& OutAffix)
{
	if (!Source || !DataTable || !DataTable->RowStruct || !Source->bUseInlineMappings || Source->InlineMappings.Num() == 0)
	{
		return false;
	}

	const uint8* const* Found = DataTable->GetRowMap().Find(RowName);
	const uint8* RowPtr = Found ? *Found : nullptr;
	if (!RowPtr)
	{
		return false;
	}

	UYIAffixAsset* Affix = NewObject<UYIAffixAsset>();

	for (const FYIFieldMapping& Mapping : Source->InlineMappings)
	{
		if (Mapping.TargetProperty.IsNone())
		{
			continue;
		}

		const FProperty* SourceProp = nullptr;
		if (!Mapping.bUseStaticValue)
		{
			for (TFieldIterator<FProperty> It(DataTable->RowStruct); It; ++It)
			{
				if ((*It)->GetAuthoredName() == Mapping.SourceField)
				{
					SourceProp = *It;
					break;
				}
			}
		}

		FProperty* TargetProp = FindFProperty<FProperty>(UYIAffixAsset::StaticClass(), Mapping.TargetProperty);

		if (!TargetProp || (!SourceProp && !Mapping.bUseStaticValue))
		{
			continue;
		}

		uint8* DstPtr = TargetProp->ContainerPtrToValuePtr<uint8>(Affix);

		if (Mapping.bUseStaticValue)
		{
			if (SetPropertyValueFromStringAffix(Mapping.StaticValue, TargetProp, DstPtr, Mapping.Conversion))
			{
				ApplyTransformFunctionAffix(Mapping, TargetProp, DstPtr, nullptr);
			}
			continue;
		}

		const uint8* SrcPtr = SourceProp->ContainerPtrToValuePtr<uint8>(RowPtr);
		bool bConverted = CopyValueBetweenPropertiesAffix(SourceProp, SrcPtr, TargetProp, DstPtr, Mapping.Conversion);
		if (bConverted && !Mapping.TransformFunction.IsNone())
		{
			ApplyTransformFunctionAffix(Mapping, TargetProp, DstPtr, nullptr);
		}
	}

	OutAffix = Affix;
	return true;
}

static void CopyAffixProperties(const UYIAffixAsset* SourceAffix, UYIAffixAsset* DestAffix)
{
	if (!SourceAffix || !DestAffix)
	{
		return;
	}

	for (TFieldIterator<FProperty> It(UYIAffixAsset::StaticClass()); It; ++It)
	{
		FProperty* Prop = *It;
		if (!Prop || Prop->HasAnyPropertyFlags(CPF_Transient))
		{
			continue;
		}
		const uint8* SrcPtr = Prop->ContainerPtrToValuePtr<uint8>(SourceAffix);
		uint8* DstPtr = Prop->ContainerPtrToValuePtr<uint8>(DestAffix);
		Prop->CopyCompleteValue(DstPtr, SrcPtr);
	}
}

static bool YIAffixDash_IsAffixFromSource(const UYIAffixAsset* Affix, const UYIDataTableAffixSource* Source)
{
	if (!Affix || !Source)
	{
		return false;
	}
	return Affix->SourceDataSource.ToSoftObjectPath() == FSoftObjectPath(Source->GetPathName());
}

static void YIAffixDash_CollectAffixesForSource(const UYIDataTableAffixSource* Source, TArray<UYIAffixAsset*>& OutAffixes)
{
	OutAffixes.Reset();
	if (!Source)
	{
		return;
	}

	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> Assets;
	ARM.Get().GetAssetsByClass(UYIAffixAsset::StaticClass()->GetClassPathName(), Assets, true);
	for (const FAssetData& AD : Assets)
	{
		UYIAffixAsset* Affix = Cast<UYIAffixAsset>(AD.GetAsset());
		if (!Affix)
		{
			continue;
		}
		if (YIAffixDash_IsAffixFromSource(Affix, Source))
		{
			OutAffixes.Add(Affix);
		}
	}
}

void SYIAffixDashboard::CacheExistingAffixesByCode(TMap<int64, TSoftObjectPtr<UYIAffixAsset>>& OutMap) const
{
	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> Assets;
	ARM.Get().GetAssetsByClass(UYIAffixAsset::StaticClass()->GetClassPathName(), Assets, true);
	for (const FAssetData& AD : Assets)
	{
		if (UYIAffixAsset* Affix = Cast<UYIAffixAsset>(AD.GetAsset()))
		{
			if (Affix->UniqueCode != 0)
			{
				OutMap.FindOrAdd(Affix->UniqueCode) = TSoftObjectPtr<UYIAffixAsset>(AD.ToSoftObjectPath());
			}
		}
	}
}

bool SYIAffixDashboard::CreateOrUpdateAffixFromRow(const UYIDataTableAffixSource* Source, const UDataTable* Table, FName RowName, const uint8* RowPtr, int64 Code, TMap<int64, TSoftObjectPtr<UYIAffixAsset>>* ExistingByCode)
{
	if (!Source || !Table || !RowPtr || Code == 0)
	{
		return false;
	}

	const EYITransformMode Mode = Source->TransformMode;
	const bool bCanInline = Source->bUseInlineMappings && Source->InlineMappings.Num() > 0;
	UYIAffixAsset* InlineAffix = nullptr;
	UYIAffixAsset* TransformerAffix = nullptr;

	auto RunTransformer = [&]() -> UYIAffixAsset*
	{
		if (!Source->TransformerClass)
		{
			return nullptr;
		}
		URowData* RowWrapper = NewObject<URowData>();
		RowWrapper->Address = const_cast<uint8*>(RowPtr);
		RowWrapper->Struct = Table->RowStruct;
		UCSVDataTransformer* Transformer = NewObject<UCSVDataTransformer>(RowWrapper, Source->TransformerClass);
		UObject* Result = Transformer ? Transformer->TransformObject(RowWrapper) : nullptr;
		return Cast<UYIAffixAsset>(Result);
	};

	switch (Mode)
	{
	case EYITransformMode::InlineOnly:
		if (!bCanInline || !ApplyInlineMappingsAffix(Source, Table, RowName, InlineAffix))
		{
			return false;
		}
		break;
	case EYITransformMode::TransformerOnly:
		TransformerAffix = RunTransformer();
		if (!TransformerAffix)
		{
			return false;
		}
		break;
	case EYITransformMode::HybridInlineThenTransformer:
		if (bCanInline)
		{
			ApplyInlineMappingsAffix(Source, Table, RowName, InlineAffix);
		}
		TransformerAffix = RunTransformer();
		break;
	case EYITransformMode::HybridTransformerThenInline:
		TransformerAffix = RunTransformer();
		if (bCanInline)
		{
			ApplyInlineMappingsAffix(Source, Table, RowName, InlineAffix);
		}
		break;
	}

	UYIAffixAsset* FinalAffix = nullptr;
	if (InlineAffix && TransformerAffix)
	{
		FinalAffix = NewObject<UYIAffixAsset>();
		if (Mode == EYITransformMode::HybridInlineThenTransformer)
		{
			CopyAffixProperties(InlineAffix, FinalAffix);
			CopyAffixProperties(TransformerAffix, FinalAffix);
		}
		else
		{
			CopyAffixProperties(TransformerAffix, FinalAffix);
			CopyAffixProperties(InlineAffix, FinalAffix);
		}
	}
	else
	{
		FinalAffix = InlineAffix ? InlineAffix : TransformerAffix;
	}

	if (!FinalAffix)
	{
		return false;
	}

	FName AssetNameField = Source->AssetNameFieldName.IsNone() ? TEXT("AssetName") : Source->AssetNameFieldName;
	FName PackagePathField = Source->PackagePathFieldName.IsNone() ? TEXT("PackagePath") : Source->PackagePathFieldName;

	FString AssetName = GetRowString(Table->RowStruct, RowPtr, AssetNameField);
	FString PackagePath = GetRowString(Table->RowStruct, RowPtr, PackagePathField);
	if (AssetName.IsEmpty())
	{
		AssetName = RowName.IsNone() ? FString::Printf(TEXT("Affix_%lld"), (long long)Code) : RowName.ToString();
	}
	if (PackagePath.IsEmpty())
	{
		PackagePath = TEXT("/Game/YOLOInventory/Affixes/Generated");
	}

	TSoftObjectPtr<UYIAffixAsset> ExistingSoft;
	if (ExistingByCode)
	{
		if (TSoftObjectPtr<UYIAffixAsset>* Found = ExistingByCode->Find(Code))
		{
			ExistingSoft = *Found;
		}
	}
	else
	{
		TMap<int64, TSoftObjectPtr<UYIAffixAsset>> LocalMap;
		CacheExistingAffixesByCode(LocalMap);
		if (TSoftObjectPtr<UYIAffixAsset>* Found = LocalMap.Find(Code))
		{
			ExistingSoft = *Found;
		}
	}

	UYIAffixAsset* Existing = ExistingSoft.IsValid() ? ExistingSoft.Get() : ExistingSoft.LoadSynchronous();
	if (!Existing)
	{
		const FString AssetLongPath = PackagePath / AssetName;
		const FString SanitizedPackage = UPackageTools::SanitizePackageName(AssetLongPath);
		const FString ObjectPath = SanitizedPackage + TEXT(".") + AssetName;
		Existing = Cast<UYIAffixAsset>(StaticLoadObject(UYIAffixAsset::StaticClass(), nullptr, *ObjectPath));
	}

	if (Existing)
	{
		Existing->Modify();
		CopyAffixProperties(FinalAffix, Existing);
		Existing->SourceDataSource = Source;
		Existing->SourceRowName = RowName;
		Existing->bGeneratedFromDataSource = true;
		Existing->MarkPackageDirty();
		if (ExistingByCode)
		{
			ExistingByCode->FindOrAdd(Code) = TSoftObjectPtr<UYIAffixAsset>(Existing);
		}
		return true;
	}

	const FString AssetLongPath = PackagePath / AssetName;
	const FString SanitizedPackage = UPackageTools::SanitizePackageName(AssetLongPath);
	UPackage* Pkg = CreatePackage(*SanitizedPackage);
	UObject* NewAsset = NewObject<UObject>(Pkg, FinalAffix->GetClass(), *AssetName, RF_Public | RF_Standalone | RF_Transactional, FinalAffix);
	if (!NewAsset)
	{
		return false;
	}

	if (UYIAffixAsset* NewAffix = Cast<UYIAffixAsset>(NewAsset))
	{
		NewAffix->SourceDataSource = Source;
		NewAffix->SourceRowName = RowName;
		NewAffix->bGeneratedFromDataSource = true;
		if (ExistingByCode)
		{
			ExistingByCode->FindOrAdd(Code) = TSoftObjectPtr<UYIAffixAsset>(NewAffix);
		}
	}

	FAssetRegistryModule::AssetCreated(NewAsset);
	Pkg->MarkPackageDirty();
	return true;
}

bool SYIAffixDashboard::SyncTargetPoolsForSource(const UYIDataTableAffixSource* Source)
{
	if (!Source || !Source->bAutoSyncTargetPools)
	{
		return false;
	}

	UYIAffixPoolAsset* PrefixPool = Source->PrefixTargetPool.IsValid() ? Source->PrefixTargetPool.Get() : Source->PrefixTargetPool.LoadSynchronous();
	UYIAffixPoolAsset* SuffixPool = Source->SuffixTargetPool.IsValid() ? Source->SuffixTargetPool.Get() : Source->SuffixTargetPool.LoadSynchronous();
	UYIAffixPoolAsset* ImplicitPool = Source->ImplicitTargetPool.IsValid() ? Source->ImplicitTargetPool.Get() : Source->ImplicitTargetPool.LoadSynchronous();
	if (!PrefixPool && !SuffixPool && !ImplicitPool)
	{
		return false;
	}

	TArray<UYIAffixAsset*> SourceAffixes;
	YIAffixDash_CollectAffixesForSource(Source, SourceAffixes);

	TMap<FSoftObjectPath, UYIAffixAsset*> AffixByPath;
	TSet<FSoftObjectPath> PrefixPaths;
	TSet<FSoftObjectPath> SuffixPaths;
	TSet<FSoftObjectPath> ImplicitPaths;
	for (UYIAffixAsset* Affix : SourceAffixes)
	{
		if (!Affix)
		{
			continue;
		}
		const FSoftObjectPath AffixPath(Affix);
		AffixByPath.FindOrAdd(AffixPath) = Affix;
		switch (Affix->Kind)
		{
		case EYIAffixKind::Prefix:
			PrefixPaths.Add(AffixPath);
			break;
		case EYIAffixKind::Suffix:
			SuffixPaths.Add(AffixPath);
			break;
		case EYIAffixKind::Implicit:
			ImplicitPaths.Add(AffixPath);
			break;
		default:
			break;
		}
	}

	bool bAnyChanged = false;
	auto SyncPool = [&](UYIAffixPoolAsset* Pool, const TSet<FSoftObjectPath>& WantedPaths, const TCHAR* PoolLabel)
	{
		if (!Pool)
		{
			return;
		}

		bool bPoolChanged = false;
		int32 AddedCount = 0;
		int32 RemovedCount = 0;
		Pool->Modify();

		TSet<FSoftObjectPath> ExistingPaths;
		for (const FYIAffixPoolEntry& Entry : Pool->Entries)
		{
			if (Entry.Affix.ToSoftObjectPath().IsValid())
			{
				ExistingPaths.Add(Entry.Affix.ToSoftObjectPath());
			}
		}

		for (const FSoftObjectPath& WantedPath : WantedPaths)
		{
			if (ExistingPaths.Contains(WantedPath))
			{
				continue;
			}

			FYIAffixPoolEntry NewEntry;
			NewEntry.Affix = TSoftObjectPtr<UYIAffixAsset>(WantedPath);
			if (UYIAffixAsset* const* FoundAffix = AffixByPath.Find(WantedPath))
			{
				if (const UYIAffixAsset* Affix = *FoundAffix)
				{
					NewEntry.Weight = FMath::Max(0.001f, Affix->Weight);
					NewEntry.MinQuality = EYIAffixQuality::Common;
				}
			}

			Pool->Entries.Add(NewEntry);
			++AddedCount;
			bPoolChanged = true;
		}

		if (Source->bPruneMissingRowsFromTargetPools)
		{
			RemovedCount = Pool->Entries.RemoveAll([&](const FYIAffixPoolEntry& Entry)
			{
				UYIAffixAsset* EntryAffix = Entry.Affix.IsValid() ? Entry.Affix.Get() : Entry.Affix.LoadSynchronous();
				if (!EntryAffix)
				{
					return false;
				}
				if (!YIAffixDash_IsAffixFromSource(EntryAffix, Source))
				{
					return false;
				}
				return !WantedPaths.Contains(Entry.Affix.ToSoftObjectPath());
			});
			bPoolChanged |= (RemovedCount > 0);
		}

		if (bPoolChanged)
		{
			Pool->MarkPackageDirty();
			bAnyChanged = true;
			FYIEditorMessageLog::Add(
				EYIEditorLogSeverity::Info,
				FText::Format(
					NSLOCTEXT("YOLOInventory", "AffixDash_PoolSyncResult", "{0} synced. Added: {1}, Removed: {2}"),
					FText::FromString(PoolLabel),
					FText::AsNumber(AddedCount),
					FText::AsNumber(RemovedCount)),
				FText::FromString(Pool->GetPathName()));
		}
	};

	SyncPool(PrefixPool, PrefixPaths, TEXT("Prefix pool"));
	SyncPool(SuffixPool, SuffixPaths, TEXT("Suffix pool"));
	SyncPool(ImplicitPool, ImplicitPaths, TEXT("Implicit pool"));

	return bAnyChanged;
}

FReply SYIAffixDashboard::ImportFromSource()
{
	UYIDataTableAffixSource* Source = ResolveCurrentSource();
	if (!Source)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error,
			NSLOCTEXT("YOLOInventory","AffixDash_NoSource","Import failed: no affix data source selected."));
		return FReply::Handled();
	}

	UDataTable* Table = Source->ResolveDataTable();
	if (!Table || !Table->RowStruct)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error,
			NSLOCTEXT("YOLOInventory","AffixDash_NoTable","Import failed: data table missing or invalid."));
		return FReply::Handled();
	}

	const FName CodeField = Source->UniqueCodeFieldName.IsNone() ? TEXT("UniqueCode") : Source->UniqueCodeFieldName;
	int32 Succeeded = 0;
	int32 Failed = 0;
	TMap<int64, TSoftObjectPtr<UYIAffixAsset>> ExistingByCode;
	CacheExistingAffixesByCode(ExistingByCode);

	for (const auto& RowPair : Table->GetRowMap())
	{
		const FName RowName = RowPair.Key;
		const uint8* RowPtr = RowPair.Value;
		const int64 Code = ExtractCodeFromRow(Table->RowStruct, RowPtr, CodeField);
		if (Code == 0)
		{
			FYIEditorMessageLog::Add(EYIEditorLogSeverity::Warning,
				NSLOCTEXT("YOLOInventory","AffixDash_RowNoCode","Row skipped: UniqueCode is 0."),
				FText::FromName(RowName));
			++Failed;
			continue;
		}

		const bool bOk = CreateOrUpdateAffixFromRow(Source, Table, RowName, RowPtr, Code, &ExistingByCode);
		if (bOk) ++Succeeded; else ++Failed;
	}

	FYIEditorMessageLog::Add(Failed > 0 ? EYIEditorLogSeverity::Warning : EYIEditorLogSeverity::Info,
		FText::Format(NSLOCTEXT("YOLOInventory","AffixDash_ImportDone","Affix import complete. Success: {0}, Failed: {1}"),
			FText::AsNumber(Succeeded), FText::AsNumber(Failed)));

	SyncTargetPoolsForSource(Source);

	RefreshList();
	return FReply::Handled();
}

FReply SYIAffixDashboard::UpdateSelectedAffix()
{
	UYIAffixAsset* Affix = LastSelectedAsset.IsValid() ? Cast<UYIAffixAsset>(LastSelectedAsset.Get()) : nullptr;
	if (!Affix)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Warning,
			NSLOCTEXT("YOLOInventory","AffixDash_NoSelectedAffix","Update failed: select an affix asset first."));
		return FReply::Handled();
	}

	UYIDataTableAffixSource* Source = Affix->SourceDataSource.LoadSynchronous();
	if (!Source || Affix->SourceRowName.IsNone())
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Warning,
			NSLOCTEXT("YOLOInventory","AffixDash_NoLinkedSource","Update failed: affix has no linked data source/row."));
		return FReply::Handled();
	}

	UDataTable* Table = Source->ResolveDataTable();
	if (!Table || !Table->RowStruct)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error,
			NSLOCTEXT("YOLOInventory","AffixDash_UpdateNoTable","Update failed: data table missing or invalid."));
		return FReply::Handled();
	}

	const uint8* const* Found = Table->GetRowMap().Find(Affix->SourceRowName);
	if (!Found || !*Found)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error,
			NSLOCTEXT("YOLOInventory","AffixDash_UpdateNoRow","Update failed: row not found in data table."),
			FText::FromName(Affix->SourceRowName));
		return FReply::Handled();
	}

	const int64 Code = Affix->UniqueCode;
	if (Code == 0)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error,
			NSLOCTEXT("YOLOInventory","AffixDash_UpdateNoCode","Update failed: affix UniqueCode is 0."));
		return FReply::Handled();
	}

	const bool bOk = CreateOrUpdateAffixFromRow(Source, Table, Affix->SourceRowName, *Found, Code, nullptr);
	if (bOk)
	{
		SyncTargetPoolsForSource(Source);
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Info,
			NSLOCTEXT("YOLOInventory","AffixDash_UpdateOk","Affix updated from data source."));
		RefreshList();
	}
	else
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Warning,
			NSLOCTEXT("YOLOInventory","AffixDash_UpdateFailed","Affix update failed."));
	}
	return FReply::Handled();
}

TSharedRef<SWidget> SYIAffixDashboard::BuildMappingPanelWidget()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		.Visibility_Lambda([this]() { return CurrentSource.IsValid() ? EVisibility::Visible : EVisibility::Collapsed; })
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(8)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "AffixDash_InlineMappings", "Inline Mappings (Affixes)"))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
						[
							SNew(SComboBox<TSharedPtr<FString>>)
								.OptionsSource(&const_cast<SYIAffixDashboard*>(this)->TargetPropertyOptions)
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
										TargetPropertyOptions.Add(MakeShared<FString>(TEXT("Transformer then Inline")));
									})
								.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewItem, ESelectInfo::Type)
									{
										if (!CurrentSource.IsValid() || !NewItem.IsValid()) return;
										CurrentSource->Modify();
										if (*NewItem == TEXT("Inline Only")) CurrentSource->TransformMode = EYITransformMode::InlineOnly;
										else if (*NewItem == TEXT("Transformer Only")) CurrentSource->TransformMode = EYITransformMode::TransformerOnly;
										else if (*NewItem == TEXT("Inline then Transformer")) CurrentSource->TransformMode = EYITransformMode::HybridInlineThenTransformer;
										else CurrentSource->TransformMode = EYITransformMode::HybridTransformerThenInline;
									})
								.Content()
								[
									SNew(STextBlock).Text_Lambda([this]()
										{
											if (!CurrentSource.IsValid()) return FText::FromString(TEXT("Mode"));
											switch (CurrentSource->TransformMode)
											{
											case EYITransformMode::InlineOnly: return FText::FromString(TEXT("Inline Only"));
											case EYITransformMode::TransformerOnly: return FText::FromString(TEXT("Transformer Only"));
											case EYITransformMode::HybridInlineThenTransformer: return FText::FromString(TEXT("Inline then Transformer"));
											case EYITransformMode::HybridTransformerThenInline: return FText::FromString(TEXT("Transformer then Inline"));
											default: return FText::FromString(TEXT("Inline then Transformer"));
											}
									})
								]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
						[
							SNew(SCheckBox)
								.IsChecked_Lambda([this]()
									{
										return (CurrentSource.IsValid() && CurrentSource->bUseInlineMappings) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
									})
								.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
									{
										if (CurrentSource.IsValid())
										{
											CurrentSource->Modify();
											CurrentSource->bUseInlineMappings = (State == ECheckBoxState::Checked);
										}
									})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
						[
							SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "AffixDash_AddMapping", "Add Mapping"))
								.OnClicked_Lambda([this]()
									{
										if (CurrentSource.IsValid())
										{
											CurrentSource->Modify();
											FYIFieldMapping NewMap;
											CurrentSource->InlineMappings.Add(NewMap);
											RefreshInlineMappingEditor(CurrentSource.Get());
										}
										return FReply::Handled();
									})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
						[
							SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "AffixDash_AutoMatch", "Auto Match"))
								.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_AutoMatch_TT", "Match fields by name and update missing source fields."))
								.OnClicked_Lambda([this]()
									{
										AutoMatchInlineMappings(false);
										return FReply::Handled();
									})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
						[
							SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "AffixDash_AddAllFields", "Add All Fields"))
								.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_AddAllFields_TT", "Add mapping rows for every affix field, and auto-match where possible."))
								.OnClicked_Lambda([this]()
									{
										AutoMatchInlineMappings(true);
										return FReply::Handled();
									})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
						[
							SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "AffixDash_ClearMapping", "Clear"))
								.OnClicked_Lambda([this]()
									{
										if (CurrentSource.IsValid())
										{
											CurrentSource->Modify();
											CurrentSource->InlineMappings.Reset();
											RefreshInlineMappingEditor(CurrentSource.Get());
										}
										return FReply::Handled();
									})
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(8, 0)
						[
							SNew(STextBlock)
								.Text_Lambda([this]()
									{
										if (!CurrentSource.IsValid())
										{
											return NSLOCTEXT("YOLOInventory", "AffixDash_Confidence_NoSource", "Confidence: N/A");
										}
										const int32 Total = CurrentSource->InlineMappings.Num();
										int32 Ready = 0;
										int32 NeedsFix = 0;
										for (const FYIFieldMapping& M : CurrentSource->InlineMappings)
										{
											const bool bHasSource = !M.SourceField.IsNone();
											const bool bHasTarget = !M.TargetProperty.IsNone();
											if (bHasSource && bHasTarget) { ++Ready; }
											else { ++NeedsFix; }
										}
										return FText::Format(NSLOCTEXT("YOLOInventory", "AffixDash_ConfidenceFmt", "Confidence: Ready {0}/{1}, Needs Fix {2}"),
											FText::AsNumber(Ready), FText::AsNumber(Total), FText::AsNumber(NeedsFix));
									})
						]
				]
				+ SVerticalBox::Slot().FillHeight(1.f).Padding(8, 6)
				[
					SNew(SSplitter)
						.Orientation(Orient_Vertical)
						+ SSplitter::Slot().Value(0.62f)
						[
							SAssignNew(MappingListView, SListView<TSharedPtr<FYIFieldMapping>>)
								.ListItemsSource(&MappingRows)
								.OnGenerateRow(this, &SYIAffixDashboard::MakeMappingRow)
								.SelectionMode(ESelectionMode::Single)
						]
						+ SSplitter::Slot().Value(0.38f)
						[
							BuildPreviewPanelWidget()
						]
				]
		];
}

TSharedRef<SWidget> SYIAffixDashboard::BuildPreviewPanelWidget()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(8)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "AffixDash_PreviewTitle", "Live Preview"))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0)
						[
							SNew(SComboBox<TSharedPtr<FString>>)
								.OptionsSource(&const_cast<SYIAffixDashboard*>(this)->PreviewRowOptions)
								.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
									{
										return SNew(STextBlock).Text(InItem.IsValid() ? FText::FromString(*InItem) : FText::GetEmpty());
									})
								.OnComboBoxOpening_Lambda([this]()
									{
										PreviewRowOptions.Reset();
										if (CurrentSource.IsValid())
										{
											if (UDataTable* Table = CurrentSource->DataTable.LoadSynchronous())
											{
												TArray<FName> RowNames = Table->GetRowNames();
												RowNames.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
												for (const FName& RowName : RowNames)
												{
													PreviewRowOptions.Add(MakeShared<FString>(RowName.ToString()));
												}
											}
										}
									})
								.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewItem, ESelectInfo::Type)
									{
										if (NewItem.IsValid())
										{
											PreviewRowName = FName(**NewItem);
											RefreshMappingPreview();
										}
									})
								.Content()
								[
									SNew(STextBlock).Text_Lambda([this]()
										{
											return PreviewRowName.IsNone() ? NSLOCTEXT("YOLOInventory", "AffixDash_PreviewRow", "Select Row") : FText::FromName(PreviewRowName);
										})
								]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0)
						[
							SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "AffixDash_PreviewRefresh", "Refresh"))
								.OnClicked_Lambda([this]()
									{
										RefreshMappingPreview();
										return FReply::Handled();
									})
						]
				]
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SAssignNew(MappingPreviewListView, SListView<TSharedPtr<FYIMappingPreviewRow>>)
						.ListItemsSource(&MappingPreviewRows)
						.OnGenerateRow(this, &SYIAffixDashboard::MakePreviewRow)
				]
		];
}

void SYIAffixDashboard::RefreshInlineMappingEditor(UYIDataTableAffixSource* Source)
{
	MappingRows.Reset();
	SourceFieldOptions.Reset();
	TargetPropertyOptions.Reset();
	ConverterOptions.Reset();
	SourceFieldPropCache.Reset();
	TargetFieldPropCache.Reset();
	TransformFunctionOptions.Reset();

	if (!Source)
	{
		if (MappingListView.IsValid())
		{
			MappingListView->RequestListRefresh();
		}
		if (MappingPreviewListView.IsValid())
		{
			MappingPreviewListView->RequestListRefresh();
		}
		return;
	}

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
	for (TFieldIterator<FProperty> It(UYIAffixAsset::StaticClass()); It; ++It)
	{
		const UClass* OwnerClass = It->GetOwnerClass();
		if (OwnerClass == UObject::StaticClass())
		{
			continue;
		}
		const FName FieldName((*It)->GetAuthoredName());
		TargetPropertyOptions.Add(MakeShared<FString>(FieldName.ToString()));
		TargetFieldPropCache.Add(FieldName, *It);
	}

	for (const FYIFieldMapping& M : Source->InlineMappings)
	{
		MappingRows.Add(MakeShared<FYIFieldMapping>(M));
	}

	BuildTransformFunctionOptions();
	RefreshMappingPreview();

	if (MappingListView.IsValid())
	{
		MappingListView->RequestListRefresh();
	}
	if (MappingPreviewListView.IsValid())
	{
		MappingPreviewListView->RequestListRefresh();
	}
}

void SYIAffixDashboard::BuildTransformFunctionOptions()
{
	TransformFunctionOptions.Reset();
	TSharedPtr<FYITransformFunctionInfo> NoneOption = MakeShared<FYITransformFunctionInfo>();
	NoneOption->DisplayName = TEXT("None");
	NoneOption->FunctionName = NAME_None;
	TransformFunctionOptions.Add(NoneOption);

	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (!Class || !Class->IsChildOf(UBlueprintFunctionLibrary::StaticClass()))
		{
			continue;
		}

		for (TFieldIterator<UFunction> FuncIt(Class, EFieldIteratorFlags::IncludeSuper); FuncIt; ++FuncIt)
		{
			UFunction* Function = *FuncIt;
			if (!Function || !Function->HasMetaData(TEXT("YIInlineTransform")))
			{
				continue;
			}

			TSharedPtr<FYITransformFunctionInfo> Info = MakeShared<FYITransformFunctionInfo>();
			Info->Library = Class;
			Info->FunctionName = Function->GetFName();
			Info->DisplayName = FString::Printf(TEXT("%s::%s"), *Class->GetName(), *Function->GetName());
			TransformFunctionOptions.Add(Info);
		}
	}

	if (TransformFunctionOptions.Num() > 1)
	{
		TransformFunctionOptions.Sort([](const TSharedPtr<FYITransformFunctionInfo>& A, const TSharedPtr<FYITransformFunctionInfo>& B)
			{
				if (!A.IsValid() || !B.IsValid()) return false;
				if (A->FunctionName.IsNone()) return true;
				if (B->FunctionName.IsNone()) return false;
				return A->DisplayName < B->DisplayName;
			});
	}
}

void SYIAffixDashboard::RefreshMappingPreview()
{
	MappingPreviewRows.Reset();
	if (!CurrentSource.IsValid())
	{
		return;
	}

	UYIDataTableAffixSource* Source = CurrentSource.Get();
	UDataTable* Table = Source ? Source->DataTable.LoadSynchronous() : nullptr;
	if (!Table || !Table->RowStruct)
	{
		return;
	}

	if (PreviewRowName.IsNone())
	{
		TArray<FName> RowNames = Table->GetRowNames();
		RowNames.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
		if (RowNames.Num() > 0)
		{
			PreviewRowName = RowNames[0];
		}
	}
	if (PreviewRowName.IsNone())
	{
		return;
	}

	const uint8* const* FoundRow = Table->GetRowMap().Find(PreviewRowName);
	const uint8* RowPtr = FoundRow ? *FoundRow : nullptr;
	if (!RowPtr)
	{
		return;
	}

	for (const FYIFieldMapping& Mapping : Source->InlineMappings)
	{
		TSharedPtr<FYIMappingPreviewRow> Row = MakeShared<FYIMappingPreviewRow>();
		Row->SourceField = Mapping.SourceField;
		Row->TargetProperty = Mapping.TargetProperty;

		const FProperty* SourceProp = Mapping.SourceField.IsNone() ? nullptr : SourceFieldPropCache.FindRef(Mapping.SourceField);
		FProperty* TargetProp = Mapping.TargetProperty.IsNone() ? nullptr : TargetFieldPropCache.FindRef(Mapping.TargetProperty);

		if (!TargetProp || (!SourceProp && !Mapping.bUseStaticValue))
		{
			Row->Status = NSLOCTEXT("YOLOInventory", "AffixDash_PreviewMissing", "Missing field.");
			Row->StatusColor = FLinearColor(1.f, 0.25f, 0.2f);
			MappingPreviewRows.Add(Row);
			continue;
		}

		const uint8* SrcPtr = SourceProp ? SourceProp->ContainerPtrToValuePtr<uint8>(RowPtr) : nullptr;
		Row->SourceValue = Mapping.bUseStaticValue
			? Mapping.StaticValue
			: ExportPropertyValueToStringAffix(SourceProp, SrcPtr);

		TArray<uint8> Temp;
		Temp.SetNumZeroed(TargetProp->GetSize());
		TargetProp->InitializeValue(Temp.GetData());

		bool bWarn = false;
		bool bConverted = false;

		if (Mapping.bUseStaticValue)
		{
			if (Mapping.Conversion == EYIFieldMappingConversion::Vector2DFromXY)
			{
				Row->ConvertedValue = TEXT("<static value does not support Vector2D conversion>");
				bWarn = true;
			}
			else
			{
				bConverted = SetPropertyValueFromStringAffix(Mapping.StaticValue, TargetProp, Temp.GetData(), Mapping.Conversion);
			}
		}
		else if (Mapping.Conversion == EYIFieldMappingConversion::Vector2DFromXY)
		{
			if (Mapping.SourceFieldB.IsNone())
			{
				Row->ConvertedValue = TEXT("<missing source B>");
				bWarn = true;
			}
			else
			{
				const FProperty* SourcePropB = SourceFieldPropCache.FindRef(Mapping.SourceFieldB);
				const FNumericProperty* NumA = CastField<FNumericProperty>(SourceProp);
				const FNumericProperty* NumB = CastField<FNumericProperty>(SourcePropB);
				if (!SourcePropB || !NumA || !NumB)
				{
					Row->ConvertedValue = TEXT("<vector2d conversion failed>");
					bWarn = true;
				}
				else if (const FStructProperty* DestStruct = CastField<FStructProperty>(TargetProp))
				{
					const uint8* SrcPtrB = SourcePropB->ContainerPtrToValuePtr<uint8>(RowPtr);
					const double X = NumA->IsFloatingPoint() ? NumA->GetFloatingPointPropertyValue(SrcPtr) : (double)NumA->GetSignedIntPropertyValue(SrcPtr);
					const double Y = NumB->IsFloatingPoint() ? NumB->GetFloatingPointPropertyValue(SrcPtrB) : (double)NumB->GetSignedIntPropertyValue(SrcPtrB);

					if (DestStruct->Struct == TBaseStructure<FVector2D>::Get())
					{
						FVector2D* Vec = reinterpret_cast<FVector2D*>(Temp.GetData());
						*Vec = FVector2D((float)X, (float)Y);
						bConverted = true;
						Row->ConvertedValue = ExportPropertyValueToStringAffix(TargetProp, Temp.GetData());
					}
					else if (DestStruct->Struct == TBaseStructure<FIntPoint>::Get())
					{
						FIntPoint* Pt = reinterpret_cast<FIntPoint*>(Temp.GetData());
						*Pt = FIntPoint((int32)X, (int32)Y);
						bConverted = true;
						Row->ConvertedValue = ExportPropertyValueToStringAffix(TargetProp, Temp.GetData());
					}
					else
					{
						Row->ConvertedValue = TEXT("<vector2d target unsupported>");
						bWarn = true;
					}
				}
				else
				{
					Row->ConvertedValue = TEXT("<vector2d target unsupported>");
					bWarn = true;
				}
			}
		}
		else
		{
			bConverted = CopyValueBetweenPropertiesAffix(SourceProp, SrcPtr, TargetProp, Temp.GetData(), Mapping.Conversion);
		}

		if (bConverted)
		{
			Row->ConvertedValue = ExportPropertyValueToStringAffix(TargetProp, Temp.GetData());
		}
		else if (Row->ConvertedValue.IsEmpty())
		{
			Row->ConvertedValue = TEXT("<conversion failed>");
			bWarn = true;
		}

		if (!Mapping.TransformFunction.IsNone())
		{
			FString TransformError;
			const bool bTransformed = ApplyTransformFunctionAffix(Mapping, TargetProp, Temp.GetData(), &TransformError);
			if (bTransformed)
			{
				Row->TransformedValue = ExportPropertyValueToStringAffix(TargetProp, Temp.GetData());
			}
			else
			{
				Row->TransformedValue = FString::Printf(TEXT("<transform failed: %s>"), *TransformError);
				bWarn = true;
			}
		}
		else
		{
			Row->TransformedValue = Row->ConvertedValue;
		}

		TargetProp->DestroyValue(Temp.GetData());

		if (bWarn)
		{
			Row->Status = NSLOCTEXT("YOLOInventory", "AffixDash_PreviewWarn", "Warn");
			Row->StatusColor = FLinearColor(1.f, 0.75f, 0.2f);
		}
		else
		{
			Row->Status = NSLOCTEXT("YOLOInventory", "AffixDash_PreviewOk", "OK");
			Row->StatusColor = FLinearColor(0.2f, 0.8f, 0.4f);
		}
		MappingPreviewRows.Add(Row);
	}

	if (MappingPreviewListView.IsValid())
	{
		MappingPreviewListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SYIAffixDashboard::MakePreviewRow(TSharedPtr<FYIMappingPreviewRow> Row, const TSharedRef<STableViewBase>& Owner)
{
	return SNew(STableRow<TSharedPtr<FYIMappingPreviewRow>>, Owner)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
				[
					SNew(STextBlock)
						.Text(Row.IsValid() ? Row->Status : FText::GetEmpty())
						.ColorAndOpacity(Row.IsValid() ? Row->StatusColor : FLinearColor::Gray)
				]
				+ SHorizontalBox::Slot().FillWidth(0.2f).Padding(4, 0)
				[
					SNew(STextBlock).Text(Row.IsValid() ? FText::FromName(Row->SourceField) : FText::GetEmpty())
				]
				+ SHorizontalBox::Slot().FillWidth(0.2f).Padding(4, 0)
				[
					SNew(STextBlock).Text(Row.IsValid() ? FText::FromName(Row->TargetProperty) : FText::GetEmpty())
				]
				+ SHorizontalBox::Slot().FillWidth(0.2f).Padding(4, 0)
				[
					SNew(STextBlock).Text(Row.IsValid() ? FText::FromString(Row->SourceValue) : FText::GetEmpty())
				]
				+ SHorizontalBox::Slot().FillWidth(0.2f).Padding(4, 0)
				[
					SNew(STextBlock).Text(Row.IsValid() ? FText::FromString(Row->ConvertedValue) : FText::GetEmpty())
				]
				+ SHorizontalBox::Slot().FillWidth(0.2f).Padding(4, 0)
				[
					SNew(STextBlock).Text(Row.IsValid() ? FText::FromString(Row->TransformedValue) : FText::GetEmpty())
				]
		];
}

void SYIAffixDashboard::AutoMatchInlineMappings(bool bAddAllFields)
{
	if (!CurrentSource.IsValid())
	{
		return;
	}

	UYIDataTableAffixSource* Source = CurrentSource.Get();
	if (!Source)
	{
		return;
	}

	UDataTable* Table = Source->DataTable.LoadSynchronous();
	if (!Table || !Table->RowStruct)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error,
			NSLOCTEXT("YOLOInventory", "AffixDash_AutoMatch_NoTable", "Auto-match failed: data table missing or no row struct."),
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
	struct FSourceCandidate { FName Name; FString Lower; FString Norm; };
	TArray<FSourceCandidate> SourceCandidates;

	for (TFieldIterator<FProperty> It(Table->RowStruct); It; ++It)
	{
		const FName FieldName((*It)->GetAuthoredName());
		const FString NameStr = FieldName.ToString();
		const FString Lower = NameStr.ToLower();
		const FString Norm = NormalizeField(NameStr);
		SourceByNorm.Add(Norm, FieldName);
		SourceByLower.Add(Lower, FieldName);
		SourceCandidates.Add({ FieldName, Lower, Norm });
	}

	TSet<FName> ExistingTargets;
	for (const FYIFieldMapping& M : Source->InlineMappings)
	{
		if (!M.TargetProperty.IsNone())
		{
			ExistingTargets.Add(M.TargetProperty);
		}
	}

	auto FindBestMatch = [&](const FName& TargetField)->FName
	{
		if (TargetField.IsNone())
		{
			return NAME_None;
		}
		const FString TargetStr = TargetField.ToString();
		const FString TargetLower = TargetStr.ToLower();
		const FString TargetNorm = NormalizeField(TargetStr);
		if (TargetLower.IsEmpty() || TargetNorm.IsEmpty())
		{
			return NAME_None;
		}

		if (const FName* Found = SourceByLower.Find(TargetLower)) return *Found;
		if (const FName* FoundNorm = SourceByNorm.Find(TargetNorm)) return *FoundNorm;

		int32 BestScore = 0;
		FName BestMatch = NAME_None;
		for (const FSourceCandidate& C : SourceCandidates)
		{
			int32 Score = 0;
			if (C.Norm == TargetNorm)
			{
				Score = 100;
			}
			else if (C.Norm.StartsWith(TargetNorm) || C.Norm.EndsWith(TargetNorm))
			{
				const int32 MinLen = FMath::Min(C.Norm.Len(), TargetNorm.Len());
				const int32 MaxLen = FMath::Max(C.Norm.Len(), TargetNorm.Len());
				Score = 85 + (MaxLen > 0 ? (MinLen * 10 / MaxLen) : 0);
			}
			else if (C.Norm.Contains(TargetNorm) || TargetNorm.Contains(C.Norm))
			{
				const int32 MinLen = FMath::Min(C.Norm.Len(), TargetNorm.Len());
				const int32 MaxLen = FMath::Max(C.Norm.Len(), TargetNorm.Len());
				Score = 70 + (MaxLen > 0 ? (MinLen * 10 / MaxLen) : 0);
			}

			if (Score > BestScore)
			{
				BestScore = Score;
				BestMatch = C.Name;
			}
		}
		return BestMatch;
	};

	bool bChanged = false;
	int32 MatchCount = 0;

	for (FYIFieldMapping& M : Source->InlineMappings)
	{
		if (M.bUseStaticValue || M.TargetProperty.IsNone() || !M.SourceField.IsNone())
		{
			continue;
		}
		const FName Match = FindBestMatch(M.TargetProperty);
		if (!Match.IsNone())
		{
			M.SourceField = Match;
			const FProperty* SourceProp = SourceFieldPropCache.FindRef(Match);
			const FProperty* TargetProp = TargetFieldPropCache.FindRef(M.TargetProperty);
			M.Conversion = GuessConversionForPropsAffix(SourceProp, TargetProp);
			bChanged = true;
			++MatchCount;
		}
	}

	for (TFieldIterator<FProperty> It(UYIAffixAsset::StaticClass()); It; ++It)
	{
		const UClass* OwnerClass = It->GetOwnerClass();
		if (OwnerClass == UObject::StaticClass())
		{
			continue;
		}
		const FName TargetName((*It)->GetAuthoredName());
		if (ExistingTargets.Contains(TargetName))
		{
			continue;
		}
		const FName Match = FindBestMatch(TargetName);
		if (!bAddAllFields && Match.IsNone())
		{
			continue;
		}

		FYIFieldMapping NewMap;
		NewMap.TargetProperty = TargetName;
		NewMap.SourceField = Match;
		NewMap.Conversion = GuessConversionForPropsAffix(SourceFieldPropCache.FindRef(Match), *It);
		Source->InlineMappings.Add(NewMap);
		ExistingTargets.Add(TargetName);
		bChanged = true;
		if (!Match.IsNone())
		{
			++MatchCount;
		}
	}

	if (bChanged)
	{
		Source->Modify();
		RefreshInlineMappingEditor(Source);
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Info,
			FText::Format(NSLOCTEXT("YOLOInventory", "AffixDash_AutoMatch_Done", "Auto-match updated inline mappings. Matched {0} fields."), FText::AsNumber(MatchCount)),
			FText::FromString(Source->GetPathName()));
	}
}

TSharedRef<ITableRow> SYIAffixDashboard::MakeMappingRow(TSharedPtr<FYIFieldMapping> Mapping, const TSharedRef<STableViewBase>& OwnerTable)
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

	auto MakeTypeBadgeDynamic = [&](TFunction<FProperty*()> PropGetter)->TSharedRef<SWidget>
	{
		return SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.Padding(FMargin(4, 1))
			.BorderBackgroundColor_Lambda([PropGetter, GetTypeInfo]()
				{
					FString Label;
					FLinearColor Color;
					GetTypeInfo(PropGetter(), Label, Color);
					return Color;
				})
			[
				SNew(STextBlock)
					.Text_Lambda([PropGetter, GetTypeInfo]()
						{
							FString Label;
							FLinearColor Color;
							GetTypeInfo(PropGetter(), Label, Color);
							return FText::FromString(Label);
						})
					.ColorAndOpacity(FSlateColor(FLinearColor::Black))
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

	auto IsStaticMapping = [Mapping]() -> bool
	{
		return Mapping.IsValid() && Mapping->bUseStaticValue;
	};

	auto GetTargetEnum = [GetTargetProp]() -> UEnum*
	{
		const FProperty* TargetProp = GetTargetProp();
		if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(TargetProp))
		{
			return EnumProp->GetEnum();
		}
		if (const FByteProperty* ByteProp = CastField<FByteProperty>(TargetProp))
		{
			return ByteProp->Enum;
		}
		return nullptr;
	};

	TSharedPtr<TArray<TSharedPtr<FString>>> StaticEnumOptions = MakeShared<TArray<TSharedPtr<FString>>>();

	auto BuildStatusWidget = [this, Mapping, GetSourceProp, GetTargetProp]() -> TSharedRef<SWidget>
	{
		auto IsCompatible = [](const FProperty* A, const FProperty* B) -> bool
		{
			if (!A || !B)
			{
				return false;
			}
			const bool bTypesMatch = A->GetClass() == B->GetClass();
			const bool bBothNumeric = CastField<FNumericProperty>(A) && CastField<FNumericProperty>(B);
			const bool bBothTextish =
				(CastField<FStrProperty>(A) || CastField<FNameProperty>(A) || CastField<FTextProperty>(A)) &&
				(CastField<FStrProperty>(B) || CastField<FNameProperty>(B) || CastField<FTextProperty>(B));
			const bool bBoolNumeric = (CastField<FBoolProperty>(A) && CastField<FNumericProperty>(B)) ||
				(CastField<FNumericProperty>(A) && CastField<FBoolProperty>(B));
			return bTypesMatch || bBothNumeric || bBothTextish || bBoolNumeric;
		};

		if (!Mapping.IsValid())
		{
			return SNew(SSpacer);
		}
		const FProperty* SourceProp = GetSourceProp();
		const FProperty* TargetProp = GetTargetProp();
		const bool bStatic = Mapping->bUseStaticValue;
		if (!TargetProp || (!SourceProp && !bStatic))
		{
			return SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.Error"))
				.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_MappingMissing", "Missing source or target field."));
		}
		if (bStatic && Mapping->Conversion == EYIFieldMappingConversion::Vector2DFromXY)
		{
			return SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
				.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_MappingStaticVector", "Static value does not support Vector2D conversion."));
		}
		if (Mapping->Conversion == EYIFieldMappingConversion::Vector2DFromXY)
		{
			if (Mapping->SourceFieldB.IsNone())
			{
				return SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
					.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_MappingMissingB", "Vector2D conversion requires Source B."));
			}
			const FProperty* SourcePropB = nullptr;
			if (FProperty** Found = SourceFieldPropCache.Find(Mapping->SourceFieldB))
			{
				SourcePropB = *Found;
			}
			if (!SourcePropB)
			{
				return SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
					.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_MappingMissingBField", "Vector2D conversion: Source B field not found."));
			}
			if (!CastField<FNumericProperty>(SourceProp) || !CastField<FNumericProperty>(SourcePropB))
			{
				return SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
					.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_MappingVectorNumeric", "Vector2D conversion requires numeric fields."));
			}
			if (const FStructProperty* DestStruct = CastField<FStructProperty>(TargetProp))
			{
				if (DestStruct->Struct == TBaseStructure<FVector2D>::Get() || DestStruct->Struct == TBaseStructure<FIntPoint>::Get())
				{
					return SNew(SImage)
						.Image(FAppStyle::Get().GetBrush("Icons.Check"))
						.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_MappingOk", "Mapping looks OK."));
				}
			}
			return SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
				.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_MappingVectorTarget", "Vector2D conversion target must be FVector2D or FIntPoint."));
		}
		const FProperty* CompatSource = bStatic ? TargetProp : SourceProp;
		const bool bTypesMatch = CompatSource->GetClass() == TargetProp->GetClass();
		const bool bBothNumeric = CastField<FNumericProperty>(CompatSource) && CastField<FNumericProperty>(TargetProp);
		const bool bBothTextish =
			(CastField<FStrProperty>(CompatSource) || CastField<FNameProperty>(CompatSource) || CastField<FTextProperty>(CompatSource)) &&
			(CastField<FStrProperty>(TargetProp) || CastField<FNameProperty>(TargetProp) || CastField<FTextProperty>(TargetProp));
		const bool bCompatible = bTypesMatch || bBothNumeric || bBothTextish;
		if (!bCompatible && Mapping->Conversion == EYIFieldMappingConversion::None && !bStatic)
		{
			return SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
				.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_MappingTypeMismatch", "Type mismatch. Add a conversion."));
		}
		if (Mapping->Conversion == EYIFieldMappingConversion::ToEnum)
		{
			if (!(CastField<FEnumProperty>(TargetProp) || (CastField<FByteProperty>(TargetProp) && CastField<FByteProperty>(TargetProp)->Enum)))
			{
				return SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
					.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_MappingEnumTarget", "Enum conversion requires an enum target."));
			}
		}

		if (Mapping->TransformFunction.IsNone())
		{
			return SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.Check"))
				.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_MappingOk", "Mapping looks OK."));
		}

		UClass* LibraryClass = Mapping->TransformLibrary.LoadSynchronous();
		if (!LibraryClass)
		{
			return SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
				.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_MappingTransformMissing", "Transform library not found."));
		}

		UFunction* Function = LibraryClass->FindFunctionByName(Mapping->TransformFunction);
		if (!Function)
		{
			return SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
				.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_MappingTransformFuncMissing", "Transform function not found."));
		}

		FProperty* InputProp = nullptr;
		FProperty* OutputProp = nullptr;
		if (!GetTransformFunctionPropsAffix(Function, InputProp, OutputProp))
		{
			return SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
				.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_MappingTransformSigBad", "Transform signature should be (In) -> Out or (In, Out)."));
		}

		if (!IsCompatible(TargetProp, InputProp))
		{
			return SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
				.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_MappingTransformInputBad", "Transform input type does not match target type."));
		}
		if (!IsCompatible(OutputProp, TargetProp))
		{
			return SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
				.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_MappingTransformOutputBad", "Transform output type does not match target type."));
		}

		return SNew(SImage)
			.Image(FAppStyle::Get().GetBrush("Icons.Check"))
			.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_MappingOk", "Mapping looks OK."));
	};

	return SNew(STableRow<TSharedPtr<FYIFieldMapping>>, OwnerTable)
	[
		SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(2).VAlign(VAlign_Center)
			[
				BuildStatusWidget()
			]
			+ SHorizontalBox::Slot().FillWidth(0.22f).Padding(2)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
					.IsEnabled_Lambda([IsStaticMapping]() { return !IsStaticMapping(); })
					.OptionsSource(&const_cast<SYIAffixDashboard*>(this)->SourceFieldOptions)
					.OnGenerateWidget_Lambda([this, DropdownText, GetTypeInfo](TSharedPtr<FString> InItem)
						{
							const FName FieldName = InItem.IsValid() ? FName(**InItem) : NAME_None;
							FString Label;
							FLinearColor Color;
							FProperty* Prop = nullptr;
							if (FieldName != NAME_None)
							{
								if (FProperty** Found = const_cast<SYIAffixDashboard*>(this)->SourceFieldPropCache.Find(FieldName))
								{
									Prop = *Found;
								}
							}
							GetTypeInfo(Prop, Label, Color);
							return SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
								[
									SNew(SBorder)
										.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
										.Padding(FMargin(3, 1))
										.BorderBackgroundColor(Color)
										[
											SNew(STextBlock).Text(FText::FromString(Label)).ColorAndOpacity(FSlateColor(FLinearColor::Black))
										]
								]
								+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4, 0)
								[
									SNew(STextBlock).Text(DropdownText(InItem))
								];
						})
					.OnSelectionChanged_Lambda([this, Mapping, GetSourceProp, GetTargetProp](TSharedPtr<FString> NewItem, ESelectInfo::Type)
						{
							if (CurrentSource.IsValid() && Mapping.IsValid() && NewItem.IsValid())
							{
								CurrentSource->Modify();
								Mapping->SourceField = FName(**NewItem);
								if (Mapping->Conversion == EYIFieldMappingConversion::None)
								{
									const FProperty* SourceProp = GetSourceProp();
									const FProperty* TargetProp = GetTargetProp();
									const EYIFieldMappingConversion Guess = GuessConversionForPropsAffix(SourceProp, TargetProp);
									Mapping->Conversion = Guess;
								}
								const int32 Index = MappingRows.Find(Mapping);
								if (Index != INDEX_NONE)
								{
									CurrentSource->InlineMappings[Index].SourceField = Mapping->SourceField;
									CurrentSource->InlineMappings[Index].Conversion = Mapping->Conversion;
								}
								RefreshMappingPreview();
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
							+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
							[
								MakeTypeBadgeDynamic(GetSourceProp)
							]
							+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4, 0)
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
			+ SHorizontalBox::Slot().FillWidth(0.08f).Padding(2)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
					.IsEnabled_Lambda([IsStaticMapping]() { return !IsStaticMapping(); })
					.Visibility_Lambda([Mapping]()
						{
							return (Mapping.IsValid() && Mapping->Conversion == EYIFieldMappingConversion::Vector2DFromXY) ? EVisibility::Visible : EVisibility::Collapsed;
						})
					.OptionsSource(&const_cast<SYIAffixDashboard*>(this)->SourceFieldOptions)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
						{
							return SNew(STextBlock).Text(InItem.IsValid() ? FText::FromString(*InItem) : FText::GetEmpty());
						})
					.OnSelectionChanged_Lambda([this, Mapping](TSharedPtr<FString> NewItem, ESelectInfo::Type)
						{
							if (CurrentSource.IsValid() && Mapping.IsValid() && NewItem.IsValid())
							{
								CurrentSource->Modify();
								Mapping->SourceFieldB = FName(**NewItem);
								const int32 Index = MappingRows.Find(Mapping);
								if (Index != INDEX_NONE)
								{
									CurrentSource->InlineMappings[Index].SourceFieldB = Mapping->SourceFieldB;
								}
								RefreshMappingPreview();
							}
						})
					.InitiallySelectedItem([this, Mapping]()
						{
							if (!Mapping.IsValid()) return TSharedPtr<FString>();
							if (const TSharedPtr<FString>* FoundPtr = SourceFieldOptions.FindByPredicate([Mapping](const TSharedPtr<FString>& Opt)
								{
									return Opt.IsValid() && FName(**Opt).IsEqual(Mapping->SourceFieldB);
								}))
							{
								return *FoundPtr;
							}
							return TSharedPtr<FString>();
						}())
					.Content()
					[
						SNew(STextBlock).Text_Lambda([Mapping]()
							{
								return Mapping.IsValid() && !Mapping->SourceFieldB.IsNone()
									? FText::FromName(Mapping->SourceFieldB)
									: NSLOCTEXT("YOLOInventory", "AffixDash_SourceFieldB", "Source B");
							})
					]
			]
			+ SHorizontalBox::Slot().FillWidth(0.16f).Padding(2)
			[
				SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(SCheckBox)
							.ToolTipText(NSLOCTEXT("YOLOInventory", "AffixDash_StaticValue_TT", "Use a static value instead of a source field."))
							.IsChecked_Lambda([IsStaticMapping]() { return IsStaticMapping() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
							.OnCheckStateChanged_Lambda([this, Mapping](ECheckBoxState State)
								{
									if (CurrentSource.IsValid() && Mapping.IsValid())
									{
										CurrentSource->Modify();
										Mapping->bUseStaticValue = (State == ECheckBoxState::Checked);
										const int32 Index = MappingRows.Find(Mapping);
										if (Index != INDEX_NONE)
										{
											CurrentSource->InlineMappings[Index].bUseStaticValue = Mapping->bUseStaticValue;
										}
										RefreshMappingPreview();
									}
								})
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4, 0)
					[
						SNew(SWidgetSwitcher)
							.WidgetIndex_Lambda([IsStaticMapping, GetTargetEnum]()
								{
									return (IsStaticMapping() && GetTargetEnum() != nullptr) ? 1 : 0;
								})
							+ SWidgetSwitcher::Slot()
							[
								SNew(SEditableTextBox)
									.IsEnabled_Lambda([IsStaticMapping, GetTargetEnum]()
										{
											return IsStaticMapping() && GetTargetEnum() == nullptr;
										})
									.Text_Lambda([Mapping]()
										{
											return Mapping.IsValid() ? FText::FromString(Mapping->StaticValue) : FText::GetEmpty();
										})
									.HintText(NSLOCTEXT("YOLOInventory", "AffixDash_StaticValueHint", "Static value"))
									.OnTextCommitted_Lambda([this, Mapping](const FText& NewText, ETextCommit::Type)
										{
											if (CurrentSource.IsValid() && Mapping.IsValid())
											{
												CurrentSource->Modify();
												Mapping->StaticValue = NewText.ToString();
												const int32 Index = MappingRows.Find(Mapping);
												if (Index != INDEX_NONE)
												{
													CurrentSource->InlineMappings[Index].StaticValue = Mapping->StaticValue;
												}
												RefreshMappingPreview();
											}
										})
							]
							+ SWidgetSwitcher::Slot()
							[
								SNew(SComboBox<TSharedPtr<FString>>)
									.OptionsSource(StaticEnumOptions.Get())
									.IsEnabled_Lambda([IsStaticMapping, GetTargetEnum]()
										{
											return IsStaticMapping() && GetTargetEnum() != nullptr;
										})
									.OnGenerateWidget_Lambda([GetTargetEnum](TSharedPtr<FString> InItem)
										{
											if (UEnum* Enum = GetTargetEnum())
											{
												if (InItem.IsValid())
												{
													int32 Index = Enum->GetIndexByNameString(*InItem);
													if (Index == INDEX_NONE)
													{
														Index = Enum->GetIndexByName(FName(**InItem));
													}
													if (Index != INDEX_NONE)
													{
														return SNew(STextBlock).Text(Enum->GetDisplayNameTextByIndex(Index));
													}
												}
											}
											return SNew(STextBlock).Text(InItem.IsValid() ? FText::FromString(*InItem) : FText::GetEmpty());
										})
									.OnComboBoxOpening_Lambda([StaticEnumOptions, GetTargetEnum]()
										{
											StaticEnumOptions->Reset();
											if (UEnum* Enum = GetTargetEnum())
											{
												const int32 Count = Enum->NumEnums();
												for (int32 Index = 0; Index < Count; ++Index)
												{
													if (Enum->HasMetaData(TEXT("Hidden"), Index))
													{
														continue;
													}
													const FString Name = Enum->GetNameStringByIndex(Index);
													if (Name.EndsWith(TEXT("_MAX")))
													{
														continue;
													}
													StaticEnumOptions->Add(MakeShared<FString>(Name));
												}
											}
										})
									.OnSelectionChanged_Lambda([this, Mapping](TSharedPtr<FString> NewItem, ESelectInfo::Type)
										{
											if (CurrentSource.IsValid() && Mapping.IsValid() && NewItem.IsValid())
											{
												CurrentSource->Modify();
												Mapping->StaticValue = *NewItem;
												Mapping->bUseStaticValue = true;
												const int32 Index = MappingRows.Find(Mapping);
												if (Index != INDEX_NONE)
												{
													CurrentSource->InlineMappings[Index].StaticValue = Mapping->StaticValue;
													CurrentSource->InlineMappings[Index].bUseStaticValue = Mapping->bUseStaticValue;
												}
												RefreshMappingPreview();
											}
										})
									.Content()
									[
										SNew(STextBlock).Text_Lambda([Mapping, GetTargetEnum]()
											{
												if (!Mapping.IsValid() || Mapping->StaticValue.IsEmpty())
												{
													return NSLOCTEXT("YOLOInventory", "AffixDash_StaticEnumHint", "Select enum");
												}
												if (UEnum* Enum = GetTargetEnum())
												{
													int32 Index = Enum->GetIndexByNameString(Mapping->StaticValue);
													if (Index == INDEX_NONE)
													{
														Index = Enum->GetIndexByName(FName(*Mapping->StaticValue));
													}
													if (Index != INDEX_NONE)
													{
														return Enum->GetDisplayNameTextByIndex(Index);
													}
												}
												return FText::FromString(Mapping->StaticValue);
											})
									]
							]
					]
			]
			+ SHorizontalBox::Slot().FillWidth(0.22f).Padding(2)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&const_cast<SYIAffixDashboard*>(this)->TargetPropertyOptions)
					.OnGenerateWidget_Lambda([this, DropdownText, GetTypeInfo](TSharedPtr<FString> InItem)
						{
							const FName FieldName = InItem.IsValid() ? FName(**InItem) : NAME_None;
							FString Label;
							FLinearColor Color;
							FProperty* Prop = nullptr;
							if (FieldName != NAME_None)
							{
								if (FProperty** Found = const_cast<SYIAffixDashboard*>(this)->TargetFieldPropCache.Find(FieldName))
								{
									Prop = *Found;
								}
							}
							GetTypeInfo(Prop, Label, Color);
							return SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
								[
									SNew(SBorder)
										.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
										.Padding(FMargin(3, 1))
										.BorderBackgroundColor(Color)
										[
											SNew(STextBlock).Text(FText::FromString(Label)).ColorAndOpacity(FSlateColor(FLinearColor::Black))
										]
								]
								+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4, 0)
								[
									SNew(STextBlock).Text(DropdownText(InItem))
								];
						})
					.OnSelectionChanged_Lambda([this, Mapping, GetSourceProp, GetTargetProp](TSharedPtr<FString> NewItem, ESelectInfo::Type)
						{
							if (CurrentSource.IsValid() && Mapping.IsValid() && NewItem.IsValid())
							{
								CurrentSource->Modify();
								Mapping->TargetProperty = FName(**NewItem);
								if (Mapping->Conversion == EYIFieldMappingConversion::None)
								{
									const FProperty* SourceProp = GetSourceProp();
									const FProperty* TargetProp = GetTargetProp();
									const EYIFieldMappingConversion Guess = GuessConversionForPropsAffix(SourceProp, TargetProp);
									Mapping->Conversion = Guess;
								}
								const int32 Index = MappingRows.Find(Mapping);
								if (Index != INDEX_NONE)
								{
									CurrentSource->InlineMappings[Index].TargetProperty = Mapping->TargetProperty;
									CurrentSource->InlineMappings[Index].Conversion = Mapping->Conversion;
								}
								RefreshMappingPreview();
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
							+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
							[
								MakeTypeBadgeDynamic(GetTargetProp)
							]
							+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4, 0)
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
			+ SHorizontalBox::Slot().FillWidth(0.10f).Padding(2)
			[
				SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&const_cast<SYIAffixDashboard*>(this)->ConverterOptions)
					.OnGenerateWidget_Lambda([DropdownText](TSharedPtr<FString> InItem)
						{
							return SNew(STextBlock).Text(DropdownText(InItem));
						})
					.OnSelectionChanged_Lambda([this, Mapping](TSharedPtr<FString> NewItem, ESelectInfo::Type)
						{
							if (CurrentSource.IsValid() && Mapping.IsValid() && NewItem.IsValid())
							{
								CurrentSource->Modify();
								EYIFieldMappingConversion NewConv = EYIFieldMappingConversion::None;
								if (*NewItem == TEXT("To Name")) NewConv = EYIFieldMappingConversion::ToName;
								else if (*NewItem == TEXT("To Text")) NewConv = EYIFieldMappingConversion::ToText;
								else if (*NewItem == TEXT("To Int")) NewConv = EYIFieldMappingConversion::ToInt;
								else if (*NewItem == TEXT("To Float")) NewConv = EYIFieldMappingConversion::ToFloat;
								else if (*NewItem == TEXT("Bool from Int>0")) NewConv = EYIFieldMappingConversion::BoolFromInt;
								else if (*NewItem == TEXT("Bool from Text (non-empty)")) NewConv = EYIFieldMappingConversion::BoolFromText;
								else if (*NewItem == TEXT("To Enum")) NewConv = EYIFieldMappingConversion::ToEnum;
								else if (*NewItem == TEXT("To Gameplay Tag")) NewConv = EYIFieldMappingConversion::ToGameplayTag;
								else if (*NewItem == TEXT("To Texture (Soft)")) NewConv = EYIFieldMappingConversion::ToSoftTexture;
								else if (*NewItem == TEXT("Vector2D from XY Fields")) NewConv = EYIFieldMappingConversion::Vector2DFromXY;
								Mapping->Conversion = NewConv;
								const int32 Index = MappingRows.Find(Mapping);
								if (Index != INDEX_NONE)
								{
									CurrentSource->InlineMappings[Index].Conversion = Mapping->Conversion;
								}
								RefreshMappingPreview();
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
							ConverterOptions.Add(MakeShared<FString>(TEXT("To Enum")));
							ConverterOptions.Add(MakeShared<FString>(TEXT("To Gameplay Tag")));
							ConverterOptions.Add(MakeShared<FString>(TEXT("To Texture (Soft)")));
							ConverterOptions.Add(MakeShared<FString>(TEXT("Vector2D from XY Fields")));
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
							case EYIFieldMappingConversion::ToEnum: Label = TEXT("To Enum"); break;
							case EYIFieldMappingConversion::ToGameplayTag: Label = TEXT("To Gameplay Tag"); break;
							case EYIFieldMappingConversion::ToSoftTexture: Label = TEXT("To Texture (Soft)"); break;
							case EYIFieldMappingConversion::Vector2DFromXY: Label = TEXT("Vector2D from XY Fields"); break;
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
									case EYIFieldMappingConversion::ToEnum: Label = TEXT("To Enum"); break;
									case EYIFieldMappingConversion::ToGameplayTag: Label = TEXT("To Gameplay Tag"); break;
									case EYIFieldMappingConversion::ToSoftTexture: Label = TEXT("To Texture (Soft)"); break;
									case EYIFieldMappingConversion::Vector2DFromXY: Label = TEXT("Vector2D from XY Fields"); break;
									default: break;
									}
								}
								return FText::FromString(Label);
							})
					]
			]
			+ SHorizontalBox::Slot().FillWidth(0.17f).Padding(2)
			[
				SNew(SComboBox<TSharedPtr<FYITransformFunctionInfo>>)
					.OptionsSource(&const_cast<SYIAffixDashboard*>(this)->TransformFunctionOptions)
					.OnGenerateWidget_Lambda([](TSharedPtr<FYITransformFunctionInfo> InItem)
						{
							return SNew(STextBlock).Text(InItem.IsValid() ? FText::FromString(InItem->DisplayName) : FText::GetEmpty());
						})
					.OnComboBoxOpening_Lambda([this]()
						{
							BuildTransformFunctionOptions();
						})
					.OnSelectionChanged_Lambda([this, Mapping](TSharedPtr<FYITransformFunctionInfo> NewItem, ESelectInfo::Type)
						{
							if (CurrentSource.IsValid() && Mapping.IsValid() && NewItem.IsValid())
							{
								CurrentSource->Modify();
								Mapping->TransformFunction = NewItem->FunctionName;
								Mapping->TransformLibrary = NewItem->Library;
								const int32 Index = MappingRows.Find(Mapping);
								if (Index != INDEX_NONE)
								{
									CurrentSource->InlineMappings[Index].TransformFunction = Mapping->TransformFunction;
									CurrentSource->InlineMappings[Index].TransformLibrary = Mapping->TransformLibrary;
								}
								RefreshMappingPreview();
							}
						})
					.InitiallySelectedItem([this, Mapping]()
						{
							if (!Mapping.IsValid())
							{
								return TSharedPtr<FYITransformFunctionInfo>();
							}
							for (const TSharedPtr<FYITransformFunctionInfo>& Opt : TransformFunctionOptions)
							{
								if (!Opt.IsValid())
								{
									continue;
								}
								if (Opt->FunctionName.IsNone() && Mapping->TransformFunction.IsNone())
								{
									return Opt;
								}
								if (Opt->FunctionName == Mapping->TransformFunction &&
									Opt->Library.ToSoftObjectPath() == Mapping->TransformLibrary.ToSoftObjectPath())
								{
									return Opt;
								}
							}
							return TSharedPtr<FYITransformFunctionInfo>();
						}())
					.Content()
					[
						SNew(STextBlock).Text_Lambda([Mapping]()
							{
								if (Mapping.IsValid() && !Mapping->TransformFunction.IsNone())
								{
									return FText::FromName(Mapping->TransformFunction);
								}
								return NSLOCTEXT("YOLOInventory", "AffixDash_TransformNone", "None");
							})
					]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2)
			[
				SNew(SButton)
					.Text(NSLOCTEXT("YOLOInventory", "AffixDash_RemoveMapping", "X"))
					.OnClicked_Lambda([this, Mapping]()
						{
							if (CurrentSource.IsValid() && Mapping.IsValid())
							{
								const int32 Index = MappingRows.Find(Mapping);
								if (Index != INDEX_NONE)
								{
									CurrentSource->Modify();
									MappingRows.RemoveAt(Index, 1, EAllowShrinking::No);
									CurrentSource->InlineMappings.RemoveAt(Index, 1, EAllowShrinking::No);
									if (MappingListView.IsValid())
									{
										MappingListView->RequestListRefresh();
									}
									RefreshMappingPreview();
								}
							}
							return FReply::Handled();
						})
			]
	];
}
