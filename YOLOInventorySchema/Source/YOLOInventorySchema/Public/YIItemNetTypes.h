#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "YIItemNetTypes.generated.h"

class UYIItemDefinition;

/** Net-safe version of item instance for replication/UI payloads (avoids TMap replication). */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIItemInstanceNet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TSoftObjectPtr<UYIItemDefinition> Definition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	int32 Count = 1;

	/** Stable runtime identity for this specific item instance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FGuid InstanceId;

	/** Stack/group identity. Copies that belong to the same stack lineage share StackId. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FGuid StackId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	int64 CustomStackKey = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FGuid ContainedBagId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	bool bRotated = false;

	/** Fragment-first runtime payload for replication/UI transport. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item|Fragments", meta=(BaseStruct="/Script/YOLOInventorySchema.YIItemFragmentBase", ExcludeBaseStruct))
	TArray<FInstancedStruct> Fragments;
};
