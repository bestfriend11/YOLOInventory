#include "Widgets/InventoryTooltipView.h"
#include "Widgets/InventoryTooltipDesignerWidget.h"
#include "Widgets/SInventoryTooltipWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/SizeBox.h"

UInventoryTooltipView::UInventoryTooltipView(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Ensure WidgetTree exists so Designer doesn't hit null when opening in UMG
	if (!WidgetTree)
	{
		WidgetTree = ObjectInitializer.CreateDefaultSubobject<UWidgetTree>(this, TEXT("WidgetTree"));
	}
}

TSharedRef<SWidget> UInventoryTooltipView::RebuildWidget()
{
	// In Designer, ensure there is a UMG root so WidgetTree is valid
	if (IsDesignTime())
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, UWidgetTree::StaticClass(), TEXT("WidgetTree"));
		}
		if (!WidgetTree->RootWidget)
		{
			// Provide a minimal root widget for preview
			USizeBox* PreviewRoot = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PreviewRoot"));
			PreviewRoot->SetWidthOverride(360.f);
			PreviewRoot->SetHeightOverride(120.f);
			WidgetTree->RootWidget = PreviewRoot;
		}
	}

	if (!IsDesignTime() && bUseDesignerTooltip && DesignerTooltipClass)
	{
		DesignerTooltipInstance = CreateWidget<UInventoryTooltipDesignerWidget>(GetWorld(), DesignerTooltipClass);
		if (DesignerTooltipInstance)
		{
			return DesignerTooltipInstance->TakeWidget();
		}
	}

	SAssignNew(SlateTooltip, SInventoryTooltipWidget)
		.TooltipData(TooltipData);
	return SlateTooltip.ToSharedRef();
}

void UInventoryTooltipView::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	SlateTooltip.Reset();
	DesignerTooltipInstance = nullptr;
}

void UInventoryTooltipView::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	PushDataToCurrentView();
}

void UInventoryTooltipView::SetTooltipData(const FYITooltipData& InData)
{
	TooltipData = InData;
	PushDataToCurrentView();
}

void UInventoryTooltipView::ClearTooltip()
{
	TooltipData = FYITooltipData();
	PushDataToCurrentView();
}

void UInventoryTooltipView::PushDataToCurrentView()
{
	if (SlateTooltip.IsValid())
	{
		SlateTooltip->SetTooltipData(TooltipData);
	}
	else if (DesignerTooltipInstance)
	{
		DesignerTooltipInstance->OnTooltipDataUpdated(TooltipData);
	}
}
