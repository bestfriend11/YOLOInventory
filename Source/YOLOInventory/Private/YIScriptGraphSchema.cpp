#include "YIScriptGraphSchema.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraph.h"

void UYIScriptGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	// TODO: Add script nodes types; for now placeholder comment in menu.
}

const FPinConnectionResponse UYIScriptGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	if (!A || !B || A == B || A->Direction == B->Direction)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Invalid"));
	}
	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
}

void UYIScriptGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	// No default nodes
}
