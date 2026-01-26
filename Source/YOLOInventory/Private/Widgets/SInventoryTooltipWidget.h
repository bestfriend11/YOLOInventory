#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "YIInventoryBlueprintLibrary.h"

/**
 * Default Slate tooltip for YOLOInventory runtime.
 * Shows name, description, affixes, attributes, durability and price.
 */
class SInventoryTooltipWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SInventoryTooltipWidget){}
		SLATE_ATTRIBUTE(FYITooltipData, TooltipData)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetTooltipData(const FYITooltipData& InData);

private:
	FYITooltipData CachedData;

	// Cached weak widgets for quick updates
	TSharedPtr<class SBorder> RootBorder;
	TSharedPtr<class STextBlock> TitleText;
	TSharedPtr<class STextBlock> DescriptionText;
	TSharedPtr<class SVerticalBox> AffixBox;
	TSharedPtr<class SVerticalBox> AttributeBox;
	TSharedPtr<class SVerticalBox> RequirementBox;
	TSharedPtr<class STextBlock> DurabilityText;
	TSharedPtr<class STextBlock> PriceText;

	void RebuildFromData();
};
