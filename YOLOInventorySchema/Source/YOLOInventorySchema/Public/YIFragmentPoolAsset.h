#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "YIFragmentPoolAsset.generated.h"

class UYIFragmentAsset;

USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIFragmentPoolEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pool")
	TSoftObjectPtr<UYIFragmentAsset> FragmentAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pool", meta=(ClampMin="0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pool", meta=(ClampMin="0"))
	int32 MinLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pool", meta=(ClampMin="0"))
	int32 MaxLevel = 999999;

	/** Optional requirements; strategy decides how to evaluate these tags. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pool")
	FGameplayTagContainer RequiredTags;
};

/**
 * Generic pool of fragment assets for runtime roll strategies.
 * This class is intentionally neutral and not tied to affix-specific semantics.
 */
UCLASS(BlueprintType)
class YOLOINVENTORYSCHEMA_API UYIFragmentPoolAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
	int64 UniqueCode = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
	FString TemplateId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pool")
	TArray<FYIFragmentPoolEntry> Entries;

	/** Weighted roll of one entry by level. Returns false when no valid candidate exists. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Fragments")
	bool RollEntry(int32 Level, int32 Seed, TSoftObjectPtr<UYIFragmentAsset>& OutFragmentAsset) const;
};

