#include "Widgets/SInventoryTooltipWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include <Widgets/SBoxPanel.h>
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

void SInventoryTooltipWidget::Construct(const FArguments& InArgs)
{
	CachedData = InArgs._TooltipData.Get(FYITooltipData());

	ChildSlot
	[
		SAssignNew(RootBorder, SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(8.f))
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,4.f)
			[
				SAssignNew(TitleText, STextBlock)
				.Font(FAppStyle::Get().GetFontStyle("BoldFont"))
				.Text(FText::GetEmpty())
				.ColorAndOpacity(FSlateColor(FLinearColor::White))
				.WrapTextAt(320.f)
			]
			+SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,6.f)
			[
				SAssignNew(DescriptionText, STextBlock)
				.Text(FText::GetEmpty())
				.WrapTextAt(320.f)
				.AutoWrapText(true)
			]
			+SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,6.f)
			[
				SAssignNew(RequirementBox, SVerticalBox)
			]
			+SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,6.f)
			[
				SAssignNew(AffixBox, SVerticalBox)
			]
			+SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,6.f)
			[
				SAssignNew(AttributeBox, SVerticalBox)
			]
			+SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,4.f)
			[
				SAssignNew(DurabilityText, STextBlock)
				.Text(FText::GetEmpty())
				.Visibility(EVisibility::Collapsed)
			]
			+SVerticalBox::Slot().AutoHeight()
			[
				SAssignNew(PriceText, STextBlock)
				.Text(FText::GetEmpty())
				.Visibility(EVisibility::Collapsed)
			]
		]
	];

	RebuildFromData();
}

void SInventoryTooltipWidget::SetTooltipData(const FYITooltipData& InData)
{
	CachedData = InData;
	RebuildFromData();
}

void SInventoryTooltipWidget::RebuildFromData()
{
	const FLinearColor TitleTint = CachedData.RarityColor.A > KINDA_SMALL_NUMBER ? CachedData.RarityColor : FLinearColor::White;

	if (RootBorder.IsValid())
	{
		FLinearColor BorderTint = TitleTint;
		BorderTint.A = 0.2f;
		RootBorder->SetBorderBackgroundColor(BorderTint);
	}

	if (TitleText.IsValid())
	{
		const FText Name = !CachedData.FullName.IsEmpty() ? CachedData.FullName : CachedData.Title;
		TitleText->SetText(Name);
		TitleText->SetColorAndOpacity(TitleTint);
	}

	if (DescriptionText.IsValid())
	{
		DescriptionText->SetText(CachedData.Description);
	}

	if (AffixBox.IsValid())
	{
		AffixBox->ClearChildren();
		for (const FText& Line : CachedData.AffixLines)
		{
			if (Line.IsEmpty()) continue;
			AffixBox->AddSlot().AutoHeight()
			[
				SNew(STextBlock).Text(Line).AutoWrapText(true)
			];
		}
	}

	if (AttributeBox.IsValid())
	{
		AttributeBox->ClearChildren();
		for (const FYITooltipAttributeLine& Attr : CachedData.AttributeLines)
		{
			AttributeBox->AddSlot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::Format(NSLOCTEXT("YOLOInventory", "AttrFmt", "{0}: {1}"), Attr.Label, FText::AsNumber(Attr.Value)))
				.AutoWrapText(true)
			];
		}
	}

	if (RequirementBox.IsValid())
	{
		RequirementBox->ClearChildren();
		for (const FYITooltipRequirementLine& Req : CachedData.RequirementLines)
		{
			const FLinearColor ReqColor = Req.bMet ? FLinearColor::White : FLinearColor(1.f,0.25f,0.25f,1.f);
			RequirementBox->AddSlot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(Req.Text)
				.ColorAndOpacity(ReqColor)
				.AutoWrapText(true)
			];
		}
	}

	if (DurabilityText.IsValid())
	{
		if (CachedData.bHasDurability && CachedData.MaxDurability > 0.f)
		{
			DurabilityText->SetText(FText::Format(NSLOCTEXT("YOLOInventory", "DurabilityFmtSlate", "Durability: {0} / {1}"), FText::AsNumber(CachedData.CurrentDurability), FText::AsNumber(CachedData.MaxDurability)));
			DurabilityText->SetVisibility(EVisibility::Visible);
		}
		else
		{
			DurabilityText->SetText(FText::GetEmpty());
			DurabilityText->SetVisibility(EVisibility::Collapsed);
		}
	}

	if (PriceText.IsValid())
	{
		if (CachedData.SellPrice > 0)
		{
			PriceText->SetText(FText::Format(NSLOCTEXT("YOLOInventory", "SellPriceFmtSlate", "Sell Price: {0}"), FText::AsNumber(CachedData.SellPrice)));
			PriceText->SetVisibility(EVisibility::Visible);
		}
		else
		{
			PriceText->SetText(FText::GetEmpty());
			PriceText->SetVisibility(EVisibility::Collapsed);
		}
	}
}
