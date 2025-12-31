#include "SYIPaletteRow.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "YIPaletteDragDropOp.h"
#include "Styling/AppStyle.h"

void SYIPaletteRow::Construct(const FArguments& InArgs)
{
	Entry = InArgs._Entry;

	ChildSlot
	[
		SNew(SBorder)
		.Padding(2)
		.BorderImage(FAppStyle::Get().GetBrush("Graph.Panel.SolidBackground"))
		.OnMouseButtonDown(this, &SYIPaletteRow::OnMouseButtonDownHandler)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()[ SNew(STextBlock).Text_Lambda([this](){ return Entry && Entry->bIsHeader ? FText::FromString(TEXT("")) : FText::FromString(TEXT("•")); }) ]
			+ SHorizontalBox::Slot().FillWidth(1.f)[ SNew(STextBlock).Text(Entry.IsValid()? Entry->Label : FText::FromString(TEXT(""))) ]
		]
	];
}

FReply SYIPaletteRow::OnMouseButtonDownHandler(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Entry.IsValid() && !Entry->bIsHeader)
	{
		TSharedRef<YIPaletteDragDropOp> Op = YIPaletteDragDropOp::New(Entry->EntryClass, Entry->StackName);
		return FReply::Handled().BeginDragDrop(Op);
	}
	return FReply::Unhandled();
}
