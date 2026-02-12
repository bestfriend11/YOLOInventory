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
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/SHeaderRow.h"
#include "Styling/AppStyle.h"
#include "Misc/PackageName.h"
#include "PackageTools.h"
#include "FileHelpers.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/MessageDialog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Factories/BlueprintFactory.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "InputCoreTypes.h"
#include "ObjectTools.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/UObjectIterator.h"
#include "UObject/StructOnScope.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Engine/Texture.h"
#include "Algo/Sort.h"
#include "YIEditorRowHelpers.h"
#include "YIEditorMessageLog.h"

static bool YIItemDash_SaveObjectPackage(UObject* ObjectToSave)
{
	if (!ObjectToSave)
	{
		return false;
	}

	UPackage* Package = ObjectToSave->GetOutermost();
	if (!Package)
	{
		return false;
	}

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(Package);
	const bool bCheckDirty = false;
	const bool bPromptToSave = false;
	return FEditorFileUtils::PromptForCheckoutAndSave(PackagesToSave, bCheckDirty, bPromptToSave) == FEditorFileUtils::PR_Success;
}

static bool TryGetEnumValueFromStringEditor(const UEnum* Enum, const FString& Value, int64& OutValue)
{
	if (!Enum)
	{
		return false;
	}

	if (Value.IsEmpty())
	{
		return false;
	}

	// Numeric strings map directly.
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

	// Case-insensitive fallback.
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

static bool SetEnumPropertyValueEditor(FProperty* DestProp, uint8* DestPtr, int64 Value)
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

static bool SetEnumPropertyValueEditor(FProperty* DestProp, uint8* DestPtr, const FString& Value)
{
	if (FEnumProperty* EnumProp = CastField<FEnumProperty>(DestProp))
	{
		if (UEnum* Enum = EnumProp->GetEnum())
		{
			int64 EnumValue = 0;
			if (TryGetEnumValueFromStringEditor(Enum, Value, EnumValue))
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
			if (TryGetEnumValueFromStringEditor(ByteProp->Enum, Value, EnumValue))
			{
				ByteProp->SetPropertyValue(DestPtr, (uint8)EnumValue);
				return true;
			}
		}
	}
	return false;
}

static bool SetPropertyValueFromStringEditor(const FString& Value, FProperty* DestProp, uint8* DestPtr, EYIFieldMappingConversion Conversion)
{
	if (!DestProp || !DestPtr)
	{
		return false;
	}

	if (Conversion == EYIFieldMappingConversion::ToEnum)
	{
		return SetEnumPropertyValueEditor(DestProp, DestPtr, Value);
	}
	if (Conversion == EYIFieldMappingConversion::None)
	{
		if (CastField<FEnumProperty>(DestProp) || (CastField<FByteProperty>(DestProp) && CastField<FByteProperty>(DestProp)->Enum))
		{
			return SetEnumPropertyValueEditor(DestProp, DestPtr, Value);
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

static bool CopyValueBetweenPropertiesEditor(const FProperty* SourceProp, const uint8* SourcePtr, FProperty* DestProp, uint8* DestPtr, EYIFieldMappingConversion Conversion)
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
				return SetEnumPropertyValueEditor(DestProp, DestPtr, Enum->GetNameStringByValue(Val));
			}
		}
		if (const FByteProperty* SrcByte = CastField<FByteProperty>(SourceProp))
		{
			if (SrcByte->Enum)
			{
				const int64 Val = SrcByte->GetPropertyValue(SourcePtr);
				return SetEnumPropertyValueEditor(DestProp, DestPtr, SrcByte->Enum->GetNameStringByValue(Val));
			}
		}
		if (const FNumericProperty* SrcNum = CastField<FNumericProperty>(SourceProp))
		{
			const int64 Val = SrcNum->GetSignedIntPropertyValue(SourcePtr);
			return SetEnumPropertyValueEditor(DestProp, DestPtr, Val);
		}
		if (const FNameProperty* SrcName = CastField<FNameProperty>(SourceProp))
		{
			return SetEnumPropertyValueEditor(DestProp, DestPtr, SrcName->GetPropertyValue(SourcePtr).ToString());
		}
		if (const FStrProperty* SrcStr = CastField<FStrProperty>(SourceProp))
		{
			return SetEnumPropertyValueEditor(DestProp, DestPtr, SrcStr->GetPropertyValue(SourcePtr));
		}
		if (const FTextProperty* SrcText = CastField<FTextProperty>(SourceProp))
		{
			return SetEnumPropertyValueEditor(DestProp, DestPtr, SrcText->GetPropertyValue(SourcePtr).ToString());
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

static bool GetTransformFunctionProps(UFunction* Function, FProperty*& OutInput, FProperty*& OutOutput)
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

static bool ApplyTransformFunctionEditor(const FYIFieldMapping& Mapping, FProperty* DestProp, uint8* DestPtr, FString* OutError = nullptr)
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
	if (!GetTransformFunctionProps(Function, InputProp, OutputProp))
	{
		if (OutError) { *OutError = TEXT("Transform function signature invalid."); }
		return false;
	}

	FStructOnScope Params(Function);
	uint8* ParamsMem = Params.GetStructMemory();
	uint8* InputPtr = InputProp->ContainerPtrToValuePtr<uint8>(ParamsMem);
	uint8* OutputPtr = OutputProp->ContainerPtrToValuePtr<uint8>(ParamsMem);

	if (!CopyValueBetweenPropertiesEditor(DestProp, DestPtr, InputProp, InputPtr, EYIFieldMappingConversion::None))
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

	if (!CopyValueBetweenPropertiesEditor(OutputProp, OutputPtr, DestProp, DestPtr, EYIFieldMappingConversion::None))
	{
		if (OutError) { *OutError = TEXT("Transform output copy failed."); }
		return false;
	}

	return true;
}

static FString ExportPropertyValueToString(const FProperty* Prop, const uint8* ValuePtr)
{
	if (!Prop || !ValuePtr)
	{
		return FString();
	}
	FString Out;
	Prop->ExportTextItem_Direct(Out, ValuePtr, nullptr, nullptr, PPF_None);
	return Out;
}

static EYIFieldMappingConversion GuessConversionForProps(const FProperty* SourceProp, const FProperty* TargetProp)
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
	LayoutMode = InArgs._LayoutMode;

	if (LayoutMode == EYIItemDashboardLayout::ItemListOnly)
	{
		ItemsPanelWidget = BuildItemsPanelWidget();
		DetailsPanelWidget = BuildDetailsPanelWidget();
		MappingPanelWidget = BuildMappingPanelWidget();
		PreviewPanelWidget = BuildPreviewPanelWidget();
		PreflightPanelWidget = BuildPreflightPanelWidget();
		DiffPanelWidget = BuildDiffPanelWidget();
		BatchPanelWidget = BuildBatchPanelWidget();
		LogsPanelWidget = BuildLogsPanelWidget();

		ChildSlot
		[
			ItemsPanelWidget.ToSharedRef()
		];

		Refresh();
		return;
	}

	ChildSlot
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(8, 6)
				[
					SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
						.Padding(10)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
								[
									SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_ToolbarPanels", "Panels:"))
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
								[
									SNew(SCheckBox)
										.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
										.IsChecked_Lambda([this]() { return bShowDetailsPanel ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
										.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bShowDetailsPanel = (State == ECheckBoxState::Checked); })
										[
											SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_ToolbarDetails", "Details"))
										]
								]
							+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
								[
									SNew(SCheckBox)
										.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
										.IsChecked_Lambda([this]() { return bShowMappingPanel ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
										.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bShowMappingPanel = (State == ECheckBoxState::Checked); })
										[
											SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_ToolbarMappings", "Mappings"))
										]
								]
							+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
								[
									SNew(SCheckBox)
										.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
										.IsChecked_Lambda([this]() { return bShowPreviewPanel ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
										.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bShowPreviewPanel = (State == ECheckBoxState::Checked); })
										[
											SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_ToolbarPreview", "Preview"))
										]
								]
							+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
								[
									SNew(SCheckBox)
										.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
										.IsChecked_Lambda([this]() { return bShowLogPanel ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
										.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
											{
												bShowLogPanel = (State == ECheckBoxState::Checked);
												if (bShowLogPanel)
												{
													ActiveBottomPanel = EYIDashboardBottomPanel::Logs;
												}
											})
										[
											SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_ToolbarLogs", "Logs"))
										]
								]
							+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
								[
									SNew(SCheckBox)
										.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
										.IsChecked_Lambda([this]() { return bShowPreflightPanel ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
										.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
											{
												bShowPreflightPanel = (State == ECheckBoxState::Checked);
												if (bShowPreflightPanel)
												{
													ActiveBottomPanel = EYIDashboardBottomPanel::Preflight;
												}
											})
										[
											SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_ToolbarPreflight", "Preflight"))
										]
								]
							+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
								[
									SNew(SCheckBox)
										.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
										.IsChecked_Lambda([this]() { return bShowDiffPanel ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
										.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
											{
												bShowDiffPanel = (State == ECheckBoxState::Checked);
												if (bShowDiffPanel)
												{
													ActiveBottomPanel = EYIDashboardBottomPanel::Diff;
												}
											})
										[
											SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_ToolbarDiff", "Diff"))
										]
								]
							+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
								[
									SNew(SCheckBox)
										.Style(FAppStyle::Get(), "ToggleButtonCheckbox")
										.IsChecked_Lambda([this]() { return bShowBatchPanel ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
										.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
											{
												bShowBatchPanel = (State == ECheckBoxState::Checked);
												if (bShowBatchPanel)
												{
													ActiveBottomPanel = EYIDashboardBottomPanel::Batch;
												}
											})
										[
											SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_ToolbarBatch", "Batch"))
										]
								]
						]
				]
			+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 6)
				[
					SNew(SBorder)
						.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
						.Padding(10)
						[
							SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
								[
									SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_ActionBar", "Actions:"))
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
								[
									SNew(SButton)
										.Text(NSLOCTEXT("YOLOInventory", "Dash_Action_Create", "Create/Update Selected"))
										.OnClicked_Lambda([this]()
											{
												bool bChanged = false;
												if (ListView.IsValid())
												{
													const TArray<TSharedPtr<FYIItemDashboardEntry>> Selected = ListView->GetSelectedItems();
													for (const TSharedPtr<FYIItemDashboardEntry>& E : Selected)
													{
														if (E.IsValid() && E->bIsDataTable)
														{
															bChanged |= CreateAssetFromEntry(*E);
														}
													}
												}
												if (bChanged)
												{
													Refresh();
												}
												return FReply::Handled();
											})
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
								[
									SNew(SButton)
										.Text(NSLOCTEXT("YOLOInventory", "Dash_Action_UpdateLinked", "Update Linked Asset"))
										.OnClicked_Lambda([this]()
											{
												if (!ListView.IsValid())
												{
													return FReply::Handled();
												}
												const TArray<TSharedPtr<FYIItemDashboardEntry>> Selected = ListView->GetSelectedItems();
												bool bChanged = false;
												for (const TSharedPtr<FYIItemDashboardEntry>& E : Selected)
												{
													if (!E.IsValid())
													{
														continue;
													}
													UObject* Obj = E->Object.LoadSynchronous();
													if (UYIItemDefinition* Def = Cast<UYIItemDefinition>(Obj))
													{
														bChanged |= UpdateAssetFromLinkedSource(Def);
													}
												}
												if (bChanged)
												{
													Refresh();
												}
												return FReply::Handled();
											})
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
								[
									SNew(SButton)
										.Text(NSLOCTEXT("YOLOInventory", "Dash_Action_Preflight", "Preflight Selected"))
										.OnClicked_Lambda([this]()
											{
												RebuildPreflightForSelection();
												return FReply::Handled();
											})
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
								[
									SNew(SButton)
										.Text(NSLOCTEXT("YOLOInventory", "Dash_Action_ApplySuggested", "Apply Suggested Mapping"))
										.OnClicked_Lambda([this]()
											{
												ApplySuggestedMappings();
												return FReply::Handled();
											})
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
								[
									SNew(SButton)
										.Text(NSLOCTEXT("YOLOInventory", "Dash_Action_Queue", "Queue Selected"))
										.OnClicked_Lambda([this]()
											{
												EnqueueSelectedRows();
												return FReply::Handled();
											})
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
								[
									SNew(SButton)
										.Text(NSLOCTEXT("YOLOInventory", "Dash_Action_RunQueue", "Run Queue"))
										.OnClicked_Lambda([this]()
											{
												ProcessBatchQueue();
												return FReply::Handled();
											})
								]
								+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right).VAlign(VAlign_Center)
								[
									SNew(STextBlock)
										.Text_Lambda([this]()
											{
												if (!ListView.IsValid())
												{
													return NSLOCTEXT("YOLOInventory", "Dash_Workflow_NoList", "Workflow: Idle");
												}
												const int32 SelectedCount = ListView->GetNumItemsSelected();
												if (SelectedCount <= 0)
												{
													return NSLOCTEXT("YOLOInventory", "Dash_Workflow_Select", "Workflow: Select row or linked item");
												}
												if (SelectedCount > 1)
												{
													return NSLOCTEXT("YOLOInventory", "Dash_Workflow_Bulk", "Workflow: Bulk validate / generate");
												}
												if (PreflightIssues.Num() > 0)
												{
													for (const TSharedPtr<FYIPreflightIssue>& Issue : PreflightIssues)
													{
														if (Issue.IsValid() && Issue->bBlocking)
														{
															return NSLOCTEXT("YOLOInventory", "Dash_Workflow_Blocking", "Workflow: Fix blocking preflight errors");
														}
													}
													return NSLOCTEXT("YOLOInventory", "Dash_Workflow_Warn", "Workflow: Ready with warnings");
												}
												return NSLOCTEXT("YOLOInventory", "Dash_Workflow_Ready", "Workflow: Ready to generate/update");
											})
								]
						]
				]
			+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SNew(SSplitter)
						+ SSplitter::Slot().Value(0.32f)
						[
							SNew(SBorder)
								.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
								.Padding(8)
								[
									SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight().Padding(12)
										[
											SNew(SHorizontalBox)
												+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0, 0, 8, 0))
												[
													SNew(SButton)
														.Text(NSLOCTEXT("YOLOInventory", "DashboardRefresh", "Refresh"))
														.OnClicked_Lambda([this]()
															{
																Refresh();
																return FReply::Handled();
															})
												]
											+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0, 0, 8, 0))
												[
													SNew(SButton)
														.Text(NSLOCTEXT("YOLOInventory", "DashboardCreateSource", "New Data Table Source"))
														.OnClicked_Lambda([this]()
															{
																CreateDataTableSourceAsset();
																return FReply::Handled();
															})
												]
											+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0, 0, 8, 0))
												[
													SNew(SButton)
														.Text(NSLOCTEXT("YOLOInventory", "DashboardValidateCodes", "Validate Unique Codes"))
														.OnClicked_Lambda([this]()
															{
																ValidateUniqueCodes();
																return FReply::Handled();
															})
												]
											+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0, 0, 8, 0))
												[
													SNew(SButton)
														.Text(NSLOCTEXT("YOLOInventory", "DashboardBulkCreate", "Create Assets (Selected)"))
														.ToolTipText(NSLOCTEXT("YOLOInventory", "DashboardBulkCreate_Tip", "Generate assets for all selected data-table rows"))
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
												+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(8, 0, 0, 0))
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
											+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(8, 0, 0, 0))
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
											+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(8, 0, 0, 0))
												[
													SNew(SCheckBox)
														.Style(&FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("ToggleButtonCheckBox"))
														.Padding(FMargin(6, 2))
														.IsChecked_Lambda([this]() { return bGroupBySource ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
														.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
															{
																bGroupBySource = (State == ECheckBoxState::Checked);
																Refresh();
															})
														[
															SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_GroupBySource", "Group by Source"))
														]
												]
										]
									+ SVerticalBox::Slot().FillHeight(1.f).Padding(12, 8, 12, 12)
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
													+ SHeaderRow::Column("Code").DefaultLabel(NSLOCTEXT("YOLOInventory", "Dash_Code", "Code")).FillWidth(0.15f)
													+ SHeaderRow::Column("Name").DefaultLabel(NSLOCTEXT("YOLOInventory", "Dash_Name", "Name")).FillWidth(0.25f)
													+ SHeaderRow::Column("Template").DefaultLabel(NSLOCTEXT("YOLOInventory", "Dash_Template", "TemplateId")).FillWidth(0.2f)
													+ SHeaderRow::Column("Type").DefaultLabel(NSLOCTEXT("YOLOInventory", "Dash_Type", "Type")).FillWidth(0.1f)
													+ SHeaderRow::Column("Source").DefaultLabel(NSLOCTEXT("YOLOInventory", "Dash_Source", "Source")).FillWidth(0.3f)
													+ SHeaderRow::Column("Asset").DefaultLabel(NSLOCTEXT("YOLOInventory", "Dash_Asset", "Asset")).FillWidth(0.1f)
												)
										]
								]
						]
					+ SSplitter::Slot().Value(0.68f)
						[
							SNew(SVerticalBox)
								+ SVerticalBox::Slot().FillHeight(0.35f).Padding(0, 0, 0, 4)
								[
									SNew(SBorder)
										.Visibility_Lambda([this]()
											{
												return bShowDetailsPanel ? EVisibility::Visible : EVisibility::Collapsed;
											})
										.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
										.Padding(8)
										[
											DetailsView.IsValid()
												? StaticCastSharedRef<SWidget>(DetailsView.ToSharedRef())
												: StaticCastSharedRef<SWidget>(SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_NoDetails", "Details panel unavailable")))
										]
								]
							+ SVerticalBox::Slot().FillHeight(0.65f).Padding(0, 4, 0, 0)
								[
									SNew(SBorder)
										.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
										.Padding(8)
										.Visibility_Lambda([this]()
											{
												return (CurrentMappingSource.IsValid() && bShowMappingPanel) ? EVisibility::Visible : EVisibility::Collapsed;
											})
										[
											SNew(SVerticalBox)
												+ SVerticalBox::Slot().AutoHeight().Padding(8)
												[
													SNew(SHorizontalBox)
														+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
														[
															SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_InlineMappings", "Inline Mappings"))
														]
														+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
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
																		TargetPropertyOptions.Add(MakeShared<FString>(TEXT("Transformer then Inline")));
																	})
																.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewItem, ESelectInfo::Type)
																	{
																		if (!CurrentMappingSource.IsValid() || !NewItem.IsValid()) return;
																		CurrentMappingSource->Modify();
																		if (*NewItem == TEXT("Inline Only")) CurrentMappingSource->TransformMode = EYITransformMode::InlineOnly;
																		else if (*NewItem == TEXT("Transformer Only")) CurrentMappingSource->TransformMode = EYITransformMode::TransformerOnly;
																		else if (*NewItem == TEXT("Inline then Transformer")) CurrentMappingSource->TransformMode = EYITransformMode::HybridInlineThenTransformer;
																		else CurrentMappingSource->TransformMode = EYITransformMode::HybridTransformerThenInline;
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
													+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
														[
															SNew(SButton)
																.Text(NSLOCTEXT("YOLOInventory", "Dash_AddMapping", "Add Mapping"))
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
													+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
														[
															SNew(SButton)
																.Text(NSLOCTEXT("YOLOInventory", "Dash_AutoMatch", "Auto Match"))
																.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_AutoMatch_TT", "Match fields by name and update missing source fields."))
																.OnClicked_Lambda([this]()
																	{
																		AutoMatchInlineMappings(false);
																		return FReply::Handled();
																	})
														]
													+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
														[
															SNew(SButton)
																.Text(NSLOCTEXT("YOLOInventory", "Dash_AddAllFields", "Add All Fields"))
																.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_AddAllFields_TT", "Add mapping rows for every item definition field, and auto-match where possible."))
																.OnClicked_Lambda([this]()
																	{
																		AutoMatchInlineMappings(true);
																		return FReply::Handled();
																	})
														]
													+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
														[
															SNew(SButton)
																.Text(NSLOCTEXT("YOLOInventory", "Dash_ClearMapping", "Clear"))
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
													+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(8, 0)
														[
															SNew(STextBlock)
																.Text_Lambda([this]()
																	{
																		if (!CurrentMappingSource.IsValid())
																		{
																			return NSLOCTEXT("YOLOInventory", "Dash_Confidence_NoSource", "Confidence: N/A");
																		}
																		const int32 Total = CurrentMappingSource->InlineMappings.Num();
																		int32 Ready = 0;
																		int32 NeedsFix = 0;
																		for (const FYIFieldMapping& M : CurrentMappingSource->InlineMappings)
																		{
																			const bool bHasSource = !M.SourceField.IsNone();
																			const bool bHasTarget = !M.TargetProperty.IsNone();
																			if (bHasSource && bHasTarget)
																			{
																				++Ready;
																			}
																			else
																			{
																				++NeedsFix;
																			}
																		}
																		return FText::Format(NSLOCTEXT("YOLOInventory", "Dash_ConfidenceFmt", "Confidence: Ready {0}/{1}, Needs Fix {2}"), FText::AsNumber(Ready), FText::AsNumber(Total), FText::AsNumber(NeedsFix));
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
																.OnGenerateRow(this, &SYIItemDashboard::MakeMappingRow)
																.SelectionMode(ESelectionMode::Single)
														]
														+ SSplitter::Slot().Value(0.38f)
														[
															SNew(SBorder)
																.Visibility_Lambda([this]()
																	{
																		return bShowPreviewPanel ? EVisibility::Visible : EVisibility::Collapsed;
																	})
																.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
																.Padding(8)
																[
																	SNew(SVerticalBox)
																		+ SVerticalBox::Slot().AutoHeight().Padding(8)
																		[
																			SNew(SHorizontalBox)
																				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
																				[
																					SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_PreviewTitle", "Live Preview"))
																				]
																				+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0)
																				[
																					SNew(SComboBox<TSharedPtr<FString>>)
																						.OptionsSource(&const_cast<SYIItemDashboard*>(this)->PreviewRowOptions)
																						.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
																							{
																								return SNew(STextBlock).Text(InItem.IsValid() ? FText::FromString(*InItem) : FText::GetEmpty());
																							})
																						.OnComboBoxOpening_Lambda([this]()
																							{
																								PreviewRowOptions.Reset();
																								if (CurrentMappingSource.IsValid())
																								{
																									if (UDataTable* Table = CurrentMappingSource->DataTable.LoadSynchronous())
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
																									return PreviewRowName.IsNone() ? NSLOCTEXT("YOLOInventory", "Dash_PreviewRow", "Select Row") : FText::FromName(PreviewRowName);
																								})
																						]
																				]
																			+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0)
																				[
																					SNew(SButton)
																						.Text(NSLOCTEXT("YOLOInventory", "Dash_PreviewRefresh", "Refresh"))
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
																				.OnGenerateRow(this, &SYIItemDashboard::MakePreviewRow)
																		]
																]
														]
												]
										]
								]
							+ SVerticalBox::Slot().FillHeight(0.55f).Padding(4)
								[
									SNew(SBorder)
										.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
										.Padding(4)
										[
											SNew(SWidgetSwitcher)
												.WidgetIndex_Lambda([this]()
													{
														switch (ActiveBottomPanel)
														{
														case EYIDashboardBottomPanel::Preflight: return 0;
														case EYIDashboardBottomPanel::Diff: return 1;
														case EYIDashboardBottomPanel::Batch: return 2;
														default: return 3;
														}
													})
												+ SWidgetSwitcher::Slot()
												[
													GetPreflightPanelWidget()
												]
												+ SWidgetSwitcher::Slot()
												[
													GetDiffPanelWidget()
												]
												+ SWidgetSwitcher::Slot()
												[
													GetBatchPanelWidget()
												]
												+ SWidgetSwitcher::Slot()
												[
													GetLogsPanelWidget()
												]
										]
								]
						]
				]
		];

	Refresh();
}

TSharedRef<SWidget> SYIItemDashboard::GetItemsPanelWidget() const
{
	SYIItemDashboard* Self = const_cast<SYIItemDashboard*>(this);
	if (!Self->ItemsPanelWidget.IsValid())
	{
		Self->ItemsPanelWidget = Self->BuildItemsPanelWidget();
	}
	return Self->ItemsPanelWidget.ToSharedRef();
}

TSharedRef<SWidget> SYIItemDashboard::GetDetailsPanelWidget() const
{
	SYIItemDashboard* Self = const_cast<SYIItemDashboard*>(this);
	if (!Self->DetailsPanelWidget.IsValid())
	{
		Self->DetailsPanelWidget = Self->BuildDetailsPanelWidget();
	}
	return Self->DetailsPanelWidget.ToSharedRef();
}

TSharedRef<SWidget> SYIItemDashboard::GetMappingPanelWidget() const
{
	SYIItemDashboard* Self = const_cast<SYIItemDashboard*>(this);
	if (!Self->MappingPanelWidget.IsValid())
	{
		Self->MappingPanelWidget = Self->BuildMappingPanelWidget();
	}
	return Self->MappingPanelWidget.ToSharedRef();
}

TSharedRef<SWidget> SYIItemDashboard::GetPreviewPanelWidget() const
{
	SYIItemDashboard* Self = const_cast<SYIItemDashboard*>(this);
	if (!Self->PreviewPanelWidget.IsValid())
	{
		Self->PreviewPanelWidget = Self->BuildPreviewPanelWidget();
	}
	return Self->PreviewPanelWidget.ToSharedRef();
}

TSharedRef<SWidget> SYIItemDashboard::GetPreflightPanelWidget() const
{
	SYIItemDashboard* Self = const_cast<SYIItemDashboard*>(this);
	if (!Self->PreflightPanelWidget.IsValid())
	{
		Self->PreflightPanelWidget = Self->BuildPreflightPanelWidget();
	}
	return Self->PreflightPanelWidget.ToSharedRef();
}

TSharedRef<SWidget> SYIItemDashboard::GetDiffPanelWidget() const
{
	SYIItemDashboard* Self = const_cast<SYIItemDashboard*>(this);
	if (!Self->DiffPanelWidget.IsValid())
	{
		Self->DiffPanelWidget = Self->BuildDiffPanelWidget();
	}
	return Self->DiffPanelWidget.ToSharedRef();
}

TSharedRef<SWidget> SYIItemDashboard::GetBatchPanelWidget() const
{
	SYIItemDashboard* Self = const_cast<SYIItemDashboard*>(this);
	if (!Self->BatchPanelWidget.IsValid())
	{
		Self->BatchPanelWidget = Self->BuildBatchPanelWidget();
	}
	return Self->BatchPanelWidget.ToSharedRef();
}

TSharedRef<SWidget> SYIItemDashboard::GetLogsPanelWidget() const
{
	SYIItemDashboard* Self = const_cast<SYIItemDashboard*>(this);
	if (!Self->LogsPanelWidget.IsValid())
	{
		Self->LogsPanelWidget = Self->BuildLogsPanelWidget();
	}
	return Self->LogsPanelWidget.ToSharedRef();
}

void SYIItemDashboard::RefreshFromToolbar()
{
	Refresh();
}

void SYIItemDashboard::CreateDataSourceFromToolbar()
{
	CreateDataTableSourceAsset();
}

void SYIItemDashboard::ValidateUniqueCodesFromToolbar()
{
	ValidateUniqueCodes();
}

void SYIItemDashboard::CreateOrUpdateSelectedFromToolbar()
{
	bool bChanged = false;
	if (ListView.IsValid())
	{
		const TArray<TSharedPtr<FYIItemDashboardEntry>> Selected = ListView->GetSelectedItems();
		for (const TSharedPtr<FYIItemDashboardEntry>& E : Selected)
		{
			if (E.IsValid() && E->bIsDataTable)
			{
				bChanged |= CreateAssetFromEntry(*E);
			}
		}
	}
	if (bChanged)
	{
		Refresh();
	}
}

void SYIItemDashboard::UpdateLinkedSelectedFromToolbar()
{
	bool bChanged = false;
	if (ListView.IsValid())
	{
		const TArray<TSharedPtr<FYIItemDashboardEntry>> Selected = ListView->GetSelectedItems();
		for (const TSharedPtr<FYIItemDashboardEntry>& E : Selected)
		{
			if (!E.IsValid())
			{
				continue;
			}
			UObject* Obj = E->Object.LoadSynchronous();
			if (UYIItemDefinition* Def = Cast<UYIItemDefinition>(Obj))
			{
				bChanged |= UpdateAssetFromLinkedSource(Def);
			}
		}
	}
	if (bChanged)
	{
		Refresh();
	}
}

void SYIItemDashboard::PreflightSelectedFromToolbar()
{
	RebuildPreflightForSelection();
}

void SYIItemDashboard::ApplySuggestedMappingsFromToolbar()
{
	ApplySuggestedMappings();
}

void SYIItemDashboard::QueueSelectedFromToolbar()
{
	EnqueueSelectedRows();
}

void SYIItemDashboard::RunQueueFromToolbar()
{
	ProcessBatchQueue();
}

void SYIItemDashboard::SaveCurrentAssetFromToolbar()
{
	UObject* ObjectToSave = nullptr;
	if (ListView.IsValid())
	{
		const TArray<TSharedPtr<FYIItemDashboardEntry>> Selected = ListView->GetSelectedItems();
		if (Selected.Num() > 0 && Selected[0].IsValid())
		{
			const TSharedPtr<FYIItemDashboardEntry> Entry = Selected[0];
			if (!Entry->bIsDataTable)
			{
				ObjectToSave = Entry->Object.LoadSynchronous();
			}
			else
			{
				// Prefer generated asset for row entries; fallback to source asset.
				if (Entry->ItemAsset.IsValid() || Entry->ItemAsset.ToSoftObjectPath().IsValid())
				{
					ObjectToSave = Entry->ItemAsset.LoadSynchronous();
				}
				if (!ObjectToSave && (Entry->DataSource.IsValid() || Entry->DataSource.ToSoftObjectPath().IsValid()))
				{
					ObjectToSave = Entry->DataSource.LoadSynchronous();
				}
			}
		}
	}
	if (!ObjectToSave)
	{
		ObjectToSave = LastDetailObject.Get();
	}

	if (!ObjectToSave)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Warning,
			NSLOCTEXT("YOLOInventory", "Dash_Save_NoSelection", "Save skipped: no selected item/data source asset."));
		return;
	}

	if (YIItemDash_SaveObjectPackage(ObjectToSave))
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Info,
			NSLOCTEXT("YOLOInventory", "Dash_Save_Success", "Saved selected asset."),
			FText::FromString(ObjectToSave->GetPathName()));
	}
	else
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Warning,
			NSLOCTEXT("YOLOInventory", "Dash_Save_Failed", "Save was canceled or failed."),
			FText::FromString(ObjectToSave->GetPathName()));
	}
}

void SYIItemDashboard::GuidedSetupFromToolbar()
{
	TArray<TSharedPtr<FYIItemDashboardEntry>> CandidateRows;
	if (ListView.IsValid())
	{
		const TArray<TSharedPtr<FYIItemDashboardEntry>> Selected = ListView->GetSelectedItems();
		for (const TSharedPtr<FYIItemDashboardEntry>& Entry : Selected)
		{
			if (Entry.IsValid() && Entry->bIsDataTable)
			{
				CandidateRows.Add(Entry);
			}
		}
	}

	// Guided fallback: run on visible data rows when no row is selected.
	if (CandidateRows.Num() == 0)
	{
		for (const TSharedPtr<FYIItemDashboardEntry>& Entry : FilteredItems)
		{
			if (Entry.IsValid() && Entry->bIsDataTable)
			{
				CandidateRows.Add(Entry);
			}
		}
	}

	if (CandidateRows.Num() == 0)
	{
		FYIEditorMessageLog::Add(
			EYIEditorLogSeverity::Warning,
			NSLOCTEXT("YOLOInventory", "Dash_Guided_NoRows", "Guided setup: no item datasource rows found in selection/view."));
		return;
	}

	int32 ReadyRows = 0;
	int32 BlockedRows = 0;
	int32 UpdatedRows = 0;
	for (const TSharedPtr<FYIItemDashboardEntry>& Entry : CandidateRows)
	{
		if (!Entry.IsValid())
		{
			continue;
		}

		TArray<FYIPreflightIssue> LocalIssues;
		if (!RunPreflightForEntry(*Entry, LocalIssues, false))
		{
			++BlockedRows;
			continue;
		}

		++ReadyRows;
		if (CreateAssetFromEntry(*Entry))
		{
			++UpdatedRows;
		}
	}

	Refresh();
	FYIEditorMessageLog::Add(
		BlockedRows > 0 ? EYIEditorLogSeverity::Warning : EYIEditorLogSeverity::Info,
		FText::Format(
			NSLOCTEXT("YOLOInventory", "Dash_Guided_Result", "Guided setup finished. Ready: {0}, Updated: {1}, Blocked: {2}"),
			FText::AsNumber(ReadyRows),
			FText::AsNumber(UpdatedRows),
			FText::AsNumber(BlockedRows)));
}

TSharedRef<SWidget> SYIItemDashboard::BuildItemsPanelWidget()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 6)
		[
			SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.Visibility_Lambda([this]() { return LayoutMode == EYIItemDashboardLayout::Full ? EVisibility::Visible : EVisibility::Collapsed; })
				.Padding(10)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
					[
						SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_ActionBar", "Actions:"))
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
					[
						SNew(SButton)
							.Text(NSLOCTEXT("YOLOInventory", "Dash_Action_Create", "Create/Update Selected"))
							.OnClicked_Lambda([this]()
								{
									bool bChanged = false;
									if (ListView.IsValid())
									{
										const TArray<TSharedPtr<FYIItemDashboardEntry>> Selected = ListView->GetSelectedItems();
										for (const TSharedPtr<FYIItemDashboardEntry>& E : Selected)
										{
											if (E.IsValid() && E->bIsDataTable)
											{
												bChanged |= CreateAssetFromEntry(*E);
											}
										}
									}
									if (bChanged)
									{
										Refresh();
									}
									return FReply::Handled();
								})
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
					[
						SNew(SButton)
							.Text(NSLOCTEXT("YOLOInventory", "Dash_Action_UpdateLinked", "Update Linked Asset"))
							.OnClicked_Lambda([this]()
								{
									if (!ListView.IsValid())
									{
										return FReply::Handled();
									}
									const TArray<TSharedPtr<FYIItemDashboardEntry>> Selected = ListView->GetSelectedItems();
									bool bChanged = false;
									for (const TSharedPtr<FYIItemDashboardEntry>& E : Selected)
									{
										if (!E.IsValid())
										{
											continue;
										}
										UObject* Obj = E->Object.LoadSynchronous();
										if (UYIItemDefinition* Def = Cast<UYIItemDefinition>(Obj))
										{
											bChanged |= UpdateAssetFromLinkedSource(Def);
										}
									}
									if (bChanged)
									{
										Refresh();
									}
									return FReply::Handled();
								})
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
					[
						SNew(SButton)
							.Text(NSLOCTEXT("YOLOInventory", "Dash_Action_Preflight", "Preflight Selected"))
							.OnClicked_Lambda([this]()
								{
									RebuildPreflightForSelection();
									return FReply::Handled();
								})
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
					[
						SNew(SButton)
							.Text(NSLOCTEXT("YOLOInventory", "Dash_Action_ApplySuggested", "Apply Suggested Mapping"))
							.OnClicked_Lambda([this]()
								{
									ApplySuggestedMappings();
									return FReply::Handled();
								})
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
					[
						SNew(SButton)
							.Text(NSLOCTEXT("YOLOInventory", "Dash_Action_Queue", "Queue Selected"))
							.OnClicked_Lambda([this]()
								{
									EnqueueSelectedRows();
									return FReply::Handled();
								})
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
					[
						SNew(SButton)
							.Text(NSLOCTEXT("YOLOInventory", "Dash_Action_RunQueue", "Run Queue"))
							.OnClicked_Lambda([this]()
								{
									ProcessBatchQueue();
									return FReply::Handled();
								})
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
							.Text_Lambda([this]()
								{
									if (!ListView.IsValid())
									{
										return NSLOCTEXT("YOLOInventory", "Dash_Workflow_NoList", "Workflow: Idle");
									}
									const int32 SelectedCount = ListView->GetNumItemsSelected();
									if (SelectedCount <= 0)
									{
										return NSLOCTEXT("YOLOInventory", "Dash_Workflow_Select", "Workflow: Select row or linked item");
									}
									if (SelectedCount > 1)
									{
										return NSLOCTEXT("YOLOInventory", "Dash_Workflow_Bulk", "Workflow: Bulk validate / generate");
									}
									if (PreflightIssues.Num() > 0)
									{
										for (const TSharedPtr<FYIPreflightIssue>& Issue : PreflightIssues)
										{
											if (Issue.IsValid() && Issue->bBlocking)
											{
												return NSLOCTEXT("YOLOInventory", "Dash_Workflow_Blocking", "Workflow: Fix blocking preflight errors");
											}
										}
										return NSLOCTEXT("YOLOInventory", "Dash_Workflow_Warn", "Workflow: Ready with warnings");
									}
									return NSLOCTEXT("YOLOInventory", "Dash_Workflow_Ready", "Workflow: Ready to generate/update");
								})
					]
				]
		]
		+ SVerticalBox::Slot().FillHeight(1.f).Padding(8, 0, 8, 8)
		[
			SNew(SBorder)
				.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
				.Padding(8)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(12)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0, 0, 8, 0))
						[
							SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "DashboardRefresh", "Refresh"))
								.Visibility_Lambda([this]() { return LayoutMode == EYIItemDashboardLayout::Full ? EVisibility::Visible : EVisibility::Collapsed; })
								.OnClicked_Lambda([this]()
									{
										Refresh();
										return FReply::Handled();
									})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0, 0, 8, 0))
						[
							SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "DashboardCreateSource", "New Data Table Source"))
								.Visibility_Lambda([this]() { return LayoutMode == EYIItemDashboardLayout::Full ? EVisibility::Visible : EVisibility::Collapsed; })
								.OnClicked_Lambda([this]()
									{
										CreateDataTableSourceAsset();
										return FReply::Handled();
									})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0, 0, 8, 0))
						[
							SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "DashboardValidateCodes", "Validate Unique Codes"))
								.Visibility_Lambda([this]() { return LayoutMode == EYIItemDashboardLayout::Full ? EVisibility::Visible : EVisibility::Collapsed; })
								.OnClicked_Lambda([this]()
									{
										ValidateUniqueCodes();
										return FReply::Handled();
									})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(0, 0, 8, 0))
						[
							SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "DashboardBulkCreate", "Create Assets (Selected)"))
								.Visibility_Lambda([this]() { return LayoutMode == EYIItemDashboardLayout::Full ? EVisibility::Visible : EVisibility::Collapsed; })
								.ToolTipText(NSLOCTEXT("YOLOInventory", "DashboardBulkCreate_Tip", "Generate assets for all selected data-table rows"))
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
						+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(8, 0, 0, 0))
						[
							SNew(SComboBox<TSharedPtr<FString>>)
								.OptionsSource(&const_cast<SYIItemDashboard*>(this)->ListTypeOptions)
								.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
									{
										return SNew(STextBlock).Text(InItem.IsValid() ? FText::FromString(*InItem) : FText::GetEmpty());
									})
								.OnComboBoxOpening_Lambda([this]()
									{
										ListTypeOptions.Reset();
										ListTypeOptions.Add(MakeShared<FString>(TEXT("Type: All")));
										ListTypeOptions.Add(MakeShared<FString>(TEXT("Type: Data Rows")));
										ListTypeOptions.Add(MakeShared<FString>(TEXT("Type: Assets Only")));
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
						+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(8, 0, 0, 0))
						[
							SNew(SComboBox<TSharedPtr<FString>>)
								.OptionsSource(&const_cast<SYIItemDashboard*>(this)->ListStatusOptions)
								.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
									{
										return SNew(STextBlock).Text(InItem.IsValid() ? FText::FromString(*InItem) : FText::GetEmpty());
									})
								.OnComboBoxOpening_Lambda([this]()
									{
										ListStatusOptions.Reset();
										ListStatusOptions.Add(MakeShared<FString>(TEXT("Status: All")));
										ListStatusOptions.Add(MakeShared<FString>(TEXT("Status: Needs Asset")));
										ListStatusOptions.Add(MakeShared<FString>(TEXT("Status: Has Asset")));
										ListStatusOptions.Add(MakeShared<FString>(TEXT("Status: Asset Only")));
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
						+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(8, 0, 0, 0))
						[
							SNew(SCheckBox)
								.Style(&FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>("ToggleButtonCheckBox"))
								.Padding(FMargin(6, 2))
								.IsChecked_Lambda([this]() { return bGroupBySource ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
								.OnCheckStateChanged_Lambda([this](ECheckBoxState State)
									{
										bGroupBySource = (State == ECheckBoxState::Checked);
										Refresh();
									})
								[
									SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_GroupBySource", "Group by Source"))
								]
						]
					]
					+ SVerticalBox::Slot().FillHeight(1.f).Padding(12, 8, 12, 12)
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
								+ SHeaderRow::Column("Code").DefaultLabel(NSLOCTEXT("YOLOInventory", "Dash_Code", "Code")).FillWidth(0.15f)
								+ SHeaderRow::Column("Name").DefaultLabel(NSLOCTEXT("YOLOInventory", "Dash_Name", "Name")).FillWidth(0.25f)
								+ SHeaderRow::Column("Template").DefaultLabel(NSLOCTEXT("YOLOInventory", "Dash_Template", "TemplateId")).FillWidth(0.2f)
								+ SHeaderRow::Column("Type").DefaultLabel(NSLOCTEXT("YOLOInventory", "Dash_Type", "Type")).FillWidth(0.1f)
								+ SHeaderRow::Column("Source").DefaultLabel(NSLOCTEXT("YOLOInventory", "Dash_Source", "Source")).FillWidth(0.3f)
								+ SHeaderRow::Column("Asset").DefaultLabel(NSLOCTEXT("YOLOInventory", "Dash_Asset", "Asset")).FillWidth(0.1f)
							)
					]
				]
		];
}

TSharedRef<SWidget> SYIItemDashboard::BuildDetailsPanelWidget()
{
	return SNew(SBorder)
		.Visibility_Lambda([this]()
			{
				return (LayoutMode == EYIItemDashboardLayout::Full && !bShowDetailsPanel) ? EVisibility::Collapsed : EVisibility::Visible;
			})
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		[
			DetailsView.IsValid()
				? StaticCastSharedRef<SWidget>(DetailsView.ToSharedRef())
				: StaticCastSharedRef<SWidget>(SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_NoDetails", "Details panel unavailable")))
		];
}

TSharedRef<SWidget> SYIItemDashboard::BuildMappingPanelWidget()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
		.Padding(8)
		.Visibility_Lambda([this]()
			{
				if (LayoutMode == EYIItemDashboardLayout::Full && !bShowMappingPanel)
				{
					return EVisibility::Collapsed;
				}
				return CurrentMappingSource.IsValid() ? EVisibility::Visible : EVisibility::Collapsed;
			})
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(8)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_InlineMappings", "Inline Mappings"))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
						[
							SNew(SComboBox<TSharedPtr<FString>>)
								.OptionsSource(&const_cast<SYIItemDashboard*>(this)->TargetPropertyOptions)
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
										if (!CurrentMappingSource.IsValid() || !NewItem.IsValid()) return;
										CurrentMappingSource->Modify();
										if (*NewItem == TEXT("Inline Only")) CurrentMappingSource->TransformMode = EYITransformMode::InlineOnly;
										else if (*NewItem == TEXT("Transformer Only")) CurrentMappingSource->TransformMode = EYITransformMode::TransformerOnly;
										else if (*NewItem == TEXT("Inline then Transformer")) CurrentMappingSource->TransformMode = EYITransformMode::HybridInlineThenTransformer;
										else CurrentMappingSource->TransformMode = EYITransformMode::HybridTransformerThenInline;
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
						+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
						[
							SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "Dash_AddMapping", "Add Mapping"))
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
						+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
						[
							SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "Dash_AutoMatch", "Auto Match"))
								.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_AutoMatch_TT", "Match fields by name and update missing source fields."))
								.OnClicked_Lambda([this]()
									{
										AutoMatchInlineMappings(false);
										return FReply::Handled();
									})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
						[
							SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "Dash_AddAllFields", "Add All Fields"))
								.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_AddAllFields_TT", "Add mapping rows for every item definition field, and auto-match where possible."))
								.OnClicked_Lambda([this]()
									{
										AutoMatchInlineMappings(true);
										return FReply::Handled();
									})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
						[
							SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "Dash_ClearMapping", "Clear"))
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
						+ SHorizontalBox::Slot().FillWidth(1.f).HAlign(HAlign_Right).VAlign(VAlign_Center).Padding(8, 0)
						[
							SNew(STextBlock)
								.Text_Lambda([this]()
									{
										if (!CurrentMappingSource.IsValid())
										{
											return NSLOCTEXT("YOLOInventory", "Dash_Confidence_NoSource", "Confidence: N/A");
										}
										const int32 Total = CurrentMappingSource->InlineMappings.Num();
										int32 Ready = 0;
										int32 NeedsFix = 0;
										for (const FYIFieldMapping& M : CurrentMappingSource->InlineMappings)
										{
											const bool bHasSource = !M.SourceField.IsNone();
											const bool bHasTarget = !M.TargetProperty.IsNone();
											if (bHasSource && bHasTarget)
											{
												++Ready;
											}
											else
											{
												++NeedsFix;
											}
										}
										return FText::Format(NSLOCTEXT("YOLOInventory", "Dash_ConfidenceFmt", "Confidence: Ready {0}/{1}, Needs Fix {2}"), FText::AsNumber(Ready), FText::AsNumber(Total), FText::AsNumber(NeedsFix));
									})
						]
				]
				+ SVerticalBox::Slot().FillHeight(1.f).Padding(8, 6)
				[
					SAssignNew(MappingListView, SListView<TSharedPtr<FYIFieldMapping>>)
						.ListItemsSource(&MappingRows)
						.OnGenerateRow(this, &SYIItemDashboard::MakeMappingRow)
						.SelectionMode(ESelectionMode::Single)
				]
		];
}

TSharedRef<SWidget> SYIItemDashboard::BuildPreviewPanelWidget()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(8)
		.Visibility_Lambda([this]()
			{
				return (LayoutMode == EYIItemDashboardLayout::Full && !bShowPreviewPanel) ? EVisibility::Collapsed : EVisibility::Visible;
			})
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(8)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_PreviewTitle", "Live Preview"))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0)
						[
							SNew(SComboBox<TSharedPtr<FString>>)
								.OptionsSource(&const_cast<SYIItemDashboard*>(this)->PreviewRowOptions)
								.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
									{
										return SNew(STextBlock).Text(InItem.IsValid() ? FText::FromString(*InItem) : FText::GetEmpty());
									})
								.OnComboBoxOpening_Lambda([this]()
									{
										PreviewRowOptions.Reset();
										if (CurrentMappingSource.IsValid())
										{
											if (UDataTable* Table = CurrentMappingSource->DataTable.LoadSynchronous())
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
											return PreviewRowName.IsNone() ? NSLOCTEXT("YOLOInventory", "Dash_PreviewRow", "Select Row") : FText::FromName(PreviewRowName);
										})
								]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0)
						[
							SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "Dash_PreviewRefresh", "Refresh"))
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
						.OnGenerateRow(this, &SYIItemDashboard::MakePreviewRow)
				]
		];
}

TSharedRef<SWidget> SYIItemDashboard::BuildPreflightPanelWidget()
{
	return SNew(SBorder)
		.Visibility_Lambda([this]() { return (LayoutMode == EYIItemDashboardLayout::Full && !bShowPreflightPanel) ? EVisibility::Collapsed : EVisibility::Visible; })
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(6, 4)
				[
					SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_PreflightTitle", "Preflight (blocking checks before update/create)"))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0, 2, 0, 0)
						[
							SNew(STextBlock)
								.Text(NSLOCTEXT("YOLOInventory", "Dash_PreflightHelp", "Preflight scans the selected rows/items and reports missing data, invalid mappings, or settings that would block generation. Fix BLOCK entries before create/update."))
								.AutoWrapText(true)
								.ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)))
						]
				]
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SAssignNew(PreflightListView, SListView<TSharedPtr<FYIPreflightIssue>>)
						.ListItemsSource(&PreflightIssues)
						.OnGenerateRow(this, &SYIItemDashboard::MakePreflightRow)
				]
		];
}

TSharedRef<SWidget> SYIItemDashboard::BuildDiffPanelWidget()
{
	return SNew(SBorder)
		.Visibility_Lambda([this]() { return (LayoutMode == EYIItemDashboardLayout::Full && !bShowDiffPanel) ? EVisibility::Collapsed : EVisibility::Visible; })
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(6, 4)
				[
					SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_DiffTitle", "Before/After Diff"))
				]
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SAssignNew(DiffListView, SListView<TSharedPtr<FYIFieldDiffRow>>)
						.ListItemsSource(&DiffRows)
						.OnGenerateRow(this, &SYIItemDashboard::MakeDiffRow)
				]
		];
}

TSharedRef<SWidget> SYIItemDashboard::BuildBatchPanelWidget()
{
	return SNew(SBorder)
		.Visibility_Lambda([this]() { return (LayoutMode == EYIItemDashboardLayout::Full && !bShowBatchPanel) ? EVisibility::Collapsed : EVisibility::Visible; })
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(6, 4)
				[
					SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_BatchTitle", "Batch Queue"))
				]
				+ SVerticalBox::Slot().FillHeight(1.f)
				[
					SAssignNew(BatchQueueListView, SListView<TSharedPtr<FYIBatchJobEntry>>)
						.ListItemsSource(&BatchQueueEntries)
						.OnGenerateRow(this, &SYIItemDashboard::MakeBatchRow)
				]
		];
}

TSharedRef<SWidget> SYIItemDashboard::BuildLogsPanelWidget()
{
	return SNew(SBorder)
		.Visibility_Lambda([this]() { return (LayoutMode == EYIItemDashboardLayout::Full && !bShowLogPanel) ? EVisibility::Collapsed : EVisibility::Visible; })
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.DarkGroupBorder"))
		[
			SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(6, 4)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(STextBlock)
								.Text(NSLOCTEXT("YOLOInventory", "Dash_ErrorsTitle", "Errors & Notifications"))
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
						[
							SNew(SCheckBox)
								.IsChecked_Lambda([this]() { return bShowErrorLogs ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
								.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { bShowErrorLogs = (S == ECheckBoxState::Checked); RefreshLogEntries(); })
								[
									SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_Filter_Error", "Errors"))
								]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
						[
							SNew(SCheckBox)
								.IsChecked_Lambda([this]() { return bShowWarningLogs ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
								.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { bShowWarningLogs = (S == ECheckBoxState::Checked); RefreshLogEntries(); })
								[
									SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_Filter_Warn", "Warnings"))
								]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
						[
							SNew(SCheckBox)
								.IsChecked_Lambda([this]() { return bShowInfoLogs ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
								.OnCheckStateChanged_Lambda([this](ECheckBoxState S) { bShowInfoLogs = (S == ECheckBoxState::Checked); RefreshLogEntries(); })
								[
									SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_Filter_Info", "Info"))
								]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0)
						[
							SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "Dash_ErrorsClear", "Clear"))
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
						.ListItemsSource(&FilteredLogEntries)
						.OnGenerateRow(this, &SYIItemDashboard::MakeLogRow)
				]
		];
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
	FilteredLogEntries.Reset();
	for (const FYIEditorLogEntry& Entry : FYIEditorMessageLog::GetEntries())
	{
		TSharedPtr<FYIEditorLogEntry> Wrapped = MakeShared<FYIEditorLogEntry>(Entry);
		LogEntries.Add(Wrapped);
		const bool bPass =
			(Entry.Severity == EYIEditorLogSeverity::Error && bShowErrorLogs) ||
			(Entry.Severity == EYIEditorLogSeverity::Warning && bShowWarningLogs) ||
			(Entry.Severity == EYIEditorLogSeverity::Info && bShowInfoLogs);
		if (bPass)
		{
			FilteredLogEntries.Add(Wrapped);
		}
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
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
				[
					SNew(STextBlock).Text(TimeText).ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(6, 0)
				[
					SNew(STextBlock)
						.Text(Entry.IsValid() ? Entry->Message : FText::GetEmpty())
						.ColorAndOpacity(FSlateColor(SeverityColor))
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(6, 0)
				[
					SNew(STextBlock)
						.Text(ContextText)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)))
						.WrapTextAt(420.f)
				]
		];
}

bool SYIItemDashboard::RunPreflightForEntry(const FYIItemDashboardEntry& Entry, TArray<FYIPreflightIssue>& OutIssues, bool bLogIssues) const
{
	auto AddIssue = [&](EYIDashboardIssueSeverity Severity, bool bBlocking, const FText& Message, const FText& Context)
	{
		FYIPreflightIssue Issue;
		Issue.Severity = Severity;
		Issue.bBlocking = bBlocking;
		Issue.Message = Message;
		Issue.Context = Context;
		OutIssues.Add(Issue);
		if (bLogIssues)
		{
			FYIEditorMessageLog::Add(
				Severity == EYIDashboardIssueSeverity::Error ? EYIEditorLogSeverity::Error :
				Severity == EYIDashboardIssueSeverity::Warning ? EYIEditorLogSeverity::Warning : EYIEditorLogSeverity::Info,
				Message,
				Context);
		}
	};

	if (Entry.Code == 0)
	{
		AddIssue(EYIDashboardIssueSeverity::Error, true,
			NSLOCTEXT("YOLOInventory", "Dash_Preflight_CodeMissing", "UniqueCode is 0."),
			FText::FromString(Entry.Name));
	}

	if (Entry.bIsDataTable)
	{
		UYIDataTableItemSource* Source = Entry.DataSource.LoadSynchronous();
		if (!Source)
		{
			AddIssue(EYIDashboardIssueSeverity::Error, true,
				NSLOCTEXT("YOLOInventory", "Dash_Preflight_SourceMissing", "Data source is missing or failed to load."),
				FText::FromString(Entry.Source));
			return false;
		}

		UDataTable* Table = Source->DataTable.LoadSynchronous();
		if (!Table || !Table->RowStruct)
		{
			AddIssue(EYIDashboardIssueSeverity::Error, true,
				NSLOCTEXT("YOLOInventory", "Dash_Preflight_TableMissing", "Data table is missing or row struct is invalid."),
				FText::FromString(Source->GetPathName()));
			return false;
		}

		if (!Table->GetRowMap().Contains(Entry.RowName))
		{
			AddIssue(EYIDashboardIssueSeverity::Error, true,
				NSLOCTEXT("YOLOInventory", "Dash_Preflight_RowMissing", "Row does not exist in data table."),
				FText::FromString(Entry.RowName.ToString()));
		}

		const bool bHasInline = Source->bUseInlineMappings && Source->InlineMappings.Num() > 0;
		if (!Source->TransformerClass && !bHasInline)
		{
			AddIssue(EYIDashboardIssueSeverity::Error, true,
				NSLOCTEXT("YOLOInventory", "Dash_Preflight_TransformMissing", "No transformer class and no inline mappings are configured."),
				FText::FromString(Source->GetPathName()));
		}
		if (Source->bUseInlineMappings && Source->TransformMode == EYITransformMode::TransformerOnly)
		{
			AddIssue(EYIDashboardIssueSeverity::Warning, false,
				NSLOCTEXT("YOLOInventory", "Dash_Preflight_InlineIgnored", "Inline mappings are enabled, but TransformMode is TransformerOnly so inline mappings will be ignored."),
				FText::FromString(Source->GetPathName()));
		}
		if (Source->bUseInlineMappings && Source->InlineMappings.Num() == 0)
		{
			AddIssue(EYIDashboardIssueSeverity::Warning, false,
				NSLOCTEXT("YOLOInventory", "Dash_Preflight_InlineEnabledEmpty", "Inline mapping is enabled but has no mappings."),
				FText::FromString(Source->GetPathName()));
		}
	}
	else
	{
		if (UYIItemDefinition* Def = Cast<UYIItemDefinition>(Entry.Object.LoadSynchronous()))
		{
			if (Def->SourceDataSource.IsNull() || Def->SourceRowName.IsNone())
			{
				AddIssue(EYIDashboardIssueSeverity::Info, false,
					NSLOCTEXT("YOLOInventory", "Dash_Preflight_UnlinkedAsset", "Item asset is not linked to a data source row."),
					FText::FromString(Def->GetPathName()));
			}
		}
	}

	bool bHasBlocking = false;
	for (const FYIPreflightIssue& Issue : OutIssues)
	{
		if (Issue.bBlocking)
		{
			bHasBlocking = true;
			break;
		}
	}
	if (!OutIssues.Num())
	{
		AddIssue(EYIDashboardIssueSeverity::Info, false,
			NSLOCTEXT("YOLOInventory", "Dash_Preflight_Clear", "Preflight passed."),
			FText::FromString(Entry.Name));
	}
	return !bHasBlocking;
}

void SYIItemDashboard::RebuildPreflightForSelection()
{
	PreflightIssues.Reset();
	if (ListView.IsValid())
	{
		const TArray<TSharedPtr<FYIItemDashboardEntry>> Selected = ListView->GetSelectedItems();
		for (const TSharedPtr<FYIItemDashboardEntry>& E : Selected)
		{
			if (!E.IsValid())
			{
				continue;
			}
			TArray<FYIPreflightIssue> LocalIssues;
			RunPreflightForEntry(*E, LocalIssues, false);
			for (const FYIPreflightIssue& Issue : LocalIssues)
			{
				PreflightIssues.Add(MakeShared<FYIPreflightIssue>(Issue));
			}
		}
	}
	if (PreflightIssues.Num() == 0)
	{
		TSharedPtr<FYIPreflightIssue> Info = MakeShared<FYIPreflightIssue>();
		Info->Severity = EYIDashboardIssueSeverity::Info;
		Info->Message = NSLOCTEXT("YOLOInventory", "Dash_Preflight_NoSelection", "Select one or more entries to run preflight.");
		PreflightIssues.Add(Info);
	}
	if (PreflightListView.IsValid())
	{
		PreflightListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SYIItemDashboard::MakePreflightRow(TSharedPtr<FYIPreflightIssue> Entry, const TSharedRef<STableViewBase>& Owner)
{
	const FLinearColor SeverityColor = Entry.IsValid() && Entry->Severity == EYIDashboardIssueSeverity::Error
		? FLinearColor(1.f, 0.25f, 0.2f)
		: Entry.IsValid() && Entry->Severity == EYIDashboardIssueSeverity::Warning
		? FLinearColor(1.f, 0.75f, 0.2f)
		: FLinearColor(0.6f, 0.9f, 0.8f);
	return SNew(STableRow<TSharedPtr<FYIPreflightIssue>>, Owner)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
				[
					SNew(STextBlock).Text(Entry.IsValid() && Entry->bBlocking ? NSLOCTEXT("YOLOInventory", "Dash_Preflight_Block", "BLOCK") : NSLOCTEXT("YOLOInventory", "Dash_Preflight_Note", "NOTE"))
						.ColorAndOpacity(FSlateColor(SeverityColor))
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(6, 0)
				[
					SNew(STextBlock).Text(Entry.IsValid() ? Entry->Message : FText::GetEmpty()).ColorAndOpacity(FSlateColor(SeverityColor))
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(6, 0)
				[
					SNew(STextBlock).Text(Entry.IsValid() ? Entry->Context : FText::GetEmpty()).ColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.7f)))
				]
		];
}

void SYIItemDashboard::RebuildDiffForSelection()
{
	DiffRows.Reset();
	if (!ListView.IsValid())
	{
		return;
	}

	const TArray<TSharedPtr<FYIItemDashboardEntry>> Selected = ListView->GetSelectedItems();
	if (Selected.Num() != 1 || !Selected[0].IsValid())
	{
		TSharedPtr<FYIFieldDiffRow> Row = MakeShared<FYIFieldDiffRow>();
		Row->Status = NSLOCTEXT("YOLOInventory", "Dash_Diff_SelectOne", "Select exactly one row for diff.");
		Row->StatusColor = FLinearColor(0.75f, 0.75f, 0.75f);
		DiffRows.Add(Row);
		if (DiffListView.IsValid())
		{
			DiffListView->RequestListRefresh();
		}
		return;
	}

	const TSharedPtr<FYIItemDashboardEntry> Entry = Selected[0];
	UYIItemDefinition* BeforeDef = nullptr;
	if (Entry->ItemAsset.IsValid())
	{
		BeforeDef = Entry->ItemAsset.LoadSynchronous();
	}
	if (!BeforeDef)
	{
		BeforeDef = Cast<UYIItemDefinition>(Entry->Object.LoadSynchronous());
	}

	UYIItemDefinition* AfterDef = nullptr;
	if (GEngine)
	{
		if (UYIItemRegistrySubsystem* Registry = GEngine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
		{
			Registry->BuildIndex(true);
			AfterDef = Registry->GetByCode(Entry->Code);
		}
	}

	if (!BeforeDef && !AfterDef)
	{
		TSharedPtr<FYIFieldDiffRow> Row = MakeShared<FYIFieldDiffRow>();
		Row->Status = NSLOCTEXT("YOLOInventory", "Dash_Diff_NoData", "No asset or transformed output available.");
		Row->StatusColor = FLinearColor(1.f, 0.75f, 0.2f);
		DiffRows.Add(Row);
		if (DiffListView.IsValid())
		{
			DiffListView->RequestListRefresh();
		}
		return;
	}

	int32 ChangedCount = 0;
	for (TFieldIterator<FProperty> It(UYIItemDefinition::StaticClass()); It; ++It)
	{
		FProperty* Prop = *It;
		if (!Prop || Prop->HasAnyPropertyFlags(CPF_Transient))
		{
			continue;
		}
		const uint8* BeforePtr = BeforeDef ? Prop->ContainerPtrToValuePtr<uint8>(BeforeDef) : nullptr;
		const uint8* AfterPtr = AfterDef ? Prop->ContainerPtrToValuePtr<uint8>(AfterDef) : nullptr;

		FString BeforeValue;
		FString AfterValue;
		if (BeforePtr)
		{
			Prop->ExportTextItem_Direct(BeforeValue, BeforePtr, nullptr, nullptr, PPF_None);
		}
		if (AfterPtr)
		{
			Prop->ExportTextItem_Direct(AfterValue, AfterPtr, nullptr, nullptr, PPF_None);
		}

		if (!BeforeValue.Equals(AfterValue, ESearchCase::CaseSensitive))
		{
			++ChangedCount;
			TSharedPtr<FYIFieldDiffRow> Row = MakeShared<FYIFieldDiffRow>();
			Row->FieldName = Prop->GetFName();
			Row->BeforeValue = BeforeValue;
			Row->AfterValue = AfterValue;
			Row->Status = NSLOCTEXT("YOLOInventory", "Dash_Diff_Changed", "Changed");
			Row->StatusColor = FLinearColor(0.95f, 0.75f, 0.2f);
			DiffRows.Add(Row);
		}
	}

	if (ChangedCount == 0)
	{
		TSharedPtr<FYIFieldDiffRow> Row = MakeShared<FYIFieldDiffRow>();
		Row->Status = NSLOCTEXT("YOLOInventory", "Dash_Diff_NoChanges", "No field differences.");
		Row->StatusColor = FLinearColor(0.2f, 0.8f, 0.4f);
		DiffRows.Add(Row);
	}

	if (DiffListView.IsValid())
	{
		DiffListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SYIItemDashboard::MakeDiffRow(TSharedPtr<FYIFieldDiffRow> Row, const TSharedRef<STableViewBase>& Owner)
{
	return SNew(STableRow<TSharedPtr<FYIFieldDiffRow>>, Owner)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.2f).Padding(2, 0)
				[
					SNew(STextBlock).Text(Row.IsValid() ? FText::FromName(Row->FieldName) : FText::GetEmpty())
				]
				+ SHorizontalBox::Slot().FillWidth(0.35f).Padding(2, 0)
				[
					SNew(STextBlock).Text(Row.IsValid() ? FText::FromString(Row->BeforeValue) : FText::GetEmpty())
				]
				+ SHorizontalBox::Slot().FillWidth(0.35f).Padding(2, 0)
				[
					SNew(STextBlock).Text(Row.IsValid() ? FText::FromString(Row->AfterValue) : FText::GetEmpty())
				]
				+ SHorizontalBox::Slot().FillWidth(0.1f).Padding(2, 0)
				[
					SNew(STextBlock).Text(Row.IsValid() ? Row->Status : FText::GetEmpty()).ColorAndOpacity(FSlateColor(Row.IsValid() ? Row->StatusColor : FLinearColor::White))
				]
		];
}

void SYIItemDashboard::EnqueueSelectedRows()
{
	if (!ListView.IsValid())
	{
		return;
	}
	const TArray<TSharedPtr<FYIItemDashboardEntry>> Selected = ListView->GetSelectedItems();
	for (const TSharedPtr<FYIItemDashboardEntry>& E : Selected)
	{
		if (!E.IsValid() || !E->bIsDataTable)
		{
			continue;
		}
		const bool bAlreadyQueued = BatchQueueEntries.ContainsByPredicate([E](const TSharedPtr<FYIBatchJobEntry>& Job)
			{
				return Job.IsValid() && Job->Entry.IsValid() && Job->Entry->Code == E->Code && Job->Entry->RowName == E->RowName && Job->Status == EYIBatchJobStatus::Pending;
			});
		if (bAlreadyQueued)
		{
			continue;
		}
		TSharedPtr<FYIBatchJobEntry> Job = MakeShared<FYIBatchJobEntry>();
		Job->Entry = E;
		Job->Status = EYIBatchJobStatus::Pending;
		Job->Result = NSLOCTEXT("YOLOInventory", "Dash_Batch_Pending", "Pending");
		BatchQueueEntries.Add(Job);
	}
	if (BatchQueueListView.IsValid())
	{
		BatchQueueListView->RequestListRefresh();
	}
}

void SYIItemDashboard::ProcessBatchQueue()
{
	int32 Succeeded = 0;
	int32 Failed = 0;
	for (const TSharedPtr<FYIBatchJobEntry>& Job : BatchQueueEntries)
	{
		if (!Job.IsValid() || !Job->Entry.IsValid() || Job->Status != EYIBatchJobStatus::Pending)
		{
			continue;
		}
		const bool bOk = CreateAssetFromEntry(*Job->Entry);
		Job->Status = bOk ? EYIBatchJobStatus::Succeeded : EYIBatchJobStatus::Failed;
		Job->Result = bOk ? NSLOCTEXT("YOLOInventory", "Dash_Batch_Success", "Success") : NSLOCTEXT("YOLOInventory", "Dash_Batch_Failed", "Failed");
		if (bOk)
		{
			++Succeeded;
		}
		else
		{
			++Failed;
		}
	}
	if (BatchQueueListView.IsValid())
	{
		BatchQueueListView->RequestListRefresh();
	}
	if (Succeeded > 0)
	{
		Refresh();
	}
	FYIEditorMessageLog::Add(
		Failed > 0 ? EYIEditorLogSeverity::Warning : EYIEditorLogSeverity::Info,
		FText::Format(NSLOCTEXT("YOLOInventory", "Dash_Batch_Result", "Batch complete. Success: {0}, Failed: {1}"), FText::AsNumber(Succeeded), FText::AsNumber(Failed)));
}

TSharedRef<ITableRow> SYIItemDashboard::MakeBatchRow(TSharedPtr<FYIBatchJobEntry> Row, const TSharedRef<STableViewBase>& Owner)
{
	const FLinearColor StatusColor = !Row.IsValid() ? FLinearColor::Gray :
		(Row->Status == EYIBatchJobStatus::Succeeded ? FLinearColor(0.2f, 0.8f, 0.4f) :
		(Row->Status == EYIBatchJobStatus::Failed ? FLinearColor(1.f, 0.25f, 0.2f) :
		FLinearColor(0.8f, 0.8f, 0.8f)));
	return SNew(STableRow<TSharedPtr<FYIBatchJobEntry>>, Owner)
		[
			SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(0.2f).Padding(2, 0)
				[
					SNew(STextBlock).Text(Row.IsValid() && Row->Entry.IsValid() ? FText::AsNumber(Row->Entry->Code) : FText::GetEmpty())
				]
				+ SHorizontalBox::Slot().FillWidth(0.45f).Padding(2, 0)
				[
					SNew(STextBlock).Text(Row.IsValid() && Row->Entry.IsValid() ? FText::FromString(Row->Entry->Name) : FText::GetEmpty())
				]
				+ SHorizontalBox::Slot().FillWidth(0.35f).Padding(2, 0)
				[
					SNew(STextBlock).Text(Row.IsValid() ? Row->Result : FText::GetEmpty()).ColorAndOpacity(FSlateColor(StatusColor))
				]
		];
}

void SYIItemDashboard::ApplySuggestedMappings()
{
	AutoMatchInlineMappings(false);
	if (CurrentMappingSource.IsValid() && CurrentMappingSource->InlineMappings.Num() == 0)
	{
		AutoMatchInlineMappings(true);
	}
	RefreshMappingPreview();
}

TSharedRef<ITableRow> SYIItemDashboard::MakeRowWidget(TSharedPtr<FYIItemDashboardEntry> Entry, const TSharedRef<STableViewBase>& Owner)
{
	auto ResolveDraggableDefinition = [this, Entry]() -> UYIItemDefinition*
		{
			if (!Entry.IsValid())
			{
				return nullptr;
			}

			if (!Entry->bIsDataTable)
			{
				return Cast<UYIItemDefinition>(Entry->Object.LoadSynchronous());
			}

			if (Entry->ItemAsset.ToSoftObjectPath().IsValid())
			{
				return Entry->ItemAsset.LoadSynchronous();
			}

			// Data rows without ItemAsset set can still resolve by matching their generated asset row.
			for (const TSharedPtr<FYIItemDashboardEntry>& Candidate : Items)
			{
				if (Candidate.IsValid() && !Candidate->bIsDataTable && Candidate->Code == Entry->Code)
				{
					return Cast<UYIItemDefinition>(Candidate->Object.LoadSynchronous());
				}
			}

			return nullptr;
		};

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
					? NSLOCTEXT("YOLOInventory", "Dash_Status_HasAsset", "Generated asset exists")
					: NSLOCTEXT("YOLOInventory", "Dash_Status_NeedsAsset", "Needs asset generation");
			}
			return NSLOCTEXT("YOLOInventory", "Dash_Status_AssetOnly", "Item asset");
		};

	return SNew(STableRow<TSharedPtr<FYIItemDashboardEntry>>, Owner)
		.ToolTipText(BuildPreviewText(Entry))
		.OnDragDetected_Lambda([ResolveDraggableDefinition](const FGeometry&, const FPointerEvent&)
			{
				if (UYIItemDefinition* Def = ResolveDraggableDefinition())
				{
					TArray<FAssetData> Assets;
					Assets.Add(FAssetData(Def));
					return FReply::Handled().BeginDragDrop(FAssetDragDropOp::New(MoveTemp(Assets)));
				}
				return FReply::Unhandled();
			})
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
						? NSLOCTEXT("YOLOInventory", "Dash_Type_DataTable", "Data Table")
						: NSLOCTEXT("YOLOInventory", "Dash_Type_Asset", "Asset"))
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
							.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_OpenAsset", "Select/Open generated item asset"))
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
								SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_AssetBadge", "Asset"))
							])
						: StaticCastSharedRef<SWidget>(SNew(SSpacer).Size(FVector2D(40.f, 1.f)))
				]
		];
}

void SYIItemDashboard::Refresh()
{
	if (ActiveBottomPanel == EYIDashboardBottomPanel::Preflight && !bShowPreflightPanel)
	{
		ActiveBottomPanel = EYIDashboardBottomPanel::Diff;
	}
	if (ActiveBottomPanel == EYIDashboardBottomPanel::Diff && !bShowDiffPanel)
	{
		ActiveBottomPanel = EYIDashboardBottomPanel::Batch;
	}
	if (ActiveBottomPanel == EYIDashboardBottomPanel::Batch && !bShowBatchPanel)
	{
		ActiveBottomPanel = EYIDashboardBottomPanel::Logs;
	}
	if (ActiveBottomPanel == EYIDashboardBottomPanel::Logs && !bShowLogPanel)
	{
		if (bShowPreflightPanel) ActiveBottomPanel = EYIDashboardBottomPanel::Preflight;
		else if (bShowDiffPanel) ActiveBottomPanel = EYIDashboardBottomPanel::Diff;
		else if (bShowBatchPanel) ActiveBottomPanel = EYIDashboardBottomPanel::Batch;
	}

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
					Entry->DataSource = Def->SourceDataSource;
					Entry->RowName = Def->SourceRowName;
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
	RebuildPreflightForSelection();
	RebuildDiffForSelection();
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
				NSLOCTEXT("YOLOInventory", "DashboardCreateAssetConfirm", "Create an item asset for row '{0}'?"),
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
			NSLOCTEXT("YOLOInventory", "Dash_CreateFail_NoDataSource", "Create failed: entry has no data source."),
			FText::FromString(Entry.Source));
		return false;
	}

	UYIDataTableItemSource* Source = Entry.DataSource.LoadSynchronous();
	if (!Source)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error,
			NSLOCTEXT("YOLOInventory", "Dash_CreateFail_SourceLoad", "Create failed: could not load data source."),
			FText::FromString(Entry.Source));
		return false;
	}

	TArray<FYIPreflightIssue> LocalIssues;
	if (!RunPreflightForEntry(Entry, LocalIssues, true))
	{
		return false;
	}

	const bool bHasInline = Source->bUseInlineMappings && Source->InlineMappings.Num() > 0;

	if (!Source->TransformerClass && !bHasInline)
	{
		const EAppReturnType::Type Res = FMessageDialog::Open(EAppMsgType::YesNo, FText::Format(
			NSLOCTEXT("YOLOInventory", "DashboardMissingTransformer", "Data Source '{0}' has no TransformerClass. Create a transformer Blueprint next to it?"),
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
				NSLOCTEXT("YOLOInventory", "Dash_CreateFail_NoTransformer", "Create failed: transformer missing (and inline mapping disabled)."),
				FText::FromString(Source->GetPathName()));
			return false;
		}
	}

	UDataTable* Table = Source->DataTable.LoadSynchronous();
	if (!Table || !Table->RowStruct)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error,
			NSLOCTEXT("YOLOInventory", "Dash_CreateFail_NoTable", "Create failed: data table missing or has no row struct."),
			FText::FromString(Source->GetPathName()));
		return false;
	}

	const uint8* const* Found = Table->GetRowMap().Find(Entry.RowName);
	const uint8* RowPtr = Found ? *Found : nullptr;
	if (!RowPtr)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error,
			NSLOCTEXT("YOLOInventory", "Dash_CreateFail_NoRow", "Create failed: row not found in data table."),
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
	if (GEngine)
	{
		if (UYIItemRegistrySubsystem* Registry = GEngine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
		{
			// Use a fresh row transform so "Update Linked Asset" always reflects current inline mapping edits.
			Transformed = Registry->TransformRowUncached(
				Entry.RowName,
				Table,
				Source->GetEffectiveTransformerClass(),
				Entry.Code,
				Source);
		}
	}
	if (!Transformed)
	{
		if (TSubclassOf<UCSVDataTransformer> Effective = Source->GetEffectiveTransformerClass())
		{
			if (UCSVDataTransformer* Transformer = NewObject<UCSVDataTransformer>(Source, Effective))
			{
				Transformed = Transformer->TransformObject(RowWrapper);
			}
		}
	}
	UYIItemDefinition* Def = Cast<UYIItemDefinition>(Transformed);
	if (!Def)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error,
			NSLOCTEXT("YOLOInventory", "Dash_CreateFail_Transform", "Create failed: transformer/inline mapping returned null."),
			FText::FromString(Entry.RowName.ToString()));
		return false;
	}

	Def->SourceDataSource = Source;
	Def->SourceRowName = Entry.RowName;
	Def->bGeneratedFromDataSource = true;

	auto RebuildRegistry = []()
	{
		if (GEngine)
		{
			if (UYIItemRegistrySubsystem* Registry = GEngine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
			{
				Registry->BuildIndex(true);
			}
		}
	};

	auto CopyDefinitionProperties = [](const UYIItemDefinition* SourceDef, UYIItemDefinition* DestDef)
	{
		if (!SourceDef || !DestDef)
		{
			return;
		}
		for (TFieldIterator<FProperty> It(UYIItemDefinition::StaticClass()); It; ++It)
		{
			FProperty* Prop = *It;
			if (!Prop || Prop->HasAnyPropertyFlags(CPF_Transient))
			{
				continue;
			}
			const uint8* SrcPtr = Prop->ContainerPtrToValuePtr<uint8>(SourceDef);
			uint8* DstPtr = Prop->ContainerPtrToValuePtr<uint8>(DestDef);
			Prop->CopyCompleteValue(DstPtr, SrcPtr);
		}
	};

	if (UYIItemDefinition* LinkedExisting = Entry.ItemAsset.LoadSynchronous())
	{
		LinkedExisting->Modify();
		CopyDefinitionProperties(Def, LinkedExisting);
		LinkedExisting->MarkPackageDirty();
		RebuildRegistry();
		return true;
	}

	const FString AssetLongPath = PackagePath / AssetName;
	const FString SanitizedPackage = UPackageTools::SanitizePackageName(AssetLongPath);
	const FString ObjectPath = SanitizedPackage + TEXT(".") + AssetName;

	if (UYIItemDefinition* Existing = Cast<UYIItemDefinition>(StaticLoadObject(UYIItemDefinition::StaticClass(), nullptr, *ObjectPath)))
	{
		// Update existing asset from transformed output so inline/transformer behavior is preserved.
		Existing->Modify();
		CopyDefinitionProperties(Def, Existing);
		Existing->MarkPackageDirty();
		RebuildRegistry();
		return true;
	}

	UPackage* Pkg = CreatePackage(*SanitizedPackage);
	UObject* NewAsset = NewObject<UObject>(Pkg, Def->GetClass(), *AssetName, RF_Public | RF_Standalone | RF_Transactional, Def);
	if (!NewAsset)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error,
			NSLOCTEXT("YOLOInventory", "Dash_CreateFail_NewAsset", "Create failed: could not create asset object."),
			FText::FromString(ObjectPath));
		return false;
	}

	FAssetRegistryModule::AssetCreated(NewAsset);
	Pkg->MarkPackageDirty();
	RebuildRegistry();
	return true;
}

bool SYIItemDashboard::UpdateAssetFromLinkedSource(UYIItemDefinition* ItemDef) const
{
	if (!IsValid(ItemDef))
	{
		return false;
	}
	UYIDataTableItemSource* Source = ItemDef->SourceDataSource.LoadSynchronous();
	if (!Source || ItemDef->SourceRowName.IsNone())
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Warning,
			NSLOCTEXT("YOLOInventory", "Dash_UpdateLinked_NoSource", "Update failed: selected item has no linked data source/row."),
			FText::FromString(ItemDef->GetPathName()));
		return false;
	}

	FYIItemDashboardEntry TempEntry;
	TempEntry.bIsDataTable = true;
	TempEntry.Code = ItemDef->UniqueCode;
	TempEntry.RowName = ItemDef->SourceRowName;
	TempEntry.DataSource = Source;
	TempEntry.Source = Source->GetPathName();
	TempEntry.ItemAsset = ItemDef;
	TempEntry.bHasAsset = true;
	return CreateAssetFromEntry(TempEntry);
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

		if (ItemDef->SourceDataSource.IsValid() && !ItemDef->SourceRowName.IsNone())
		{
			MenuBuilder.AddMenuEntry(
				NSLOCTEXT("YOLOInventory", "Dash_Context_UpdateFromLinkedSource", "Update from Linked Data Source"),
				NSLOCTEXT("YOLOInventory", "Dash_Context_UpdateFromLinkedSource_Tip", "Re-run generation using this item's linked data source and row."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([Self, ItemDef]()
					{
						if (Self && IsValid(ItemDef) && Self->UpdateAssetFromLinkedSource(ItemDef))
						{
							Self->Refresh();
						}
					})));
		}
	}

	if (!Entry->bIsDataTable)
	{
	}
	else
	{
		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("YOLOInventory", "Dash_Context_PreflightRow", "Preflight Row"),
			NSLOCTEXT("YOLOInventory", "Dash_Context_PreflightRow_Tip", "Run preflight validation for this row."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([Self, Entry]()
				{
					if (!Self || !Entry.IsValid())
					{
						return;
					}
					TArray<FYIPreflightIssue> Issues;
					Self->RunPreflightForEntry(*Entry, Issues, true);
					Self->RebuildPreflightForSelection();
				})));

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("YOLOInventory", "Dash_Context_QueueRow", "Queue Row"),
			NSLOCTEXT("YOLOInventory", "Dash_Context_QueueRow_Tip", "Add this row to the batch queue."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([Self, Entry]()
				{
					if (!Self || !Entry.IsValid())
					{
						return;
					}
					if (Self->ListView.IsValid())
					{
						Self->ListView->SetSelection(Entry);
					}
					Self->EnqueueSelectedRows();
				})));

		MenuBuilder.AddSeparator();

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
		else if (UYIItemDefinition* ItemDef = Cast<UYIItemDefinition>(Target))
		{
			if (ItemDef->SourceDataSource.IsValid())
			{
				CurrentMappingSource = ItemDef->SourceDataSource.LoadSynchronous();
				RefreshInlineMappingEditor(CurrentMappingSource.Get());
			}
			else if (Entry->DataSource.IsValid())
			{
				CurrentMappingSource = Entry->DataSource.LoadSynchronous();
				RefreshInlineMappingEditor(CurrentMappingSource.Get());
			}
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
	RebuildPreflightForSelection();
	RebuildDiffForSelection();
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
				NSLOCTEXT("YOLOInventory", "Dash_ConfirmCreate", "Create/update item assets for {0} selected row(s)?"),
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
			NSLOCTEXT("YOLOInventory", "Dash_ConfirmDelete", "Delete {0} asset(s)? This cannot be undone."),
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
		const UClass* OwnerClass = It->GetOwnerClass();
		if (OwnerClass == UPrimaryDataAsset::StaticClass() || OwnerClass == UObject::StaticClass())
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

void SYIItemDashboard::BuildTransformFunctionOptions()
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

void SYIItemDashboard::RefreshMappingPreview()
{
	MappingPreviewRows.Reset();

	if (!CurrentMappingSource.IsValid())
	{
		return;
	}

	UYIDataTableItemSource* Source = CurrentMappingSource.Get();
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
		TArray<FName> RowNames = Table->GetRowNames();
		RowNames.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
		if (RowNames.Num() == 0)
		{
			return;
		}
		PreviewRowName = RowNames[0];
		FoundRow = Table->GetRowMap().Find(PreviewRowName);
		RowPtr = FoundRow ? *FoundRow : nullptr;
		if (!RowPtr)
		{
			return;
		}
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
			Row->Status = NSLOCTEXT("YOLOInventory", "Dash_PreviewMissing", "Missing field.");
			Row->StatusColor = FLinearColor(1.f, 0.25f, 0.2f);
			MappingPreviewRows.Add(Row);
			continue;
		}

		const uint8* SrcPtr = SourceProp ? SourceProp->ContainerPtrToValuePtr<uint8>(RowPtr) : nullptr;
		Row->SourceValue = Mapping.bUseStaticValue
			? Mapping.StaticValue
			: ExportPropertyValueToString(SourceProp, SrcPtr);

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
				bConverted = SetPropertyValueFromStringEditor(Mapping.StaticValue, TargetProp, Temp.GetData(), Mapping.Conversion);
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
						Row->ConvertedValue = ExportPropertyValueToString(TargetProp, Temp.GetData());
					}
					else if (DestStruct->Struct == TBaseStructure<FIntPoint>::Get())
					{
						FIntPoint* Pt = reinterpret_cast<FIntPoint*>(Temp.GetData());
						*Pt = FIntPoint((int32)X, (int32)Y);
						bConverted = true;
						Row->ConvertedValue = ExportPropertyValueToString(TargetProp, Temp.GetData());
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
			bConverted = CopyValueBetweenPropertiesEditor(SourceProp, SrcPtr, TargetProp, Temp.GetData(), Mapping.Conversion);
		}

		if (bConverted)
		{
			Row->ConvertedValue = ExportPropertyValueToString(TargetProp, Temp.GetData());
		}
		else if (Row->ConvertedValue.IsEmpty())
		{
			Row->ConvertedValue = TEXT("<conversion failed>");
			bWarn = true;
		}

		if (!Mapping.TransformFunction.IsNone())
		{
			FString TransformError;
			const bool bTransformed = ApplyTransformFunctionEditor(Mapping, TargetProp, Temp.GetData(), &TransformError);
			if (bTransformed)
			{
				Row->TransformedValue = ExportPropertyValueToString(TargetProp, Temp.GetData());
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
			Row->Status = NSLOCTEXT("YOLOInventory", "Dash_PreviewWarn", "Warn");
			Row->StatusColor = FLinearColor(1.f, 0.75f, 0.2f);
		}
		else
		{
			Row->Status = NSLOCTEXT("YOLOInventory", "Dash_PreviewOk", "OK");
			Row->StatusColor = FLinearColor(0.2f, 0.8f, 0.4f);
		}
		MappingPreviewRows.Add(Row);
	}

	if (MappingPreviewListView.IsValid())
	{
		MappingPreviewListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> SYIItemDashboard::MakePreviewRow(TSharedPtr<FYIMappingPreviewRow> Row, const TSharedRef<STableViewBase>& Owner)
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
			NSLOCTEXT("YOLOInventory", "Dash_AutoMatch_NoTable", "Auto-match failed: data table missing or no row struct."),
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

	bool bChanged = false;

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

	int32 MatchCount = 0;

	// First, patch existing mappings that have missing source fields
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
			M.Conversion = GuessConversionForProps(SourceProp, TargetProp);
			bChanged = true;
			++MatchCount;
		}
	}

	for (TFieldIterator<FProperty> It(UYIItemDefinition::StaticClass()); It; ++It)
	{
		const UClass* OwnerClass = It->GetOwnerClass();
		if (OwnerClass == UPrimaryDataAsset::StaticClass() || OwnerClass == UObject::StaticClass())
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
		NewMap.Conversion = GuessConversionForProps(SourceFieldPropCache.FindRef(Match), *It);
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
			FText::Format(NSLOCTEXT("YOLOInventory", "Dash_AutoMatch_Done", "Auto-match updated inline mappings. Matched {0} fields."), FText::AsNumber(MatchCount)),
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
	auto RefreshMappingUi = [this]()
		{
			RefreshMappingPreview();
			if (MappingListView.IsValid())
			{
				MappingListView->RequestListRefresh();
			}
		};

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
					.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_MappingMissing", "Missing source or target field."));
			}
			if (bStatic && Mapping->Conversion == EYIFieldMappingConversion::Vector2DFromXY)
			{
				return SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
					.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_MappingStaticVector", "Static value does not support Vector2D conversion."));
			}
			if (Mapping->Conversion == EYIFieldMappingConversion::Vector2DFromXY)
			{
				if (Mapping->SourceFieldB.IsNone())
				{
					return SNew(SImage)
						.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
						.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_MappingMissingB", "Vector2D conversion requires Source B."));
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
						.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_MappingMissingBField", "Vector2D conversion: Source B field not found."));
				}
				if (!CastField<FNumericProperty>(SourceProp) || !CastField<FNumericProperty>(SourcePropB))
				{
					return SNew(SImage)
						.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
						.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_MappingVectorNumeric", "Vector2D conversion requires numeric fields."));
				}
				if (const FStructProperty* DestStruct = CastField<FStructProperty>(TargetProp))
				{
					if (DestStruct->Struct == TBaseStructure<FVector2D>::Get() || DestStruct->Struct == TBaseStructure<FIntPoint>::Get())
					{
						return SNew(SImage)
							.Image(FAppStyle::Get().GetBrush("Icons.Check"))
							.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_MappingOk", "Mapping looks OK."));
					}
				}
				return SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
					.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_MappingVectorTarget", "Vector2D conversion target must be FVector2D or FIntPoint."));
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
					.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_MappingTypeMismatch", "Type mismatch. Add a conversion."));
			}
			if (Mapping->Conversion == EYIFieldMappingConversion::ToEnum)
			{
				if (!(CastField<FEnumProperty>(TargetProp) || (CastField<FByteProperty>(TargetProp) && CastField<FByteProperty>(TargetProp)->Enum)))
				{
					return SNew(SImage)
						.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
						.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_MappingEnumTarget", "Enum conversion requires an enum target."));
				}
			}

			if (Mapping->TransformFunction.IsNone())
			{
				return SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Check"))
					.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_MappingOk", "Mapping looks OK."));
			}

			UClass* LibraryClass = Mapping->TransformLibrary.LoadSynchronous();
			if (!LibraryClass)
			{
				return SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
					.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_MappingTransformMissing", "Transform library not found."));
			}

			UFunction* Function = LibraryClass->FindFunctionByName(Mapping->TransformFunction);
			if (!Function)
			{
				return SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
					.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_MappingTransformFuncMissing", "Transform function not found."));
			}

			FProperty* InputProp = nullptr;
			FProperty* OutputProp = nullptr;
			if (!GetTransformFunctionProps(Function, InputProp, OutputProp))
			{
				return SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
					.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_MappingTransformSigBad", "Transform signature should be (In) -> Out or (In, Out)."));
			}

			if (!IsCompatible(TargetProp, InputProp))
			{
				return SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
					.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_MappingTransformInputBad", "Transform input type does not match target type."));
			}
			if (!IsCompatible(OutputProp, TargetProp))
			{
				return SNew(SImage)
					.Image(FAppStyle::Get().GetBrush("Icons.Warning"))
					.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_MappingTransformOutputBad", "Transform output type does not match target type."));
			}

			return SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("Icons.Check"))
				.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_MappingOk", "Mapping looks OK."));
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
						.OnSelectionChanged_Lambda([this, Mapping, GetSourceProp, GetTargetProp, RefreshMappingUi](TSharedPtr<FString> NewItem, ESelectInfo::Type)
							{
								if (CurrentMappingSource.IsValid() && Mapping.IsValid() && NewItem.IsValid())
								{
									CurrentMappingSource->Modify();
									Mapping->SourceField = FName(**NewItem);
									if (Mapping->Conversion == EYIFieldMappingConversion::None)
									{
										const FProperty* SourceProp = GetSourceProp();
										const FProperty* TargetProp = GetTargetProp();
										const EYIFieldMappingConversion Guess = GuessConversionForProps(SourceProp, TargetProp);
										Mapping->Conversion = Guess;
									}
									const int32 Index = MappingRows.Find(Mapping);
									if (Index != INDEX_NONE)
									{
										CurrentMappingSource->InlineMappings[Index].SourceField = Mapping->SourceField;
										CurrentMappingSource->InlineMappings[Index].Conversion = Mapping->Conversion;
									}
									RefreshMappingUi();
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
						.OptionsSource(&const_cast<SYIItemDashboard*>(this)->SourceFieldOptions)
						.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
							{
								return SNew(STextBlock).Text(InItem.IsValid() ? FText::FromString(*InItem) : FText::GetEmpty());
							})
						.OnSelectionChanged_Lambda([this, Mapping, RefreshMappingUi](TSharedPtr<FString> NewItem, ESelectInfo::Type)
							{
								if (CurrentMappingSource.IsValid() && Mapping.IsValid() && NewItem.IsValid())
								{
									CurrentMappingSource->Modify();
									Mapping->SourceFieldB = FName(**NewItem);
									const int32 Index = MappingRows.Find(Mapping);
									if (Index != INDEX_NONE)
									{
										CurrentMappingSource->InlineMappings[Index].SourceFieldB = Mapping->SourceFieldB;
									}
									RefreshMappingUi();
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
										: NSLOCTEXT("YOLOInventory", "Dash_SourceFieldB", "Source B");
								})
						]
				]
			+ SHorizontalBox::Slot().FillWidth(0.16f).Padding(2)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
						[
							SNew(SCheckBox)
								.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_StaticValue_TT", "Use a static value instead of a source field."))
								.IsChecked_Lambda([IsStaticMapping]() { return IsStaticMapping() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
								.OnCheckStateChanged_Lambda([this, Mapping, RefreshMappingUi](ECheckBoxState State)
									{
										if (CurrentMappingSource.IsValid() && Mapping.IsValid())
										{
											CurrentMappingSource->Modify();
											Mapping->bUseStaticValue = (State == ECheckBoxState::Checked);
											const int32 Index = MappingRows.Find(Mapping);
											if (Index != INDEX_NONE)
											{
												CurrentMappingSource->InlineMappings[Index].bUseStaticValue = Mapping->bUseStaticValue;
											}
											RefreshMappingUi();
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
										.HintText(NSLOCTEXT("YOLOInventory", "Dash_StaticValueHint", "Static value"))
										.OnTextCommitted_Lambda([this, Mapping, RefreshMappingUi](const FText& NewText, ETextCommit::Type)
											{
												if (CurrentMappingSource.IsValid() && Mapping.IsValid())
												{
													CurrentMappingSource->Modify();
													Mapping->StaticValue = NewText.ToString();
													const int32 Index = MappingRows.Find(Mapping);
													if (Index != INDEX_NONE)
													{
														CurrentMappingSource->InlineMappings[Index].StaticValue = Mapping->StaticValue;
													}
													RefreshMappingUi();
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
										.OnSelectionChanged_Lambda([this, Mapping, RefreshMappingUi](TSharedPtr<FString> NewItem, ESelectInfo::Type)
											{
												if (CurrentMappingSource.IsValid() && Mapping.IsValid() && NewItem.IsValid())
												{
													CurrentMappingSource->Modify();
													Mapping->StaticValue = *NewItem;
													Mapping->bUseStaticValue = true;
													const int32 Index = MappingRows.Find(Mapping);
													if (Index != INDEX_NONE)
													{
														CurrentMappingSource->InlineMappings[Index].StaticValue = Mapping->StaticValue;
														CurrentMappingSource->InlineMappings[Index].bUseStaticValue = Mapping->bUseStaticValue;
													}
													RefreshMappingUi();
												}
											})
										.Content()
										[
											SNew(STextBlock).Text_Lambda([Mapping, GetTargetEnum]()
												{
													if (!Mapping.IsValid() || Mapping->StaticValue.IsEmpty())
													{
														return NSLOCTEXT("YOLOInventory", "Dash_StaticEnumHint", "Select enum");
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
						.OnSelectionChanged_Lambda([this, Mapping, GetSourceProp, GetTargetProp, RefreshMappingUi](TSharedPtr<FString> NewItem, ESelectInfo::Type)
							{
								if (CurrentMappingSource.IsValid() && Mapping.IsValid() && NewItem.IsValid())
								{
									CurrentMappingSource->Modify();
									Mapping->TargetProperty = FName(**NewItem);
									if (Mapping->Conversion == EYIFieldMappingConversion::None)
									{
										const FProperty* SourceProp = GetSourceProp();
										const FProperty* TargetProp = GetTargetProp();
										const EYIFieldMappingConversion Guess = GuessConversionForProps(SourceProp, TargetProp);
										Mapping->Conversion = Guess;
									}
									const int32 Index = MappingRows.Find(Mapping);
									if (Index != INDEX_NONE)
									{
										CurrentMappingSource->InlineMappings[Index].TargetProperty = Mapping->TargetProperty;
										CurrentMappingSource->InlineMappings[Index].Conversion = Mapping->Conversion;
									}
									RefreshMappingUi();
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
						.OptionsSource(&const_cast<SYIItemDashboard*>(this)->ConverterOptions)
						.OnGenerateWidget_Lambda([DropdownText](TSharedPtr<FString> InItem)
							{
								return SNew(STextBlock).Text(DropdownText(InItem));
							})
						.OnSelectionChanged_Lambda([this, Mapping, RefreshMappingUi](TSharedPtr<FString> NewItem, ESelectInfo::Type)
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
									else if (*NewItem == TEXT("To Enum")) NewConv = EYIFieldMappingConversion::ToEnum;
									else if (*NewItem == TEXT("To Gameplay Tag")) NewConv = EYIFieldMappingConversion::ToGameplayTag;
									else if (*NewItem == TEXT("To Texture (Soft)")) NewConv = EYIFieldMappingConversion::ToSoftTexture;
									else if (*NewItem == TEXT("Vector2D from XY Fields")) NewConv = EYIFieldMappingConversion::Vector2DFromXY;
									Mapping->Conversion = NewConv;
									const int32 Index = MappingRows.Find(Mapping);
									if (Index != INDEX_NONE)
									{
										CurrentMappingSource->InlineMappings[Index].Conversion = Mapping->Conversion;
									}
									RefreshMappingUi();
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
						.OptionsSource(&const_cast<SYIItemDashboard*>(this)->TransformFunctionOptions)
						.OnGenerateWidget_Lambda([](TSharedPtr<FYITransformFunctionInfo> InItem)
							{
								return SNew(STextBlock).Text(InItem.IsValid() ? FText::FromString(InItem->DisplayName) : FText::GetEmpty());
							})
						.OnComboBoxOpening_Lambda([this]()
							{
								BuildTransformFunctionOptions();
							})
						.OnSelectionChanged_Lambda([this, Mapping, RefreshMappingUi](TSharedPtr<FYITransformFunctionInfo> NewItem, ESelectInfo::Type)
							{
								if (CurrentMappingSource.IsValid() && Mapping.IsValid() && NewItem.IsValid())
								{
									CurrentMappingSource->Modify();
									Mapping->TransformFunction = NewItem->FunctionName;
									Mapping->TransformLibrary = NewItem->Library;
									const int32 Index = MappingRows.Find(Mapping);
									if (Index != INDEX_NONE)
									{
										CurrentMappingSource->InlineMappings[Index].TransformFunction = Mapping->TransformFunction;
										CurrentMappingSource->InlineMappings[Index].TransformLibrary = Mapping->TransformLibrary;
									}
									RefreshMappingUi();
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
									return NSLOCTEXT("YOLOInventory", "Dash_TransformNone", "None");
								})
						]
				]
			+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					SNew(SButton)
						.Text(NSLOCTEXT("YOLOInventory", "Dash_RemoveMapping", "X"))
						.OnClicked_Lambda([this, Mapping, RefreshMappingUi]()
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
										RefreshMappingUi();
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
