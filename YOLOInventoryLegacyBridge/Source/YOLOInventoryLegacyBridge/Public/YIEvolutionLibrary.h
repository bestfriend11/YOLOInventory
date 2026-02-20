#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "YIEvolutionLibrary.generated.h"

class UYIEvolutionPath;

UCLASS()
class YOLOINVENTORYLEGACYBRIDGE_API UYIEvolutionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Evolution")
	static TArray<class UYIItemDefinition*> EvaluateEvolution(const UYIEvolutionPath* Path, const TMap<FName,float>& Attributes, const FGameplayTagContainer& OwnedTags, int32 XP);
};
