#include "YINode_Base.h"
#include "YIInventoryGraphSchema.h"

void UYINode_Base::AllocateDefaultPins()
{
	// No pins; stacking model like Niagara, connections are not used.
}

FText UYINode_Base::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return Title.IsEmpty() ? FText::FromString(TEXT("Node")) : Title;
}

FLinearColor UYINode_Base::GetNodeTitleColor() const
{
	return FLinearColor(0.3f, 0.1f, 0.4f);
}
