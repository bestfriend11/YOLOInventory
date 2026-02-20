#include "YIRequirement_MinAttributesGASList.h"
#include "AbilitySystemComponent.h"

bool UYIRequirement_MinAttributesGASList::EvaluateRequirement_Implementation(const FYIRequirementContext& Context) const
{
    for (const FYIGASAttributeMin& Pair : Requirements)
    {
        float V = 0.f;
        bool bHasValue = false;
        if (UAbilitySystemComponent* ASC = Context.AbilitySystem.Get())
        {
            V = ASC->GetNumericAttribute(Pair.Attribute);
            bHasValue = true;
        }
        else if (!Pair.Attribute.AttributeName.IsEmpty())
        {
            if (const float* PV = Context.PreviewAttributes.Find(FName(*Pair.Attribute.AttributeName)))
            {
                V = *PV; bHasValue = true;
            }
        }
        if (!bHasValue || V < Pair.MinValue)
        {
            return false;
        }
    }
    return true;
}

FText UYIRequirement_MinAttributesGASList::GetDisplayText_Implementation(const FYIRequirementContext& Context) const
{
    FString Accum;
    for (int32 i=0;i<Requirements.Num();++i)
    {
        const auto& P = Requirements[i];
        const FString Label = P.Attribute.AttributeName;
        Accum += FString::Printf(TEXT("%s >= %.1f"), *Label, P.MinValue);
        if (i < Requirements.Num()-1) Accum += TEXT(", ");
    }
    if (Accum.IsEmpty())
    {
        return UYIRequirement::GetDisplayText_Implementation(Context);
    }
    return FText::FromString(Accum);
}
