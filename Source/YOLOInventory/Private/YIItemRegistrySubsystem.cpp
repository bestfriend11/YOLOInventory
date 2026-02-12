#include "YIItemRegistrySubsystem.h"
#include "YIItemDefinition.h"
#include "Data/YIDataTableItemSource.h"
#include "CSVDataTransformer.h"
#include "RowData.h"
#include "Engine/DataTable.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "Data/YIDataTableItemSource.h"
#include "YIItemDefinition.h"
#include "UObject/StructOnScope.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "Engine/Texture.h"

DEFINE_LOG_CATEGORY_STATIC(LogYIItemRegistry, Log, All);

void UYIItemRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bIndexed = false;
}

// Uses the authored name (source field name from the table/CSV) to match, avoiding display-name ambiguity.
static bool MatchesFieldName(const FProperty* Prop, FName FieldName)
{
	if (!Prop || FieldName.IsNone())
	{
		return false;
	}

	const FString Authored = Prop->GetAuthoredName();
	const FString Target = FieldName.ToString();
	return Authored.Equals(Target, ESearchCase::IgnoreCase);
}

static bool TryGetEnumValueFromString(const UEnum* Enum, const FString& Value, int64& OutValue)
{
	if (!Enum)
	{
		return false;
	}

	int64 NumericValue = 0;
	if (LexTryParseString(NumericValue, *Value))
	{
		OutValue = NumericValue;
		return true;
	}

	// Try direct lookup (case sensitive)
	int64 Found = Enum->GetValueByNameString(Value);
	if (Found != INDEX_NONE)
	{
		OutValue = Found;
		return true;
	}

	// Case-insensitive fallback
	for (int32 Index = 0; Index < Enum->NumEnums(); ++Index)
	{
		const FString Name = Enum->GetNameStringByIndex(Index);
		if (Name.Equals(Value, ESearchCase::IgnoreCase))
		{
			OutValue = Enum->GetValueByIndex(Index);
			return true;
		}
	}

	return false;
}

static bool SetEnumPropertyValue(FProperty* DestProp, uint8* DestPtr, const FString& Value)
{
	if (!DestProp || !DestPtr)
	{
		return false;
	}

	if (FEnumProperty* DestEnum = CastField<FEnumProperty>(DestProp))
	{
		const UEnum* Enum = DestEnum->GetEnum();
		int64 EnumValue = 0;
		if (!TryGetEnumValueFromString(Enum, Value, EnumValue))
		{
			return false;
		}
		if (FNumericProperty* Underlying = DestEnum->GetUnderlyingProperty())
		{
			Underlying->SetIntPropertyValue(DestPtr, EnumValue);
			return true;
		}
		return false;
	}

	if (FByteProperty* DestByte = CastField<FByteProperty>(DestProp))
	{
		if (const UEnum* Enum = DestByte->Enum)
		{
			int64 EnumValue = 0;
			if (!TryGetEnumValueFromString(Enum, Value, EnumValue))
			{
				return false;
			}
			DestByte->SetPropertyValue(DestPtr, (uint8)EnumValue);
			return true;
		}
	}

	return false;
}

static bool SetPropertyValueFromString(const FString& Value, FProperty* DestProp, uint8* DestPtr, EYIFieldMappingConversion Conversion)
{
	if (!DestProp || !DestPtr)
	{
		return false;
	}

	if (SetEnumPropertyValue(DestProp, DestPtr, Value))
	{
		return true;
	}

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
	if (FBoolProperty* DestBool = CastField<FBoolProperty>(DestProp))
	{
		bool bParsed = false;
		const bool bVal = LexTryParseString(bParsed, *Value) ? bParsed : !Value.IsEmpty();
		DestBool->SetPropertyValue(DestPtr, bVal);
		return true;
	}
	if (FNumericProperty* DestNum = CastField<FNumericProperty>(DestProp))
	{
		double NumVal = 0.0;
		if (LexTryParseString(NumVal, *Value))
		{
			if (DestNum->IsInteger())
			{
				DestNum->SetIntPropertyValue(DestPtr, (int64)NumVal);
			}
			else
			{
				DestNum->SetFloatingPointPropertyValue(DestPtr, NumVal);
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
				FGameplayTag* TagPtr = reinterpret_cast<FGameplayTag*>(DestPtr);
				*TagPtr = FGameplayTag::RequestGameplayTag(FName(*Value), false);
				return true;
			}
		}
	}

	return DestProp->ImportText_Direct(*Value, DestPtr, nullptr, PPF_None) != nullptr;
}

int64 UYIItemRegistrySubsystem::ExtractCodeFromRow(const UScriptStruct* Struct, const uint8* RowData, FName FieldName) const
{
	if (!Struct || !RowData || FieldName.IsNone())
	{
		return 0;
	}

	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		const FProperty* Prop = *It;
		if (MatchesFieldName(Prop, FieldName))
		{
			if (const FInt64Property* Int64Prop = CastField<FInt64Property>(Prop))
			{
				return Int64Prop->GetPropertyValue_InContainer(RowData);
			}
			if (const FIntProperty* IntProp = CastField<FIntProperty>(Prop))
			{
				return (int64)IntProp->GetPropertyValue_InContainer(RowData);
			}
		}
	}
	return 0;
}

FString UYIItemRegistrySubsystem::ExtractTemplateIdFromRow(const UScriptStruct* Struct, const uint8* RowData, FName FieldName) const
{
	if (!Struct || !RowData || FieldName.IsNone())
	{
		return FString();
	}

	for (TFieldIterator<FProperty> It(Struct); It; ++It)
	{
		const FProperty* Prop = *It;
		if (MatchesFieldName(Prop, FieldName))
		{
			if (const FStrProperty* StrProp = CastField<FStrProperty>(Prop))
			{
				return StrProp->GetPropertyValue_InContainer(RowData);
			}
			if (const FNameProperty* NameProp = CastField<FNameProperty>(Prop))
			{
				return NameProp->GetPropertyValue_InContainer(RowData).ToString();
			}
		}
	}
	return FString();
}

static bool CopyValueBetweenProperties(const FProperty* SourceProp, const uint8* SourcePtr, FProperty* DestProp, uint8* DestPtr, EYIFieldMappingConversion Conversion)
{
	if (!SourceProp || !DestProp || !SourcePtr || !DestPtr)
	{
		return false;
	}

	// Exact type match
	if (SourceProp->SameType(DestProp))
	{
		SourceProp->CopyCompleteValue(DestPtr, SourcePtr);
		return true;
	}

	// Enum conversions
	if (FEnumProperty* DestEnum = CastField<FEnumProperty>(DestProp))
	{
		if (const FEnumProperty* SrcEnum = CastField<FEnumProperty>(SourceProp))
		{
			if (SrcEnum->GetEnum() == DestEnum->GetEnum())
			{
				if (FNumericProperty* Underlying = DestEnum->GetUnderlyingProperty())
				{
					const int64 Val = SrcEnum->GetUnderlyingProperty()->GetSignedIntPropertyValue(SourcePtr);
					Underlying->SetIntPropertyValue(DestPtr, Val);
					return true;
				}
			}
		}
		if (const FNumericProperty* SrcNum = CastField<FNumericProperty>(SourceProp))
		{
			const int64 Val = SrcNum->GetSignedIntPropertyValue(SourcePtr);
			if (FNumericProperty* Underlying = DestEnum->GetUnderlyingProperty())
			{
				Underlying->SetIntPropertyValue(DestPtr, Val);
				return true;
			}
		}
		if (const FNameProperty* SrcName = CastField<FNameProperty>(SourceProp))
		{
			return SetEnumPropertyValue(DestProp, DestPtr, SrcName->GetPropertyValue(SourcePtr).ToString());
		}
		if (const FStrProperty* SrcStr = CastField<FStrProperty>(SourceProp))
		{
			return SetEnumPropertyValue(DestProp, DestPtr, SrcStr->GetPropertyValue(SourcePtr));
		}
		if (const FTextProperty* SrcText = CastField<FTextProperty>(SourceProp))
		{
			return SetEnumPropertyValue(DestProp, DestPtr, SrcText->GetPropertyValue(SourcePtr).ToString());
		}
	}
	if (FByteProperty* DestByte = CastField<FByteProperty>(DestProp))
	{
		if (DestByte->Enum)
		{
			if (const FNumericProperty* SrcNum = CastField<FNumericProperty>(SourceProp))
			{
				const int64 Val = SrcNum->GetSignedIntPropertyValue(SourcePtr);
				DestByte->SetPropertyValue(DestPtr, (uint8)Val);
				return true;
			}
			if (const FNameProperty* SrcName = CastField<FNameProperty>(SourceProp))
			{
				return SetEnumPropertyValue(DestProp, DestPtr, SrcName->GetPropertyValue(SourcePtr).ToString());
			}
			if (const FStrProperty* SrcStr = CastField<FStrProperty>(SourceProp))
			{
				return SetEnumPropertyValue(DestProp, DestPtr, SrcStr->GetPropertyValue(SourcePtr));
			}
			if (const FTextProperty* SrcText = CastField<FTextProperty>(SourceProp))
			{
				return SetEnumPropertyValue(DestProp, DestPtr, SrcText->GetPropertyValue(SourcePtr).ToString());
			}
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

	// Simple coercions (string/text/name)
	if (const FStrProperty* SrcStr = CastField<FStrProperty>(SourceProp))
	{
		FString Value = SrcStr->GetPropertyValue(SourcePtr);
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
		if (Conversion == EYIFieldMappingConversion::ToEnum)
		{
			return SetEnumPropertyValue(DestProp, DestPtr, Value);
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
		if (Conversion == EYIFieldMappingConversion::ToEnum)
		{
			return SetEnumPropertyValue(DestProp, DestPtr, Value.ToString());
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
		if (Conversion == EYIFieldMappingConversion::ToEnum)
		{
			return SetEnumPropertyValue(DestProp, DestPtr, Value.ToString());
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

	// Numeric conversions (int/float)
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
				// Handle signed/unsigned integers safely
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

	// Bool conversions (bool <-> numeric/bool)
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

static bool ApplyTransformFunction(const FYIFieldMapping& Mapping, FProperty* DestProp, uint8* DestPtr)
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
		return false;
	}

	UFunction* Function = LibraryClass->FindFunctionByName(Mapping.TransformFunction);
	if (!Function)
	{
		return false;
	}

	FProperty* InputProp = nullptr;
	FProperty* OutputProp = nullptr;
	if (!GetTransformFunctionProps(Function, InputProp, OutputProp))
	{
		return false;
	}

	FStructOnScope Params(Function);
	uint8* ParamsMem = Params.GetStructMemory();

	uint8* InputPtr = InputProp->ContainerPtrToValuePtr<uint8>(ParamsMem);
	uint8* OutputPtr = OutputProp->ContainerPtrToValuePtr<uint8>(ParamsMem);

	if (!CopyValueBetweenProperties(DestProp, DestPtr, InputProp, InputPtr, EYIFieldMappingConversion::None))
	{
		return false;
	}

	UObject* CDO = LibraryClass->GetDefaultObject();
	if (!CDO)
	{
		return false;
	}

	CDO->ProcessEvent(Function, ParamsMem);

	if (!CopyValueBetweenProperties(OutputProp, OutputPtr, DestProp, DestPtr, EYIFieldMappingConversion::None))
	{
		return false;
	}

	return true;
}

static bool ApplyInlineMappings(const UYIDataTableItemSource* Source, const UDataTable* DataTable, FName RowName, UYIItemDefinition*& OutDef)
{
	if (!Source || !DataTable || !DataTable->RowStruct || !Source->bUseInlineMappings || Source->InlineMappings.Num() == 0)
	{
		return false;
	}

	const uint8* const* FoundRow = DataTable->GetRowMap().Find(RowName);
	const uint8* RowPtr = FoundRow ? *FoundRow : nullptr;
	if (!RowPtr)
	{
		return false;
	}

	UYIItemDefinition* Def = NewObject<UYIItemDefinition>();
	if (!Def)
	{
		return false;
	}

	for (const FYIFieldMapping& Mapping : Source->InlineMappings)
	{
		if (Mapping.TargetProperty.IsNone())
		{
			continue;
		}

		FProperty* SourceProp = nullptr;
		if (!Mapping.bUseStaticValue)
		{
			for (TFieldIterator<FProperty> It(DataTable->RowStruct); It; ++It)
			{
				if (It->GetAuthoredName().Equals(Mapping.SourceField.ToString(), ESearchCase::IgnoreCase))
				{
					SourceProp = *It;
					break;
				}
			}
			if (!SourceProp)
			{
				continue;
			}
		}

		FProperty* DestProp = nullptr;
		for (TFieldIterator<FProperty> It(UYIItemDefinition::StaticClass()); It; ++It)
		{
			if (It->GetAuthoredName().Equals(Mapping.TargetProperty.ToString(), ESearchCase::IgnoreCase))
			{
				DestProp = *It;
				break;
			}
		}
		if (!DestProp)
		{
			continue;
		}

		uint8* DestPtr = DestProp->ContainerPtrToValuePtr<uint8>(Def);

		if (Mapping.bUseStaticValue)
		{
			if (SetPropertyValueFromString(Mapping.StaticValue, DestProp, DestPtr, Mapping.Conversion))
			{
				ApplyTransformFunction(Mapping, DestProp, DestPtr);
			}
			continue;
		}

		const uint8* SrcPtr = SourceProp->ContainerPtrToValuePtr<uint8>(RowPtr);
		if (Mapping.Conversion == EYIFieldMappingConversion::Vector2DFromXY)
		{
			if (Mapping.SourceFieldB.IsNone())
			{
				continue;
			}
			FProperty* SourcePropB = nullptr;
			for (TFieldIterator<FProperty> It(DataTable->RowStruct); It; ++It)
			{
				if (It->GetAuthoredName().Equals(Mapping.SourceFieldB.ToString(), ESearchCase::IgnoreCase))
				{
					SourcePropB = *It;
					break;
				}
			}
			if (!SourcePropB)
			{
				continue;
			}

			const FNumericProperty* NumA = CastField<FNumericProperty>(SourceProp);
			const FNumericProperty* NumB = CastField<FNumericProperty>(SourcePropB);
			if (!NumA || !NumB)
			{
				continue;
			}

			const double X = NumA->IsFloatingPoint() ? NumA->GetFloatingPointPropertyValue(SrcPtr) : (double)NumA->GetSignedIntPropertyValue(SrcPtr);
			const uint8* SrcPtrB = SourcePropB->ContainerPtrToValuePtr<uint8>(RowPtr);
			const double Y = NumB->IsFloatingPoint() ? NumB->GetFloatingPointPropertyValue(SrcPtrB) : (double)NumB->GetSignedIntPropertyValue(SrcPtrB);

			if (const FStructProperty* DestStruct = CastField<FStructProperty>(DestProp))
			{
				if (DestStruct->Struct == TBaseStructure<FVector2D>::Get())
				{
					FVector2D* Vec = reinterpret_cast<FVector2D*>(DestPtr);
					*Vec = FVector2D((float)X, (float)Y);
					ApplyTransformFunction(Mapping, DestProp, DestPtr);
					continue;
				}
				if (DestStruct->Struct == TBaseStructure<FIntPoint>::Get())
				{
					FIntPoint* Pt = reinterpret_cast<FIntPoint*>(DestPtr);
					*Pt = FIntPoint((int32)X, (int32)Y);
					ApplyTransformFunction(Mapping, DestProp, DestPtr);
					continue;
				}
			}
			continue;
		}

		if (CopyValueBetweenProperties(SourceProp, SrcPtr, DestProp, DestPtr, Mapping.Conversion))
		{
			ApplyTransformFunction(Mapping, DestProp, DestPtr);
		}
	}

	OutDef = Def;
	return true;
}

static UYIItemDefinition* RunTransformerForRow(UObject* Outer, const UDataTable* DataTable, FName RowName, TSubclassOf<UCSVDataTransformer> TransformerClass)
{
	if (!Outer || !DataTable || !TransformerClass)
	{
		return nullptr;
	}

	const uint8* const* FoundRow = DataTable->GetRowMap().Find(RowName);
	const uint8* RowPtr = FoundRow ? *FoundRow : nullptr;
	if (!RowPtr)
	{
		return nullptr;
	}

	URowData* RowWrapper = NewObject<URowData>(Outer);
	RowWrapper->Address = const_cast<uint8*>(RowPtr);
	RowWrapper->Struct = DataTable->RowStruct;

	UCSVDataTransformer* Transformer = NewObject<UCSVDataTransformer>(Outer, TransformerClass);
	UObject* Result = Transformer ? Transformer->TransformObject(RowWrapper) : nullptr;
	return Cast<UYIItemDefinition>(Result);
}

static void CopyItemDefinitionBaseProperties(const UYIItemDefinition* SourceDef, UYIItemDefinition* DestDef)
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
}

UYIItemDefinition* UYIItemRegistrySubsystem::TransformRow(FName RowName, const UDataTable* DataTable, TSubclassOf<UCSVDataTransformer> TransformerClass, bool bCacheResult, int64 Code, const UYIDataTableItemSource* Source)
{
	if (!DataTable)
	{
		return nullptr;
	}

	UYIItemDefinition* CachedResult = nullptr;

	if (bCacheResult)
	{
		if (TObjectPtr<UYIItemDefinition>* Cached = CachedGeneratedDefinitions.Find(Code))
		{
			return Cached->Get();
		}
	}

	const EYITransformMode Mode = Source ? Source->TransformMode : EYITransformMode::TransformerOnly;
	const bool bCanInline = Source && Source->bUseInlineMappings && Source->InlineMappings.Num() > 0;

	UYIItemDefinition* InlineDef = nullptr;
	UYIItemDefinition* TransformerDef = nullptr;

	auto CacheAndReturn = [this, bCacheResult, Code](UYIItemDefinition* ResultDef) -> UYIItemDefinition*
	{
		if (ResultDef && bCacheResult)
		{
			CachedGeneratedDefinitions.FindOrAdd(Code) = ResultDef;
		}
		return ResultDef;
	};

	switch (Mode)
	{
	case EYITransformMode::InlineOnly:
		if (!bCanInline || !ApplyInlineMappings(Source, DataTable, RowName, InlineDef))
		{
			return nullptr;
		}
		return CacheAndReturn(InlineDef);

	case EYITransformMode::TransformerOnly:
		if (!TransformerClass)
		{
			UE_LOG(LogYIItemRegistry, Warning, TEXT("TransformRow skipped for code %lld (row %s): TransformerClass is null."), (long long)Code, *RowName.ToString());
			return nullptr;
		}
		TransformerDef = RunTransformerForRow(this, DataTable, RowName, TransformerClass);
		return CacheAndReturn(TransformerDef);

	case EYITransformMode::HybridInlineThenTransformer:
		if (bCanInline)
		{
			ApplyInlineMappings(Source, DataTable, RowName, InlineDef);
		}
		if (TransformerClass)
		{
			TransformerDef = RunTransformerForRow(this, DataTable, RowName, TransformerClass);
		}
		if (!InlineDef && !TransformerDef)
		{
			return nullptr;
		}
		if (!InlineDef)
		{
			return CacheAndReturn(TransformerDef);
		}
		if (!TransformerDef)
		{
			return CacheAndReturn(InlineDef);
		}
		{
			UYIItemDefinition* Merged = NewObject<UYIItemDefinition>(this, TransformerDef->GetClass());
			CopyItemDefinitionBaseProperties(InlineDef, Merged);
			CopyItemDefinitionBaseProperties(TransformerDef, Merged);
			return CacheAndReturn(Merged);
		}

	case EYITransformMode::HybridTransformerThenInline:
		if (TransformerClass)
		{
			TransformerDef = RunTransformerForRow(this, DataTable, RowName, TransformerClass);
		}
		if (bCanInline)
		{
			ApplyInlineMappings(Source, DataTable, RowName, InlineDef);
		}
		if (!InlineDef && !TransformerDef)
		{
			return nullptr;
		}
		if (!InlineDef)
		{
			return CacheAndReturn(TransformerDef);
		}
		if (!TransformerDef)
		{
			return CacheAndReturn(InlineDef);
		}
		{
			UYIItemDefinition* Merged = NewObject<UYIItemDefinition>(this, TransformerDef->GetClass());
			CopyItemDefinitionBaseProperties(TransformerDef, Merged);
			CopyItemDefinitionBaseProperties(InlineDef, Merged);
			return CacheAndReturn(Merged);
		}
	default:
		return nullptr;
	}
}

void UYIItemRegistrySubsystem::BuildIndex(bool bForce)
{
	if (bIndexed && !bForce)
	{
		return;
	}

	CodeToEntry.Reset();
	CachedGeneratedDefinitions.Reset();

	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	// Scan native item definition assets
	TArray<FAssetData> DefinitionAssets;
	ARM.Get().GetAssetsByClass(UYIItemDefinition::StaticClass()->GetClassPathName(), DefinitionAssets, true);
	for (const FAssetData& AD : DefinitionAssets)
	{
		int64 Code = 0;
		if (UYIItemDefinition* Def = Cast<UYIItemDefinition>(AD.GetAsset()))
		{
			Code = Def->UniqueCode;
		}

		if (Code != 0)
		{
			FYIItemRegistryEntry& Entry = CodeToEntry.FindOrAdd(Code);
			if (Entry.Asset.ToSoftObjectPath().IsValid() || Entry.IsDataTable())
			{
				UE_LOG(LogYIItemRegistry, Warning, TEXT("Duplicate UniqueCode %lld found on asset %s; keeping first entry."), (long long)Code, *AD.GetObjectPathString());
				continue;
			}
			Entry.Asset = TSoftObjectPtr<UYIItemDefinition>(AD.ToSoftObjectPath());
		}
		else
		{
			UE_LOG(LogYIItemRegistry, Warning, TEXT("Skipping UYIItemDefinition %s because UniqueCode is zero."), *AD.GetObjectPathString());
		}
	}

	// Scan data table item sources
	TArray<FAssetData> SourceAssets;
	ARM.Get().GetAssetsByClass(UYIDataTableItemSource::StaticClass()->GetClassPathName(), SourceAssets, true);
	for (const FAssetData& AD : SourceAssets)
	{
		UYIDataTableItemSource* Source = Cast<UYIDataTableItemSource>(AD.GetAsset());
		if (!Source)
		{
			continue;
		}

		UDataTable* Table = Source->DataTable.LoadSynchronous();
		if (!Table)
		{
			UE_LOG(LogYIItemRegistry, Warning, TEXT("UYIDataTableItemSource %s has no DataTable."), *AD.GetObjectPathString());
			continue;
		}

		if (!Table->RowStruct)
		{
			UE_LOG(LogYIItemRegistry, Warning, TEXT("DataTable %s in %s has no RowStruct."), *Table->GetPathName(), *AD.GetObjectPathString());
			continue;
		}

		for (const TPair<FName, uint8*>& RowPair : Table->GetRowMap())
		{
			const int64 Code = ExtractCodeFromRow(Table->RowStruct, RowPair.Value, Source->UniqueCodeFieldName);
			if (Code == 0)
			{
				if (!Source->TemplateIdFieldName.IsNone())
				{
					const FString TemplateId = ExtractTemplateIdFromRow(Table->RowStruct, RowPair.Value, Source->TemplateIdFieldName);
					UE_LOG(LogYIItemRegistry, Verbose, TEXT("Row %s in %s skipped: UniqueCode is zero (TemplateId: %s)"), *RowPair.Key.ToString(), *Table->GetPathName(), *TemplateId);
				}
				continue;
			}

			FYIItemRegistryEntry& Entry = CodeToEntry.FindOrAdd(Code);
			if (Entry.Asset.ToSoftObjectPath().IsValid())
			{
				// Keep native asset over data table row
				UE_LOG(LogYIItemRegistry, Verbose, TEXT("Code %lld already mapped to asset %s; skipping data table row %s.%s"), (long long)Code, *Entry.Asset.ToString(), *Table->GetPathName(), *RowPair.Key.ToString());
				continue;
			}
			if (Entry.IsDataTable())
			{
				UE_LOG(LogYIItemRegistry, Warning, TEXT("Duplicate UniqueCode %lld in data tables (%s row %s). Keeping first."), (long long)Code, *Table->GetPathName(), *RowPair.Key.ToString());
				continue;
			}

			Entry.DataTableSource = Source;
			Entry.RowName = RowPair.Key;
		}
	}

	bIndexed = true;
}

UYIItemDefinition* UYIItemRegistrySubsystem::GetByCode(int64 Code)
{
	if (!bIndexed)
	{
		BuildIndex(false);
	}

	const FYIItemRegistryEntry* Entry = CodeToEntry.Find(Code);
	if (!Entry)
	{
		return nullptr;
	}

	if (Entry->Asset.ToSoftObjectPath().IsValid())
	{
		return Entry->Asset.LoadSynchronous();
	}

	if (!Entry->IsDataTable())
	{
		return nullptr;
	}

	UYIDataTableItemSource* Source = Entry->DataTableSource.LoadSynchronous();
	if (!Source)
	{
		UE_LOG(LogYIItemRegistry, Warning, TEXT("GetByCode(%lld) failed: DataTableSource is null."), (long long)Code);
		return nullptr;
	}

	UDataTable* Table = Source->DataTable.LoadSynchronous();
	if (!Table || !Table->RowStruct)
	{
		UE_LOG(LogYIItemRegistry, Warning, TEXT("GetByCode(%lld) failed: DataTable is missing or has no RowStruct (%s)."), (long long)Code, *Source->GetPathName());
		return nullptr;
	}

	if (!Table->GetRowMap().Contains(Entry->RowName))
	{
		UE_LOG(LogYIItemRegistry, Warning, TEXT("GetByCode(%lld) failed: Row %s not found in table %s."), (long long)Code, *Entry->RowName.ToString(), *Table->GetPathName());
		return nullptr;
	}

	const bool bHasInline = Source->bUseInlineMappings && Source->InlineMappings.Num() > 0;
	if (!Source->TransformerClass && !bHasInline && Source->bRequireTransformer)
	{
		UE_LOG(LogYIItemRegistry, Warning, TEXT("GetByCode(%lld) failed: TransformerClass is required but not set on %s."), (long long)Code, *Source->GetPathName());
		return nullptr;
	}

	const TSubclassOf<UCSVDataTransformer> TransformerClass = Source->GetEffectiveTransformerClass();
	UYIItemDefinition* Def = TransformRow(Entry->RowName, Table, TransformerClass, Source->bCacheGeneratedItems, Code, Source);
	if (!Def)
	{
		UE_LOG(LogYIItemRegistry, Warning, TEXT("GetByCode(%lld) failed: Transformer returned null for row %s in %s."), (long long)Code, *Entry->RowName.ToString(), *Table->GetPathName());
	}
	return Def;
}

UYIItemDefinition* UYIItemRegistrySubsystem::TransformRowUncached(
	FName RowName,
	const UDataTable* DataTable,
	TSubclassOf<UCSVDataTransformer> TransformerClass,
	int64 Code,
	const UYIDataTableItemSource* Source)
{
	return TransformRow(RowName, DataTable, TransformerClass, false, Code, Source);
}

bool UYIItemRegistrySubsystem::EnsureUniqueCodes(bool bAutoFix)
{
	BuildIndex(true);
	TSet<int64> Seen;
	bool bOk = true;
	for (const TPair<int64, FYIItemRegistryEntry>& P : CodeToEntry)
	{
		const int64 Code = P.Key;
		if (Code == 0 || Seen.Contains(Code))
		{
			bOk = false;
			if (bAutoFix && P.Value.Asset.ToSoftObjectPath().IsValid())
			{
#if WITH_EDITOR
				UYIItemDefinition* Def = P.Value.Asset.LoadSynchronous();
				if (Def)
				{
					int64 NewCode = 0;
					do { NewCode = (int64)FMath::RandRange(100000, INT32_MAX) * 1000ll + (int64)FMath::RandRange(0,999); } while (Seen.Contains(NewCode));
					Def->Modify();
					Def->UniqueCode = NewCode;
					Seen.Add(NewCode);
				}
#endif
			}
		}
		else
		{
			Seen.Add(Code);
		}
	}
	return bOk;
}

void UYIItemRegistrySubsystem::GetAllItems(TArray<FYIItemRegistryView>& OutItems, bool bForceRebuild)
{
	if (bForceRebuild || !bIndexed)
	{
		BuildIndex(bForceRebuild);
	}

	OutItems.Reset();
	for (const TPair<int64, FYIItemRegistryEntry>& Pair : CodeToEntry)
	{
		const int64 Code = Pair.Key;
		const FYIItemRegistryEntry& Entry = Pair.Value;

		FYIItemRegistryView View;
		View.UniqueCode = Code;
		View.bIsDataTable = Entry.IsDataTable();

		if (Entry.Asset.ToSoftObjectPath().IsValid())
		{
			View.Object = Entry.Asset;
			View.SourcePath = Entry.Asset.ToSoftObjectPath().ToString();
			if (UYIItemDefinition* Def = Entry.Asset.LoadSynchronous())
			{
				View.TemplateId = Def->TemplateId;
			}
		}
		else if (Entry.IsDataTable())
		{
			if (UYIDataTableItemSource* Source = Entry.DataTableSource.LoadSynchronous())
			{
				UDataTable* Table = Source->DataTable.LoadSynchronous();
				View.SourcePath = Source->DataTable.ToSoftObjectPath().ToString();
				View.RowName = Entry.RowName;
				View.DataSource = Source;

				if (Table && Table->RowStruct && Table->GetRowMap().Contains(Entry.RowName))
				{
					const uint8* const* Found = Table->GetRowMap().Find(Entry.RowName);
					if (Found && *Found)
					{
						View.TemplateId = ExtractTemplateIdFromRow(Table->RowStruct, *Found, Source->TemplateIdFieldName);
					}
				}
			}
		}

		OutItems.Add(View);
	}
}
