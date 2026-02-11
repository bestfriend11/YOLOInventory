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
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
			[
				DetailsView.ToSharedRef()
			]
		]
	];
}

TSharedRef<SWidget> SYIAffixDashboard::BuildAssetPicker()
{
	FAssetPickerConfig Picker;
	Picker.InitialAssetViewType = EAssetViewType::Tile;
	Picker.Filter.ClassPaths.Add(UYIAffixAsset::StaticClass()->GetClassPathName());
	Picker.Filter.ClassPaths.Add(UYIAffixPoolAsset::StaticClass()->GetClassPathName());
	Picker.bAllowNullSelection = false;
	Picker.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SYIAffixDashboard::OnAssetSelected);
	Picker.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SYIAffixDashboard::OnAssetDoubleClicked);

	FContentBrowserModule& CB = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	return CB.Get().CreateAssetPicker(Picker);
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
	}
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
		if (Mapping.SourceField.IsNone() || Mapping.TargetProperty.IsNone())
		{
			continue;
		}

		const FProperty* SourceProp = nullptr;
		for (TFieldIterator<FProperty> It(DataTable->RowStruct); It; ++It)
		{
			if ((*It)->GetAuthoredName() == Mapping.SourceField)
			{
				SourceProp = *It;
				break;
			}
		}

		FProperty* TargetProp = FindFProperty<FProperty>(UYIAffixAsset::StaticClass(), Mapping.TargetProperty);

		if (!SourceProp || !TargetProp)
		{
			continue;
		}

		const uint8* SrcPtr = SourceProp->ContainerPtrToValuePtr<uint8>(RowPtr);
		uint8* DstPtr = TargetProp->ContainerPtrToValuePtr<uint8>(Affix);

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
	}

	FAssetRegistryModule::AssetCreated(NewAsset);
	Pkg->MarkPackageDirty();
	return true;
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
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Info,
			NSLOCTEXT("YOLOInventory","AffixDash_UpdateOk","Affix updated from data source."));
	}
	else
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Warning,
			NSLOCTEXT("YOLOInventory","AffixDash_UpdateFailed","Affix update failed."));
	}
	return FReply::Handled();
}
