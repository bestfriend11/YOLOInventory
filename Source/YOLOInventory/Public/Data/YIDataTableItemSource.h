#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CSVDataTransformer.h"
#include "YIDataTableItemSource.generated.h"

UENUM(BlueprintType)
enum class EYIFieldMappingConversion : uint8
{
	None        UMETA(DisplayName="None"),
	ToName      UMETA(DisplayName="To Name"),
	ToText      UMETA(DisplayName="To Text"),
	ToInt       UMETA(DisplayName="To Int"),
	ToFloat     UMETA(DisplayName="To Float"),
	BoolFromInt UMETA(DisplayName="Bool from Int>0"),
	BoolFromText UMETA(DisplayName="Bool from Text (non-empty)")
};

UENUM(BlueprintType)
enum class EYITransformMode : uint8
{
	InlineOnly              UMETA(DisplayName="Inline Mappings Only"),
	TransformerOnly         UMETA(DisplayName="Transformer Only"),
	HybridInlineThenTransformer UMETA(DisplayName="Inline then Transformer")
};

USTRUCT(BlueprintType)
struct FYIFieldMapping
{
	GENERATED_BODY()

	/** Column name in the data table row (authored name). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mapping")
	FName SourceField;

	/** Property name on the target item definition to write into. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mapping")
	FName TargetProperty;

	/** Optional conversion applied before assigning to the target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mapping")
	EYIFieldMappingConversion Conversion = EYIFieldMappingConversion::None;
};

class UDataTable;
class UCSVDataTransformer;

/**
 * Describes a data table whose rows can be transformed into runtime item definitions.
 * Designers can use a Blueprint transformer or simple inline field mappings.
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

	/** The effective transformer class, preferring inline mappings if enabled. */
	UFUNCTION(BlueprintCallable, Category="ItemSource")
	TSubclassOf<UCSVDataTransformer> GetEffectiveTransformerClass() const;

	/** Data table containing item rows. Must include a numeric field used as the UniqueCode. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ItemSource")
	TSoftObjectPtr<UDataTable> DataTable;

	/** Blueprint transformer that maps a row (URowData) to a UYIItemDefinition. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ItemSource")
	TSubclassOf<UCSVDataTransformer> TransformerClass;

	/** If true, the registry/dashboard will warn/skip rows when no transformer (inline or Blueprint) is available. Leave true to avoid silent missing items. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ItemSource", meta=(ToolTip="If true, the registry/dashboard will warn/skip rows when no transformer (inline or Blueprint) is available. Leave true to avoid silent missing items."))
	bool bRequireTransformer = true;

	/** Enable simple inline field mappings instead of (or alongside) a transformer Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inline Mapping")
	bool bUseInlineMappings = false;

	/** How to combine inline mappings and transformer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inline Mapping")
	EYITransformMode TransformMode = EYITransformMode::InlineOnly;

	/** Mappings from data table columns to item definition properties (type-compatible copies). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inline Mapping")
	TArray<FYIFieldMapping> InlineMappings;

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
