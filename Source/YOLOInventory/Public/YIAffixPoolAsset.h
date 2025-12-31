#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "YIAffixAsset.h"
#include "YIAffixPoolAsset.generated.h"

USTRUCT(BlueprintType)
struct FYIAffixPoolEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AffixPool")
    TSoftObjectPtr<UYIAffixAsset> Affix;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AffixPool", meta=(ClampMin="0.0"))
    float Weight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AffixPool", meta=(ClampMin="0"))
    int32 MinLevel = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AffixPool", meta=(ClampMin="0"))
    int32 MaxLevel = 9999;

    // Optional: require a minimum affix quality
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AffixPool")
    EYIAffixQuality MinQuality = EYIAffixQuality::Common;
};

UCLASS(BlueprintType)
class YOLOINVENTORY_API UYIAffixPoolAsset : public UObject
{
    GENERATED_BODY()
public:
    // List of candidate affixes
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AffixPool")
    TArray<FYIAffixPoolEntry> Entries;

    // Sample a single affix entry by weight for the given level; returns nullptr if none found
    UFUNCTION(BlueprintCallable, Category="AffixPool")
    UYIAffixAsset* SampleAffix(FRandomStream& RNG, int32 Level) const;
};