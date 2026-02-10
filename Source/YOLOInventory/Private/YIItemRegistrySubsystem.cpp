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
		if (Mapping.SourceField.IsNone() || Mapping.TargetProperty.IsNone())
		{
			continue;
		}

		FProperty* SourceProp = nullptr;
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

		const uint8* SrcPtr = SourceProp->ContainerPtrToValuePtr<uint8>(RowPtr);
		uint8* DestPtr = DestProp->ContainerPtrToValuePtr<uint8>(Def);
		if (CopyValueBetweenProperties(SourceProp, SrcPtr, DestProp, DestPtr, Mapping.Conversion))
		{
			ApplyTransformFunction(Mapping, DestProp, DestPtr);
		}
	}

	OutDef = Def;
	return true;
}

UYIItemDefinition* UYIItemRegistrySubsystem::TransformRow(FName RowName, const UDataTable* DataTable, TSubclassOf<UCSVDataTransformer> TransformerClass, bool bCacheResult, int64 Code, const UYIDataTableItemSource* Source)
{
	if (!DataTable)
	{
		return nullptr;
	}

	UYIItemDefinition* CachedResult = nullptr;

	// Inline mapping path (editor-friendly, no blueprint needed)
	const bool bInlineFirst = Source && Source->TransformMode != EYITransformMode::TransformerOnly;
	if (bInlineFirst && ApplyInlineMappings(Source, DataTable, RowName, CachedResult))
	{
		if (bCacheResult)
		{
			CachedGeneratedDefinitions.FindOrAdd(Code) = CachedResult;
		}
		return CachedResult;
	}

	if (!TransformerClass)
	{
		UE_LOG(LogYIItemRegistry, Warning, TEXT("TransformRow skipped for code %lld (row %s): TransformerClass is null."), (long long)Code, *RowName.ToString());
		return nullptr;
	}

	if (bCacheResult)
	{
		if (TObjectPtr<UYIItemDefinition>* Cached = CachedGeneratedDefinitions.Find(Code))
		{
			return Cached->Get();
		}
	}

	const uint8* const* FoundRow = DataTable->GetRowMap().Find(RowName);
	const uint8* RowPtr = FoundRow ? *FoundRow : nullptr;
	if (!RowPtr)
	{
		return nullptr;
	}

	URowData* RowWrapper = NewObject<URowData>(this);
	RowWrapper->Address = const_cast<uint8*>(RowPtr);
	RowWrapper->Struct = DataTable->RowStruct;

	UCSVDataTransformer* Transformer = NewObject<UCSVDataTransformer>(this, TransformerClass);
	UObject* Result = Transformer ? Transformer->TransformObject(RowWrapper) : nullptr;
	UYIItemDefinition* Definition = Cast<UYIItemDefinition>(Result);

	if (Definition && bCacheResult)
	{
		CachedGeneratedDefinitions.FindOrAdd(Code) = Definition;
	}

	// Hybrid: transformer may further adjust the definition after inline applied; inline already returned early.
	return Definition;
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
