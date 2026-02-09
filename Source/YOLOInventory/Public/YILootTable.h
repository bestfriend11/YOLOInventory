#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "YILootTable.generated.h"

class UYIItemDefinition;

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYILootTableEntry
{
	GENERATED_BODY()

	// Item definition to roll.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot")
	TSoftObjectPtr<UYIItemDefinition> Definition;

	// Relative weight when sampling.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot", meta=(ClampMin="0.0"))
	float Weight = 1.f;

	// Count range for stackable items.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot", meta=(ClampMin="1"))
	int32 MinCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot", meta=(ClampMin="1"))
	int32 MaxCount = 1;

	// Optional level gate.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot", meta=(ClampMin="0"))
	int32 MinLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot", meta=(ClampMin="0"))
	int32 MaxLevel = 9999;

	// Optional tag filters; if set, the item definition must match at least one.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot")
	FGameplayTagContainer RequiredTags;
};

UCLASS(BlueprintType)
class YOLOINVENTORY_API UYILootTable : public UDataAsset
{
	GENERATED_BODY()
public:
	// Entries used for weighted selection.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Loot")
	TArray<FYILootTableEntry> Entries;

	// Weighted roll; returns false if nothing eligible.
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Loot")
	bool RollDefinition(int32 Level, int32 Seed, TSoftObjectPtr<UYIItemDefinition>& OutDefinition, int32& OutCount) const;

	// Helper to gather valid entries for a level.
	bool GetEligibleEntries(int32 Level, TArray<const FYILootTableEntry*>& OutEntries) const;
};
