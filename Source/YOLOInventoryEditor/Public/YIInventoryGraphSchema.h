#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "ToolMenu.h"
#include "YIInventoryGraphSchema.generated.h"

USTRUCT()
struct YOLOINVENTORYEDITOR_API FYIInventoryPinType
{
	GENERATED_BODY()
};

UCLASS()
class YOLOINVENTORYEDITOR_API UYIInventoryGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()
public:
	static const FName PC_Flow;
	static const FName PN_Exec;

	// UEdGraphSchema interface
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const override;
#if WITH_EDITOR
	virtual void GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;
#endif
};

