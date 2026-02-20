#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
// #include "YIUnlock.h" // removed unlock criteria
#include "YIEvolutionPath.generated.h"

class UYIItemDefinition;

USTRUCT(BlueprintType)
struct YOLOINVENTORYLEGACYBRIDGE_API FYIEvoNode
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UYIItemDefinition> Item;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer Tags;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYLEGACYBRIDGE_API FYIEvoEdge
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 From = INDEX_NONE;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 To = INDEX_NONE;
	// Unlock criteria removed
};

UENUM(BlueprintType)
enum class EYIEvolutionPolicy : uint8
{
	FirstValid,
	HighestIndex,
	AllValid
};

UCLASS(BlueprintType)
class YOLOINVENTORYLEGACYBRIDGE_API UYIEvolutionPath : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FYIEvoNode> Nodes;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FYIEvoEdge> Edges;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EYIEvolutionPolicy Policy = EYIEvolutionPolicy::FirstValid;
};
