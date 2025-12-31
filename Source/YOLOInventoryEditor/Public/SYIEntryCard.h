#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "YIStackEntry_Examples.h"

class UYIStackEntry;

DECLARE_DELEGATE_OneParam(FYIEntryAction, int32 /*Index*/)
DECLARE_DELEGATE_OneParam(FYIEntrySelect, UObject* /*SelectedObject*/)

class SYIEntryCard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYIEntryCard){}
		SLATE_ARGUMENT(int32, Index)
		SLATE_ARGUMENT(UYIStackEntry*, Entry)
		SLATE_ARGUMENT(FName, StackName)
		SLATE_EVENT(FYIEntryAction, OnUp)
		SLATE_EVENT(FYIEntryAction, OnDown)
		SLATE_EVENT(FYIEntryAction, OnDup)
		SLATE_EVENT(FYIEntryAction, OnDel)
		SLATE_EVENT(FYIEntrySelect, OnSelected)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	int32 Index = INDEX_NONE;
	FName StackName;
	UYIStackEntry* Entry = nullptr;
	FYIEntryAction OnUp, OnDown, OnDup, OnDel;
	FYIEntrySelect OnSelected;
	bool bHovered = false;

	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override { bHovered = true; }
	virtual void OnMouseLeave(const FPointerEvent& MouseEvent) override { bHovered = false; }
};
