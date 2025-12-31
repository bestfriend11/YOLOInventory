#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"
#include "Widgets/SBoxPanel.h" // SHorizontalBox, SVerticalBox
#include "Widgets/Text/STextBlock.h"

class SGraphNode_YINodeBase : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SGraphNode_YINodeBase) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphNode* InNode)
	{
		GraphNode = InNode;
		UpdateGraphNode();
	}

	virtual void UpdateGraphNode() override
	{
		InputPins.Empty();
		OutputPins.Empty();

		this->ContentScale.Bind(this, &SGraphNode::GetContentScale);

		// Containers for pins
		SAssignNew(LeftNodeBox, SVerticalBox);
		SAssignNew(RightNodeBox, SVerticalBox);

		TSharedRef<SVerticalBox> NodeBox = SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
			.Text_Lambda([this]() -> FText { return GraphNode ? GraphNode->GetNodeTitle(ENodeTitleType::FullTitle) : FText::FromString(TEXT("")); })
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[ LeftNodeBox.ToSharedRef() ]
			+ SHorizontalBox::Slot().FillWidth(1.f)
			+ SHorizontalBox::Slot().AutoWidth()[ RightNodeBox.ToSharedRef() ]
		];

		this->GetOrAddSlot(ENodeZone::Center)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			NodeBox
		];

		CreatePinWidgets();
	}
};
