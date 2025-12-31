#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "YINode_Base.generated.h"

UCLASS(Abstract)
class YOLOINVENTORY_API UYINode_Base : public UEdGraphNode
{
	GENERATED_BODY()
public:
	// UEdGraphNode
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void PostPlacedNewNode() override { Super::PostPlacedNewNode(); }

	// Runtime: each node can carry attributes or item data later
	UPROPERTY(EditAnywhere, Category="YOLO")
	FText Title;
};
