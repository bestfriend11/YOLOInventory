#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CSVDataTransformer.h"
#include "YIDataTableItemSource.generated.h"

class UDataTable;
class UCSVDataTransformer;

/**
 * Describes a data table whose rows can be transformed into runtime item definitions.
 * Designers author a Blueprint transformer (UCSVDataTransformer) that converts a row into a UYIItemDefinition.
 */
UCLASS(BlueprintType)
class YOLOINVENTORY_API UYIDataTableItemSource : public UDataAsset
{
	GENERATED_BODY()
public:
	/** Load the data table (synchronous). Returns nullptr if missing. */
	UFUNCTION(BlueprintCallable, Category="ItemSource")
	UDataTable* ResolveDataTable() const;

	/** Validate required fields. OutError contains a user-facing reason on failure. */
	UFUNCTION(BlueprintCallable, Category="ItemSource")
	bool ValidateSource(FString& OutError) const;

	/** Get all row names, sorted for determinism (empty if table missing). */
	UFUNCTION(BlueprintCallable, Category="ItemSource")
	TArray<FName> GetRowNames() const;

	/** Data table containing item rows. Must include a numeric field used as the UniqueCode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ItemSource")
	TSoftObjectPtr<UDataTable> DataTable;

	/** Blueprint transformer that maps a row (URowData) to a UYIItemDefinition. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ItemSource")
	TSubclassOf<UCSVDataTransformer> TransformerClass;

	/** If true, registry will log and ignore rows when the transformer is missing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ItemSource")
	bool bRequireTransformer = true;

	/** Name of the field in the row struct that holds the UniqueCode (int32/int64). Defaults to 'UniqueCode'. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ItemSource")
	FName UniqueCodeFieldName = TEXT("UniqueCode");

	/** Optional string field for TemplateId; used for logging/introspection only. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ItemSource")
	FName TemplateIdFieldName = TEXT("TemplateId");

	/** Optional field for asset naming. If unset, defaults to a synthetic name from the row name / UniqueCode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ItemSource")
	FName AssetNameFieldName = TEXT("AssetName");

	/** Optional field for package path (e.g., /Game/Items). If unset, defaults to /Game/YOLOInventory/Generated. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ItemSource")
	FName PackagePathFieldName = TEXT("PackagePath");

	/** Optional preview name field to show in dashboards without instantiating the transformer. Defaults to DisplayName. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ItemSource")
	FName PreviewNameFieldName = TEXT("DisplayName");

	/** Optional preview description field used for dashboard tooltips. Defaults to Description. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ItemSource")
	FName PreviewDescriptionFieldName = TEXT("Description");

	/** Cache transformed item definitions for reuse at runtime. Disable if you want a fresh transform each request. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ItemSource")
	bool bCacheGeneratedItems = true;
};
