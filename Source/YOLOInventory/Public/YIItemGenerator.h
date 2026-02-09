#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "YIInventoryBag.h"
#include "YIItemGenerator.generated.h"

class UYILootTable;
class UYIRarityProfile;
class UYIAffixPoolAsset;
class UYIItemDefinition;

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIItemGenerationResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generator")
	bool bSuccess = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generator")
	FYIBagItem Item;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generator")
	FGameplayTag RarityTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generator")
	int32 NumPrefixes = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generator")
	int32 NumSuffixes = 0;
};

UCLASS(BlueprintType)
class YOLOINVENTORY_API UYIItemGenerator : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generator")
	TSoftObjectPtr<UYILootTable> LootTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generator")
	TSoftObjectPtr<UYIRarityProfile> RarityProfile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generator")
	bool bApplyTemplateAffixes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generator")
	bool bGenerateRandomAffixes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generator", meta=(ToolTip="If true, use item definition pools when no overrides are provided"))
	bool bUseDefinitionAffixPools = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generator")
	TSoftObjectPtr<UYIAffixPoolAsset> PrefixPoolOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generator")
	TSoftObjectPtr<UYIAffixPoolAsset> SuffixPoolOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generator", meta=(ClampMin="1"))
	int32 MinItemLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generator", meta=(ClampMin="1"))
	int32 MaxItemLevel = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generator")
	bool bClampLevel = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Generator", meta=(ToolTip="Fallback rarity tag when no profile is used"))
	FGameplayTag DefaultRarityTag;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Generator")
	bool GenerateItem(int32 Level, int32 Seed, FYIBagItem& OutItem, FGameplayTag& OutRarity, int32& OutPrefixes, int32& OutSuffixes) const;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Generator")
	FYIItemGenerationResult GenerateItemResult(int32 Level, int32 Seed) const;

private:
	static int32 RollAffixesFromPool(UYIAffixPoolAsset* Pool, FYIBagItem& Item, int32 Count, int32 Level, FRandomStream& RNG);
	static TSoftObjectPtr<UYIAffixPoolAsset> ResolvePoolOverride(const TSoftObjectPtr<UYIAffixPoolAsset>& RuleOverride, const TSoftObjectPtr<UYIAffixPoolAsset>& GeneratorOverride, const TSoftObjectPtr<UYIAffixPoolAsset>& DefinitionPool, bool bUseDefinitionPools);
};
