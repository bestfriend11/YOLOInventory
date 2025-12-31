#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "YIScriptGraphSchema.generated.h"

UCLASS()
class YOLOINVENTORY_API UYIScriptGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()
public:
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const override;
};
