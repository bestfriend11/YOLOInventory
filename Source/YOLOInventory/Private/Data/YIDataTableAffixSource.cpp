#include "Data/YIDataTableAffixSource.h"
#include "Engine/DataTable.h"

UDataTable* UYIDataTableAffixSource::ResolveDataTable() const
{
	return DataTable.IsValid() ? DataTable.Get() : DataTable.LoadSynchronous();
}

bool UYIDataTableAffixSource::ValidateSource(FString& OutError) const
{
	UDataTable* Table = ResolveDataTable();
	if (!Table)
	{
		OutError = TEXT("Data table is missing.");
		return false;
	}
	if (!Table->RowStruct)
	{
		OutError = TEXT("Data table row struct is missing.");
		return false;
	}
	if (UniqueCodeFieldName.IsNone())
	{
		OutError = TEXT("UniqueCodeFieldName is not set.");
		return false;
	}
	return true;
}

TArray<FName> UYIDataTableAffixSource::GetRowNames() const
{
	TArray<FName> Names;
	if (UDataTable* Table = ResolveDataTable())
	{
		Names = Table->GetRowNames();
		Names.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
	}
	return Names;
}

TSubclassOf<UCSVDataTransformer> UYIDataTableAffixSource::GetEffectiveTransformerClass() const
{
	return TransformerClass;
}
