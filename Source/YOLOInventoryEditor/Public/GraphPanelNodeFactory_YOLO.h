#pragma once

#include "CoreMinimal.h"
#include "EdGraphUtilities.h"

class FGraphPanelNodeFactory_YOLO : public FGraphPanelNodeFactory
{
public:
	virtual TSharedPtr<SGraphNode> CreateNode(UEdGraphNode* Node) const override;
};
