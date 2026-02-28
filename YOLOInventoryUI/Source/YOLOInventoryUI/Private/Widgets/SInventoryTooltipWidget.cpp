#include "Widgets/SInventoryTooltipWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

void SInventoryTooltipWidget::Construct(const FArguments& InArgs)
{
	CachedData = InArgs._TooltipData.Get(FYITooltipData());
	IconBrush = FSlateNoResource();
	IconBrush.ImageSize = FVector2D(44.0f, 44.0f);

	ChildSlot
	[
		SAssignNew(RootBorder, SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		.Padding(FMargin(10.f))
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,6.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f,0.f,8.f,0.f)
				[
					SNew(SBox)
					.WidthOverride(44.f)
					.HeightOverride(44.f)
					[
						SAssignNew(IconImage, SImage)
						.Image(&IconBrush)
						.Visibility(EVisibility::Collapsed)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,1.f)
					[
						SAssignNew(TitleText, STextBlock)
						.Font(FAppStyle::Get().GetFontStyle("BoldFont"))
						.Text(FText::GetEmpty())
						.ColorAndOpacity(FSlateColor(FLinearColor::White))
						.WrapTextAt(320.f)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SAssignNew(SubtitleText, STextBlock)
						.Text(FText::GetEmpty())
						.Visibility(EVisibility::Collapsed)
						.ColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.77f, 0.82f, 1.0f)))
						.WrapTextAt(320.f)
					]
				]
			]
			+SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,4.f)
			[
				SAssignNew(DescriptionText, STextBlock)
				.Text(FText::GetEmpty())
				.WrapTextAt(320.f)
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.88f, 0.88f, 1.0f)))
			]
			+SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,5.f)
			[
				SNew(SSeparator)
			]
			+SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,4.f)
			[
				SAssignNew(SectionBox, SVerticalBox)
				.Visibility(EVisibility::Collapsed)
			]
			+SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,2.f)
			[
				SAssignNew(DurabilityBar, SProgressBar)
				.Visibility(EVisibility::Collapsed)
				.Percent(0.0f)
			]
			+SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,6.f)
			[
				SAssignNew(DurabilityText, STextBlock)
				.Text(FText::GetEmpty())
				.Visibility(EVisibility::Collapsed)
			]
			+SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,6.f)
			[
				SAssignNew(AffixBox, SVerticalBox)
			]
			+SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,6.f)
			[
				SAssignNew(AttributeBox, SVerticalBox)
			]
			+SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,6.f)
			[
				SAssignNew(RequirementBox, SVerticalBox)
			]
			+SVerticalBox::Slot().AutoHeight().Padding(0.f,0.f,0.f,2.f)
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

	if (SubtitleText.IsValid())
	{
		if (!CachedData.Subtitle.IsEmpty())
		{
			SubtitleText->SetText(CachedData.Subtitle);
			SubtitleText->SetVisibility(EVisibility::Visible);
		}
		else
		{
			SubtitleText->SetText(FText::GetEmpty());
			SubtitleText->SetVisibility(EVisibility::Collapsed);
		}
	}

	if (IconImage.IsValid())
	{
		if (UTexture2D* IconTexture = CachedData.Icon.Get())
		{
			IconBrush.SetResourceObject(IconTexture);
			IconImage->SetVisibility(EVisibility::Visible);
		}
		else
		{
			IconBrush.SetResourceObject(nullptr);
			IconImage->SetVisibility(EVisibility::Collapsed);
		}
	}

	if (DescriptionText.IsValid())
	{
		DescriptionText->SetText(CachedData.Description);
	}

	const bool bUseRichSections = CachedData.Sections.Num() > 0;
	if (SectionBox.IsValid())
	{
		SectionBox->ClearChildren();
		if (bUseRichSections)
		{
			for (int32 SectionIndex = 0; SectionIndex < CachedData.Sections.Num(); ++SectionIndex)
			{
				const FYITooltipSection& Section = CachedData.Sections[SectionIndex];
				if (Section.Header.IsEmpty() && Section.Lines.Num() <= 0)
				{
					continue;
				}

				if (!Section.Header.IsEmpty())
				{
					SectionBox->AddSlot().AutoHeight().Padding(0.f, SectionIndex == 0 ? 0.f : 6.f, 0.f, 2.f)
					[
						SNew(STextBlock)
						.Text(Section.Header)
						.Font(FAppStyle::Get().GetFontStyle("BoldFont"))
						.ColorAndOpacity(FSlateColor(Section.HeaderColor))
					];
				}

				for (const FYITooltipStyledLine& Line : Section.Lines)
				{
					if (Line.Text.IsEmpty())
					{
						continue;
					}
					SectionBox->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 1.f)
					[
						SNew(STextBlock)
						.Text(Line.Text)
						.ColorAndOpacity(FSlateColor(Line.Color))
						.AutoWrapText(true)
					];
				}
			}
		}
		SectionBox->SetVisibility(bUseRichSections ? EVisibility::Visible : EVisibility::Collapsed);
	}

	if (AffixBox.IsValid())
	{
		AffixBox->SetVisibility(bUseRichSections ? EVisibility::Collapsed : EVisibility::Visible);
		AffixBox->ClearChildren();
		if (!bUseRichSections)
		{
			for (const FText& Line : CachedData.AffixLines)
			{
				if (Line.IsEmpty()) continue;
				AffixBox->AddSlot().AutoHeight()
				[
					SNew(STextBlock).Text(Line).AutoWrapText(true)
				];
			}
		}
	}

	if (AttributeBox.IsValid())
	{
		AttributeBox->SetVisibility(bUseRichSections ? EVisibility::Collapsed : EVisibility::Visible);
		AttributeBox->ClearChildren();
		if (!bUseRichSections)
		{
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
	}

	if (RequirementBox.IsValid())
	{
		RequirementBox->SetVisibility(bUseRichSections ? EVisibility::Collapsed : EVisibility::Visible);
		RequirementBox->ClearChildren();
		if (!bUseRichSections)
		{
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
	}

	if (DurabilityBar.IsValid())
	{
		if (CachedData.bHasDurability && CachedData.MaxDurability > 0.f)
		{
			const float DurabilityPct = FMath::Clamp(CachedData.CurrentDurability / CachedData.MaxDurability, 0.0f, 1.0f);
			DurabilityBar->SetPercent(DurabilityPct);
			const FLinearColor DurabilityColor = DurabilityPct > 0.65f
				? FLinearColor(0.48f, 0.90f, 0.55f, 1.0f)
				: (DurabilityPct > 0.30f ? FLinearColor(0.92f, 0.80f, 0.40f, 1.0f) : FLinearColor(0.92f, 0.38f, 0.38f, 1.0f));
			DurabilityBar->SetFillColorAndOpacity(DurabilityColor);
			DurabilityBar->SetVisibility(EVisibility::Visible);
		}
		else
		{
			DurabilityBar->SetPercent(0.0f);
			DurabilityBar->SetVisibility(EVisibility::Collapsed);
		}
	}

	if (DurabilityText.IsValid())
	{
		if (CachedData.bHasDurability && CachedData.MaxDurability > 0.f)
		{
			DurabilityText->SetText(FText::Format(
				NSLOCTEXT("YOLOInventory", "DurabilityFmtSlate", "Durability: {0} / {1}"),
				FText::AsNumber(FMath::RoundToInt(CachedData.CurrentDurability)),
				FText::AsNumber(FMath::RoundToInt(CachedData.MaxDurability))));
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
		if (bUseRichSections)
		{
			PriceText->SetText(FText::GetEmpty());
			PriceText->SetVisibility(EVisibility::Collapsed);
		}
		else if (!CachedData.EconomyLine.IsEmpty())
		{
			PriceText->SetText(CachedData.EconomyLine);
			PriceText->SetVisibility(EVisibility::Visible);
		}
		else if (CachedData.SellPrice > 0)
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
