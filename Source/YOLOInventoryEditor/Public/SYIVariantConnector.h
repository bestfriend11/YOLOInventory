#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SYIVariantConnector : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYIVariantConnector){}
		SLATE_ARGUMENT(FText, Label)
		SLATE_ARGUMENT(FLinearColor, Color)
		SLATE_ARGUMENT(FText, Tooltip)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs) { Label = InArgs._Label; Color = InArgs._Color; if (!InArgs._Tooltip.IsEmpty()) { SetToolTip(SNew(SToolTip).Text(InArgs._Tooltip)); } }

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	FText Label;
	FLinearColor Color = FLinearColor(0.3f,0.6f,1.f,0.8f);
};
