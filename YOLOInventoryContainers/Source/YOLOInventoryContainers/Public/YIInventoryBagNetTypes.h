#pragma once

#include "CoreMinimal.h"
#include "YIInventoryBagNetTypes.generated.h"

/** Minimal replicated view of a bag item to keep network footprint low. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYCONTAINERS_API FYINetBagItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag")
	int64 Code = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag")
	int32 Count = 1;

	/** Stable runtime identity for this specific item instance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag")
	FGuid InstanceId;

	/** Stack/group identity shared by split/merged derivatives. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag")
	FGuid StackId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag")
	FIntPoint Pos = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag")
	FIntPoint Size = FIntPoint(1,1);

	/** CustomStackKey to distinguish rolled variants without sending full affix data. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag")
	int64 CustomStackKey = 0;

	/** Optional nested container linkage (bag-in-bag). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bag")
	FGuid ContainedBagId;
};

