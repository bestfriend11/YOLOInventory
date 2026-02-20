#pragma once
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YIRequirement.generated.h"

class UAbilitySystemComponent;

USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIRequirementContext
{
	GENERATED_BODY()
	// Optional AbilitySystem to query attributes at runtime
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Context")
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystem;
	// Optional XP and owned tags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Context")
	int32 XP = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Context")
	FGameplayTagContainer OwnedTags;
	// Preview map for attributes when no ASC provided
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Context")
	TMap<FName,float> PreviewAttributes;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class YOLOINVENTORYSCHEMA_API UYIRequirement : public UObject
{
	GENERATED_BODY()
public:
	// Return true if the requirement is satisfied under Context
	UFUNCTION(BlueprintNativeEvent, Category="YOLOInventory|Requirement")
	bool EvaluateRequirement(const FYIRequirementContext& Context) const;
	// Build a display text for UI/preview
	UFUNCTION(BlueprintNativeEvent, Category="YOLOInventory|Requirement")
	FText GetDisplayText(const FYIRequirementContext& Context) const;

	// Optional description convenience field
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirement")
	FText Description;
};
