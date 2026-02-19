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

    /** Cached source code for deterministic save/replication and anti-tamper validation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
    int64 SourceCode = 0;

    /** Cached source template id for diagnostics and content debugging. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
    FString SourceTemplateId;

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

    /** Cached kind used by generators/UI without forcing source asset load. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
    EYIAffixKind KindCache = EYIAffixKind::Prefix;
};
