#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FYIPaletteEntry
{
	FText Label;
	FText Tooltip;
	FName StackName;
	UClass* EntryClass = nullptr;
	FName Category; // UI/Ability/Upgrade/Economy
	bool bIsHeader = false;
};

class SYIPaletteRow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYIPaletteRow){}
		SLATE_ARGUMENT(TSharedPtr<FYIPaletteEntry>, Entry)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	TSharedPtr<FYIPaletteEntry> Entry;

private:
	FReply OnMouseButtonDownHandler(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
};
