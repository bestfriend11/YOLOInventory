#pragma once

#include "CoreMinimal.h"
#include "YIInventoryCoreTypes.generated.h"

/** Stable runtime identity for a specific item instance. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYCORE_API FYIInventoryItemHandle
{
	GENERATED_BODY()

	/** Primary runtime identity (authoritative, deterministic). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid ItemInstanceId;

	/** Legacy fallback identity key used by older stack-based paths. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int64 LegacyStackKey = 0;

	/** Optional static item code for additional fallback matching. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int64 ItemCode = 0;
};

/** Stable runtime identity for a bag/inventory container. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYCORE_API FYIInventoryBagHandle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid BagId;
};

/** Canonical item reference used by core APIs: bag + item runtime identity. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYCORE_API FYIInventoryItemRef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FYIInventoryBagHandle Bag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FYIInventoryItemHandle Item;
};

/** Canonical lock identity (bag + item handle) used by authoritative lock tracking. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYCORE_API FYIInventoryLockRef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FYIInventoryItemRef ItemRef;
};
