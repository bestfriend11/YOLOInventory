#include "GraphPanelNodeFactory_YOLO.h"
#include "SGraphNode_YINodeBase.h"
#include "YINode_Base.h"
#include "YINode_Item.h"
// #include "SGraphNode_YIItem.h" // removed

TSharedPtr<SGraphNode> FGraphPanelNodeFactory_YOLO::CreateNode(UEdGraphNode* Node) const
{
	// Legacy item node removed; always use base node
	return SNew(SGraphNode_YINodeBase, Node);
}
