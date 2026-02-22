#pragma once

#include "CoreMinimal.h"
#include "YIFragmentRollStrategy.h"
#include "YIFragmentPoolRollStrategy.generated.h"

class UYIFragmentPoolAsset;

/**
 * Default fragment roll strategy that samples fragment assets from a generic pool
 * and appends their runtime item-instance fragments to generated items.
 */
UCLASS(BlueprintType)
class YOLOINVENTORYLOOT_API UYIFragmentPoolRollStrategy : public UYIFragmentRollStrategy
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pool")
	TSoftObjectPtr<UYIFragmentPoolAsset> Pool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pool", meta=(ClampMin="0"))
	int32 MinRolls = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pool", meta=(ClampMin="0"))
	int32 MaxRolls = 0;

	/** Avoid adding duplicate runtime fragment struct types in one generation pass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pool")
	bool bPreventDuplicateStructTypes = true;

	virtual bool ApplyGeneratedFragments_Implementation(const UYIItemDefinition* ItemDefinition, int32 Level, int32 Seed, FYIBagItem& InOutItem) const override;
};

