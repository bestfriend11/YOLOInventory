#pragma once
#include "CoreMinimal.h"
#include "YIRequirement.h"
#include "GameplayEffectTypes.h"
#include "YIRequirement_MinAttributesGASList.generated.h"

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIGASAttributeMin
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirement")
    FGameplayAttribute Attribute;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirement")
    float MinValue = 0.f;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class YOLOINVENTORY_API UYIRequirement_MinAttributesGASList : public UYIRequirement
{
    GENERATED_BODY()
public:
    // List of required attributes and minimum values
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirement")
    TArray<FYIGASAttributeMin> Requirements;

    virtual bool EvaluateRequirement_Implementation(const FYIRequirementContext& Context) const override;
    virtual FText GetDisplayText_Implementation(const FYIRequirementContext& Context) const override;
};
