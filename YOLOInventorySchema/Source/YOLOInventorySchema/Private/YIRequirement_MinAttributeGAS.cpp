#include "YIRequirement_MinAttributeGAS.h"
#include "AbilitySystemComponent.h"

bool UYIRequirement_MinAttributeGAS::EvaluateRequirement_Implementation(const FYIRequirementContext& Context) const
{
	float V = 0.f;
	if (UAbilitySystemComponent* ASC = Context.AbilitySystem.Get())
	{
		V = ASC->GetNumericAttribute(Attribute);
	}
	else if (const float* PV = Context.PreviewAttributes.Find(FName(*Attribute.AttributeName)))
	{
		V = *PV;
	}
	return V >= MinValue;
}

FText UYIRequirement_MinAttributeGAS::GetDisplayText_Implementation(const FYIRequirementContext& Context) const
{
	FString Label = Attribute.AttributeName;
	return FText::FromString(FString::Printf(TEXT("%s >= %.1f"), *Label, MinValue));
}
