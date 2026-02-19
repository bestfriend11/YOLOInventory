#pragma once

#include "CoreMinimal.h"
#include "YIAffix.h"
#include "YIItemNetTypes.generated.h"

class UYIItemDefinition;

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIAttributeKV
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	float Value = 0.f;
};

/** Net-safe version of item instance for replication/UI payloads (avoids TMap replication). */
USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIItemInstanceNet
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TArray<FYIAffixInstance> Affixes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TArray<FYIAttributeKV> Attributes;
};
