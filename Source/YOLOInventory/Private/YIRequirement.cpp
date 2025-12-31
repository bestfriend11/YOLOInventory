#include "YIRequirement.h"

bool UYIRequirement::EvaluateRequirement_Implementation(const FYIRequirementContext& Context) const
{
	(void)Context; return true; // default permissive
}

FText UYIRequirement::GetDisplayText_Implementation(const FYIRequirementContext& Context) const
{
	(void)Context; return Description.IsEmpty()? FText::FromString(TEXT("Requirement")) : Description;
}
