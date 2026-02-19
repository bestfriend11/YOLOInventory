#include "YIInventoryGraphSchema.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraph.h"

#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraph.h"
#include "YINode_Item.h"
#include "YINode_Attribute.h"

const FName UYIInventoryGraphSchema::PC_Flow(TEXT("Flow"));
const FName UYIInventoryGraphSchema::PN_Exec(TEXT("Exec"));

struct FYIAction_NewNode : public FEdGraphSchemaAction
{
	UClass* NodeClass;
	FYIAction_NewNode(const FText& InNodeCategory, const FText& InMenuDesc, UClass* InNodeClass)
		: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, FText(), 0), NodeClass(InNodeClass) {}

	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode) override
	{
		UEdGraphNode* NewNode = NewObject<UEdGraphNode>(ParentGraph, NodeClass);
		ParentGraph->AddNode(NewNode, true, bSelectNewNode);
		NewNode->NodePosX = Location.X;
		NewNode->NodePosY = Location.Y;
		NewNode->AllocateDefaultPins();
		return NewNode;
	}
};

void UYIInventoryGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	// Background context menu: allow creating an Item node only when not from a pin
	if (ContextMenuBuilder.FromPin == nullptr)
	{
		ContextMenuBuilder.AddAction(MakeShared<FYIAction_NewNode>(FText::FromString("YOLO"), NSLOCTEXT("YOLOInventory", "AddItemNode", "Add Item"), UYINode_Item::StaticClass()));
	}
}

const FPinConnectionResponse UYIInventoryGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	if (!A || !B || A == B)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid pins"));
	}
	if (A->Direction == B->Direction)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Same direction"));
	}
	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
}

FLinearColor UYIInventoryGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	if (PinType.PinCategory == PC_Flow)
	{
		return FLinearColor(0.8f, 0.8f, 0.1f);
	}
	return FLinearColor::White;
}

void UYIInventoryGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	UYINode_Item* Root = NewObject<UYINode_Item>(&Graph);
	Graph.AddNode(Root, true, false);
	Root->NodePosX = 0;
	Root->NodePosY = 0;
}
