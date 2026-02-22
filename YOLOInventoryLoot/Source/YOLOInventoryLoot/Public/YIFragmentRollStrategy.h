#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "YIInventoryBag.h"
#include "YIFragmentRollStrategy.generated.h"

class UYIItemDefinition;

/**
 * Optional, non-opinionated fragment generation hook.
 *
 * Designers can author Blueprint-derived strategy assets to inject runtime
 * fragment payloads into generated items without changing C++ generator code.
 * The default implementation is a no-op.
 */
UCLASS(Abstract, BlueprintType)
class YOLOINVENTORYLOOT_API UYIFragmentRollStrategy : public UDataAsset
{
	GENERATED_BODY()
public:
	/**
	 * Apply runtime fragment rolls to InOutItem.
	 * Return true when strategy applied any change.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="YOLOInventory|Generator|Fragments")
	bool ApplyGeneratedFragments(const UYIItemDefinition* ItemDefinition, int32 Level, int32 Seed, UPARAM(ref) FYIBagItem& InOutItem) const;
	virtual bool ApplyGeneratedFragments_Implementation(const UYIItemDefinition* ItemDefinition, int32 Level, int32 Seed, FYIBagItem& InOutItem) const;
};

