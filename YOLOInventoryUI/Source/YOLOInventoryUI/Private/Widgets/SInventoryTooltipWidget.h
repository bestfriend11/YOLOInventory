#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Styling/SlateBrush.h"
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
	FSlateBrush IconBrush;

	// Cached weak widgets for quick updates
	TSharedPtr<class SBorder> RootBorder;
	TSharedPtr<class SImage> IconImage;
	TSharedPtr<class STextBlock> TitleText;
	TSharedPtr<class STextBlock> SubtitleText;
	TSharedPtr<class STextBlock> DescriptionText;
	TSharedPtr<class SVerticalBox> SectionBox;
	TSharedPtr<class SVerticalBox> AffixBox;
	TSharedPtr<class SVerticalBox> AttributeBox;
	TSharedPtr<class SVerticalBox> RequirementBox;
	TSharedPtr<class SProgressBar> DurabilityBar;
	TSharedPtr<class STextBlock> DurabilityText;
	TSharedPtr<class STextBlock> PriceText;

	void RebuildFromData();
};
