#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SYICompileResults : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYICompileResults){}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs) { ChildSlot[SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory","CompileEmpty","No compile results yet."))]; }

	void SetMessages(const TArray<FText>& InMessages)
	{
		// Minimal stub: could be an SListView; keep simple
		ChildSlot[SNew(STextBlock).Text(InMessages.Num()>0? InMessages.Last(): NSLOCTEXT("YOLOInventory","CompileEmpty2","No issues found."))];
	}
};
