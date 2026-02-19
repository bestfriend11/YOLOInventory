#pragma once

#include "CoreMinimal.h"
#include "YINode_Base.h"
#include "YINode_Attribute.generated.h"

UCLASS()
class YOLOINVENTORYEDITOR_API UYINode_Attribute : public UYINode_Base
{
	GENERATED_BODY()
public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override { return NSLOCTEXT("YOLOInventory", "AttributeNode", "Attribute"); }

	// Inline attribute data that designers can edit per node
	UPROPERTY(EditAnywhere, Instanced, Category="Attribute")
	class UYIItemAttribute* Attribute;
};

