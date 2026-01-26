#include "YIItemRegistrySubsystem.h"
#include "YIItemDefinition.h"
#include "Data/YIDataTableItemSource.h"
#include "CSVDataTransformer.h"
#include "RowData.h"
#include "Engine/DataTable.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

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

UYIItemDefinition* UYIItemRegistrySubsystem::TransformRow(FName RowName, const UDataTable* DataTable, TSubclassOf<UCSVDataTransformer> TransformerClass, bool bCacheResult, int64 Code)
{
	if (!DataTable)
	{
		return nullptr;
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

	if (!Source->TransformerClass && Source->bRequireTransformer)
	{
		UE_LOG(LogYIItemRegistry, Warning, TEXT("GetByCode(%lld) failed: TransformerClass is required but not set on %s."), (long long)Code, *Source->GetPathName());
		return nullptr;
	}

	UYIItemDefinition* Def = TransformRow(Entry->RowName, Table, Source->TransformerClass, Source->bCacheGeneratedItems, Code);
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
