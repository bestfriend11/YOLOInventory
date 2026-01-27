#include "Data/YIDataTableItemSource.h"
#include "Engine/DataTable.h"
#include "UObject/UnrealType.h"

UDataTable* UYIDataTableItemSource::ResolveDataTable() const
{
	return DataTable.LoadSynchronous();
}

bool UYIDataTableItemSource::ValidateSource(FString& OutError) const
{
	OutError.Reset();

	const UDataTable* Table = DataTable.Get();
	if (!Table)
	{
		OutError = TEXT("DataTable is not set.");
		return false;
	}

	if (!Table->RowStruct)
	{
		OutError = FString::Printf(TEXT("DataTable %s is missing RowStruct."), *Table->GetName());
		return false;
	}

	const bool bHasInline = bUseInlineMappings && InlineMappings.Num() > 0;

	if (!TransformerClass && !bHasInline)
	{
		OutError = TEXT("TransformerClass is not set.");
		return !bRequireTransformer;
	}

	const FName CodeField = UniqueCodeFieldName.IsNone() ? FName(TEXT("UniqueCode")) : UniqueCodeFieldName;
	bool bHasCodeField = false;
	for (TFieldIterator<FProperty> It(Table->RowStruct); It; ++It)
	{
		if ((*It)->GetFName() == CodeField)
		{
			bHasCodeField = true;
			break;
		}
	}

	if (!bHasCodeField)
	{
		OutError = FString::Printf(TEXT("RowStruct %s does not contain field '%s'."), *Table->RowStruct->GetName(), *CodeField.ToString());
		return false;
	}

	return true;
}

TArray<FName> UYIDataTableItemSource::GetRowNames() const
{
	TArray<FName> Out;
	if (const UDataTable* Table = DataTable.Get())
	{
		Table->GetRowMap().GetKeys(Out);
		Out.Sort(FNameLexicalLess());
	}
	return Out;
}

TSubclassOf<UCSVDataTransformer> UYIDataTableItemSource::GetEffectiveTransformerClass() const
{
	const bool bInlineActive = (TransformMode != EYITransformMode::TransformerOnly) && bUseInlineMappings && InlineMappings.Num() > 0;
	if (bInlineActive)
	{
		// Inline mappings handled directly by the registry; no transformer class needed.
		return nullptr;
	}
	return TransformerClass;
}
