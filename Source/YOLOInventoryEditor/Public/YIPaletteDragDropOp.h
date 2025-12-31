#pragma once

#include "CoreMinimal.h"
#include "DragAndDrop/DecoratedDragDropOp.h"

class YIPaletteDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(YIPaletteDragDropOp, FDecoratedDragDropOp)

	UClass* EntryClass = nullptr;
	FName StackName = NAME_None;

	static TSharedRef<YIPaletteDragDropOp> New(UClass* InClass, FName InStack)
	{
		TSharedRef<YIPaletteDragDropOp> Op = MakeShareable(new YIPaletteDragDropOp);
		Op->EntryClass = InClass;
		Op->StackName = InStack;
		Op->DefaultHoverText = FText::FromString(InClass ? InClass->GetName() : TEXT("Entry"));
		Op->Construct();
		return Op;
	}
};
