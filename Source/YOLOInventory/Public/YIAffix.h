#pragma once

#include "CoreMinimal.h"
#include "YIAffixAsset.h"
#include "YIAffix.generated.h"

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIAffixInstance
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
    TSoftObjectPtr<UYIAffixAsset> Source;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
    int32 TierRolled = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
    float RolledValue = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
    int32 Seed = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
    FName ConflictGroupCache;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
    FText DisplayNameCache;
};
