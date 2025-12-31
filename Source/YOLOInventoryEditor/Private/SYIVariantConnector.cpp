#include "SYIVariantConnector.h"
#include "Rendering/DrawElements.h"
#include "Styling/AppStyle.h"
#include "Widgets/Text/STextBlock.h"

int32 SYIVariantConnector::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const FVector2D A(0.f, Size.Y*0.5f);
	const FVector2D B(Size.X-24.f, Size.Y*0.5f);
	const FLinearColor LineColor = Color;
	const float Thickness = 2.f;
	// Line
	FSlateDrawElement::MakeDrawSpaceSpline(OutDrawElements, LayerId,
		A, FVector2D(Size.X*0.25f, 0), B, FVector2D(Size.X*0.25f, 0),
		Thickness, ESlateDrawEffect::None, LineColor);
	// Arrow head (two lines)
	{
		TArray<FVector2D> Line1; Line1.Add(B); Line1.Add(B + FVector2D(-10.f, 6.f));
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId+1, AllottedGeometry.ToPaintGeometry(), Line1, ESlateDrawEffect::None, LineColor, true, Thickness);
		TArray<FVector2D> Line2; Line2.Add(B); Line2.Add(B + FVector2D(-10.f, -6.f));
		FSlateDrawElement::MakeLines(OutDrawElements, LayerId+1, AllottedGeometry.ToPaintGeometry(), Line2, ESlateDrawEffect::None, LineColor, true, Thickness);
	}
	// Label text
	FSlateFontInfo Font = FAppStyle::Get().GetFontStyle("NormalFont");
	// Use new API: size then layout transform (offset)
	FSlateDrawElement::MakeText(
		OutDrawElements,
		LayerId+2,
		AllottedGeometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(8, 2))),
		Label,
		Font,
		ESlateDrawEffect::None,
		FLinearColor::White);
	return LayerId+3;
}
