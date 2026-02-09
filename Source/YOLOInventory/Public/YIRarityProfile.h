#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "YIRarityProfile.generated.h"

class UYIAffixPoolAsset;

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIRarityRule
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rarity")
	FGameplayTag RarityTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rarity", meta=(ClampMin="0.0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rarity", meta=(ClampMin="0"))
	int32 MinLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rarity", meta=(ClampMin="0"))
	int32 MaxLevel = 9999;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affixes", meta=(ClampMin="0"))
	int32 MinPrefixes = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affixes", meta=(ClampMin="0"))
	int32 MaxPrefixes = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affixes", meta=(ClampMin="0"))
	int32 MinSuffixes = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affixes", meta=(ClampMin="0"))
	int32 MaxSuffixes = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affixes", meta=(ToolTip="Optional prefix pool override for this rarity tier"))
	TSoftObjectPtr<UYIAffixPoolAsset> PrefixPoolOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affixes", meta=(ToolTip="Optional suffix pool override for this rarity tier"))
	TSoftObjectPtr<UYIAffixPoolAsset> SuffixPoolOverride;
};

UCLASS(BlueprintType)
class YOLOINVENTORY_API UYIRarityProfile : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rarity")
	TArray<FYIRarityRule> Rules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rarity", meta=(ToolTip="Fallback rarity tag when no rule is eligible"))
	FGameplayTag DefaultRarityTag;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Rarity")
	bool RollRarity(int32 Level, int32 Seed, FYIRarityRule& OutRule) const;
};
