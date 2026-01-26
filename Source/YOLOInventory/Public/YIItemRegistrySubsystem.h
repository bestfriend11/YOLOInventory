#pragma once
#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "UObject/SoftObjectPtr.h"
#include "YIItemRegistrySubsystem.generated.h"

class UDataTable;
class UCSVDataTransformer;
class UYIItemDefinition;
class UYIDataTableItemSource;

USTRUCT()
struct FYIItemRegistryEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftObjectPtr<UYIItemDefinition> Asset;

	UPROPERTY()
	TSoftObjectPtr<UYIDataTableItemSource> DataTableSource;

	UPROPERTY()
	FName RowName = NAME_None;

	bool IsDataTable() const { return DataTableSource.ToSoftObjectPath().IsValid(); }
};

USTRUCT(BlueprintType)
struct FYIItemRegistryView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="YOLOInventory|Registry")
	int64 UniqueCode = 0;

	UPROPERTY(BlueprintReadOnly, Category="YOLOInventory|Registry")
	FString TemplateId;

	UPROPERTY(BlueprintReadOnly, Category="YOLOInventory|Registry")
	FString SourcePath;

	UPROPERTY(BlueprintReadOnly, Category="YOLOInventory|Registry")
	bool bIsDataTable = false;

	UPROPERTY(BlueprintReadOnly, Category="YOLOInventory|Registry")
	FName RowName = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category="YOLOInventory|Registry")
	TSoftObjectPtr<UObject> Object;

	UPROPERTY(BlueprintReadOnly, Category="YOLOInventory|Registry")
	TSoftObjectPtr<UYIDataTableItemSource> DataSource;
};

UCLASS()
class YOLOINVENTORY_API UYIItemRegistrySubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Build or rebuild the index by scanning asset registry for UYIItemDefinition and UYIDataTableItemSource
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Registry")
	void BuildIndex(bool bForce = false);

	// Find an item definition by its UniqueCode. Returns nullptr if not found.
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Registry")
	UYIItemDefinition* GetByCode(int64 Code);

	// Ensures codes are unique. If bAutoFix, assigns new random codes for duplicates or zero codes (editor-only recommended)
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Registry")
	bool EnsureUniqueCodes(bool bAutoFix);

	// Get all known items (assets + data table rows) for tooling/dashboard
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Registry")
	void GetAllItems(TArray<FYIItemRegistryView>& OutItems, bool bForceRebuild = false);

private:
	int64 ExtractCodeFromRow(const UScriptStruct* Struct, const uint8* RowData, FName FieldName) const;
	FString ExtractTemplateIdFromRow(const UScriptStruct* Struct, const uint8* RowData, FName FieldName) const;
	UYIItemDefinition* TransformRow(FName RowName, const UDataTable* DataTable, TSubclassOf<UCSVDataTransformer> TransformerClass, bool bCacheResult, int64 Code);

	UPROPERTY()
	TMap<int64, TObjectPtr<UYIItemDefinition>> CachedGeneratedDefinitions;

	UPROPERTY()
	TMap<int64, FYIItemRegistryEntry> CodeToEntry;

	bool bIndexed = false;
};
