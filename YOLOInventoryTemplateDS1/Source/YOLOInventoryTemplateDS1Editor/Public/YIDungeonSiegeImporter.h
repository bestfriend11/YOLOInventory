#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YIDungeonSiegeImporter.generated.h"

USTRUCT(BlueprintType)
struct YOLOINVENTORYTEMPLATEDS1EDITOR_API FYIDungeonSiegeImportOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DungeonSiege")
	FString GasFilePath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DungeonSiege")
	FString OutputPath = TEXT("/Game/YOLOInventory/DS1/Imported");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DungeonSiege")
	bool bCreateItemDefinitions = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DungeonSiege")
	bool bCreateLootTable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DungeonSiege")
	bool bCreateRarityProfile = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DungeonSiege")
	bool bCreateItemGenerator = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DungeonSiege")
	FName LootTableAssetName = TEXT("DS1_LootTable");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DungeonSiege")
	FName RarityProfileAssetName = TEXT("DS1_RarityProfile");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DungeonSiege")
	FName ItemGeneratorAssetName = TEXT("DS1_ItemGenerator");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DungeonSiege")
	bool bOverwriteExisting = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="DungeonSiege", meta=(ClampMin="0"))
	int32 MaxAssetsToCreate = 0;
};

UCLASS()
class YOLOINVENTORYTEMPLATEDS1EDITOR_API UYIDungeonSiegeImporter : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// Parses template names from a DS1 .gas file and creates item definition assets.
	UFUNCTION(CallInEditor, BlueprintCallable, Category="YOLOInventory|DungeonSiege")
	static int32 ImportTemplatesFromGas(const FYIDungeonSiegeImportOptions& Options);
};
