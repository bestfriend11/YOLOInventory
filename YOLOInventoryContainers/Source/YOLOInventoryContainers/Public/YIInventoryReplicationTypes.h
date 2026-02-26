#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YIInventoryBagNetTypes.h"
#include "YIInventoryReplicationTypes.generated.h"

USTRUCT(BlueprintType)
struct YOLOINVENTORYCONTAINERS_API FYINetBagDescriptor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid BagId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGameplayTag BagRoleTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FIntPoint GridSize = FIntPoint(0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 ItemCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 RuntimeRevision = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid ParentBagId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid ParentItemInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bIsNestedContainer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bIsActive = false;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYCONTAINERS_API FYINetBagMirrorView
{
	GENERATED_BODY()

	/** Bag identity this mirror payload represents. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid BagId;

	/** Lightweight UI grid size for the mirrored bag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FIntPoint GridSize = FIntPoint::ZeroValue;

	/** Runtime revision mirrored for optimistic client-side request guards and UI diagnostics. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 RuntimeRevision = 0;

	/** Minimal item payloads for the mirrored bag. Owner-only replicated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	TArray<FYINetBagItem> Items;
};
