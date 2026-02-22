#include "SYIItemDashboard.h"
#include "YIItemRegistrySubsystem.h"
#include "Data/YIDataTableItemSource.h"
#include "YIItemDefinition.h"
#include "YIFragmentAsset.h"
#include "YIItemSchemaResolver.h"
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
#include "Widgets/Input/SNumericEntryBox.h"
#include "InputCoreTypes.h"
#include "ObjectTools.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UObject/UObjectIterator.h"
#include "UObject/StructOnScope.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsManager.h"
#include "Engine/Texture.h"
#include "YIItemFragments.h"
#include "EdGraphSchema_K2.h"
#include "EdGraph/EdGraphPin.h"
#include "Algo/Sort.h"
#include "Algo/Unique.h"
#include "YIEditorRowHelpers.h"
#include "YIEditorMessageLog.h"
#include "YIInlineMappingResolvers.h"

static FProperty* FindPropertyByAuthoredNameEditor(const UStruct* OwnerStruct, FName FieldName)
{
	if (!OwnerStruct || FieldName.IsNone())
	{
		return nullptr;
	}

	const FString FieldNameString = FieldName.ToString();
	for (TFieldIterator<FProperty> It(OwnerStruct); It; ++It)
	{
		FProperty* Property = *It;
		if (Property && Property->GetAuthoredName().Equals(FieldNameString, ESearchCase::IgnoreCase))
		{
			return Property;
		}
	}
	return nullptr;
}

static bool MatchesFieldByAuthoredNameEditor(const FProperty* Property, const FName FieldName)
{
	if (!Property || FieldName.IsNone())
	{
		return false;
	}
	return Property->GetAuthoredName().Equals(FieldName.ToString(), ESearchCase::IgnoreCase);
}

static bool TryReadInt64FromRowPropertyEditor(const FProperty* Property, const uint8* RowData, int64& OutValue)
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

static bool TryExtractRowCodeEditor(const UScriptStruct* RowStruct, const uint8* RowData, const FName ConfiguredField, int64& OutCode)
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
			if (MatchesFieldByAuthoredNameEditor(Prop, FieldName) && TryReadInt64FromRowPropertyEditor(Prop, RowData, OutCode))
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

static bool TryExtractRowTextFieldEditor(const UScriptStruct* RowStruct, const uint8* RowData, const FName FieldName, FString& OutValue)
{
	if (!RowStruct || !RowData || FieldName.IsNone())
	{
		return false;
	}

	for (TFieldIterator<FProperty> It(RowStruct); It; ++It)
	{
		const FProperty* Prop = *It;
		if (!MatchesFieldByAuthoredNameEditor(Prop, FieldName))
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

static bool TryExtractRowTextFromMappingsEditor(
	const UYIDataTableItemSource* Source,
	const UScriptStruct* RowStruct,
	const uint8* RowData,
	const TFunctionRef<bool(const FYIFieldMapping&)>& MatchesTarget,
	FString& OutValue)
{
	if (!Source || !RowStruct || !RowData)
	{
		return false;
	}

	for (const FYIFieldMapping& Mapping : Source->InlineMappings)
	{
		if (!MatchesTarget(Mapping))
		{
			continue;
		}

		if (Mapping.bUseStaticValue)
		{
			const FString StaticText = Mapping.StaticValue.TrimStartAndEnd();
			if (!StaticText.IsEmpty())
			{
				OutValue = StaticText;
				return true;
			}
			continue;
		}

		if (Mapping.SourceField.IsNone())
		{
			continue;
		}

		FString TextValue = YIEditor_GetRowStringFromStruct(RowStruct, RowData, Mapping.SourceField);
		if (TextValue.IsEmpty())
		{
			TryExtractRowTextFieldEditor(RowStruct, RowData, Mapping.SourceField, TextValue);
		}
		TextValue = TextValue.TrimStartAndEnd();
		if (!TextValue.IsEmpty())
		{
			OutValue = TextValue;
			return true;
		}
	}

	return false;
}

static bool TryExtractRowDisplayNameFromMappingsEditor(
	const UYIDataTableItemSource* Source,
	const UScriptStruct* RowStruct,
	const uint8* RowData,
	FString& OutValue)
{
	return TryExtractRowTextFromMappingsEditor(
		Source,
		RowStruct,
		RowData,
		[](const FYIFieldMapping& Mapping) -> bool
		{
			if (Mapping.TargetLayer == EYIFieldMappingTargetLayer::LegacyProperty)
			{
				return Mapping.TargetProperty == FName(TEXT("DisplayName"));
			}

			if (Mapping.TargetLayer != EYIFieldMappingTargetLayer::StaticDefinitionFragment)
			{
				return false;
			}

			const UScriptStruct* FragmentStruct = Mapping.TargetFragmentStruct.Get();
			if (!FragmentStruct || !FragmentStruct->IsChildOf(FYIItemUIDefinitionFragment::StaticStruct()))
			{
				return false;
			}

			return YIGetResolvedTargetFieldName(Mapping) == GET_MEMBER_NAME_CHECKED(FYIItemUIDefinitionFragment, DisplayName);
		},
		OutValue);
}

static bool TryExtractRowTemplateIdFromMappingsEditor(
	const UYIDataTableItemSource* Source,
	const UScriptStruct* RowStruct,
	const uint8* RowData,
	FString& OutValue)
{
	return TryExtractRowTextFromMappingsEditor(
		Source,
		RowStruct,
		RowData,
		[](const FYIFieldMapping& Mapping) -> bool
		{
			return Mapping.TargetLayer == EYIFieldMappingTargetLayer::LegacyProperty
				&& Mapping.TargetProperty == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, TemplateId);
		},
		OutValue);
}

static UScriptStruct* ResolveStructFromPathString(const FString& StructPath)
{
	if (StructPath.IsEmpty())
	{
		return nullptr;
	}

	if (UScriptStruct* Found = FindObject<UScriptStruct>(nullptr, *StructPath))
	{
		return Found;
	}
	return LoadObject<UScriptStruct>(nullptr, *StructPath);
}

static void CollectFragmentStructOptions(const UScriptStruct* BaseStruct, TArray<TSharedPtr<FString>>& OutOptions)
{
	OutOptions.Reset();
	if (!BaseStruct)
	{
		return;
	}

	TArray<FString> Paths;
	TSet<FString> SeenPaths;
	for (TObjectIterator<UScriptStruct> It; It; ++It)
	{
		UScriptStruct* ScriptStruct = *It;
		if (!ScriptStruct || !ScriptStruct->IsChildOf(BaseStruct) || ScriptStruct == BaseStruct)
		{
			continue;
		}
		const FString Path = ScriptStruct->GetPathName();
		if (Path.Contains(TEXT("TRASH")) || Path.Contains(TEXT("REINST")))
		{
			continue;
		}
		if (SeenPaths.Contains(Path))
		{
			continue;
		}
		SeenPaths.Add(Path);
		Paths.Add(Path);
	}

	// Fallback: ensure core schema fragment structs are available even before first asset usage.
	auto AddIfMissing = [&Paths, &SeenPaths, BaseStruct](UScriptStruct* Struct)
	{
		if (!Struct)
		{
			return;
		}
		if (BaseStruct && !Struct->IsChildOf(BaseStruct))
		{
			return;
		}
		const FString Path = Struct->GetPathName();
		if (Path.IsEmpty() || SeenPaths.Contains(Path))
		{
			return;
		}
		SeenPaths.Add(Path);
		Paths.Add(Path);
	};
	AddIfMissing(FYIItemUIDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemClassificationDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemAudioDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemLayoutDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemStackingDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemRulesDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemContainerDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemAttributeModsDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemPickupDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemWeightDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemEquipmentDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemAffixDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemCustomDefinitionFragment::StaticStruct());
	AddIfMissing(FYIItemCustomRuntimeFragment::StaticStruct());

	Paths.Sort();
	for (const FString& Path : Paths)
	{
		OutOptions.Add(MakeShared<FString>(Path));
	}
}

static FString MakeReadableFragmentName(const UScriptStruct* FragmentStruct)
{
	if (!FragmentStruct)
	{
		return TEXT("Fragment");
	}

	const FString MetaDisplayName = FragmentStruct->GetMetaData(TEXT("DisplayName"));
	if (!MetaDisplayName.IsEmpty())
	{
		return MetaDisplayName;
	}

	FString Name = FragmentStruct->GetName();
	Name.RemoveFromStart(TEXT("FYI"));
	Name.RemoveFromStart(TEXT("YI"));
	Name.RemoveFromEnd(TEXT("DefinitionFragment"));
	Name.RemoveFromEnd(TEXT("Fragment"));
	Name.RemoveFromStart(TEXT("Item"));
	Name = Name.TrimStartAndEnd();
	if (Name.IsEmpty())
	{
		return FragmentStruct->GetName();
	}

	FString Readable;
	Readable.Reserve(Name.Len() + 8);
	for (int32 Index = 0; Index < Name.Len(); ++Index)
	{
		const TCHAR C = Name[Index];
		const bool bUpper = FChar::IsUpper(C);
		const bool bPrevLower = (Index > 0) ? FChar::IsLower(Name[Index - 1]) : false;
		if (Index > 0 && bUpper && bPrevLower)
		{
			Readable.AppendChar(TEXT(' '));
		}
		Readable.AppendChar(C);
	}
	return Readable;
}

static FString MakeReadableFragmentNameFromPath(const FString& StructPath)
{
	if (StructPath.IsEmpty())
	{
		return TEXT("Fragment");
	}
	if (UScriptStruct* Struct = ResolveStructFromPathString(StructPath))
	{
		return MakeReadableFragmentName(Struct);
	}

	FString Leaf = FPackageName::ObjectPathToObjectName(StructPath);
	if (Leaf.IsEmpty())
	{
		Leaf = StructPath;
	}
	if (Leaf.RemoveFromStart(TEXT("FYI")))
	{
	}
	Leaf.RemoveFromEnd(TEXT("DefinitionFragment"));
	Leaf.RemoveFromEnd(TEXT("Fragment"));
	return Leaf;
}

static bool IsMappableFragmentField(const FProperty* Property)
{
	return Property
		&& !Property->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated)
		&& !Property->HasMetaData(TEXT("YIInlineMapIgnore"));
}

static bool IsIdentityMappingProperty(const FName FieldName)
{
	return FieldName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, UniqueCode)
		|| FieldName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, TemplateId);
}

static bool YIResolveDashboardDisplayName(const UYIItemDefinition* Def, FString& OutName, bool& bOutMissingUIDisplayName)
{
	OutName.Reset();
	bOutMissingUIDisplayName = false;
	if (!Def)
	{
		return false;
	}

	const FInstancedStruct* UIFragmentInst = YIItemSchema::FindResolvedDefinitionFragmentByStruct(Def, FYIItemUIDefinitionFragment::StaticStruct());
	if (const FYIItemUIDefinitionFragment* UIFragment = UIFragmentInst ? UIFragmentInst->GetPtr<FYIItemUIDefinitionFragment>() : nullptr)
	{
		const FString UIDisplayName = UIFragment->DisplayName.ToString().TrimStartAndEnd();
		if (!UIDisplayName.IsEmpty())
		{
			OutName = UIDisplayName;
			return true;
		}
		bOutMissingUIDisplayName = true;
	}
	else
	{
		// Dashboard relies on UI fragment naming for user-facing labels.
		bOutMissingUIDisplayName = true;
	}

	const FString SnapshotDisplayName = YIItemSchema::GetDisplayName(Def).ToString().TrimStartAndEnd();
	if (!SnapshotDisplayName.IsEmpty())
	{
		OutName = SnapshotDisplayName;
		return true;
	}

	OutName = Def->GetName();
	return true;
}

static FString YIMakeDashboardName(const FString& Name, const bool bMissingUIDisplayName)
{
	return bMissingUIDisplayName
		? FString::Printf(TEXT("[Missing UI.DisplayName] %s"), *Name)
		: Name;
}

static bool TryAssignDefaultStaticFragmentTarget(FYIFieldMapping& Mapping)
{
	TArray<TSharedPtr<FString>> FragmentOptions;
	CollectFragmentStructOptions(FYIItemDefinitionFragmentBase::StaticStruct(), FragmentOptions);
	if (FragmentOptions.Num() == 0 || !FragmentOptions[0].IsValid())
	{
		return false;
	}

	UScriptStruct* FragmentStruct = ResolveStructFromPathString(*FragmentOptions[0]);
	if (!FragmentStruct)
	{
		return false;
	}

	FName FirstField = NAME_None;
	for (TFieldIterator<FProperty> It(FragmentStruct); It; ++It)
	{
		if (IsMappableFragmentField(*It))
		{
			FirstField = FName((*It)->GetAuthoredName());
			break;
		}
	}

	Mapping.TargetLayer = EYIFieldMappingTargetLayer::StaticDefinitionFragment;
	Mapping.TargetFragmentStruct = FragmentStruct;
	Mapping.TargetFragmentField = FirstField;
	Mapping.TargetProperty = FirstField;
	return !FirstField.IsNone();
}

static bool TryPromoteLegacyItemMappingToFragment(FYIFieldMapping& Mapping)
{
	if (Mapping.TargetLayer != EYIFieldMappingTargetLayer::LegacyProperty || Mapping.TargetProperty.IsNone())
	{
		return false;
	}
	if (IsIdentityMappingProperty(Mapping.TargetProperty))
	{
		return false;
	}

	const FString WantedField = Mapping.TargetProperty.ToString();
	UScriptStruct* MatchStruct = nullptr;
	FName MatchField = NAME_None;
	int32 MatchCount = 0;

	TArray<TSharedPtr<FString>> FragmentOptions;
	CollectFragmentStructOptions(FYIItemDefinitionFragmentBase::StaticStruct(), FragmentOptions);
	for (const TSharedPtr<FString>& FragmentPath : FragmentOptions)
	{
		if (!FragmentPath.IsValid())
		{
			continue;
		}
		UScriptStruct* FragmentStruct = ResolveStructFromPathString(*FragmentPath);
		if (!FragmentStruct)
		{
			continue;
		}

		if (FProperty* Field = FindPropertyByAuthoredNameEditor(FragmentStruct, FName(*WantedField)))
		{
			if (!IsMappableFragmentField(Field))
			{
				continue;
			}
			++MatchCount;
			MatchStruct = FragmentStruct;
			MatchField = FName(Field->GetAuthoredName());
			if (MatchCount > 1)
			{
				break;
			}
		}
	}

	if (MatchCount != 1 || !MatchStruct || MatchField.IsNone())
	{
		return false;
	}

	Mapping.TargetLayer = EYIFieldMappingTargetLayer::StaticDefinitionFragment;
	Mapping.TargetFragmentStruct = MatchStruct;
	Mapping.TargetFragmentField = MatchField;
	Mapping.TargetProperty = MatchField;
	return true;
}

static void CollectStructFieldOptions(const UStruct* OwnerStruct, TArray<TSharedPtr<FString>>& OutOptions)
{
	OutOptions.Reset();
	if (!OwnerStruct)
	{
		return;
	}

	auto ShouldExpandStruct = [](const UScriptStruct* StructType, int32 Depth) -> bool
	{
		if (!StructType || Depth >= 2)
		{
			return false;
		}
		if (StructType == FGameplayTag::StaticStruct())
		{
			return false;
		}
		if (StructType == FInstancedPropertyBag::StaticStruct())
		{
			return false;
		}
		return true;
	};

	TArray<FString> Names;
	TFunction<void(const UStruct*, const FString&, int32)> CollectRecursive;
	CollectRecursive = [&](const UStruct* Struct, const FString& Prefix, int32 Depth)
	{
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			FProperty* Property = *It;
			if (!Property)
			{
				continue;
			}

			const FString LocalName = Property->GetAuthoredName();
			const FString FullName = Prefix.IsEmpty()
				? LocalName
				: FString::Printf(TEXT("%s.%s"), *Prefix, *LocalName);
			Names.Add(FullName);

			if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
			{
				if (ShouldExpandStruct(StructProp->Struct, Depth))
				{
					CollectRecursive(StructProp->Struct, FullName, Depth + 1);
				}
			}
		}
	};

	CollectRecursive(OwnerStruct, FString(), 0);
	Names.Sort();
	Names.SetNum(Algo::Unique(Names));
	for (const FString& Name : Names)
	{
		OutOptions.Add(MakeShared<FString>(Name));
	}
}

static bool IsCustomDefinitionFragmentStructEditor(const UScriptStruct* FragmentStruct)
{
	return FragmentStruct && FragmentStruct->IsChildOf(FYIItemCustomDefinitionFragment::StaticStruct());
}

static bool IsCustomRuntimeFragmentStructEditor(const UScriptStruct* FragmentStruct)
{
	return FragmentStruct && FragmentStruct->IsChildOf(FYIItemCustomRuntimeFragment::StaticStruct());
}

static bool IsCustomFragmentStructEditor(const UScriptStruct* FragmentStruct)
{
	return IsCustomDefinitionFragmentStructEditor(FragmentStruct) || IsCustomRuntimeFragmentStructEditor(FragmentStruct);
}

static FString MakePropertyBagFieldPathEditor(const FName FieldName)
{
	return FString::Printf(TEXT("Properties.%s"), *FieldName.ToString());
}

static bool TryParsePropertyBagFieldPathEditor(const FString& FieldPath, FName& OutFieldName)
{
	OutFieldName = NAME_None;
	FString Remainder;
	if (!FieldPath.Split(TEXT("."), nullptr, &Remainder))
	{
		return false;
	}
	if (!FieldPath.StartsWith(TEXT("Properties.")) || Remainder.IsEmpty())
	{
		return false;
	}
	OutFieldName = FName(*Remainder);
	return !OutFieldName.IsNone();
}

static void CollectPropertyBagFieldOptionsForFragmentMappingEditor(
	const UYIDataTableItemSource* Source,
	const UScriptStruct* FragmentStruct,
	const UObject* DetailObject,
	TArray<TSharedPtr<FString>>& InOutOptions)
{
	TSet<FString> ExistingPaths;
	for (const TSharedPtr<FString>& Opt : InOutOptions)
	{
		if (Opt.IsValid())
		{
			ExistingPaths.Add(*Opt);
		}
	}

	auto AddBagFieldName = [&ExistingPaths, &InOutOptions](FName BagFieldName)
	{
		if (BagFieldName.IsNone())
		{
			return;
		}
		const FString Path = MakePropertyBagFieldPathEditor(BagFieldName);
		if (!ExistingPaths.Contains(Path))
		{
			ExistingPaths.Add(Path);
			InOutOptions.Add(MakeShared<FString>(Path));
		}
	};

	if (Source)
	{
		for (const FYIFieldMapping& Mapping : Source->InlineMappings)
		{
			if (Mapping.TargetFragmentStruct.Get() != FragmentStruct || !Mapping.bTargetPropertyBagField)
			{
				continue;
			}
			AddBagFieldName(Mapping.TargetPropertyBagFieldName);
		}

		for (const TSoftObjectPtr<UYIFragmentAsset>& FragmentAssetSoft : Source->PresetFragmentAssets)
		{
			const UYIFragmentAsset* FragmentAsset = FragmentAssetSoft.IsValid() ? FragmentAssetSoft.Get() : FragmentAssetSoft.LoadSynchronous();
			if (!FragmentAsset)
			{
				continue;
			}

			const TArray<FInstancedStruct>* FragmentArray = nullptr;
			if (IsCustomDefinitionFragmentStructEditor(FragmentStruct))
			{
				FragmentArray = &FragmentAsset->ItemDefinitionFragments;
			}
			else if (IsCustomRuntimeFragmentStructEditor(FragmentStruct))
			{
				FragmentArray = &FragmentAsset->ItemInstanceFragments;
			}

			if (!FragmentArray)
			{
				continue;
			}

			for (const FInstancedStruct& FragmentInst : *FragmentArray)
			{
				if (FragmentInst.GetScriptStruct() != FragmentStruct)
				{
					continue;
				}

				const FInstancedPropertyBag* Bag = nullptr;
				if (const FYIItemCustomDefinitionFragment* CustomDef = FragmentInst.GetPtr<FYIItemCustomDefinitionFragment>())
				{
					Bag = &CustomDef->Properties;
				}
				else if (const FYIItemCustomRuntimeFragment* CustomRt = FragmentInst.GetPtr<FYIItemCustomRuntimeFragment>())
				{
					Bag = &CustomRt->Properties;
				}

				if (const UPropertyBag* BagStruct = Bag ? Bag->GetPropertyBagStruct() : nullptr)
				{
					for (TFieldIterator<FProperty> It(BagStruct); It; ++It)
					{
						if (FProperty* Prop = *It)
						{
							AddBagFieldName(FName(*Prop->GetAuthoredName()));
						}
					}
				}
			}
		}
	}

	if (const UYIItemDefinition* ItemDef = Cast<UYIItemDefinition>(DetailObject))
	{
		const TArray<FInstancedStruct>* FragmentArray = nullptr;
		if (IsCustomDefinitionFragmentStructEditor(FragmentStruct))
		{
			FragmentArray = &ItemDef->DefinitionFragments;
		}
		else if (IsCustomRuntimeFragmentStructEditor(FragmentStruct))
		{
			FragmentArray = &ItemDef->DefaultInstanceFragments;
		}

		if (FragmentArray)
		{
			for (const FInstancedStruct& FragmentInst : *FragmentArray)
			{
				if (FragmentInst.GetScriptStruct() != FragmentStruct)
				{
					continue;
				}

				const FInstancedPropertyBag* Bag = nullptr;
				if (const FYIItemCustomDefinitionFragment* CustomDef = FragmentInst.GetPtr<FYIItemCustomDefinitionFragment>())
				{
					Bag = &CustomDef->Properties;
				}
				else if (const FYIItemCustomRuntimeFragment* CustomRt = FragmentInst.GetPtr<FYIItemCustomRuntimeFragment>())
				{
					Bag = &CustomRt->Properties;
				}

				if (!Bag)
				{
					continue;
				}

				if (const UPropertyBag* BagStruct = Bag->GetPropertyBagStruct())
				{
					for (TFieldIterator<FProperty> It(BagStruct); It; ++It)
					{
						if (FProperty* Prop = *It)
						{
							AddBagFieldName(FName(*Prop->GetAuthoredName()));
						}
					}
				}
			}
		}
	}

	InOutOptions.Sort([](const TSharedPtr<FString>& A, const TSharedPtr<FString>& B)
	{
		return (A.IsValid() ? *A : FString()) < (B.IsValid() ? *B : FString());
	});
}

static FProperty* FindPropertyByAuthoredPathEditor(const UStruct* OwnerStruct, const FString& FieldPath)
{
	if (!OwnerStruct || FieldPath.IsEmpty())
	{
		return nullptr;
	}

	TArray<FString> Segments;
	FieldPath.ParseIntoArray(Segments, TEXT("."), true);
	if (Segments.Num() == 0)
	{
		Segments.Add(FieldPath);
	}

	const UStruct* CurrentStruct = OwnerStruct;
	FProperty* CurrentProperty = nullptr;
	for (int32 Index = 0; Index < Segments.Num(); ++Index)
	{
		CurrentProperty = FindPropertyByAuthoredNameEditor(CurrentStruct, FName(*Segments[Index]));
		if (!CurrentProperty)
		{
			return nullptr;
		}

		const bool bLast = (Index == Segments.Num() - 1);
		if (!bLast)
		{
			const FStructProperty* StructProp = CastField<FStructProperty>(CurrentProperty);
			if (!StructProp || !StructProp->Struct)
			{
				return nullptr;
			}
			CurrentStruct = StructProp->Struct;
		}
	}

	return CurrentProperty;
}

static bool YIItemDash_PromptSavePackages(const TArray<UObject*>& ObjectsToSave, int32& OutRequestedCount)
{
	TArray<UPackage*> PackagesToSave;
	TSet<FString> AddedPackages;
	for (UObject* ObjectToSave : ObjectsToSave)
	{
		if (!ObjectToSave)
		{
			continue;
		}
		UPackage* Package = ObjectToSave->GetOutermost();
		if (!Package)
		{
			continue;
		}

		const FString PackageName = Package->GetName();
		if (AddedPackages.Contains(PackageName))
		{
			continue;
		}
		AddedPackages.Add(PackageName);
		PackagesToSave.Add(Package);
	}

	OutRequestedCount = PackagesToSave.Num();
	if (OutRequestedCount == 0)
	{
		return false;
	}

	const bool bCheckDirty = false;
	const bool bPromptToSave = true;
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

static bool YIIsSoftObjectConversionEditor(EYIFieldMappingConversion Conversion)
{
	return Conversion == EYIFieldMappingConversion::ToSoftObject || Conversion == EYIFieldMappingConversion::ToSoftTexture;
}

static bool YICanAssignSoftObjectToTargetEditor(const FSoftObjectProperty* DestSoftObj, UObject* ObjectValue, EYIFieldMappingConversion Conversion)
{
	if (!DestSoftObj)
	{
		return false;
	}

	const UClass* RequiredClass = DestSoftObj->PropertyClass.Get();
	if (!RequiredClass)
	{
		RequiredClass = UObject::StaticClass();
	}
	if (Conversion == EYIFieldMappingConversion::ToSoftTexture && !RequiredClass->IsChildOf(UTexture::StaticClass()))
	{
		return false;
	}
	if (!ObjectValue)
	{
		return true;
	}
	if (Conversion == EYIFieldMappingConversion::ToSoftTexture && !ObjectValue->IsA(UTexture::StaticClass()))
	{
		return false;
	}
	return ObjectValue->IsA(RequiredClass);
}

static bool YISetSoftObjectFromPathStringEditor(const FString& Value, FSoftObjectProperty* DestSoftObj, uint8* DestPtr, EYIFieldMappingConversion Conversion)
{
	if (!DestSoftObj || !DestPtr || !YIIsSoftObjectConversionEditor(Conversion))
	{
		return false;
	}

	const UClass* RequiredClass = DestSoftObj->PropertyClass.Get();
	if (!RequiredClass)
	{
		RequiredClass = UObject::StaticClass();
	}
	if (Conversion == EYIFieldMappingConversion::ToSoftTexture && !RequiredClass->IsChildOf(UTexture::StaticClass()))
	{
		return false;
	}

	if (Value.IsEmpty())
	{
		DestSoftObj->SetPropertyValue(DestPtr, FSoftObjectPtr());
		return true;
	}

	const FSoftObjectPath SoftPath(Value);
	if (!SoftPath.IsValid())
	{
		return false;
	}

	DestSoftObj->SetPropertyValue(DestPtr, FSoftObjectPtr(SoftPath));
	return true;
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

	if (YIIsSoftObjectConversionEditor(Conversion))
	{
		if (FSoftObjectProperty* DestSoftObj = CastField<FSoftObjectProperty>(DestProp))
		{
			if (YISetSoftObjectFromPathStringEditor(Value, DestSoftObj, DestPtr, Conversion))
			{
				return true;
			}
		}
	}

	return DestProp->ImportText_Direct(*Value, DestPtr, nullptr, PPF_None) != nullptr;
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

	if (YIIsSoftObjectConversionEditor(Conversion))
	{
		if (FSoftObjectProperty* DestSoftObj = CastField<FSoftObjectProperty>(DestProp))
		{
			if (const FSoftObjectProperty* SrcSoftObj = CastField<FSoftObjectProperty>(SourceProp))
			{
				UObject* ResolvedObject = SrcSoftObj->GetPropertyValue(SourcePtr).Get();
				if (YICanAssignSoftObjectToTargetEditor(DestSoftObj, ResolvedObject, Conversion))
				{
					DestSoftObj->SetPropertyValue(DestPtr, SrcSoftObj->GetPropertyValue(SourcePtr));
					return true;
				}
			}
			if (const FObjectPropertyBase* SrcObj = CastField<FObjectPropertyBase>(SourceProp))
			{
				if (UObject* Obj = SrcObj->GetObjectPropertyValue(SourcePtr))
				{
					if (YICanAssignSoftObjectToTargetEditor(DestSoftObj, Obj, Conversion))
					{
						DestSoftObj->SetPropertyValue(DestPtr, FSoftObjectPtr(Obj));
						return true;
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
		if (YIIsSoftObjectConversionEditor(Conversion))
		{
			if (FSoftObjectProperty* DestSoftObj = CastField<FSoftObjectProperty>(DestProp))
			{
				if (YISetSoftObjectFromPathStringEditor(Value, DestSoftObj, DestPtr, Conversion))
				{
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
		if (YIIsSoftObjectConversionEditor(Conversion))
		{
			if (FSoftObjectProperty* DestSoftObj = CastField<FSoftObjectProperty>(DestProp))
			{
				if (YISetSoftObjectFromPathStringEditor(Value.ToString(), DestSoftObj, DestPtr, Conversion))
				{
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
		if (YIIsSoftObjectConversionEditor(Conversion))
		{
			if (FSoftObjectProperty* DestSoftObj = CastField<FSoftObjectProperty>(DestProp))
			{
				if (YISetSoftObjectFromPathStringEditor(Value.ToString(), DestSoftObj, DestPtr, Conversion))
				{
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
		return EYIFieldMappingConversion::ToSoftObject;
	}
	if (const FNumericProperty* NumTarget = CastField<FNumericProperty>(TargetProp))
	{
		if (NumTarget->IsInteger()) return EYIFieldMappingConversion::ToInt;
		return EYIFieldMappingConversion::ToFloat;
	}
	return EYIFieldMappingConversion::None;
}

static bool HasMappingTargetV2(const FYIFieldMapping& Mapping)
{
	if (Mapping.TargetLayer == EYIFieldMappingTargetLayer::LegacyProperty)
	{
		return IsIdentityMappingProperty(Mapping.TargetProperty);
	}

	return Mapping.TargetFragmentStruct != nullptr && !YIGetResolvedTargetFieldName(Mapping).IsNone();
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
										.Text(NSLOCTEXT("YOLOInventory", "Dash_Action_UpdateLinked", "Update Linked (Selected)"))
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

													if (E->bIsDataTable)
													{
														if (E->bHasAsset || E->ItemAsset.IsValid())
														{
															bChanged |= CreateAssetFromEntry(*E);
														}
														continue;
													}

													UObject* Obj = E->Object.LoadSynchronous();
													if (!Obj && E->ItemAsset.IsValid())
													{
														Obj = E->ItemAsset.LoadSynchronous();
													}
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
																TargetPropertyOptions.Add(MakeShared<FString>(TEXT("View: All")));
																TargetPropertyOptions.Add(MakeShared<FString>(TEXT("View: Items from Data Sources")));
																TargetPropertyOptions.Add(MakeShared<FString>(TEXT("View: Data Sources + Physical Assets")));
															})
														.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewItem, ESelectInfo::Type)
															{
																if (!NewItem.IsValid()) return;
																if (*NewItem == TEXT("View: Items from Data Sources")) TypeFilter = EDashTypeFilter::DataTableRows;
																else if (*NewItem == TEXT("View: Data Sources + Physical Assets")) TypeFilter = EDashTypeFilter::AssetsOnly;
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
																	case EDashTypeFilter::DataTableRows: return FText::FromString(TEXT("View: Items from Data Sources"));
																	case EDashTypeFilter::AssetsOnly: return FText::FromString(TEXT("View: Data Sources + Physical Assets"));
																	default: return FText::FromString(TEXT("View: All"));
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
															SNew(SComboBox<TSharedPtr<FString>>)
																.OptionsSource(&AddFragmentStructOptions)
																.OnComboBoxOpening_Lambda([this]()
																	{
																		RefreshAddFragmentStructOptions();
																	})
																.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewItem, ESelectInfo::Type)
																	{
																		if (NewItem.IsValid())
																		{
																			SelectedAddFragmentStructOption = NewItem;
																		}
																	})
																.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
																	{
																		return SNew(STextBlock).Text(FText::FromString(MakeReadableFragmentNameFromPath(InItem.IsValid() ? *InItem : FString())));
																	})
																.Content()
																[
																	SNew(STextBlock).Text_Lambda([this]()
																		{
																			if (!SelectedAddFragmentStructOption.IsValid())
																			{
																				return NSLOCTEXT("YOLOInventory", "Dash_SelectFragmentForAdd", "Select Fragment");
																			}
																			return FText::FromString(MakeReadableFragmentNameFromPath(*SelectedAddFragmentStructOption));
																		})
																]
														]
													+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
														[
															SNew(SButton)
																.Text(NSLOCTEXT("YOLOInventory", "Dash_AddMapping", "Add Fragment (+Fields)"))
																.OnClicked_Lambda([this]()
																	{
																		AddFragmentMappingsForSelection();
																		return FReply::Handled();
																	})
														]
													+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
														[
															SNew(SButton)
																.Text(NSLOCTEXT("YOLOInventory", "Dash_AutoMatch", "Auto Match"))
																.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_AutoMatch_TT", "Match data source columns to fragment fields by name and patch incomplete mappings."))
																.OnClicked_Lambda([this]()
																	{
																		AutoMatchInlineMappings(false);
																		return FReply::Handled();
																	})
														]
													+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
														[
															SNew(SButton)
																.Text(NSLOCTEXT("YOLOInventory", "Dash_AddAllFields", "Add All Fragment Fields"))
																.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_AddAllFields_TT", "Generate mapping rows for every writable static fragment field and auto-match source columns where possible."))
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
																			const bool bHasTarget = HasMappingTargetV2(M);
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
														default: return 2;
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
	TArray<UObject*> ObjectsToSave;
	TSet<FString> ObjectPathsToSave;
	auto AddSaveObject = [&ObjectsToSave, &ObjectPathsToSave](UObject* InObject)
	{
		if (!InObject)
		{
			return;
		}
		const FString Path = InObject->GetPathName();
		if (ObjectPathsToSave.Contains(Path))
		{
			return;
		}
		ObjectPathsToSave.Add(Path);
		ObjectsToSave.Add(InObject);
	};
	auto AddSourceAndTable = [&AddSaveObject](UYIDataTableItemSource* Source)
	{
		if (!Source)
		{
			return;
		}
		AddSaveObject(Source);
		if (UDataTable* Table = Source->DataTable.LoadSynchronous())
		{
			AddSaveObject(Table);
		}
	};

	if (ListView.IsValid())
	{
		const TArray<TSharedPtr<FYIItemDashboardEntry>> Selected = ListView->GetSelectedItems();
		for (const TSharedPtr<FYIItemDashboardEntry>& Entry : Selected)
		{
			if (!Entry.IsValid())
			{
				continue;
			}
			if (!Entry->bIsDataTable)
			{
				if (UObject* PrimaryObject = Entry->Object.LoadSynchronous())
				{
					AddSaveObject(PrimaryObject);
					if (const UYIItemDefinition* Def = Cast<UYIItemDefinition>(PrimaryObject))
					{
						AddSourceAndTable(Def->SourceDataSource.LoadSynchronous());
					}
				}
				if (UYIDataTableItemSource* Source = Entry->DataSource.LoadSynchronous())
				{
					AddSourceAndTable(Source);
				}
			}
			else
			{
				// Save generated asset + data source + data table for row entries.
				if (Entry->ItemAsset.IsValid() || Entry->ItemAsset.ToSoftObjectPath().IsValid())
				{
					AddSaveObject(Entry->ItemAsset.LoadSynchronous());
				}
				if (Entry->DataSource.IsValid() || Entry->DataSource.ToSoftObjectPath().IsValid())
				{
					AddSourceAndTable(Entry->DataSource.LoadSynchronous());
				}
				else if (Entry->DataTable.IsValid() || Entry->DataTable.ToSoftObjectPath().IsValid())
				{
					AddSaveObject(Entry->DataTable.LoadSynchronous());
				}
			}
		}
	}
	if (ObjectsToSave.Num() == 0)
	{
		if (UObject* DetailObject = LastDetailObject.Get())
		{
			AddSaveObject(DetailObject);
			if (const UYIItemDefinition* Def = Cast<UYIItemDefinition>(DetailObject))
			{
				AddSourceAndTable(Def->SourceDataSource.LoadSynchronous());
			}
			else if (UYIDataTableItemSource* Source = Cast<UYIDataTableItemSource>(DetailObject))
			{
				AddSourceAndTable(Source);
			}
			else if (UDataTable* DataTable = Cast<UDataTable>(DetailObject))
			{
				AddSaveObject(DataTable);
			}
		}
	}

	if (ObjectsToSave.Num() == 0)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Warning,
			NSLOCTEXT("YOLOInventory", "Dash_Save_NoSelection", "Save skipped: no selected item/data source asset."));
		return;
	}

	int32 RequestedCount = 0;
	const bool bSaved = YIItemDash_PromptSavePackages(ObjectsToSave, RequestedCount);
	const int32 SavedCount = bSaved ? RequestedCount : 0;

	FYIEditorMessageLog::Add(
		SavedCount == RequestedCount ? EYIEditorLogSeverity::Info : EYIEditorLogSeverity::Warning,
		FText::Format(
			NSLOCTEXT("YOLOInventory", "Dash_Save_Summary", "Save selected assets complete. Saved: {0}/{1}"),
			FText::AsNumber(SavedCount),
			FText::AsNumber(RequestedCount)));

	if (bSaved)
	{
		Refresh();
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
							.Text(NSLOCTEXT("YOLOInventory", "Dash_Action_UpdateLinked", "Update Linked (Selected)"))
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

										if (E->bIsDataTable)
										{
											if (E->bHasAsset || E->ItemAsset.IsValid())
											{
												bChanged |= CreateAssetFromEntry(*E);
											}
											continue;
										}

										UObject* Obj = E->Object.LoadSynchronous();
										if (!Obj && E->ItemAsset.IsValid())
										{
											Obj = E->ItemAsset.LoadSynchronous();
										}
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
										ListTypeOptions.Add(MakeShared<FString>(TEXT("View: All")));
										ListTypeOptions.Add(MakeShared<FString>(TEXT("View: Items from Data Sources")));
										ListTypeOptions.Add(MakeShared<FString>(TEXT("View: Data Sources + Physical Assets")));
									})
								.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewItem, ESelectInfo::Type)
									{
										if (!NewItem.IsValid()) return;
										if (*NewItem == TEXT("View: Items from Data Sources")) TypeFilter = EDashTypeFilter::DataTableRows;
										else if (*NewItem == TEXT("View: Data Sources + Physical Assets")) TypeFilter = EDashTypeFilter::AssetsOnly;
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
											case EDashTypeFilter::DataTableRows: return FText::FromString(TEXT("View: Items from Data Sources"));
											case EDashTypeFilter::AssetsOnly: return FText::FromString(TEXT("View: Data Sources + Physical Assets"));
											default: return FText::FromString(TEXT("View: All"));
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
							SNew(SComboBox<TSharedPtr<FString>>)
								.OptionsSource(&AddFragmentStructOptions)
								.OnComboBoxOpening_Lambda([this]()
									{
										RefreshAddFragmentStructOptions();
									})
								.OnSelectionChanged_Lambda([this](TSharedPtr<FString> NewItem, ESelectInfo::Type)
									{
										if (NewItem.IsValid())
										{
											SelectedAddFragmentStructOption = NewItem;
										}
									})
								.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
									{
										return SNew(STextBlock).Text(FText::FromString(MakeReadableFragmentNameFromPath(InItem.IsValid() ? *InItem : FString())));
									})
								.Content()
								[
									SNew(STextBlock).Text_Lambda([this]()
										{
											if (!SelectedAddFragmentStructOption.IsValid())
											{
												return NSLOCTEXT("YOLOInventory", "Dash_SelectFragmentForAdd", "Select Fragment");
											}
											return FText::FromString(MakeReadableFragmentNameFromPath(*SelectedAddFragmentStructOption));
										})
								]
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
						[
							SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "Dash_AddMapping", "Add Fragment (+Fields)"))
								.OnClicked_Lambda([this]()
									{
										AddFragmentMappingsForSelection();
										return FReply::Handled();
									})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
						[
							SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "Dash_AutoMatch", "Auto Match"))
								.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_AutoMatch_TT", "Match data source columns to fragment fields by name and patch incomplete mappings."))
								.OnClicked_Lambda([this]()
									{
										AutoMatchInlineMappings(false);
										return FReply::Handled();
									})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(8, 0)
						[
							SNew(SButton)
								.Text(NSLOCTEXT("YOLOInventory", "Dash_AddAllFields", "Add All Fragment Fields"))
								.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_AddAllFields_TT", "Generate mapping rows for every writable static fragment field and auto-match source columns where possible."))
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
											const bool bHasTarget = HasMappingTargetV2(M);
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
	auto ValidateItemDisplayNaming = [&](const UYIItemDefinition* Def, const FText& Context)
	{
		if (!Def)
		{
			return;
		}

		FString ResolvedName;
		bool bMissingUIDisplayName = false;
		YIResolveDashboardDisplayName(Def, ResolvedName, bMissingUIDisplayName);
		if (bMissingUIDisplayName)
		{
			AddIssue(EYIDashboardIssueSeverity::Warning, false,
				NSLOCTEXT("YOLOInventory", "Dash_Preflight_MissingUIDisplayName", "UI fragment DisplayName is missing. Dashboard labels should come from UI.DisplayName."),
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
		if (Entry.bHasAsset)
		{
			ValidateItemDisplayNaming(Entry.ItemAsset.LoadSynchronous(), FText::FromString(Entry.Name));
		}
	}
	else
	{
		if (UYIItemDefinition* Def = Cast<UYIItemDefinition>(Entry.Object.LoadSynchronous()))
		{
			ValidateItemDisplayNaming(Def, FText::FromString(Def->GetPathName()));
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

			if (Entry->bIsDataSourceEntry)
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

			if (GEngine)
			{
				if (UYIItemRegistrySubsystem* Registry = GEngine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
				{
					return Registry->GetByCode(Entry->Code);
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
			if (Entry->bIsDataSourceEntry)
			{
				return FLinearColor(0.45f, 0.24f, 0.85f, 0.9f); // purple tint for source assets
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
			if (Entry->bIsDataSourceEntry)
			{
				return NSLOCTEXT("YOLOInventory", "Dash_Status_DataSource", "Item data source");
			}
			if (Entry->bIsDataTable)
			{
				return Entry->bHasAsset
					? NSLOCTEXT("YOLOInventory", "Dash_Status_HasAsset", "Generated asset exists")
					: NSLOCTEXT("YOLOInventory", "Dash_Status_NeedsAsset", "Needs asset generation");
			}
			return NSLOCTEXT("YOLOInventory", "Dash_Status_AssetOnly", "Item asset");
		};
	auto IsEntryDirty = [Entry]() -> bool
		{
			auto IsDirtyObject = [](UObject* Object) -> bool
				{
					return Object && Object->GetOutermost() && Object->GetOutermost()->IsDirty();
				};

			if (!Entry.IsValid())
			{
				return false;
			}

			if (!Entry->bIsDataTable)
			{
				return IsDirtyObject(Entry->Object.Get()) || IsDirtyObject(Entry->DataSource.Get());
			}

			return IsDirtyObject(Entry->ItemAsset.Get())
				|| IsDirtyObject(Entry->DataSource.Get())
				|| IsDirtyObject(Entry->DataTable.Get());
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
						.ColorAndOpacity(IsEntryDirty() ? FLinearColor(1.f, 0.85f, 0.2f) : StatusColor())
				]
				+ SHorizontalBox::Slot().FillWidth(0.25f)
				[
					SNew(STextBlock)
						.Text(FText::FromString(IsEntryDirty() ? FString::Printf(TEXT("* %s"), *Entry->Name) : Entry->Name))
				]
				+ SHorizontalBox::Slot().FillWidth(0.2f)
				[
					SNew(STextBlock).Text(FText::FromString(Entry->TemplateId))
				]
				+ SHorizontalBox::Slot().FillWidth(0.1f)
				[
					SNew(STextBlock).Text(Entry->bIsDataTable
						? NSLOCTEXT("YOLOInventory", "Dash_Type_DataTable", "Data Table")
						: (Entry->bIsDataSourceEntry
							? NSLOCTEXT("YOLOInventory", "Dash_Type_DataSource", "Data Source")
							: NSLOCTEXT("YOLOInventory", "Dash_Type_Asset", "Asset")))
						.ColorAndOpacity(StatusColor())
				]
				+ SHorizontalBox::Slot().FillWidth(0.3f)
				[
					SNew(STextBlock).Text(FText::FromString(Entry->Source))
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2)
				[
					Entry->bIsDataTable
						? StaticCastSharedRef<SWidget>(
							SNew(SButton)
							.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
							.ToolTipText(Entry->bHasAsset
								? NSLOCTEXT("YOLOInventory", "Dash_OpenAsset", "Select/Open generated item asset")
								: NSLOCTEXT("YOLOInventory", "Dash_MakeAsset", "Create a physical item asset from this source row"))
							.OnClicked_Lambda([this, Entry]()
								{
									if (!Entry.IsValid())
									{
										return FReply::Handled();
									}

									if (!Entry->bHasAsset)
									{
										if (CreateAssetFromEntry(*Entry))
										{
											Refresh();
										}
										return FReply::Handled();
									}

									if (!ListView.IsValid())
									{
										return FReply::Handled();
									}

									TSharedPtr<FYIItemDashboardEntry> AssetEntry;
									for (const TSharedPtr<FYIItemDashboardEntry>& It : Items)
									{
										if (It.IsValid() && !It->bIsDataTable && !It->bIsDataSourceEntry && It->Code == Entry->Code)
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
								SNew(STextBlock).Text(Entry->bHasAsset
									? NSLOCTEXT("YOLOInventory", "Dash_AssetBadge", "Asset")
									: NSLOCTEXT("YOLOInventory", "Dash_MakeAssetShort", "Make"))
							])
						: (Entry->bIsDataSourceEntry
							? StaticCastSharedRef<SWidget>(
								SNew(SButton)
									.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("SimpleButton"))
									.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_OpenSourceDetails", "Open data source details"))
									.OnClicked_Lambda([this, Entry]()
										{
											OpenDataSource(Entry);
											return FReply::Handled();
										})
									.Content()
									[
										SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Dash_SourceBadge", "Source"))
									])
							: StaticCastSharedRef<SWidget>(SNew(SSpacer).Size(FVector2D(40.f, 1.f))))
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
		ActiveBottomPanel = EYIDashboardBottomPanel::Logs;
	}
	if (ActiveBottomPanel == EYIDashboardBottomPanel::Logs && !bShowLogPanel)
	{
		if (bShowPreflightPanel) ActiveBottomPanel = EYIDashboardBottomPanel::Preflight;
		else if (bShowDiffPanel) ActiveBottomPanel = EYIDashboardBottomPanel::Diff;
	}

	Items.Reset();
	FilteredItems.Reset();
	TMap<int64, TSoftObjectPtr<UYIItemDefinition>> ExistingAssets;
	TSet<FString> ExistingRowKeys;
	TSet<FString> ExistingSourcePaths;

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
				Entry->bIsDataSourceEntry = false;
				Entry->RowName = View.RowName;
				Entry->Object = View.Object;
				Entry->DataSource = View.DataSource;
				if (Entry->DataSource.ToSoftObjectPath().IsValid())
				{
					ExistingSourcePaths.Add(Entry->DataSource.ToSoftObjectPath().ToString());
				}

				if (!Entry->bIsDataTable)
				{
					UObject* LoadedObject = View.Object.LoadSynchronous();
					if (UYIItemDefinition* Def = Cast<UYIItemDefinition>(LoadedObject))
					{
						Entry->ItemAsset = TSoftObjectPtr<UYIItemDefinition>(View.Object.ToSoftObjectPath());
						ExistingAssets.Add(Entry->Code, Entry->ItemAsset);
						FString ResolvedName;
						bool bMissingUIDisplayName = false;
						YIResolveDashboardDisplayName(Def, ResolvedName, bMissingUIDisplayName);
						Entry->Name = YIMakeDashboardName(ResolvedName, bMissingUIDisplayName);
						Entry->DataSource = Def->SourceDataSource;
						Entry->RowName = Def->SourceRowName;
					}
					else if (UYIDataTableItemSource* SourceAsset = Cast<UYIDataTableItemSource>(LoadedObject))
					{
						Entry->bIsDataSourceEntry = true;
						Entry->Name = SourceAsset->GetName();
						Entry->DataSource = SourceAsset;
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
						UYIDataTableItemSource* Source = Entry->DataSource.LoadSynchronous();
						if (Source)
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
							else if (Source)
							{
								FString MappedDisplayName;
								if (TryExtractRowDisplayNameFromMappingsEditor(Source, Table->RowStruct, *Found, MappedDisplayName))
								{
									Entry->Name = MappedDisplayName;
								}
							}

							if (Entry->TemplateId.IsEmpty() && Source)
							{
								FString MappedTemplateId;
								if (TryExtractRowTemplateIdFromMappingsEditor(Source, Table->RowStruct, *Found, MappedTemplateId))
								{
									Entry->TemplateId = MappedTemplateId;
								}
							}
						}
					}
				}

				Items.Add(Entry);
			}
		}
	}

	// Normalize row labels against generated item assets when available.
	for (const TSharedPtr<FYIItemDashboardEntry>& Entry : Items)
	{
		if (!Entry.IsValid() || !Entry->bIsDataTable || Entry->bIsDataSourceEntry)
		{
			continue;
		}

		if (TSoftObjectPtr<UYIItemDefinition>* FoundAsset = ExistingAssets.Find(Entry->Code))
		{
			Entry->bHasAsset = true;
			Entry->ItemAsset = *FoundAsset;

			if (UYIItemDefinition* Def = FoundAsset->LoadSynchronous())
			{
				FString ResolvedName;
				bool bMissingUIDisplayName = false;
				YIResolveDashboardDisplayName(Def, ResolvedName, bMissingUIDisplayName);
				Entry->Name = YIMakeDashboardName(ResolvedName, bMissingUIDisplayName);
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
		const FSoftObjectPath SourceSoftPath = SourceData.GetSoftObjectPath();
		const FString SourceObjectPath = SourceSoftPath.ToString();
		TSoftObjectPtr<UYIDataTableItemSource> SourcePtr(SourceSoftPath);

		if (!ExistingSourcePaths.Contains(SourceObjectPath))
		{
			TSharedPtr<FYIItemDashboardEntry> SourceEntry = MakeShared<FYIItemDashboardEntry>();
			SourceEntry->Code = 0;
			SourceEntry->Name = SourceData.AssetName.ToString();
			SourceEntry->TemplateId = TEXT("DataSource");
			SourceEntry->Source = SourceObjectPath;
			SourceEntry->bIsDataTable = false;
			SourceEntry->bIsDataSourceEntry = true;
			SourceEntry->Object = TSoftObjectPtr<UObject>(SourceSoftPath);
			SourceEntry->DataSource = SourcePtr;
			Items.Add(SourceEntry);
			ExistingSourcePaths.Add(SourceObjectPath);
		}

		UYIDataTableItemSource* Source = SourcePtr.LoadSynchronous();
		if (!Source)
		{
			continue;
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

				const bool bFoundCode = TryExtractRowCodeEditor(Table->RowStruct, RowData, CodeField, CodeValue);
				if (!bFoundCode)
				{
					continue;
				}
				if (TemplateField != NAME_None)
				{
					TryExtractRowTextFieldEditor(Table->RowStruct, RowData, TemplateField, TemplateIdValue);
				}
				if (TemplateIdValue.IsEmpty())
				{
					TryExtractRowTemplateIdFromMappingsEditor(Source, Table->RowStruct, RowData, TemplateIdValue);
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
				Entry->bIsDataSourceEntry = false;
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
				else
				{
					FString MappedDisplayName;
					if (TryExtractRowDisplayNameFromMappingsEditor(Source, Table->RowStruct, RowData, MappedDisplayName))
					{
						Entry->Name = MappedDisplayName;
					}
				}

				if (TSoftObjectPtr<UYIItemDefinition>* FoundAsset = ExistingAssets.Find(CodeValue))
				{
					Entry->bHasAsset = true;
					Entry->ItemAsset = *FoundAsset;
					if (UYIItemDefinition* Def = FoundAsset->LoadSynchronous())
					{
						FString ResolvedName;
						bool bMissingUIDisplayName = false;
						YIResolveDashboardDisplayName(Def, ResolvedName, bMissingUIDisplayName);
						Entry->Name = YIMakeDashboardName(ResolvedName, bMissingUIDisplayName);
					}
				}

				Items.Add(Entry);
				ExistingRowKeys.Add(RowKey);
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
		if (TypeFilter == EDashTypeFilter::DataTableRows && !(Entry->bIsDataTable || Entry->bIsDataSourceEntry))
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

void SYIItemDashboard::CreateDataTableSourceAsset()
{
	IAssetTools& Tools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
	Factory->DataAssetClass = UYIDataTableItemSource::StaticClass();

	const FString TargetPath = TEXT("/Game/YOLOInventory/ItemSources");
	const FString BaseName = TEXT("ItemSource");
	FString PackageName, AssetName;
	Tools.CreateUniqueAssetName(TargetPath / BaseName, TEXT(""), PackageName, AssetName);
	UObject* NewAsset = Tools.CreateAsset(AssetName, FPackageName::GetLongPackagePath(PackageName), Factory->DataAssetClass, Factory);
	Refresh();
	if (NewAsset)
	{
		OpenAsset(NewAsset);
	}
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

	if (Entry->bIsDataSourceEntry)
	{
		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("YOLOInventory", "Dash_Context_OpenDataSourceAsset", "Open Data Source Asset"),
			NSLOCTEXT("YOLOInventory", "Dash_Context_OpenDataSourceAsset_Tip", "Open this item data source in the details panel."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([Self, Entry]()
				{
					if (Self)
					{
						Self->OpenDataSource(Entry);
					}
				})));

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("YOLOInventory", "Dash_Context_ShowSourceInBrowser", "Show Data Source in Content Browser"),
			NSLOCTEXT("YOLOInventory", "Dash_Context_ShowSourceInBrowser_Tip", "Highlight this data source asset in the Content Browser."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([Entry]()
				{
					if (GEditor && Entry.IsValid())
					{
						if (UObject* SourceObj = Entry->Object.LoadSynchronous())
						{
							TArray<UObject*> Objects;
							Objects.Add(SourceObj);
							GEditor->SyncBrowserToObjects(Objects);
						}
					}
				})));

		MenuBuilder.AddSeparator();
		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("YOLOInventory", "Dash_Context_DeleteDataSource", "Delete Data Source Asset"),
			NSLOCTEXT("YOLOInventory", "Dash_Context_DeleteDataSource_Tip", "Delete this data source asset from disk."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([Self, Entry]()
				{
					if (!Self || !Entry.IsValid())
					{
						return;
					}

					if (UObject* SourceObj = Entry->Object.LoadSynchronous())
					{
						const FText Msg = FText::Format(
							NSLOCTEXT("YOLOInventory", "Dash_ConfirmDeleteDataSource", "Delete data source '{0}'?"),
							FText::FromString(SourceObj->GetName()));
						if (FMessageDialog::Open(EAppMsgType::YesNo, Msg) == EAppReturnType::Yes)
						{
							TArray<UObject*> ToDelete;
							ToDelete.Add(SourceObj);
							ObjectTools::DeleteObjects(ToDelete, /*bShowConfirmation=*/false);
							Self->Refresh();
						}
					}
				})));

		return MenuBuilder.MakeWidget();
	}

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

	// For source rows, keep details anchored to source assets so authoring stays data-source-first.
	if (Entry.bIsDataTable)
	{
		if (Entry.DataSource.IsValid())
		{
			if (UObject* Source = Entry.DataSource.LoadSynchronous())
			{
				return Source;
			}
		}

		if (Entry.DataTable.IsValid())
		{
			if (UObject* Table = Entry.DataTable.LoadSynchronous())
			{
				return Table;
			}
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

		auto GetCurrentSelection = [Self]() -> TArray<TSharedPtr<FYIItemDashboardEntry>>
		{
			if (!Self || !Self->ListView.IsValid())
			{
				return TArray<TSharedPtr<FYIItemDashboardEntry>>();
			}
			return Self->ListView->GetSelectedItems();
		};

		if (bAnyRows)
		{
			MenuBuilder.AddMenuEntry(
				NSLOCTEXT("YOLOInventory", "Dash_Context_BulkCreate", "Create Assets for Selected Rows"),
				NSLOCTEXT("YOLOInventory", "Dash_Context_BulkCreate_Tip", "Run transformer/inline mappings for all selected data table rows."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([Self, GetCurrentSelection]()
					{
						const TArray<TSharedPtr<FYIItemDashboardEntry>> CurrentSelected = GetCurrentSelection();
						bool bChanged = false;
						for (const TSharedPtr<FYIItemDashboardEntry>& E : CurrentSelected)
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

		const bool bAnyAssets = Selected.ContainsByPredicate([](const TSharedPtr<FYIItemDashboardEntry>& E)
			{
				return E.IsValid() && !E->bIsDataTable && !E->bIsDataSourceEntry;
			});
		const bool bAnyLinkedRows = Selected.ContainsByPredicate([](const TSharedPtr<FYIItemDashboardEntry>& E)
			{
				return E.IsValid() && E->bIsDataTable && (E->bHasAsset || E->ItemAsset.IsValid());
			});

		if (bAnyAssets || bAnyLinkedRows)
		{
			MenuBuilder.AddMenuEntry(
				NSLOCTEXT("YOLOInventory", "Dash_Context_BulkUpdateLinked", "Update Linked Assets for Selection"),
				NSLOCTEXT("YOLOInventory", "Dash_Context_BulkUpdateLinked_Tip", "Refresh linked item assets from current source row and inline mappings."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([Self, GetCurrentSelection]()
					{
						const TArray<TSharedPtr<FYIItemDashboardEntry>> CurrentSelected = GetCurrentSelection();
						bool bChanged = false;
						for (const TSharedPtr<FYIItemDashboardEntry>& E : CurrentSelected)
						{
							if (!E.IsValid())
							{
								continue;
							}

							if (E->bIsDataTable)
							{
								if (E->bHasAsset || E->ItemAsset.IsValid())
								{
									bChanged |= Self->CreateAssetFromEntry(*E);
								}
								continue;
							}

							if (UYIItemDefinition* Def = Cast<UYIItemDefinition>(E->Object.LoadSynchronous()))
							{
								bChanged |= Self->UpdateAssetFromLinkedSource(Def);
							}
						}

						if (bChanged)
						{
							Self->Refresh();
						}
					})));
		}

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("YOLOInventory", "Dash_Context_BulkPreflight", "Run Preflight for Selection"),
			NSLOCTEXT("YOLOInventory", "Dash_Context_BulkPreflight_Tip", "Run preflight checks for all selected entries."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([Self]()
				{
					if (Self)
					{
						Self->RebuildPreflightForSelection();
					}
				})));

		MenuBuilder.AddMenuEntry(
			NSLOCTEXT("YOLOInventory", "Dash_Context_BulkSave", "Save Selected Assets"),
			NSLOCTEXT("YOLOInventory", "Dash_Context_BulkSave_Tip", "Save all unique selected item and source assets."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([Self]()
				{
					if (Self)
					{
						Self->SaveCurrentAssetFromToolbar();
					}
				})));

		if (bAnyRows)
		{
			MenuBuilder.AddMenuEntry(
				NSLOCTEXT("YOLOInventory", "Dash_Context_BulkQueueRows", "Queue Selected Rows"),
				NSLOCTEXT("YOLOInventory", "Dash_Context_BulkQueueRows_Tip", "Add all selected data-table rows to the batch queue."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([Self]()
					{
						if (Self)
						{
							Self->EnqueueSelectedRows();
						}
					})));
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
	RefreshAddFragmentStructOptions();

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
	Source->Modify();

	// Build source-field options from the selected data source table.
	if (UDataTable* Table = Source->DataTable.LoadSynchronous())
	{
		if (UScriptStruct* RowStruct = Table->RowStruct)
		{
			TArray<TSharedPtr<FString>> SourcePaths;
			CollectStructFieldOptions(RowStruct, SourcePaths);
			for (const TSharedPtr<FString>& PathPtr : SourcePaths)
			{
				if (!PathPtr.IsValid())
				{
					continue;
				}

				const FString& PathString = *PathPtr;
				const FName FieldName(*PathString);
				FProperty* LeafProp = FindPropertyByAuthoredPathEditor(RowStruct, PathString);
				if (!LeafProp)
				{
					continue;
				}

				SourceFieldOptions.Add(MakeShared<FString>(PathString));
				SourceFieldPropCache.Add(FieldName, LeafProp);
			}
		}
	}

	// Build target cache from static definition fragments only (legacy properties are intentionally excluded).
	TArray<TSharedPtr<FString>> FragmentStructPaths;
	CollectFragmentStructOptions(FYIItemDefinitionFragmentBase::StaticStruct(), FragmentStructPaths);
	for (const TSharedPtr<FString>& StructPathPtr : FragmentStructPaths)
	{
		if (!StructPathPtr.IsValid())
		{
			continue;
		}
		UScriptStruct* FragmentStruct = ResolveStructFromPathString(*StructPathPtr);
		if (!FragmentStruct)
		{
			continue;
		}
		TArray<TSharedPtr<FString>> FieldOptions;
		CollectStructFieldOptions(FragmentStruct, FieldOptions);
		for (const TSharedPtr<FString>& FieldPathPtr : FieldOptions)
		{
			if (!FieldPathPtr.IsValid())
			{
				continue;
			}

			const FString FieldPath = *FieldPathPtr;
			FProperty* LeafProperty = FindPropertyByAuthoredPathEditor(FragmentStruct, FieldPath);
			if (!LeafProperty || !IsMappableFragmentField(LeafProperty))
			{
				continue;
			}

			const FString ScopedName = FString::Printf(TEXT("%s.%s"), *MakeReadableFragmentName(FragmentStruct), *FieldPath);
			TargetPropertyOptions.Add(MakeShared<FString>(ScopedName));
		}
	}
	TargetPropertyOptions.Sort([](const TSharedPtr<FString>& A, const TSharedPtr<FString>& B)
		{
			if (!A.IsValid() || !B.IsValid())
			{
				return A.IsValid();
			}
			return *A < *B;
		});

	bool bSourceModified = false;
	for (const FYIFieldMapping& M : Source->InlineMappings)
	{
		FYIFieldMapping Copy = M;
		if (TryPromoteLegacyItemMappingToFragment(Copy))
		{
			bSourceModified = true;
		}
		const bool bIdentityLegacy = (Copy.TargetLayer == EYIFieldMappingTargetLayer::LegacyProperty) && IsIdentityMappingProperty(Copy.TargetProperty);
		if (!bIdentityLegacy && Copy.TargetLayer != EYIFieldMappingTargetLayer::StaticDefinitionFragment)
		{
			Copy.TargetLayer = EYIFieldMappingTargetLayer::StaticDefinitionFragment;
			if (!Copy.TargetFragmentStruct || YIGetResolvedTargetFieldName(Copy).IsNone())
			{
				TryAssignDefaultStaticFragmentTarget(Copy);
			}
			bSourceModified = true;
		}
		MappingRows.Add(MakeShared<FYIFieldMapping>(Copy));
	}

	if (bSourceModified)
	{
		Source->Modify();
		Source->InlineMappings.Reset();
		Source->InlineMappings.Reserve(MappingRows.Num());
		for (const TSharedPtr<FYIFieldMapping>& Row : MappingRows)
		{
			if (Row.IsValid())
			{
				Source->InlineMappings.Add(*Row);
			}
		}
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

void SYIItemDashboard::RefreshAddFragmentStructOptions()
{
	AddFragmentStructOptions.Reset();
	CollectFragmentStructOptions(FYIItemDefinitionFragmentBase::StaticStruct(), AddFragmentStructOptions);
	TArray<TSharedPtr<FString>> RuntimeOptions;
	CollectFragmentStructOptions(FYIItemFragmentBase::StaticStruct(), RuntimeOptions);
	AddFragmentStructOptions.Append(RuntimeOptions);

	if (!SelectedAddFragmentStructOption.IsValid()
		|| !AddFragmentStructOptions.ContainsByPredicate([this](const TSharedPtr<FString>& Entry)
			{
				return Entry.IsValid()
					&& SelectedAddFragmentStructOption.IsValid()
					&& **Entry == **SelectedAddFragmentStructOption;
			}))
	{
		SelectedAddFragmentStructOption = AddFragmentStructOptions.Num() > 0
			? AddFragmentStructOptions[0]
			: TSharedPtr<FString>();
	}
}

void SYIItemDashboard::AddFragmentMappingsForStruct(UYIDataTableItemSource* Source, const UScriptStruct* FragmentStruct)
{
	if (!Source || !FragmentStruct)
	{
		return;
	}

	Source->Modify();
	UDataTable* Table = Source->DataTable.LoadSynchronous();
	const UScriptStruct* RowStruct = (Table ? Table->RowStruct : nullptr);

	TArray<TSharedPtr<FString>> FragmentFields;
	CollectStructFieldOptions(FragmentStruct, FragmentFields);

	bool bAddedAny = false;
	const bool bRuntimeFragmentStruct = FragmentStruct->IsChildOf(FYIItemFragmentBase::StaticStruct());
	const EYIFieldMappingTargetLayer TargetLayer = bRuntimeFragmentStruct
		? EYIFieldMappingTargetLayer::DynamicInstanceFragment
		: EYIFieldMappingTargetLayer::StaticDefinitionFragment;
	for (const TSharedPtr<FString>& FieldPathPtr : FragmentFields)
	{
		if (!FieldPathPtr.IsValid())
		{
			continue;
		}

		const FString FieldPath = *FieldPathPtr;
		FProperty* TargetProp = FindPropertyByAuthoredPathEditor(FragmentStruct, FieldPath);
		if (!TargetProp || !IsMappableFragmentField(TargetProp))
		{
			continue;
		}
		if (bRuntimeFragmentStruct && FieldPath == TEXT("Properties"))
		{
			continue;
		}
		if (!bRuntimeFragmentStruct && FieldPath == TEXT("Properties") && IsCustomDefinitionFragmentStructEditor(FragmentStruct))
		{
			continue;
		}

		const FName TargetFieldName(*FieldPath);
		const bool bAlreadyMapped = Source->InlineMappings.ContainsByPredicate([FragmentStruct, TargetFieldName](const FYIFieldMapping& Existing)
			{
				if (Existing.TargetLayer != (FragmentStruct->IsChildOf(FYIItemFragmentBase::StaticStruct())
					? EYIFieldMappingTargetLayer::DynamicInstanceFragment
					: EYIFieldMappingTargetLayer::StaticDefinitionFragment))
				{
					return false;
				}
				if (Existing.TargetFragmentStruct.Get() != FragmentStruct)
				{
					return false;
				}
				return YIGetResolvedTargetFieldName(Existing).IsEqual(TargetFieldName);
			});
		if (bAlreadyMapped)
		{
			continue;
		}

		FYIFieldMapping Mapping;
		Mapping.TargetLayer = TargetLayer;
		Mapping.TargetFragmentStruct = const_cast<UScriptStruct*>(FragmentStruct);
		Mapping.TargetFragmentField = TargetFieldName;
		Mapping.TargetProperty = TargetFieldName;

		if (RowStruct)
		{
			// Exact nested-path match first (e.g. Stats.Attack -> Fragment.Attack).
			if (FProperty* SourceProp = FindPropertyByAuthoredPathEditor(RowStruct, FieldPath))
			{
				Mapping.SourceField = FName(*FieldPath);
				Mapping.Conversion = GuessConversionForProps(SourceProp, TargetProp);
			}
			else
			{
				// Fallback to leaf-name match.
				const FName LeafName(*TargetProp->GetAuthoredName());
				if (FProperty* SourceLeafProp = FindPropertyByAuthoredNameEditor(RowStruct, LeafName))
				{
					Mapping.SourceField = LeafName;
					Mapping.Conversion = GuessConversionForProps(SourceLeafProp, TargetProp);
				}
			}
		}

		Source->InlineMappings.Add(Mapping);
		bAddedAny = true;
	}

	if (bAddedAny)
	{
		RefreshInlineMappingEditor(Source);
	}
}

void SYIItemDashboard::AddFragmentMappingsForSelection()
{
	if (!CurrentMappingSource.IsValid())
	{
		return;
	}

	RefreshAddFragmentStructOptions();
	if (!SelectedAddFragmentStructOption.IsValid())
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error, NSLOCTEXT("YOLOInventory", "Dash_NoFragmentForAdd", "No fragment type is available to add mappings."));
		return;
	}

	UScriptStruct* FragmentStruct = ResolveStructFromPathString(*SelectedAddFragmentStructOption);
	if (!FragmentStruct)
	{
		FYIEditorMessageLog::Add(EYIEditorLogSeverity::Error, FText::Format(
			NSLOCTEXT("YOLOInventory", "Dash_InvalidFragmentSelection", "Invalid fragment selection: {0}"),
			FText::FromString(*SelectedAddFragmentStructOption)));
		return;
	}

	AddFragmentMappingsForStruct(CurrentMappingSource.Get(), FragmentStruct);
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

	UYIItemDefinition* PreviewDef = NewObject<UYIItemDefinition>();

	for (const FYIFieldMapping& Mapping : Source->InlineMappings)
	{
		TSharedPtr<FYIMappingPreviewRow> Row = MakeShared<FYIMappingPreviewRow>();
		Row->SourceField = Mapping.SourceField;
		{
			const FName ResolvedTargetField = YIGetResolvedTargetFieldName(Mapping);
			if (Mapping.TargetLayer == EYIFieldMappingTargetLayer::StaticDefinitionFragment && Mapping.TargetFragmentStruct && !ResolvedTargetField.IsNone())
			{
				const FString Scoped = FString::Printf(TEXT("%s.%s"), *MakeReadableFragmentName(Mapping.TargetFragmentStruct.Get()), *ResolvedTargetField.ToString());
				Row->TargetProperty = FName(*Scoped);
			}
			else
			{
				Row->TargetProperty = ResolvedTargetField;
			}
		}

		const FProperty* SourceProp = nullptr;
		const uint8* SrcPtr = nullptr;
		if (!Mapping.bUseStaticValue)
		{
			YIResolveMappingSource(Table->RowStruct, RowPtr, Mapping.SourceField, SourceProp, SrcPtr, nullptr);
		}

		FYIResolvedMappingTarget ResolvedTarget;
		FProperty* TargetProp = nullptr;
		uint8* TargetPtr = nullptr;
		if (PreviewDef)
		{
			YIResolveMappingTarget(PreviewDef, Mapping, ResolvedTarget, nullptr);
			TargetProp = ResolvedTarget.Property;
			TargetPtr = ResolvedTarget.ValuePtr;
		}

		if (!TargetProp || !TargetPtr || (!SourceProp && !Mapping.bUseStaticValue))
		{
			Row->Status = NSLOCTEXT("YOLOInventory", "Dash_PreviewMissing", "Missing field.");
			Row->StatusColor = FLinearColor(1.f, 0.25f, 0.2f);
			MappingPreviewRows.Add(Row);
			continue;
		}

		Row->SourceValue = Mapping.bUseStaticValue
			? Mapping.StaticValue
			: ExportPropertyValueToString(SourceProp, SrcPtr);

		TArray<uint8> Temp;
		Temp.SetNumZeroed(TargetProp->GetSize());
		TargetProp->InitializeValue(Temp.GetData());
		TargetProp->CopyCompleteValue(Temp.GetData(), TargetPtr);

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
				const FProperty* SourcePropB = nullptr;
				const uint8* SrcPtrB = nullptr;
				YIResolveMappingSource(Table->RowStruct, RowPtr, Mapping.SourceFieldB, SourcePropB, SrcPtrB, nullptr);
				const FNumericProperty* NumA = CastField<FNumericProperty>(SourceProp);
				const FNumericProperty* NumB = CastField<FNumericProperty>(SourcePropB);
				if (!SourcePropB || !NumA || !NumB)
				{
					Row->ConvertedValue = TEXT("<vector2d conversion failed>");
					bWarn = true;
				}
				else if (const FStructProperty* DestStruct = CastField<FStructProperty>(TargetProp))
				{
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
	TArray<TSharedPtr<FString>> SourcePaths;
	CollectStructFieldOptions(Table->RowStruct, SourcePaths);
	for (const TSharedPtr<FString>& SourcePathPtr : SourcePaths)
	{
		if (!SourcePathPtr.IsValid())
		{
			continue;
		}
		const FName FieldName(**SourcePathPtr);
		const FString NameStr = FieldName.ToString();
		const FString Lower = NameStr.ToLower();
		const FString Norm = NormalizeField(NameStr);
		SourceByNorm.Add(Norm, FieldName);
		SourceByLower.Add(Lower, FieldName);
		SourceCandidates.Add({ FieldName, Lower, Norm });
	}

	struct FTargetCandidate
	{
		UScriptStruct* FragmentStruct = nullptr;
		FName FieldName = NAME_None;
		FProperty* Property = nullptr;
		FString Lower;
		FString Norm;
		FString Key;
	};

	TArray<FTargetCandidate> TargetCandidates;
	TMap<FString, int32> TargetByLower;
	TMap<FString, int32> TargetByNorm;

	TArray<TSharedPtr<FString>> FragmentOptions;
	CollectFragmentStructOptions(FYIItemDefinitionFragmentBase::StaticStruct(), FragmentOptions);
	for (const TSharedPtr<FString>& FragmentPath : FragmentOptions)
	{
		if (!FragmentPath.IsValid())
		{
			continue;
		}
		UScriptStruct* FragmentStruct = ResolveStructFromPathString(*FragmentPath);
		if (!FragmentStruct)
		{
			continue;
		}

		TArray<TSharedPtr<FString>> FragmentFieldPaths;
		CollectStructFieldOptions(FragmentStruct, FragmentFieldPaths);
		for (const TSharedPtr<FString>& FragmentFieldPathPtr : FragmentFieldPaths)
		{
			if (!FragmentFieldPathPtr.IsValid())
			{
				continue;
			}
			const FString FieldString = **FragmentFieldPathPtr;
			FProperty* TargetProp = FindPropertyByAuthoredPathEditor(FragmentStruct, FieldString);
			if (!IsMappableFragmentField(TargetProp))
			{
				continue;
			}

			const FName FieldName(*FieldString);
			const FString Lower = FieldString.ToLower();
			const FString Norm = NormalizeField(FieldString);
			const FString Key = FString::Printf(TEXT("%s|%s"), *FragmentStruct->GetPathName(), *FieldString);
			const int32 NewIndex = TargetCandidates.Add({ FragmentStruct, FieldName, TargetProp, Lower, Norm, Key });
			TargetByLower.Add(Lower, NewIndex);
			TargetByNorm.Add(Norm, NewIndex);
		}
	}

	auto BuildTargetKey = [](const FYIFieldMapping& Mapping) -> FString
		{
			if (Mapping.TargetLayer == EYIFieldMappingTargetLayer::StaticDefinitionFragment && Mapping.TargetFragmentStruct)
			{
				const FName Resolved = YIGetResolvedTargetFieldName(Mapping);
				if (!Resolved.IsNone())
				{
					return FString::Printf(TEXT("%s|%s"), *Mapping.TargetFragmentStruct->GetPathName(), *Resolved.ToString());
				}
			}
			return FString();
		};

	bool bChanged = false;
	int32 MatchCount = 0;
	TSet<FString> ExistingTargetKeys;
	for (FYIFieldMapping& Mapping : Source->InlineMappings)
	{
		TryPromoteLegacyItemMappingToFragment(Mapping);
		if (Mapping.TargetLayer == EYIFieldMappingTargetLayer::LegacyProperty && IsIdentityMappingProperty(Mapping.TargetProperty))
		{
			if (Mapping.SourceField.IsNone())
			{
				if (Mapping.TargetProperty == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, UniqueCode))
				{
					Mapping.SourceField = Source->UniqueCodeFieldName;
				}
				else if (Mapping.TargetProperty == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, TemplateId))
				{
					Mapping.SourceField = Source->TemplateIdFieldName;
				}
				if (!Mapping.SourceField.IsNone())
				{
					Mapping.Conversion = GuessConversionForProps(SourceFieldPropCache.FindRef(Mapping.SourceField), FindPropertyByAuthoredNameEditor(UYIItemDefinition::StaticClass(), Mapping.TargetProperty));
					bChanged = true;
					++MatchCount;
				}
			}
		}
		const FString Key = BuildTargetKey(Mapping);
		if (!Key.IsEmpty())
		{
			ExistingTargetKeys.Add(Key);
		}
	}

	auto FindBestSourceField = [&](const FName& TargetField)->FName
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

	// Patch existing mappings that have missing source fields.
	for (FYIFieldMapping& M : Source->InlineMappings)
	{
		if (M.bUseStaticValue)
		{
			continue;
		}

		if (M.TargetLayer == EYIFieldMappingTargetLayer::LegacyProperty && IsIdentityMappingProperty(M.TargetProperty))
		{
			if (M.SourceField.IsNone())
			{
				const FName FallbackSource = (M.TargetProperty == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, UniqueCode))
					? Source->UniqueCodeFieldName
					: Source->TemplateIdFieldName;
				if (!FallbackSource.IsNone())
				{
					M.SourceField = FallbackSource;
					M.Conversion = GuessConversionForProps(SourceFieldPropCache.FindRef(M.SourceField), FindPropertyByAuthoredNameEditor(UYIItemDefinition::StaticClass(), M.TargetProperty));
					bChanged = true;
					++MatchCount;
				}
			}
			continue;
		}

		if (M.TargetLayer != EYIFieldMappingTargetLayer::StaticDefinitionFragment)
		{
			M.TargetLayer = EYIFieldMappingTargetLayer::StaticDefinitionFragment;
			if (!M.TargetFragmentStruct || YIGetResolvedTargetFieldName(M).IsNone())
			{
				TryAssignDefaultStaticFragmentTarget(M);
			}
			bChanged = true;
		}

		const FName TargetField = YIGetResolvedTargetFieldName(M);
		if (TargetField.IsNone() || !M.SourceField.IsNone())
		{
			continue;
		}

		const FName Match = FindBestSourceField(TargetField);
		if (!Match.IsNone())
		{
			M.SourceField = Match;
			const FProperty* SourceProp = SourceFieldPropCache.FindRef(Match);
			const FProperty* TargetProp = nullptr;
			if (M.TargetFragmentStruct)
			{
				TargetProp = FindPropertyByAuthoredPathEditor(M.TargetFragmentStruct.Get(), TargetField.ToString());
			}
			M.Conversion = GuessConversionForProps(SourceProp, TargetProp);
			bChanged = true;
			++MatchCount;
		}
	}

	if (bAddAllFields)
	{
		auto EnsureIdentityMapping = [&](FName TargetProperty, FName PreferredSource)
			{
				if (TargetProperty.IsNone())
				{
					return;
				}

				bool bExists = false;
				for (const FYIFieldMapping& Existing : Source->InlineMappings)
				{
					if (Existing.TargetLayer == EYIFieldMappingTargetLayer::LegacyProperty && Existing.TargetProperty == TargetProperty)
					{
						bExists = true;
						break;
					}
				}
				if (bExists)
				{
					return;
				}

				FYIFieldMapping IdentityMap;
				IdentityMap.TargetLayer = EYIFieldMappingTargetLayer::LegacyProperty;
				IdentityMap.TargetProperty = TargetProperty;
				IdentityMap.SourceField = PreferredSource;
				IdentityMap.Conversion = GuessConversionForProps(SourceFieldPropCache.FindRef(IdentityMap.SourceField), FindPropertyByAuthoredNameEditor(UYIItemDefinition::StaticClass(), TargetProperty));
				Source->InlineMappings.Add(IdentityMap);
				bChanged = true;
				if (!IdentityMap.SourceField.IsNone())
				{
					++MatchCount;
				}
			};

		EnsureIdentityMapping(GET_MEMBER_NAME_CHECKED(UYIItemDefinition, UniqueCode), Source->UniqueCodeFieldName);
		EnsureIdentityMapping(GET_MEMBER_NAME_CHECKED(UYIItemDefinition, TemplateId), Source->TemplateIdFieldName);

		for (const FTargetCandidate& Target : TargetCandidates)
		{
			if (!Target.FragmentStruct || Target.FieldName.IsNone() || ExistingTargetKeys.Contains(Target.Key))
			{
				continue;
			}

			FYIFieldMapping NewMap;
			NewMap.TargetLayer = EYIFieldMappingTargetLayer::StaticDefinitionFragment;
			NewMap.TargetFragmentStruct = Target.FragmentStruct;
			NewMap.TargetFragmentField = Target.FieldName;
			NewMap.TargetProperty = Target.FieldName;
			NewMap.SourceField = FindBestSourceField(Target.FieldName);
			NewMap.Conversion = GuessConversionForProps(SourceFieldPropCache.FindRef(NewMap.SourceField), Target.Property);

			Source->InlineMappings.Add(NewMap);
			ExistingTargetKeys.Add(Target.Key);
			bChanged = true;
			if (!NewMap.SourceField.IsNone())
			{
				++MatchCount;
			}
		}
	}

	if (bChanged)
	{
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
			if (const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>())
			{
				FEdGraphPinType PinType;
				if (K2Schema->ConvertPropertyToPinType(Prop, PinType))
				{
					OutColor = K2Schema->GetPinTypeColor(PinType);
				}
			}
			if (const FNumericProperty* Num = CastField<FNumericProperty>(Prop))
			{
				OutLabel = Num->IsInteger() ? TEXT("Int") : TEXT("Float");
				return;
			}
			if (CastField<FBoolProperty>(Prop)) { OutLabel = TEXT("Bool"); return; }
			if (CastField<FNameProperty>(Prop)) { OutLabel = TEXT("Name"); return; }
			if (CastField<FStrProperty>(Prop)) { OutLabel = TEXT("String"); return; }
			if (CastField<FTextProperty>(Prop)) { OutLabel = TEXT("Text"); return; }
			if (CastField<FEnumProperty>(Prop)) { OutLabel = TEXT("Enum"); return; }
			if (CastField<FStructProperty>(Prop)) { OutLabel = TEXT("Struct"); return; }
			if (CastField<FObjectPropertyBase>(Prop)) { OutLabel = TEXT("Object"); return; }
			if (CastField<FSoftObjectProperty>(Prop)) { OutLabel = TEXT("SoftObj"); return; }
			if (CastField<FArrayProperty>(Prop)) { OutLabel = TEXT("Array"); return; }
			if (CastField<FMapProperty>(Prop)) { OutLabel = TEXT("Map"); return; }
			if (CastField<FSetProperty>(Prop)) { OutLabel = TEXT("Set"); return; }
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

	TSharedPtr<FInstancedPropertyBag> TargetPropertyBagScratch = MakeShared<FInstancedPropertyBag>();

	auto IsPropertyBagTarget = [Mapping]() -> bool
		{
			return Mapping.IsValid() && Mapping->bTargetPropertyBagField && !Mapping->TargetPropertyBagFieldName.IsNone();
		};

	auto IsCustomFragmentTarget = [Mapping]() -> bool
		{
			return Mapping.IsValid() && IsCustomFragmentStructEditor(Mapping->TargetFragmentStruct.Get());
		};

	auto InferPropertyBagTypeFromSourceProp = [](const FProperty* SourceProp, EPropertyBagPropertyType& OutType, UObject*& OutTypeObject) -> bool
		{
			OutType = EPropertyBagPropertyType::String;
			OutTypeObject = nullptr;
			if (!SourceProp)
			{
				return false;
			}

			if (CastField<FBoolProperty>(SourceProp))
			{
				OutType = EPropertyBagPropertyType::Bool;
				return true;
			}
			if (const FIntProperty* IntProp = CastField<FIntProperty>(SourceProp))
			{
				(void)IntProp;
				OutType = EPropertyBagPropertyType::Int32;
				return true;
			}
			if (const FInt64Property* Int64Prop = CastField<FInt64Property>(SourceProp))
			{
				(void)Int64Prop;
				OutType = EPropertyBagPropertyType::Int64;
				return true;
			}
			if (const FFloatProperty* FloatProp = CastField<FFloatProperty>(SourceProp))
			{
				(void)FloatProp;
				OutType = EPropertyBagPropertyType::Float;
				return true;
			}
			if (const FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(SourceProp))
			{
				(void)DoubleProp;
				OutType = EPropertyBagPropertyType::Double;
				return true;
			}
			if (CastField<FNameProperty>(SourceProp))
			{
				OutType = EPropertyBagPropertyType::Name;
				return true;
			}
			if (CastField<FStrProperty>(SourceProp))
			{
				OutType = EPropertyBagPropertyType::String;
				return true;
			}
			if (CastField<FTextProperty>(SourceProp))
			{
				OutType = EPropertyBagPropertyType::Text;
				return true;
			}
			if (const FStructProperty* StructProp = CastField<FStructProperty>(SourceProp))
			{
				if (StructProp->Struct == FGameplayTag::StaticStruct())
				{
					OutType = EPropertyBagPropertyType::Struct;
					OutTypeObject = StructProp->Struct.Get();
					return true;
				}
				OutType = EPropertyBagPropertyType::Struct;
				OutTypeObject = StructProp->Struct.Get();
				return true;
			}
			if (const FSoftObjectProperty* SoftObj = CastField<FSoftObjectProperty>(SourceProp))
			{
				OutType = EPropertyBagPropertyType::SoftObject;
				OutTypeObject = SoftObj->PropertyClass.Get();
				return true;
			}
			if (const FObjectPropertyBase* Obj = CastField<FObjectPropertyBase>(SourceProp))
			{
				OutType = EPropertyBagPropertyType::Object;
				OutTypeObject = Obj->PropertyClass.Get();
				return true;
			}
			if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(SourceProp))
			{
				OutType = EPropertyBagPropertyType::Enum;
				OutTypeObject = const_cast<UEnum*>(EnumProp->GetEnum());
				return true;
			}
			if (const FByteProperty* ByteProp = CastField<FByteProperty>(SourceProp); ByteProp && ByteProp->Enum)
			{
				OutType = EPropertyBagPropertyType::Enum;
				OutTypeObject = ByteProp->Enum.Get();
				return true;
			}
			return false;
		};

	auto GetTargetProp = [this, Mapping, TargetPropertyBagScratch]() -> FProperty*
		{
			if (!Mapping.IsValid())
			{
				return nullptr;
			}

			if (Mapping->TargetLayer == EYIFieldMappingTargetLayer::LegacyProperty)
			{
				if (!IsIdentityMappingProperty(Mapping->TargetProperty))
				{
					return nullptr;
				}
				return FindPropertyByAuthoredPathEditor(UYIItemDefinition::StaticClass(), Mapping->TargetProperty.ToString());
			}

			const UScriptStruct* FragmentStruct = Mapping->TargetFragmentStruct.Get();
			if (!FragmentStruct)
			{
				return nullptr;
			}

			if (Mapping->bTargetPropertyBagField && !Mapping->TargetPropertyBagFieldName.IsNone() && IsCustomFragmentStructEditor(FragmentStruct))
			{
				TargetPropertyBagScratch->Reset();
				const FName SafeName = FInstancedPropertyBag::SanitizePropertyName(Mapping->TargetPropertyBagFieldName);
				const EPropertyBagPropertyType BagType = Mapping->TargetPropertyBagFieldType;
				if (SafeName.IsNone() || BagType == EPropertyBagPropertyType::None || BagType == EPropertyBagPropertyType::Count)
				{
					return nullptr;
				}

				UObject* TypeObject = nullptr;
				if (Mapping->TargetPropertyBagFieldTypeObject.ToSoftObjectPath().IsValid())
				{
					TypeObject = Mapping->TargetPropertyBagFieldTypeObject.LoadSynchronous();
				}
				if (TargetPropertyBagScratch->AddProperty(SafeName, BagType, TypeObject, true) != EPropertyBagAlterationResult::Success)
				{
					return nullptr;
				}
				const UPropertyBag* BagStruct = TargetPropertyBagScratch->GetPropertyBagStruct();
				return BagStruct ? FindPropertyByAuthoredPathEditor(BagStruct, SafeName.ToString()) : nullptr;
			}

			return FindPropertyByAuthoredPathEditor(FragmentStruct, YIGetResolvedTargetFieldName(*Mapping).ToString());
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

	auto GetTargetBool = [GetTargetProp]() -> const FBoolProperty*
		{
			return CastField<FBoolProperty>(GetTargetProp());
		};

	auto GetTargetNumeric = [GetTargetProp]() -> const FNumericProperty*
		{
			return CastField<FNumericProperty>(GetTargetProp());
		};

	auto IsGameplayTagStaticTarget = [Mapping, GetTargetProp]() -> bool
		{
			const FProperty* TargetProp = GetTargetProp();
			if (const FStructProperty* StructProp = CastField<FStructProperty>(TargetProp))
			{
				if (StructProp->Struct == FGameplayTag::StaticStruct())
				{
					return true;
				}
			}
			return Mapping.IsValid() && Mapping->Conversion == EYIFieldMappingConversion::ToGameplayTag;
		};

	auto ParseStaticNumber = [Mapping]() -> TOptional<double>
		{
			if (!Mapping.IsValid() || Mapping->StaticValue.IsEmpty())
			{
				return TOptional<double>();
			}
			double Parsed = 0.0;
			if (LexTryParseString(Parsed, *Mapping->StaticValue))
			{
				return Parsed;
			}
			return TOptional<double>();
		};

	auto GetNumericClamp = [GetTargetNumeric](const TCHAR* MetaKey) -> TOptional<double>
		{
			const FNumericProperty* NumProp = GetTargetNumeric();
			if (!NumProp)
			{
				return TOptional<double>();
			}
			const FString Meta = NumProp->GetMetaData(MetaKey);
			if (Meta.IsEmpty())
			{
				return TOptional<double>();
			}
			double Value = 0.0;
			return LexTryParseString(Value, *Meta) ? TOptional<double>(Value) : TOptional<double>();
		};

	TSharedPtr<TArray<TSharedPtr<FString>>> StaticEnumOptions = MakeShared<TArray<TSharedPtr<FString>>>();
	TSharedPtr<TArray<TSharedPtr<FString>>> StaticGameplayTagOptions = MakeShared<TArray<TSharedPtr<FString>>>();
	auto RefreshMappingUi = [this]()
		{
			RefreshMappingPreview();
			if (MappingListView.IsValid())
			{
				MappingListView->RequestListRefresh();
			}
		};

	auto RefreshStaticGameplayTagOptions = [StaticGameplayTagOptions]()
		{
			StaticGameplayTagOptions->Reset();
			FGameplayTagContainer AllTags;
			UGameplayTagsManager::Get().RequestAllGameplayTags(AllTags, true);
			TArray<FGameplayTag> TagArray;
			AllTags.GetGameplayTagArray(TagArray);
			TagArray.Sort([](const FGameplayTag& A, const FGameplayTag& B)
				{
					return A.ToString() < B.ToString();
				});
			for (const FGameplayTag& Tag : TagArray)
			{
				StaticGameplayTagOptions->Add(MakeShared<FString>(Tag.ToString()));
			}
		};

	TSharedPtr<TArray<TSharedPtr<FString>>> TargetLayerOptions = MakeShared<TArray<TSharedPtr<FString>>>();
	TSharedPtr<TArray<TSharedPtr<FString>>> IdentityTargetOptions = MakeShared<TArray<TSharedPtr<FString>>>();
	TSharedPtr<TArray<TSharedPtr<FString>>> TargetFragmentStructOptions = MakeShared<TArray<TSharedPtr<FString>>>();
	TSharedPtr<TArray<TSharedPtr<FString>>> TargetFragmentFieldOptions = MakeShared<TArray<TSharedPtr<FString>>>();

	auto GetTargetLayerLabel = [](EYIFieldMappingTargetLayer Layer) -> FString
		{
			switch (Layer)
			{
			case EYIFieldMappingTargetLayer::DynamicInstanceFragment:
				return TEXT("Runtime Fragment");
			case EYIFieldMappingTargetLayer::StaticDefinitionFragment:
				return TEXT("Fragment");
			case EYIFieldMappingTargetLayer::LegacyProperty:
			default:
				return TEXT("Identity");
			}
		};

	auto RefreshTargetLayerOptions = [TargetLayerOptions]()
		{
			TargetLayerOptions->Reset();
			TargetLayerOptions->Add(MakeShared<FString>(TEXT("Fragment")));
			TargetLayerOptions->Add(MakeShared<FString>(TEXT("Runtime Fragment")));
			TargetLayerOptions->Add(MakeShared<FString>(TEXT("Identity")));
		};

	auto RefreshIdentityTargetOptions = [IdentityTargetOptions]()
		{
			IdentityTargetOptions->Reset();
			IdentityTargetOptions->Add(MakeShared<FString>(GET_MEMBER_NAME_STRING_CHECKED(UYIItemDefinition, UniqueCode)));
			IdentityTargetOptions->Add(MakeShared<FString>(GET_MEMBER_NAME_STRING_CHECKED(UYIItemDefinition, TemplateId)));
		};

	auto RefreshTargetFragmentStructOptions = [Mapping, TargetFragmentStructOptions]()
		{
			TargetFragmentStructOptions->Reset();
			if (Mapping.IsValid() && Mapping->TargetLayer == EYIFieldMappingTargetLayer::DynamicInstanceFragment)
			{
				CollectFragmentStructOptions(FYIItemFragmentBase::StaticStruct(), *TargetFragmentStructOptions);
				return;
			}
			CollectFragmentStructOptions(FYIItemDefinitionFragmentBase::StaticStruct(), *TargetFragmentStructOptions);
		};

	auto RefreshTargetFragmentFieldOptions = [this, Mapping, TargetFragmentFieldOptions]()
		{
			TargetFragmentFieldOptions->Reset();
			if (!Mapping.IsValid())
			{
				return;
			}
			const UScriptStruct* FragmentStruct = Mapping->TargetFragmentStruct.Get();
			if (!FragmentStruct)
			{
				return;
			}
			CollectStructFieldOptions(FragmentStruct, *TargetFragmentFieldOptions);
			if (IsCustomFragmentStructEditor(FragmentStruct))
			{
				TargetFragmentFieldOptions->RemoveAll([](const TSharedPtr<FString>& Entry)
				{
					return Entry.IsValid() && *Entry == TEXT("Properties");
				});
				CollectPropertyBagFieldOptionsForFragmentMappingEditor(CurrentMappingSource.Get(), FragmentStruct, LastDetailObject.Get(), *TargetFragmentFieldOptions);
			}
		};

	RefreshTargetFragmentStructOptions();
	RefreshTargetFragmentFieldOptions();

	TSharedPtr<TArray<TSharedPtr<FString>>> PropertyBagTypePresetOptions = MakeShared<TArray<TSharedPtr<FString>>>();
	auto RefreshPropertyBagTypePresetOptions = [PropertyBagTypePresetOptions]()
		{
			PropertyBagTypePresetOptions->Reset();
			const TCHAR* Labels[] =
			{
				TEXT("Bool"),
				TEXT("Int32"),
				TEXT("Int64"),
				TEXT("Float"),
				TEXT("Double"),
				TEXT("Name"),
				TEXT("String"),
				TEXT("Text"),
				TEXT("GameplayTag"),
				TEXT("SoftObject")
			};
			for (const TCHAR* Label : Labels)
			{
				PropertyBagTypePresetOptions->Add(MakeShared<FString>(Label));
			}
		};
	RefreshPropertyBagTypePresetOptions();

	auto ApplyPropertyBagPresetByLabel = [](const FString& Label, FYIFieldMapping& InOutMapping)
		{
			InOutMapping.TargetPropertyBagFieldTypeObject.Reset();
			if (Label == TEXT("Bool")) { InOutMapping.TargetPropertyBagFieldType = EPropertyBagPropertyType::Bool; return; }
			if (Label == TEXT("Int32")) { InOutMapping.TargetPropertyBagFieldType = EPropertyBagPropertyType::Int32; return; }
			if (Label == TEXT("Int64")) { InOutMapping.TargetPropertyBagFieldType = EPropertyBagPropertyType::Int64; return; }
			if (Label == TEXT("Float")) { InOutMapping.TargetPropertyBagFieldType = EPropertyBagPropertyType::Float; return; }
			if (Label == TEXT("Double")) { InOutMapping.TargetPropertyBagFieldType = EPropertyBagPropertyType::Double; return; }
			if (Label == TEXT("Name")) { InOutMapping.TargetPropertyBagFieldType = EPropertyBagPropertyType::Name; return; }
			if (Label == TEXT("String")) { InOutMapping.TargetPropertyBagFieldType = EPropertyBagPropertyType::String; return; }
			if (Label == TEXT("Text")) { InOutMapping.TargetPropertyBagFieldType = EPropertyBagPropertyType::Text; return; }
			if (Label == TEXT("SoftObject")) { InOutMapping.TargetPropertyBagFieldType = EPropertyBagPropertyType::SoftObject; return; }
			if (Label == TEXT("GameplayTag"))
			{
				InOutMapping.TargetPropertyBagFieldType = EPropertyBagPropertyType::Struct;
				InOutMapping.TargetPropertyBagFieldTypeObject = TSoftObjectPtr<UObject>(FGameplayTag::StaticStruct());
				return;
			}
		};

	auto GetPropertyBagPresetLabel = [Mapping]() -> FString
		{
			if (!Mapping.IsValid())
			{
				return TEXT("String");
			}
			if (Mapping->TargetPropertyBagFieldType == EPropertyBagPropertyType::Struct)
			{
				UObject* TypeObject = Mapping->TargetPropertyBagFieldTypeObject.Get();
				if (!TypeObject && Mapping->TargetPropertyBagFieldTypeObject.ToSoftObjectPath().IsValid())
				{
					TypeObject = Mapping->TargetPropertyBagFieldTypeObject.LoadSynchronous();
				}
				if (TypeObject == FGameplayTag::StaticStruct())
				{
					return TEXT("GameplayTag");
				}
			}

			switch (Mapping->TargetPropertyBagFieldType)
			{
			case EPropertyBagPropertyType::Bool: return TEXT("Bool");
			case EPropertyBagPropertyType::Int32: return TEXT("Int32");
			case EPropertyBagPropertyType::Int64: return TEXT("Int64");
			case EPropertyBagPropertyType::Float: return TEXT("Float");
			case EPropertyBagPropertyType::Double: return TEXT("Double");
			case EPropertyBagPropertyType::Name: return TEXT("Name");
			case EPropertyBagPropertyType::String: return TEXT("String");
			case EPropertyBagPropertyType::Text: return TEXT("Text");
			case EPropertyBagPropertyType::SoftObject: return TEXT("SoftObject");
			default: return TEXT("String");
			}
		};

	auto PersistCurrentMapping = [this, Mapping]()
		{
			if (!CurrentMappingSource.IsValid() || !Mapping.IsValid())
			{
				return;
			}

			const int32 Index = MappingRows.Find(Mapping);
			if (Index != INDEX_NONE)
			{
				CurrentMappingSource->InlineMappings[Index] = *Mapping;
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
								.WidgetIndex_Lambda([IsStaticMapping, GetTargetEnum, GetTargetBool, IsGameplayTagStaticTarget, GetTargetNumeric]()
									{
										if (!IsStaticMapping())
										{
											return 0;
										}
										if (GetTargetEnum() != nullptr)
										{
											return 1;
										}
										if (GetTargetBool() != nullptr)
										{
											return 2;
										}
										if (IsGameplayTagStaticTarget())
										{
											return 3;
										}
										if (GetTargetNumeric() != nullptr)
										{
											return 4;
										}
										return 0;
									})
								+ SWidgetSwitcher::Slot()
								[
									SNew(SEditableTextBox)
										.IsEnabled_Lambda([IsStaticMapping, GetTargetEnum, GetTargetBool, IsGameplayTagStaticTarget, GetTargetNumeric]()
											{
												return IsStaticMapping()
													&& GetTargetEnum() == nullptr
													&& GetTargetBool() == nullptr
													&& !IsGameplayTagStaticTarget()
													&& GetTargetNumeric() == nullptr;
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
								+ SWidgetSwitcher::Slot()
								[
									SNew(SCheckBox)
										.IsChecked_Lambda([Mapping]()
											{
												if (!Mapping.IsValid())
												{
													return ECheckBoxState::Unchecked;
												}
												const FString Lower = Mapping->StaticValue.ToLower();
												const bool bTrue = (Lower == TEXT("true") || Lower == TEXT("1") || Lower == TEXT("yes") || Lower == TEXT("on"));
												return bTrue ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
											})
										.OnCheckStateChanged_Lambda([this, Mapping, RefreshMappingUi](ECheckBoxState State)
											{
												if (CurrentMappingSource.IsValid() && Mapping.IsValid())
												{
													CurrentMappingSource->Modify();
													Mapping->StaticValue = (State == ECheckBoxState::Checked) ? TEXT("true") : TEXT("false");
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
										[
											SNew(STextBlock)
												.Text(NSLOCTEXT("YOLOInventory", "Dash_StaticBoolLabel", "Static Bool"))
										]
								]
								+ SWidgetSwitcher::Slot()
								[
									SNew(SComboBox<TSharedPtr<FString>>)
										.OptionsSource(StaticGameplayTagOptions.Get())
										.IsEnabled_Lambda([IsStaticMapping, IsGameplayTagStaticTarget]()
											{
												return IsStaticMapping() && IsGameplayTagStaticTarget();
											})
										.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
											{
												return SNew(STextBlock).Text(InItem.IsValid() ? FText::FromString(*InItem) : FText::GetEmpty());
											})
										.OnComboBoxOpening_Lambda([RefreshStaticGameplayTagOptions]()
											{
												RefreshStaticGameplayTagOptions();
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
											SNew(STextBlock).Text_Lambda([Mapping]()
												{
													if (!Mapping.IsValid() || Mapping->StaticValue.IsEmpty())
													{
														return NSLOCTEXT("YOLOInventory", "Dash_StaticGameplayTagHint", "Select gameplay tag");
													}
													return FText::FromString(Mapping->StaticValue);
												})
										]
								]
								+ SWidgetSwitcher::Slot()
								[
									SNew(SNumericEntryBox<double>)
										.IsEnabled_Lambda([IsStaticMapping, GetTargetNumeric]()
											{
												return IsStaticMapping() && GetTargetNumeric() != nullptr;
											})
										.Value_Lambda([ParseStaticNumber]()
											{
												return ParseStaticNumber();
											})
										.MinValue_Lambda([GetNumericClamp]()
											{
												return GetNumericClamp(TEXT("ClampMin"));
											})
										.MaxValue_Lambda([GetNumericClamp]()
											{
												return GetNumericClamp(TEXT("ClampMax"));
											})
										.MinSliderValue_Lambda([GetNumericClamp]()
											{
												return GetNumericClamp(TEXT("UIMin"));
											})
										.MaxSliderValue_Lambda([GetNumericClamp]()
											{
												return GetNumericClamp(TEXT("UIMax"));
											})
										.OnValueCommitted_Lambda([this, Mapping, GetTargetNumeric, RefreshMappingUi](double NewValue, ETextCommit::Type)
											{
												if (CurrentMappingSource.IsValid() && Mapping.IsValid())
												{
													CurrentMappingSource->Modify();
													if (const FNumericProperty* Num = GetTargetNumeric())
													{
														if (Num->IsInteger())
														{
															Mapping->StaticValue = LexToString((int64)FMath::RoundToInt64(NewValue));
														}
														else
														{
															Mapping->StaticValue = LexToString(NewValue);
														}
													}
													else
													{
														Mapping->StaticValue = LexToString(NewValue);
													}
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
								]
						]
				]
			+ SHorizontalBox::Slot().FillWidth(0.30f).Padding(2)
				[
					SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(0.36f).Padding(0, 0, 4, 0)
						[
							SNew(SComboBox<TSharedPtr<FString>>)
								.OptionsSource(TargetLayerOptions.Get())
								.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
									{
										return SNew(STextBlock).Text(InItem.IsValid() ? FText::FromString(*InItem) : FText::GetEmpty());
									})
								.OnComboBoxOpening_Lambda([RefreshTargetLayerOptions]()
									{
										RefreshTargetLayerOptions();
									})
								.OnSelectionChanged_Lambda([this, Mapping, GetSourceProp, GetTargetProp, RefreshTargetFragmentStructOptions, RefreshTargetFragmentFieldOptions, TargetFragmentStructOptions, TargetFragmentFieldOptions, PersistCurrentMapping, RefreshMappingUi](TSharedPtr<FString> NewItem, ESelectInfo::Type)
									{
										if (!CurrentMappingSource.IsValid() || !Mapping.IsValid() || !NewItem.IsValid())
										{
											return;
										}

										CurrentMappingSource->Modify();
										const bool bIdentity = (*NewItem == TEXT("Identity"));
										const bool bRuntime = (*NewItem == TEXT("Runtime Fragment"));
										if (bIdentity)
										{
											Mapping->TargetLayer = EYIFieldMappingTargetLayer::LegacyProperty;
											if (!IsIdentityMappingProperty(Mapping->TargetProperty))
											{
												Mapping->TargetProperty = GET_MEMBER_NAME_CHECKED(UYIItemDefinition, UniqueCode);
											}
											Mapping->TargetFragmentStruct = nullptr;
											Mapping->TargetFragmentField = NAME_None;
										}
										else
										{
											Mapping->TargetLayer = bRuntime
												? EYIFieldMappingTargetLayer::DynamicInstanceFragment
												: EYIFieldMappingTargetLayer::StaticDefinitionFragment;

											if (!Mapping->TargetFragmentStruct)
											{
												RefreshTargetFragmentStructOptions();
												if (TargetFragmentStructOptions->Num() > 0 && (*TargetFragmentStructOptions)[0].IsValid())
												{
													Mapping->TargetFragmentStruct = ResolveStructFromPathString(**(*TargetFragmentStructOptions)[0]);
												}
											}
											RefreshTargetFragmentFieldOptions();
											if (Mapping->TargetFragmentField.IsNone() && !Mapping->TargetProperty.IsNone())
											{
												Mapping->TargetFragmentField = Mapping->TargetProperty;
											}
											if (Mapping->TargetFragmentField.IsNone() && TargetFragmentFieldOptions->Num() > 0 && (*TargetFragmentFieldOptions)[0].IsValid())
											{
												Mapping->TargetFragmentField = FName(**(*TargetFragmentFieldOptions)[0]);
											}
											Mapping->bTargetPropertyBagField = false;
											Mapping->TargetPropertyBagFieldName = NAME_None;
											Mapping->TargetProperty = YIGetResolvedTargetFieldName(*Mapping);
										}

										if (Mapping->Conversion == EYIFieldMappingConversion::None)
										{
											const FProperty* SourceProp = GetSourceProp();
											const FProperty* TargetProp = GetTargetProp();
											Mapping->Conversion = GuessConversionForProps(SourceProp, TargetProp);
										}
										PersistCurrentMapping();
										RefreshMappingUi();
									})
								.Content()
								[
									SNew(STextBlock).Text_Lambda([Mapping, GetTargetLayerLabel]()
										{
											return Mapping.IsValid()
												? FText::FromString(GetTargetLayerLabel(Mapping->TargetLayer))
												: FText::FromString(TEXT("Fragment"));
										})
								]
						]
						+ SHorizontalBox::Slot().FillWidth(0.64f)
						[
							SNew(SWidgetSwitcher)
								.WidgetIndex_Lambda([Mapping]()
									{
										if (!Mapping.IsValid())
										{
											return 1;
										}
										return (Mapping->TargetLayer == EYIFieldMappingTargetLayer::LegacyProperty) ? 0 : 1;
									})
								+ SWidgetSwitcher::Slot()
								[
									SNew(SComboBox<TSharedPtr<FString>>)
										.OptionsSource(IdentityTargetOptions.Get())
										.OnComboBoxOpening_Lambda([RefreshIdentityTargetOptions]()
											{
												RefreshIdentityTargetOptions();
											})
										.OnGenerateWidget_Lambda([this, DropdownText, GetTypeInfo](TSharedPtr<FString> InItem)
											{
												const FName FieldName = InItem.IsValid() ? FName(**InItem) : NAME_None;
												FString Label;
												FLinearColor Color;
												FProperty* Prop = (FieldName != NAME_None)
													? FindPropertyByAuthoredNameEditor(UYIItemDefinition::StaticClass(), FieldName)
													: nullptr;
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
										.OnSelectionChanged_Lambda([this, Mapping, GetSourceProp, GetTargetProp, PersistCurrentMapping, RefreshMappingUi](TSharedPtr<FString> NewItem, ESelectInfo::Type)
											{
												if (CurrentMappingSource.IsValid() && Mapping.IsValid() && NewItem.IsValid())
												{
													CurrentMappingSource->Modify();
													Mapping->TargetLayer = EYIFieldMappingTargetLayer::LegacyProperty;
													Mapping->TargetProperty = FName(**NewItem);
													if (Mapping->Conversion == EYIFieldMappingConversion::None)
													{
														const FProperty* SourceProp = GetSourceProp();
														const FProperty* TargetProp = GetTargetProp();
														const EYIFieldMappingConversion Guess = GuessConversionForProps(SourceProp, TargetProp);
														Mapping->Conversion = Guess;
													}
													PersistCurrentMapping();
													RefreshMappingUi();
												}
											})
										.InitiallySelectedItem([IdentityTargetOptions, Mapping]()
											{
												if (!Mapping.IsValid()) return TSharedPtr<FString>();
												if (const TSharedPtr<FString>* FoundPtr = IdentityTargetOptions->FindByPredicate([Mapping](const TSharedPtr<FString>& Opt)
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
													SNew(STextBlock).Text_Lambda([DropdownText, Mapping, IdentityTargetOptions]()
														{
															const TSharedPtr<FString>* Found = IdentityTargetOptions->FindByPredicate([Mapping](const TSharedPtr<FString>& Opt)
																{
																	return Mapping.IsValid() && Opt.IsValid() && FName(**Opt).IsEqual(Mapping->TargetProperty);
																});
															return Found ? DropdownText(*Found) : FText::FromString(Mapping.IsValid() ? Mapping->TargetProperty.ToString() : TEXT(""));
														})
												]
										]
								]
								+ SWidgetSwitcher::Slot()
								[
									SNew(SHorizontalBox)
										+ SHorizontalBox::Slot().FillWidth(0.56f).Padding(0, 0, 4, 0)
										[
											SNew(SComboBox<TSharedPtr<FString>>)
												.OptionsSource(TargetFragmentStructOptions.Get())
												.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
													{
														const FString PathString = InItem.IsValid() ? *InItem : FString();
														return SNew(STextBlock)
															.Text(FText::FromString(MakeReadableFragmentNameFromPath(PathString)))
															.ToolTipText(PathString.IsEmpty() ? FText::GetEmpty() : FText::FromString(PathString));
													})
												.OnComboBoxOpening_Lambda([RefreshTargetFragmentStructOptions]()
													{
														RefreshTargetFragmentStructOptions();
													})
												.OnSelectionChanged_Lambda([this, Mapping, GetSourceProp, GetTargetProp, RefreshTargetFragmentFieldOptions, TargetFragmentFieldOptions, PersistCurrentMapping, RefreshMappingUi](TSharedPtr<FString> NewItem, ESelectInfo::Type)
													{
														if (!CurrentMappingSource.IsValid() || !Mapping.IsValid() || !NewItem.IsValid())
														{
															return;
														}

														CurrentMappingSource->Modify();
														if (Mapping->TargetLayer == EYIFieldMappingTargetLayer::LegacyProperty)
														{
															Mapping->TargetLayer = EYIFieldMappingTargetLayer::StaticDefinitionFragment;
														}
														Mapping->TargetFragmentStruct = ResolveStructFromPathString(**NewItem);
														Mapping->TargetFragmentField = NAME_None;
														Mapping->bTargetPropertyBagField = false;
														Mapping->TargetPropertyBagFieldName = NAME_None;
														RefreshTargetFragmentFieldOptions();
														if (TargetFragmentFieldOptions->Num() > 0 && (*TargetFragmentFieldOptions)[0].IsValid())
														{
															Mapping->TargetFragmentField = FName(**(*TargetFragmentFieldOptions)[0]);
														}
														Mapping->TargetProperty = YIGetResolvedTargetFieldName(*Mapping);
														if (Mapping->Conversion == EYIFieldMappingConversion::None)
														{
															const FProperty* SourceProp = GetSourceProp();
															const FProperty* TargetProp = GetTargetProp();
															Mapping->Conversion = GuessConversionForProps(SourceProp, TargetProp);
														}
														PersistCurrentMapping();
														RefreshMappingUi();
													})
												.Content()
												[
													SNew(STextBlock).Text_Lambda([Mapping]()
														{
															if (!Mapping.IsValid() || !Mapping->TargetFragmentStruct)
															{
																return NSLOCTEXT("YOLOInventory", "Dash_TargetFragmentStructHint", "Select fragment");
															}
															return FText::FromString(MakeReadableFragmentName(Mapping->TargetFragmentStruct.Get()));
														})
												]
										]
										+ SHorizontalBox::Slot().FillWidth(0.44f)
										[
											SNew(SComboBox<TSharedPtr<FString>>)
												.OptionsSource(TargetFragmentFieldOptions.Get())
												.OnGenerateWidget_Lambda([DropdownText](TSharedPtr<FString> InItem)
													{
														return SNew(STextBlock).Text(DropdownText(InItem));
													})
												.OnComboBoxOpening_Lambda([RefreshTargetFragmentFieldOptions]()
													{
														RefreshTargetFragmentFieldOptions();
													})
												.OnSelectionChanged_Lambda([this, Mapping, GetSourceProp, GetTargetProp, PersistCurrentMapping, RefreshMappingUi, InferPropertyBagTypeFromSourceProp](TSharedPtr<FString> NewItem, ESelectInfo::Type)
													{
														if (!CurrentMappingSource.IsValid() || !Mapping.IsValid() || !NewItem.IsValid())
														{
															return;
														}

														CurrentMappingSource->Modify();
														if (Mapping->TargetLayer == EYIFieldMappingTargetLayer::LegacyProperty)
														{
															Mapping->TargetLayer = EYIFieldMappingTargetLayer::StaticDefinitionFragment;
														}
														const FString SelectedField = **NewItem;
														FName BagFieldName = NAME_None;
														if (TryParsePropertyBagFieldPathEditor(SelectedField, BagFieldName))
														{
															Mapping->bTargetPropertyBagField = true;
															Mapping->TargetPropertyBagFieldName = BagFieldName;
															Mapping->TargetFragmentField = FName(TEXT("Properties"));
															if (const FProperty* SourceProp = GetSourceProp())
															{
																EPropertyBagPropertyType InferredType = Mapping->TargetPropertyBagFieldType;
																UObject* InferredTypeObject = nullptr;
																if (InferPropertyBagTypeFromSourceProp(SourceProp, InferredType, InferredTypeObject))
																{
																	Mapping->TargetPropertyBagFieldType = InferredType;
																	Mapping->TargetPropertyBagFieldTypeObject = InferredTypeObject;
																}
															}
														}
														else
														{
															Mapping->bTargetPropertyBagField = false;
															Mapping->TargetPropertyBagFieldName = NAME_None;
															Mapping->TargetFragmentField = FName(*SelectedField);
														}
														Mapping->TargetProperty = YIGetResolvedTargetFieldName(*Mapping);
														if (Mapping->Conversion == EYIFieldMappingConversion::None)
														{
															const FProperty* SourceProp = GetSourceProp();
															const FProperty* TargetProp = GetTargetProp();
															Mapping->Conversion = GuessConversionForProps(SourceProp, TargetProp);
														}
														PersistCurrentMapping();
														RefreshMappingUi();
													})
												.Content()
												[
													SNew(SHorizontalBox)
														+ SHorizontalBox::Slot().AutoWidth().Padding(2, 0)
														[
															MakeTypeBadgeDynamic(GetTargetProp)
														]
														+ SHorizontalBox::Slot().FillWidth(1.f).Padding(4, 0)
														[
															SNew(STextBlock).Text_Lambda([Mapping]()
																{
																	if (!Mapping.IsValid())
																	{
																		return FText::GetEmpty();
																	}
																	const FName ResolvedField = YIGetResolvedTargetFieldName(*Mapping);
																	return ResolvedField.IsNone()
																		? NSLOCTEXT("YOLOInventory", "Dash_TargetFragmentFieldHint", "Select field")
																		: FText::FromName(ResolvedField);
																})
														]
												]
										]
										+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0, 0, 0)
										[
											SNew(SWidgetSwitcher)
												.WidgetIndex_Lambda([IsCustomFragmentTarget]()
													{
														return IsCustomFragmentTarget() ? 1 : 0;
													})
												+ SWidgetSwitcher::Slot()
												[
													SNew(SSpacer)
												]
												+ SWidgetSwitcher::Slot()
												[
													SNew(SHorizontalBox)
														+ SHorizontalBox::Slot().FillWidth(0.42f).Padding(0, 0, 4, 0)
														[
															SNew(SEditableTextBox)
																.MinDesiredWidth(110.f)
																.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_PropertyBagFieldName_TT", "PropertyBag field name inside Custom Fragment.Properties."))
																.Text_Lambda([Mapping]()
																	{
																		return (Mapping.IsValid() && Mapping->bTargetPropertyBagField && !Mapping->TargetPropertyBagFieldName.IsNone())
																			? FText::FromName(Mapping->TargetPropertyBagFieldName)
																			: FText::GetEmpty();
																	})
																.HintText(NSLOCTEXT("YOLOInventory", "Dash_PropertyBagFieldNameHint", "Bag field"))
																.OnTextCommitted_Lambda([this, Mapping, PersistCurrentMapping, RefreshMappingUi](const FText& NewText, ETextCommit::Type)
																	{
																		if (!CurrentMappingSource.IsValid() || !Mapping.IsValid())
																		{
																			return;
																		}

																		CurrentMappingSource->Modify();
																		const FName Sanitized = FInstancedPropertyBag::SanitizePropertyName(NewText.ToString());
																		Mapping->TargetPropertyBagFieldName = Sanitized;
																		if (Mapping->bTargetPropertyBagField && !Sanitized.IsNone())
																		{
																			Mapping->TargetFragmentField = FName(TEXT("Properties"));
																			Mapping->TargetProperty = YIGetResolvedTargetFieldName(*Mapping);
																		}
																		PersistCurrentMapping();
																		RefreshMappingUi();
																	})
														]
														+ SHorizontalBox::Slot().FillWidth(0.30f).Padding(0, 0, 4, 0)
														[
															SNew(SComboBox<TSharedPtr<FString>>)
																.OptionsSource(PropertyBagTypePresetOptions.Get())
																.OnGenerateWidget_Lambda([DropdownText](TSharedPtr<FString> InItem)
																	{
																		return SNew(STextBlock).Text(DropdownText(InItem));
																	})
																.OnComboBoxOpening_Lambda([RefreshPropertyBagTypePresetOptions]()
																	{
																		RefreshPropertyBagTypePresetOptions();
																	})
																.OnSelectionChanged_Lambda([this, Mapping, PersistCurrentMapping, RefreshMappingUi, ApplyPropertyBagPresetByLabel](TSharedPtr<FString> NewItem, ESelectInfo::Type)
																	{
																		if (!CurrentMappingSource.IsValid() || !Mapping.IsValid() || !NewItem.IsValid())
																		{
																			return;
																		}
																		CurrentMappingSource->Modify();
																		ApplyPropertyBagPresetByLabel(*NewItem, *Mapping);
																		PersistCurrentMapping();
																		RefreshMappingUi();
																	})
																.Content()
																[
																	SNew(STextBlock).Text_Lambda([GetPropertyBagPresetLabel]()
																		{
																			return FText::FromString(GetPropertyBagPresetLabel());
																		})
																]
														]
														+ SHorizontalBox::Slot().AutoWidth()
														[
															SNew(SButton)
																.ToolTipText(NSLOCTEXT("YOLOInventory", "Dash_PropertyBagBind_TT", "Bind/create a PropertyBag field target on the selected custom fragment."))
																.Text(NSLOCTEXT("YOLOInventory", "Dash_PropertyBagBind", "+Bag"))
																.OnClicked_Lambda([this, Mapping, GetSourceProp, GetTargetProp, PersistCurrentMapping, RefreshMappingUi, RefreshTargetFragmentFieldOptions, InferPropertyBagTypeFromSourceProp]()
																	{
																		if (!CurrentMappingSource.IsValid() || !Mapping.IsValid())
																		{
																			return FReply::Handled();
																		}

																		CurrentMappingSource->Modify();
																		Mapping->bTargetPropertyBagField = true;
																		Mapping->TargetFragmentField = FName(TEXT("Properties"));

																		if (Mapping->TargetPropertyBagFieldName.IsNone())
																		{
																			FName WantedName = !Mapping->SourceField.IsNone() ? Mapping->SourceField : FName(TEXT("Field"));
																			Mapping->TargetPropertyBagFieldName = FInstancedPropertyBag::SanitizePropertyName(WantedName);
																		}

																		if (const FProperty* SourceProp = GetSourceProp())
																		{
																			EPropertyBagPropertyType InferredType = Mapping->TargetPropertyBagFieldType;
																			UObject* InferredTypeObject = nullptr;
																			if (InferPropertyBagTypeFromSourceProp(SourceProp, InferredType, InferredTypeObject))
																			{
																				Mapping->TargetPropertyBagFieldType = InferredType;
																				Mapping->TargetPropertyBagFieldTypeObject = InferredTypeObject;
																			}
																		}

																		Mapping->TargetProperty = YIGetResolvedTargetFieldName(*Mapping);
																		if (Mapping->Conversion == EYIFieldMappingConversion::None)
																		{
																			const FProperty* SourceProp = GetSourceProp();
																			const FProperty* TargetProp = GetTargetProp();
																			Mapping->Conversion = GuessConversionForProps(SourceProp, TargetProp);
																		}

																		RefreshTargetFragmentFieldOptions();
																		PersistCurrentMapping();
																		RefreshMappingUi();
																		return FReply::Handled();
																	})
														]
												]
										]
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
									else if (*NewItem == TEXT("To Soft Object")) NewConv = EYIFieldMappingConversion::ToSoftObject;
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
								ConverterOptions.Add(MakeShared<FString>(TEXT("To Soft Object")));
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
								case EYIFieldMappingConversion::ToSoftObject: Label = TEXT("To Soft Object"); break;
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
										case EYIFieldMappingConversion::ToSoftObject: Label = TEXT("To Soft Object"); break;
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
				const FText DefDescription = YIItemSchema::GetDescription(Def);
				if (!DefDescription.IsEmpty())
				{
					Summary += FString::Printf(TEXT("\n\n%s"), *DefDescription.ToString());
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
