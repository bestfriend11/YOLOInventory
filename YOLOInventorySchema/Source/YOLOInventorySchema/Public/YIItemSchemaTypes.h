#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YIItemSchemaTypes.generated.h"

class UTexture2D;

/** Schema-facing display payload decoupled from legacy item definition fields. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemSchemaDisplayData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	TSoftObjectPtr<UTexture2D> Icon;
};

/** Schema-facing layout payload used by grid and non-grid consumers. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemSchemaLayoutData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema", meta=(ClampMin="1"))
	FIntPoint DefaultSize = FIntPoint(1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	bool bAllowRotation = true;
};

/** Schema-facing stacking payload (independent from legacy field names). */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemSchemaStackingData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	bool bAllowStacking = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema", meta=(ClampMin="1"))
	int32 MaxStackCount = 99;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	bool bUseRiskChecks = true;
};

/** Schema-facing affix generation payload that does not depend on legacy asset class headers. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemSchemaAffixData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	TArray<FSoftObjectPath> TemplateAffixes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema", meta=(ClampMin="0"))
	int32 MinRandomModifiers = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema", meta=(ClampMin="0"))
	int32 MaxRandomModifiers = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FSoftObjectPath PrefixPool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FSoftObjectPath SuffixPool;
};

/** Combined schema snapshot exported from legacy item definitions for suite-side consumers. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemSchemaSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	int64 UniqueCode = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FString TemplateId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FGameplayTag ItemType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FGameplayTagContainer Tags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FYIItemSchemaDisplayData Display;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FYIItemSchemaLayoutData Layout;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FYIItemSchemaStackingData Stacking;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Schema")
	FYIItemSchemaAffixData Affix;
};
