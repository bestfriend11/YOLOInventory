#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CSVDataTransformer.h"
#include "UObject/SoftObjectPtr.h"
#include "Data/YIDataTableItemSource.h"
#include "YIDataTableAffixSource.generated.h"

class UDataTable;
class UCSVDataTransformer;

/**
 * Describes a data table whose rows can be transformed into affix assets.
 * Designers can use a Blueprint transformer or inline field mappings.
 */
UCLASS(BlueprintType)
class YOLOINVENTORY_API UYIDataTableAffixSource : public UDataAsset
{
	GENERATED_BODY()
public:
	/** Load the data table (synchronous). Returns nullptr if missing. */
	UFUNCTION(BlueprintCallable, Category="AffixSource")
	UDataTable* ResolveDataTable() const;

	/** Validate required fields. OutError contains a user-facing reason on failure. */
	UFUNCTION(BlueprintCallable, Category="AffixSource")
	bool ValidateSource(FString& OutError) const;

	/** Get all row names, sorted for determinism (empty if table missing). */
	UFUNCTION(BlueprintCallable, Category="AffixSource")
	TArray<FName> GetRowNames() const;

	/** The effective transformer class, preferring inline mappings if enabled. */
	UFUNCTION(BlueprintCallable, Category="AffixSource")
	TSubclassOf<UCSVDataTransformer> GetEffectiveTransformerClass() const;

	/** Data table containing affix rows. Must include a numeric field used as the UniqueCode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AffixSource")
	TSoftObjectPtr<UDataTable> DataTable;

	/** Blueprint transformer that maps a row (URowData) to a UYIAffixAsset. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AffixSource")
	TSubclassOf<UCSVDataTransformer> TransformerClass;

	/** If true, the dashboard will warn/skip rows when no transformer (inline or Blueprint) is available. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AffixSource", meta=(ToolTip="If true, the dashboard will warn/skip rows when no transformer (inline or Blueprint) is available."))
	bool bRequireTransformer = true;

	/** Enable simple inline field mappings instead of (or alongside) a transformer Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inline Mapping")
	bool bUseInlineMappings = false;

	/** How to combine inline mappings and transformer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inline Mapping")
	EYITransformMode TransformMode = EYITransformMode::InlineOnly;

	/** Mappings from data table columns to affix asset properties (type-compatible copies). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inline Mapping")
	TArray<FYIFieldMapping> InlineMappings;

	/** Name of the field in the row struct that holds the UniqueCode (int32/int64). Defaults to 'UniqueCode'. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AffixSource")
	FName UniqueCodeFieldName = TEXT("UniqueCode");

	/** Optional field for asset naming. If unset, defaults to a synthetic name from the row name / UniqueCode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AffixSource")
	FName AssetNameFieldName = TEXT("AssetName");

	/** Optional field for package path (e.g., /Game/Affixes). If unset, defaults to /Game/YOLOInventory/Affixes/Generated. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AffixSource")
	FName PackagePathFieldName = TEXT("PackagePath");

	/** Optional preview name field to show in dashboards without instantiating the transformer. Defaults to DisplayName. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AffixSource")
	FName PreviewNameFieldName = TEXT("DisplayName");

	/** Optional preview description field used for dashboard tooltips. Defaults to Description. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="AffixSource")
	FName PreviewDescriptionFieldName = TEXT("Description");
};
