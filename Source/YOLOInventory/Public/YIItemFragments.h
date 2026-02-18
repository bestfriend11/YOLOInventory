#pragma once

#include "CoreMinimal.h"
#include "YIAffix.h"
#include "YIItemFragments.generated.h"

/** Marker base for instanced item fragments. */
USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIItemFragmentBase
{
	GENERATED_BODY()
};

/** Dynamic key/value attributes carried by an item instance. */
USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIItemAttributesFragment : public FYIItemFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	TMap<FName, float> Values;
};

/** Rolled affix payload carried by an item instance. */
USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIItemAffixesFragment : public FYIItemFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	TArray<FYIAffixInstance> Values;
};

/** Optional durability state carried by an item instance. */
USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIItemDurabilityFragment : public FYIItemFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	bool bEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	float Current = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	float Max = 0.f;
};

