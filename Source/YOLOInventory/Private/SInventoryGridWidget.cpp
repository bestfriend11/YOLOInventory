#include "SInventoryGridWidget.h"
#include "InventoryGridWidget.h"
#include "YIInventoryBag.h"
#include "YIInventoryTypes.h" // for YI_GetRarityColor
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"
#include "Engine/Texture2D.h"
#include "YIItemDefinition.h"
#include "Framework/Application/SlateApplication.h"

#pragma optimize("", off)

void SInventoryGridWidget::RebuildOccupancy()
{
	CellToItemIndex.Reset();
	if (!Bag.IsValid()) return;
	CellToItemIndex.Init(INDEX_NONE, Bag->GridSize.X * Bag->GridSize.Y);
	for (int32 i=0;i<Bag->Items.Num();++i)
	{
		const auto& It = Bag->Items[i];
		const FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
		for (int y=0; y<Eff.Y; ++y)
		{
			for (int x=0; x<Eff.X; ++x)
			{
				const int GX = It.Pos.X + x;
				const int GY = It.Pos.Y + y;
				if (GX>=0 && GY>=0 && GX < Bag->GridSize.X && GY < Bag->GridSize.Y)
				{
					CellToItemIndex[GY * Bag->GridSize.X + GX] = i;
				}
			}
		}
	}
}

int32 SInventoryGridWidget::GetItemIndexAtCell(const FIntPoint& Cell) const
{
	if (!Bag.IsValid()) return INDEX_NONE;
	if (Cell.X < 0 || Cell.Y < 0 || Cell.X >= Bag->GridSize.X || Cell.Y >= Bag->GridSize.Y) return INDEX_NONE;
	const int32 Idx = Cell.Y * Bag->GridSize.X + Cell.X;
	if (!CellToItemIndex.IsValidIndex(Idx)) return INDEX_NONE;
	return CellToItemIndex[Idx];
}

void SInventoryGridWidget::UpdateHoverSelection()
{
	if (!Bag.IsValid())
	{
		HoveredItemIndex = INDEX_NONE;
		HoveredItemTopLeft = FIntPoint(-1,-1);
		HoveredItemSize = FIntPoint::ZeroValue;
		if (OnHoveredItemChanged.IsBound()) { OnHoveredItemChanged.Execute(HoveredItemIndex); }
		return;
	}

	const FIntPoint GridSize = Bag->GridSize;
	// If HoverCell is outside the grid area, treat it as no hover
	if (HoverCell.X < 0 || HoverCell.Y < 0 || HoverCell.X >= GridSize.X || HoverCell.Y >= GridSize.Y)
	{
		if (HoveredItemIndex != INDEX_NONE)
		{
			HoveredItemIndex = INDEX_NONE;
			HoveredItemTopLeft = FIntPoint(-1,-1);
			HoveredItemSize = FIntPoint::ZeroValue;
			if (OnHoveredItemChanged.IsBound()) { OnHoveredItemChanged.Execute(HoveredItemIndex); }
		}
		return;
	}

	const int32 Idx = GetItemIndexAtCell(HoverCell);
	if (Idx != HoveredItemIndex)
	{
		HoveredItemIndex = Idx;
		if (Idx != INDEX_NONE)
		{
			const auto& It = Bag->Items[Idx];
			HoveredItemTopLeft = It.Pos;
			HoveredItemSize = Bag->GetEffectiveSize(It.Size);
		}
		else
		{
			HoveredItemTopLeft = FIntPoint(-1,-1);
			HoveredItemSize = FIntPoint::ZeroValue;
		}
		if (OnHoveredItemChanged.IsBound()) { OnHoveredItemChanged.Execute(HoveredItemIndex); }
	}
} 


void SInventoryGridWidget::Construct(const FArguments& InArgs)
{
	OwnerWidget = InArgs._OwnerWidget;
	Bag = InArgs._Bag;
	CellSize = FVector2D(InArgs._CellPixelSize, InArgs._CellPixelSize);
	bWholeItemHover = InArgs._bWholeItemHover;
	bWholeItemSelection = InArgs._bWholeItemSelection;
	bWrapNavigation = InArgs._bWrapNavigation;
	bEnableCellHover = InArgs._bEnableCellHover;
	bEnableMouseSelection = InArgs._bEnableMouseSelection;

	// Hook up Slate-provided callbacks
	OnHoveredItemChanged = InArgs._OnHoveredItemChanged;
	OnSelectedCellChanged = InArgs._OnSelectedCellChanged;
	OnCellClicked = InArgs._OnCellClicked;

	if (Bag.IsValid())
	{
		// Build occupancy once and bind bag change to keep in sync
		RebuildOccupancy();
		Bag->OnChanged.AddLambda([WeakThis = TWeakPtr<SInventoryGridWidget>(SharedThis(this))]()
		{
			if (!WeakThis.IsValid()) return;
			auto Pinned = WeakThis.Pin();
			if (!Pinned->Bag.IsValid()) return;
			Pinned->RebuildOccupancy();
			Pinned->UpdateHoverSelection();
			Pinned->Invalidate(EInvalidateWidgetReason::Paint);
		});
	}
}

int32 SInventoryGridWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 L = LayerId;
	if (!Bag.IsValid()) return L;
	const FSlateBrush* Box = FAppStyle::Get().GetBrush("WhiteBrush");
	// Use explicit pixel cell size as configured (do not stretch to allotted geometry)
	const FVector2D LocalCell = CellSize;
	const FVector2D SizePix = FVector2D(Bag->GridSize) * LocalCell;
	FSlateDrawElement::MakeBox(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(FVector2f(SizePix), FSlateLayoutTransform()), Box, ESlateDrawEffect::None, Bag->CellBgColor);
	// Grid lines
	const float Half=0.5f; const float MaxX=SizePix.X-Half; const float MaxY=SizePix.Y-Half;
	for(int x=0;x<=Bag->GridSize.X;++x){ float X=x*LocalCell.X+Half; TArray<FVector2D> Seg={FVector2D(X,Half),FVector2D(X,MaxY)}; FSlateDrawElement::MakeLines(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(), Seg, ESlateDrawEffect::None, Bag->GridLineColor, false, FMath::Max(1.f, Bag->GridThickness)); }
	for(int y=0;y<=Bag->GridSize.Y;++y){ float Y=y*LocalCell.Y+Half; TArray<FVector2D> Seg={FVector2D(Half,Y),FVector2D(MaxX,Y)}; FSlateDrawElement::MakeLines(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(), Seg, ESlateDrawEffect::None, Bag->GridLineColor, false, FMath::Max(1.f, Bag->GridThickness)); }
	// Items
	for (int32 i=0;i<Bag->Items.Num();++i)
	{
		const auto& It = Bag->Items[i];
		FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
		FVector2D P = FVector2D(It.Pos) * LocalCell;
		FVector2D S = FVector2D(Eff) * LocalCell;
		FLinearColor Fill = FLinearColor(1,1,1,0.08f);
		// Try load asset for rarity and icon later
		UYIItemDefinition* Def = It.Item.Definition.IsValid() ? It.Item.Definition.Get() : It.Item.Definition.LoadSynchronous();
		UTexture2D* IconTex = (Bag->bEnableThumbnails && Def) ? (Def->Icon.IsValid() ? Def->Icon.Get() : Def->Icon.LoadSynchronous()) : nullptr;
		FLinearColor Border = FLinearColor(0.8f,0.8f,0.8f,0.5f);
		if (Def)
		{
			Border.A = 0.65f; // border tint TBD (tag-based)
		}
		// Fill
		FSlateDrawElement::MakeBox(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(FVector2f(S), FSlateLayoutTransform(FVector2f(P))), Box, ESlateDrawEffect::None, Fill);
		// Icon (centered within the item footprint)
		if (IconTex)
		{
			FSlateBrush IconBrush;
			IconBrush.SetResourceObject(IconTex);
			IconBrush.ImageSize = FVector2D(IconTex->GetSizeX(), IconTex->GetSizeY());
			IconBrush.DrawAs = ESlateBrushDrawType::Image;
			const float Pad = 2.f;
			FVector2D MaxIconSize = S - FVector2D(Pad * 2.f, Pad * 2.f);
			MaxIconSize.X = FMath::Max(1.f, MaxIconSize.X);
			MaxIconSize.Y = FMath::Max(1.f, MaxIconSize.Y);
			const float TexAspect = (IconBrush.ImageSize.Y > KINDA_SMALL_NUMBER) ? (IconBrush.ImageSize.X / IconBrush.ImageSize.Y) : 1.f;
			const float SlotAspect = MaxIconSize.X / MaxIconSize.Y;
			FVector2D IconSize = MaxIconSize;
			if (TexAspect > SlotAspect)
			{
				IconSize = FVector2D(MaxIconSize.X, MaxIconSize.X / FMath::Max(TexAspect, KINDA_SMALL_NUMBER));
			}
			else
			{
				IconSize = FVector2D(MaxIconSize.Y * TexAspect, MaxIconSize.Y);
			}
			const FVector2D IconPos = P + (S - IconSize) * 0.5f;
			FSlateDrawElement::MakeBox(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(FVector2f(IconSize), FSlateLayoutTransform(FVector2f(IconPos))), &IconBrush, ESlateDrawEffect::None, FLinearColor::White);
		}
		// Border
		{
			TArray<FVector2D> Seg;
			Seg.Add(P + FVector2D(0,0));
			Seg.Add(P + FVector2D(S.X,0));
			Seg.Add(P + FVector2D(S.X,0));
			Seg.Add(P + FVector2D(S.X,S.Y));
			Seg.Add(P + FVector2D(S.X,S.Y));
			Seg.Add(P + FVector2D(0,S.Y));
			Seg.Add(P + FVector2D(0,S.Y));
			Seg.Add(P + FVector2D(0,0));
			FSlateDrawElement::MakeLines(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(), Seg, ESlateDrawEffect::None, Border, false, FMath::Max(1.f, Bag->GridThickness));
		}
		// Stack count label (bottom-right)
		if (It.Item.Count > 1)
		{
			FString CountStr = FString::FromInt(It.Item.Count);
			FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", 10);
			FVector2D TextSize = FVector2D(40, 14);
			FVector2D TP = P + S - TextSize - FVector2D(2,2);
			FSlateDrawElement::MakeText(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(FVector2f(TP))), FText::FromString(CountStr), Font, ESlateDrawEffect::None, FLinearColor::White);
		}
	}
	// Hover highlight: only when not actively dragging a ghost, to avoid visual conflict
	if (!bGhostActive && bEnableCellHover)
	{
		if (bWholeItemHover && HoveredItemIndex != INDEX_NONE)
		{
			const FVector2D P = FVector2D(HoveredItemTopLeft) * LocalCell;
			const FVector2D S = FVector2D(HoveredItemSize) * LocalCell;
			FSlateDrawElement::MakeBox(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(FVector2f(S), FSlateLayoutTransform(FVector2f(P))), Box, ESlateDrawEffect::None, FLinearColor(0.1f,0.8f,0.2f,0.12f));
			// Re-draw item icon above the highlight so it stays visible
			if (Bag->Items.IsValidIndex(HoveredItemIndex))
			{
				const FYIBagItem& HoverItem = Bag->Items[HoveredItemIndex];
				UYIItemDefinition* HoverDef = HoverItem.Item.Definition.IsValid() ? HoverItem.Item.Definition.Get() : HoverItem.Item.Definition.LoadSynchronous();
				UTexture2D* HoverIcon = HoverDef ? (HoverDef->Icon.IsValid() ? HoverDef->Icon.Get() : HoverDef->Icon.LoadSynchronous()) : nullptr;
				if (HoverIcon)
				{
					FSlateBrush HoverIconBrush;
					HoverIconBrush.SetResourceObject(HoverIcon);
					HoverIconBrush.ImageSize = FVector2D(HoverIcon->GetSizeX(), HoverIcon->GetSizeY());
					HoverIconBrush.DrawAs = ESlateBrushDrawType::Image;
					const float Pad = 2.f;
					FVector2D MaxIconSize = S - FVector2D(Pad * 2.f, Pad * 2.f);
					MaxIconSize.X = FMath::Max(1.f, MaxIconSize.X);
					MaxIconSize.Y = FMath::Max(1.f, MaxIconSize.Y);
					const float TexAspect = (HoverIconBrush.ImageSize.Y > KINDA_SMALL_NUMBER) ? (HoverIconBrush.ImageSize.X / HoverIconBrush.ImageSize.Y) : 1.f;
					const float SlotAspect = MaxIconSize.X / MaxIconSize.Y;
					FVector2D IconSize = MaxIconSize;
					if (TexAspect > SlotAspect)
					{
						IconSize = FVector2D(MaxIconSize.X, MaxIconSize.X / FMath::Max(TexAspect, KINDA_SMALL_NUMBER));
					}
					else
					{
						IconSize = FVector2D(MaxIconSize.Y * TexAspect, MaxIconSize.Y);
					}
					const FVector2D IconPos = P + (S - IconSize) * 0.5f;
					FSlateDrawElement::MakeBox(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(FVector2f(IconSize), FSlateLayoutTransform(FVector2f(IconPos))), &HoverIconBrush, ESlateDrawEffect::None, FLinearColor::White);
				}
			}
		}
		else
		{
			FVector2D P = FVector2D(HoverCell) * LocalCell;
			FSlateDrawElement::MakeBox(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(FVector2f(LocalCell), FSlateLayoutTransform(FVector2f(P))), Box, ESlateDrawEffect::None, FLinearColor(0.1f,0.6f,1.f,0.08f));
		}
	}
	// Selected cell cursor (thicker outline)
	if (SelectedCell.X >= 0 && SelectedCell.Y >= 0)
	{
		FVector2D P = FVector2D(SelectedCell) * LocalCell;
		TArray<FVector2D> Rect = { P, P + FVector2D(LocalCell.X, 0), P + FVector2D(LocalCell.X, LocalCell.Y), P + FVector2D(0, LocalCell.Y), P };
		FLinearColor CursorColor = FLinearColor(0.2f, 0.6f, 1.f, 0.9f);
		FSlateDrawElement::MakeLines(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(), Rect, ESlateDrawEffect::None, CursorColor, true, 2.5f);
	}
	// Ghost visual (click-to-drag without holding)
	if (bGhostActive)
	{
		// Always draw the footprint highlight so users can see valid/invalid placement while dragging,
		// even when a global ghost renderer is used.
		const FLinearColor GhostTint = bGhostPlacementValid ? FLinearColor(0.2f,0.8f,0.2f,0.18f) : FLinearColor(0.8f,0.2f,0.2f,0.18f);
		const FVector2D FootP = FVector2D(GhostTopLeft) * LocalCell;
		const FVector2D FootS = FVector2D(GhostFootprint) * LocalCell;
		FSlateDrawElement::MakeBox(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(FVector2f(FootS), FSlateLayoutTransform(FVector2f(FootP))), Box, ESlateDrawEffect::None, GhostTint);

		// Only draw the ghost icon locally if we are not using a global overlay ghost.
		if (!bUseGlobalDragGhost)
		{
			const FVector2D P = GhostCursorLocal - (GhostSize * 0.5f);
			const FSlateBrush* BrushToUse = bGhostHasIcon ? &GhostBrush : FAppStyle::Get().GetBrush("WhiteBrush");
			const FLinearColor Tint = bGhostHasIcon ? FLinearColor(1,1,1,0.9f) : FLinearColor(1,1,1,0.18f);
			FSlateDrawElement::MakeBox(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(FVector2f(GhostSize), FSlateLayoutTransform(FVector2f(P))), BrushToUse, ESlateDrawEffect::None, Tint);
			// Outline for visibility
			TArray<FVector2D> Seg;
			Seg.Add(P);
			Seg.Add(P + FVector2D(GhostSize.X, 0));
			Seg.Add(P + FVector2D(GhostSize.X, GhostSize.Y));
			Seg.Add(P + FVector2D(0, GhostSize.Y));
			Seg.Add(P);
			FSlateDrawElement::MakeLines(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(), Seg, ESlateDrawEffect::None, FLinearColor(0.2f,0.8f,1.f,0.6f), true, 1.5f);
		}
	}
	return L;
}

void SInventoryGridWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	// Safety: if this grid isn't hovered and a drag is active, ensure its ghost is disabled
	FYIBagItem DragItem; UYIInventoryBag* SrcBag=nullptr;
	UWorld* ContextWorld = OwnerWidget.IsValid() ? OwnerWidget.Pin()->GetWorld() : nullptr;
	const bool bDragActive = UInventoryGridWidget::GetActiveDraggedItem(DragItem, SrcBag, ContextWorld);
	if (!bDragActive)
	{
		if (bGhostActive)
		{
			bGhostActive = false;
			Invalidate(EInvalidateWidgetReason::Paint);
		}
		return;
	}
	// If we are dragging, check current cursor against our bounds each frame (covers cases when OnMouseLeave didn't fire)
	const FVector2D ScreenPos = FSlateApplication::Get().GetCursorPos();
	const FVector2D Local = AllottedGeometry.AbsoluteToLocal(ScreenPos);
	const FVector2D VisualSize = Bag.IsValid() ? FVector2D(Bag->GridSize) * CellSize : FVector2D::ZeroVector;
	const bool bInside = (Local.X >= 0.f && Local.Y >= 0.f && Local.X < VisualSize.X && Local.Y < VisualSize.Y);
	if (!bInside && bGhostActive)
	{
		bGhostActive = false;
		Invalidate(EInvalidateWidgetReason::Paint);
	}
}

FVector2D SInventoryGridWidget::ComputeDesiredSize(float) const
{
	if (!Bag.IsValid()) return FVector2D::Zero();
	return FVector2D(Bag->GridSize) * CellSize;
}

FReply SInventoryGridWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Always sync ghost to current global drag (covers displacement)
	FYIBagItem DragItem;
	UYIInventoryBag* SrcBag = nullptr;
	UWorld* ContextWorld = OwnerWidget.IsValid() ? OwnerWidget.Pin()->GetWorld() : nullptr;
	const bool bDragActive = UInventoryGridWidget::GetActiveDraggedItem(DragItem, SrcBag, ContextWorld);

	if (bDragActive)
	{
		const FIntPoint Eff = Bag.IsValid() ? Bag->GetEffectiveSize(DragItem.Size) : DragItem.Size;
		if (Eff != GhostFootprint)
		{
			GhostFootprint = Eff;
			GhostSize = FVector2D(GhostFootprint) * CellSize;
		}
		UYIItemDefinition* Def = DragItem.Item.Definition.IsValid() ? DragItem.Item.Definition.Get() : DragItem.Item.Definition.LoadSynchronous();
		UTexture2D* Icon = Def && Def->Icon.IsValid() ? Def->Icon.Get() : (Def ? Def->Icon.LoadSynchronous() : nullptr);
		if (Icon)
		{
			GhostBrush.SetResourceObject(Icon);
			GhostBrush.ImageSize = GhostSize;
			bGhostHasIcon = true;
		}
		else
		{
			GhostBrush = *FAppStyle::GetBrush("WhiteBrush");
			GhostBrush.TintColor = FSlateColor(FLinearColor(1,1,1,0.25f));
			GhostBrush.ImageSize = GhostSize;
			bGhostHasIcon = false;
		}
		GhostIgnoreIndex = INDEX_NONE;
	}

	if (bGhostActive || bDragActive)
	{
		if (!bDragActive)
		{
			bGhostActive = false;
		}
		else
		{
			// We only render the ghost inside this grid's bounds. If cursor is outside, disable the ghost for this grid.
			const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
			const FVector2D VisualSize = Bag.IsValid() ? FVector2D(Bag->GridSize) * CellSize : FVector2D::ZeroVector;
			const bool bInside = (Local.X >= 0.f && Local.Y >= 0.f && Local.X < VisualSize.X && Local.Y < VisualSize.Y);
			if (bInside)
			{
				GhostCursorLocal = Local;
				// Use configured cell pixel size for placement
				UpdateGhostPlacement(GhostCursorLocal, CellSize);
				bGhostActive = true;
			}
			else
			{
				bGhostActive = false;
			}
		}
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	if (!bEnableCellHover)
	{
		return bGhostActive ? FReply::Handled() : FReply::Unhandled();
	}

	if (Bag.IsValid())
	{
		const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		const FVector2D VisualSize = FVector2D(Bag->GridSize) * CellSize;
		// If visual size is empty, clear hover and ignore
		if (VisualSize.X <= 0.f || VisualSize.Y <= 0.f)
		{
			if (HoverCell != FIntPoint(-1,-1))
			{
				HoverCell = FIntPoint(-1,-1);
				UpdateHoverSelection();
				Invalidate(EInvalidateWidgetReason::Paint);
			}
			bGhostActive = false;
			return FReply::Unhandled();
		}
		// If outside visual grid area, clear hover and ignore
		if (Local.X < 0.f || Local.Y < 0.f || Local.X >= VisualSize.X || Local.Y >= VisualSize.Y)
		{
			if (HoverCell != FIntPoint(-1,-1))
			{
				HoverCell = FIntPoint(-1,-1);
				UpdateHoverSelection();
				Invalidate(EInvalidateWidgetReason::Paint);
			}
			bGhostActive = false;
			return FReply::Unhandled();
		}
		const FIntPoint NewCell = ToCell(Local);
		if (NewCell != HoverCell)
		{
			HoverCell = NewCell;
			UpdateHoverSelection();
			Invalidate(EInvalidateWidgetReason::Paint);
		}
	}
	return bGhostActive ? FReply::Handled() : FReply::Unhandled();
}

void SInventoryGridWidget::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	// Clear hover and disable ghost when the cursor leaves this grid entirely
	bool bInvalidate = false;
	if (bEnableCellHover && HoverCell != FIntPoint(-1,-1))
	{
		HoverCell = FIntPoint(-1,-1);
		UpdateHoverSelection();
		bInvalidate = true;
	}
	if (bGhostActive)
	{
		bGhostActive = false;
		bInvalidate = true;
	}
	if (bInvalidate)
	{
		Invalidate(EInvalidateWidgetReason::Paint);
	}
	SCompoundWidget::OnMouseLeave(MouseEvent);
}

void SInventoryGridWidget::UpdateGhostPlacement(const FVector2D& LocalCursor, const FVector2D& LocalCell)
{
	if (!Bag.IsValid())
	{
		bGhostPlacementValid = false;
		GhostOverlapIndex = INDEX_NONE;
		return;
	}

	// Compute top-left cell by centering the footprint on the cursor, then rounding to nearest cell
	const FVector2D CellPos = FVector2D(LocalCursor.X / LocalCell.X, LocalCursor.Y / LocalCell.Y);
	const FVector2D TopLeftCellF = CellPos - FVector2D((float)GhostFootprint.X, (float)GhostFootprint.Y) * 0.5f;
	const FIntPoint Candidate(
		FMath::RoundToInt(TopLeftCellF.X),
		FMath::RoundToInt(TopLeftCellF.Y)
	);
	GhostTopLeft = Candidate;
	int32 Overlap = INDEX_NONE;
	bGhostPlacementValid = EvaluateGhostPlacement(Candidate, Overlap);
	GhostOverlapIndex = Overlap;
}

bool SInventoryGridWidget::EvaluateGhostPlacement(const FIntPoint& TopLeft, int32& OutOverlapIdx) const
{
	OutOverlapIdx = INDEX_NONE;
	if (!Bag.IsValid()) return false;
	if (GhostFootprint.X <= 0 || GhostFootprint.Y <= 0) return false;
	// bounds
	if (TopLeft.X < 0 || TopLeft.Y < 0 || TopLeft.X + GhostFootprint.X > Bag->GridSize.X || TopLeft.Y + GhostFootprint.Y > Bag->GridSize.Y)
	{
		return false;
	}
	// Scan footprint for occupants, allowing at most one distinct item
	for (int32 y = 0; y < GhostFootprint.Y; ++y)
	{
		for (int32 x = 0; x < GhostFootprint.X; ++x)
		{
			const int GX = TopLeft.X + x;
			const int GY = TopLeft.Y + y;
			const int32 Idx = CellToItemIndex.IsValidIndex(GY * Bag->GridSize.X + GX) ? CellToItemIndex[GY * Bag->GridSize.X + GX] : INDEX_NONE;
			if (Idx != INDEX_NONE && Idx != GhostIgnoreIndex)
			{
				if (OutOverlapIdx == INDEX_NONE) { OutOverlapIdx = Idx; }
				else if (OutOverlapIdx != Idx) { return false; } // multiple different items overlap
			}
		}
	}
	return true;
}
FReply SInventoryGridWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (Bag.IsValid() && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		const FVector2D LocalSize = MyGeometry.GetLocalSize();
		if (Local.X < 0.f || Local.Y < 0.f || Local.X >= LocalSize.X || Local.Y >= LocalSize.Y) return FReply::Unhandled();
		// If we're dragging but the footprint is invalid, ignore click
		if (bGhostActive && !bGhostPlacementValid)
		{
			return FReply::Handled();
		}
		// Determine exact drop cell: respect highlighted footprint top-left when dragging
		FIntPoint DropCell;
		if (bGhostActive && bGhostPlacementValid)
		{
			DropCell = GhostTopLeft;
		}
		else
		{
			DropCell = ToCell(Local);
		}
		// Update selection to reflect the drop target if mouse selection is enabled
		if (bEnableMouseSelection)
		{
			SelectedCell = DropCell;
			if (OnSelectedCellChanged.IsBound()) { OnSelectedCellChanged.Execute(SelectedCell); }
		}
		UWorld* ContextWorld = OwnerWidget.IsValid() ? OwnerWidget.Pin()->GetWorld() : nullptr;
		const bool bWasDragging = UInventoryGridWidget::IsItemDragActive(ContextWorld);
		// Explicit click callback (used for pick-up / drop behaviour). This will drop if a drag is already active.
		if (OnCellClicked.IsBound()) { OnCellClicked.Execute(DropCell); }

		// If we were dragging and the drop succeeded, the global drag flag will now be false. Clear ghost and exit.
		if (bWasDragging && UInventoryGridWidget::IsItemDragActive(ContextWorld) == false)
		{
			bGhostActive = false;
			Invalidate(EInvalidateWidgetReason::Paint);
			return FReply::Handled();
		}

		// If a drag is still active (e.g., we picked up a displaced item), do not start another drag
		if (UInventoryGridWidget::IsItemDragActive(ContextWorld))
		{
			return FReply::Handled();
		}

		// No active drag: start one immediately on click (no hold)
		if (OwnerWidget.IsValid())
		{
			const int32 Idx = GetItemIndexAtCell(DropCell);
if (Idx != INDEX_NONE)
{
const FYIBagItem CachedItem = Bag->Items[Idx];
if (OwnerWidget->BeginDragFromCell(DropCell))
				{
					// Build ghost visual using the cached item copy (item was removed from bag on pickup)
					const FYIBagItem& Item = CachedItem;
					const FIntPoint Eff = Bag->GetEffectiveSize(Item.Size);
					GhostSize = FVector2D(Eff) * CellSize;
					GhostFootprint = Eff;
					UYIItemDefinition* Def = Item.Item.Definition.IsValid() ? Item.Item.Definition.Get() : Item.Item.Definition.LoadSynchronous();
					UTexture2D* Icon = Def && Def->Icon.IsValid() ? Def->Icon.Get() : (Def ? Def->Icon.LoadSynchronous() : nullptr);
					if (Icon)
					{
						GhostBrush.SetResourceObject(Icon);
						GhostBrush.ImageSize = GhostSize;
						bGhostHasIcon = true;
					}
					else
					{
						GhostBrush = *FAppStyle::GetBrush("WhiteBrush");
					GhostBrush.TintColor = FSlateColor(FLinearColor(1,1,1,0.25f));
					GhostBrush.ImageSize = GhostSize;
					bGhostHasIcon = false;
				}
					GhostIgnoreIndex = INDEX_NONE;
					GhostCursorLocal = Local;
				UpdateGhostPlacement(Local, CellSize);
					bGhostActive = true;
					Invalidate(EInvalidateWidgetReason::Paint);
				}
			}
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
} 

bool SInventoryGridWidget::MoveSelection(const FIntPoint& Delta)
{
	if (!Bag.IsValid()) return false;
	int32 MaxX = Bag->GridSize.X;
	int32 MaxY = Bag->GridSize.Y;
	FIntPoint New = SelectedCell + Delta;

	if (bWrapNavigation && MaxX > 0 && MaxY > 0)
	{
		// Positive modulo for wrap-around
		New.X = ((New.X % MaxX) + MaxX) % MaxX;
		New.Y = ((New.Y % MaxY) + MaxY) % MaxY;
	}
	else
	{
		// Clamp to valid range (safeguard against zero or negative grid sizes)
		New.X = FMath::Clamp(New.X, 0, FMath::Max(0, MaxX - 1));
		New.Y = FMath::Clamp(New.Y, 0, FMath::Max(0, MaxY - 1));
	}

	if (New != SelectedCell)
	{
		SelectedCell = New;
		if (bWholeItemSelection)
		{
			// Mirror hover behavior for selection: select whole item if selection intersects one
			const int32 Idx = GetItemIndexAtCell(SelectedCell);
			if (Idx != INDEX_NONE)
			{
				HoveredItemIndex = Idx;
				const auto& It = Bag->Items[Idx];
				HoveredItemTopLeft = It.Pos;
				HoveredItemSize = Bag->GetEffectiveSize(It.Size);
			}
			else
			{
				HoveredItemIndex = INDEX_NONE;
				HoveredItemTopLeft = FIntPoint(-1,-1);
				HoveredItemSize = FIntPoint::ZeroValue;
			}
		}
		// Notify owner about the new selected cell
		if (OnSelectedCellChanged.IsBound()) { OnSelectedCellChanged.Execute(SelectedCell); }
		Invalidate(EInvalidateWidgetReason::Paint);
		return true;
	}
	return false;
}

#pragma optimize("", on)
