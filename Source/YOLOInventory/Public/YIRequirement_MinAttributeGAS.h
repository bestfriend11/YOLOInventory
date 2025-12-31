#pragma once
#include "CoreMinimal.h"
#include "YIRequirement.h"
#include "GameplayEffectTypes.h"
#include "YIRequirement_MinAttributeGAS.generated.h"

UCLASS(BlueprintType, Blueprintable, EditInlineNew)
class UYIRequirement_MinAttributeGAS : public UYIRequirement
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirement")
	FGameplayAttribute Attribute;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Requirement")
	float MinValue = 0.f;

	virtual bool EvaluateRequirement_Implementation(const FYIRequirementContext& Context) const override;
	virtual FText GetDisplayText_Implementation(const FYIRequirementContext& Context) const override;
};
