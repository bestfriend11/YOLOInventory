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
class UYIAffixAsset;

USTRUCT(BlueprintType)
struct YOLOINVENTORYLOOT_API FYIAffixRollCriteria
{
	GENERATED_BODY()

	/** Master switch for this criteria block. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Criteria")
	bool bEnabled = false;

	/** Use item level as baseline for modifier power windows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Criteria", meta=(EditCondition="bEnabled", EditConditionHides))
	bool bUseItemLevelAsPowerBaseline = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Criteria", meta=(EditCondition="bEnabled", ClampMin="0", EditConditionHides))
	int32 MinTier = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Criteria", meta=(EditCondition="bEnabled", ClampMin="0", EditConditionHides))
	int32 MaxTier = 9999;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Criteria", meta=(EditCondition="bEnabled && !bUseItemLevelAsPowerBaseline", ClampMin="0", EditConditionHides))
	int32 MinPowerLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Criteria", meta=(EditCondition="bEnabled && !bUseItemLevelAsPowerBaseline", ClampMin="0", EditConditionHides))
	int32 MaxPowerLevel = 9999;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Criteria", meta=(EditCondition="bEnabled && bUseItemLevelAsPowerBaseline", EditConditionHides))
	int32 MinPowerLevelOffset = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Criteria", meta=(EditCondition="bEnabled && bUseItemLevelAsPowerBaseline", EditConditionHides))
	int32 MaxPowerLevelOffset = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Criteria", meta=(EditCondition="bEnabled", EditConditionHides))
	EYIAffixQuality MinQuality = EYIAffixQuality::Common;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Criteria", meta=(EditCondition="bEnabled", EditConditionHides))
	EYIAffixQuality MaxQuality = EYIAffixQuality::Unique;

	/** If set, only these template ids are accepted (case-insensitive exact match). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Criteria", meta=(EditCondition="bEnabled", EditConditionHides))
	TArray<FString> AllowedTemplateIds;

	/** These template ids are always rejected (case-insensitive exact match). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Criteria", meta=(EditCondition="bEnabled", EditConditionHides))
	TArray<FString> BlockedTemplateIds;

	/** Reject affixes whose conflict group appears here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Criteria", meta=(EditCondition="bEnabled", EditConditionHides))
	TArray<FName> BlockedConflictGroups;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYLOOT_API FYIItemGenerationResult
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
class YOLOINVENTORYLOOT_API UYIItemGenerator : public UDataAsset
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

	/** Prefix roll filter (criteria on affix tier/power/etc). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix Criteria")
	FYIAffixRollCriteria PrefixCriteria;

	/** Suffix roll filter (criteria on affix tier/power/etc). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix Criteria")
	FYIAffixRollCriteria SuffixCriteria;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Generator")
	bool GenerateItem(int32 Level, int32 Seed, FYIBagItem& OutItem, FGameplayTag& OutRarity, int32& OutPrefixes, int32& OutSuffixes) const;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Generator")
	FYIItemGenerationResult GenerateItemResult(int32 Level, int32 Seed) const;

private:
	static int32 RollAffixesFromPool(UYIAffixPoolAsset* Pool, const UYIItemDefinition* ItemDef, FYIBagItem& Item, int32 Count, int32 Level, FRandomStream& RNG, const FYIAffixRollCriteria& Criteria, EYIAffixKind ExpectedKind);
	static TSoftObjectPtr<UYIAffixPoolAsset> ResolvePoolOverride(const TSoftObjectPtr<UYIAffixPoolAsset>& RuleOverride, const TSoftObjectPtr<UYIAffixPoolAsset>& GeneratorOverride, const TSoftObjectPtr<UYIAffixPoolAsset>& DefinitionPool, bool bUseDefinitionPools);
	static bool DoesAffixPassCriteria(const UYIAffixAsset* Affix, const UYIItemDefinition* ItemDef, int32 ItemLevel, const FYIAffixRollCriteria& Criteria, EYIAffixKind ExpectedKind);
};
