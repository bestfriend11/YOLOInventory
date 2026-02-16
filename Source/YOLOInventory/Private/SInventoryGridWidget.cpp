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
#include "YIInventoryGridStyleAsset.h"

#pragma optimize("", off)

static void YI_DrawBrushSlot(
	FSlateWindowElementList& OutDrawElements,
	int32& LayerId,
	const FGeometry& AllottedGeometry,
	const FVector2D& Position,
	const FVector2D& Size,
	const FYIGridStyleBrushSlot& Slot,
	const FSlateBrush* FallbackBrush)
{
	if (!Slot.bEnabled || Size.X <= 0.f || Size.Y <= 0.f)
	{
		return;
	}

	const bool bHasBrushData = Slot.Brush.DrawAs != ESlateBrushDrawType::NoDrawType;
	const FSlateBrush* Brush = bHasBrushData ? &Slot.Brush : FallbackBrush;
	if (!Brush)
	{
		return;
	}

	FSlateDrawElement::MakeBox(
		OutDrawElements,
		++LayerId,
		AllottedGeometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Position))),
		Brush,
		ESlateDrawEffect::None,
		Slot.Tint);
}

static void YI_DrawRectOutline(
	FSlateWindowElementList& OutDrawElements,
	int32& LayerId,
	const FGeometry& AllottedGeometry,
	const FVector2D& Position,
	const FVector2D& Size,
	const FLinearColor& Color,
	float Thickness)
{
	TArray<FVector2D> Seg;
	Seg.Add(Position);
	Seg.Add(Position + FVector2D(Size.X, 0.f));
	Seg.Add(Position + FVector2D(Size.X, Size.Y));
	Seg.Add(Position + FVector2D(0.f, Size.Y));
	Seg.Add(Position);
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		++LayerId,
		AllottedGeometry.ToPaintGeometry(),
		Seg,
		ESlateDrawEffect::None,
		Color,
		true,
		FMath::Max(0.5f, Thickness));
}

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
	if (LastHoverCell != HoverCell)
	{
		LastHoverCell = HoverCell;
		if (OnHoveredCellChanged.IsBound()) { OnHoveredCellChanged.Execute(HoverCell); }
	}

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
	OnHoveredCellChanged = InArgs._OnHoveredCellChanged;
	OnSelectedCellChanged = InArgs._OnSelectedCellChanged;
	OnGhostPlacementChanged = InArgs._OnGhostPlacementChanged;
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
	const UInventoryGridWidget* Owner = OwnerWidget.Get();
	const UYIInventoryGridStyleAsset* GridStyle = Owner ? Owner->GetResolvedGridStyleAsset() : nullptr;
	const FSlateBrush* Box = FAppStyle::Get().GetBrush("WhiteBrush");

	// Use explicit pixel cell size as configured (do not stretch to allotted geometry)
	const FVector2D LocalCell = CellSize;
	const FVector2D SizePix = FVector2D(Bag->GridSize) * LocalCell;
	const float GridLineThickness = FMath::Max(0.5f, GridStyle ? GridStyle->GridLineThickness : Bag->GridThickness);
	const FLinearColor GridLineColor = GridStyle ? GridStyle->GridLineColor : Bag->GridLineColor;
	const FLinearColor OuterLineColor = GridStyle ? GridStyle->GridLineColor : Bag->OuterLineColor;
	const FLinearColor CellFillColor = GridStyle ? GridStyle->CellFill.Tint : Bag->CellBgColor;
	const bool bShowItemIcons = Bag->bEnableThumbnails && (!GridStyle || GridStyle->bDrawItemIcon);

	// Grid background / cell fill
	if (GridStyle && GridStyle->CellFill.bEnabled)
	{
		YI_DrawBrushSlot(OutDrawElements, L, AllottedGeometry, FVector2D::ZeroVector, SizePix, GridStyle->CellFill, Box);
	}
	else
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			++L,
			AllottedGeometry.ToPaintGeometry(FVector2f(SizePix), FSlateLayoutTransform()),
			Box,
			ESlateDrawEffect::None,
			CellFillColor);
	}

	// Grid lines
	const float Half = 0.5f;
	const float MaxX = SizePix.X - Half;
	const float MaxY = SizePix.Y - Half;
	for (int x = 0; x <= Bag->GridSize.X; ++x)
	{
		const float X = x * LocalCell.X + Half;
		TArray<FVector2D> Seg = { FVector2D(X, Half), FVector2D(X, MaxY) };
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			++L,
			AllottedGeometry.ToPaintGeometry(),
			Seg,
			ESlateDrawEffect::None,
			GridLineColor,
			false,
			GridLineThickness);
	}
	for (int y = 0; y <= Bag->GridSize.Y; ++y)
	{
		const float Y = y * LocalCell.Y + Half;
		TArray<FVector2D> Seg = { FVector2D(Half, Y), FVector2D(MaxX, Y) };
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			++L,
			AllottedGeometry.ToPaintGeometry(),
			Seg,
			ESlateDrawEffect::None,
			GridLineColor,
			false,
			GridLineThickness);
	}

	// Optional outer border style; fallback to bag outer line color.
	if (GridStyle && GridStyle->OuterBorder.bEnabled)
	{
		YI_DrawBrushSlot(OutDrawElements, L, AllottedGeometry, FVector2D::ZeroVector, SizePix, GridStyle->OuterBorder, Box);
	}
	else
	{
		YI_DrawRectOutline(OutDrawElements, L, AllottedGeometry, FVector2D::ZeroVector, SizePix, OuterLineColor, GridLineThickness);
	}

	auto DrawItemIcon = [&](UTexture2D* IconTex, const FVector2D& ItemPos, const FVector2D& ItemSize)
	{
		if (!IconTex || !bShowItemIcons)
		{
			return;
		}

		FVector2D DrawPos = ItemPos;
		FVector2D DrawSize = ItemSize;
		if (GridStyle)
		{
			const FMargin Pad = GridStyle->ItemIconPadding;
			DrawPos.X += Pad.Left;
			DrawPos.Y += Pad.Top;
			DrawSize.X -= (Pad.Left + Pad.Right);
			DrawSize.Y -= (Pad.Top + Pad.Bottom);
		}

		if (DrawSize.X <= 0.f || DrawSize.Y <= 0.f)
		{
			return;
		}

		if (GridStyle && !GridStyle->bStretchItemIconToBounds)
		{
			const float SourceW = FMath::Max(1.f, static_cast<float>(IconTex->GetSizeX()));
			const float SourceH = FMath::Max(1.f, static_cast<float>(IconTex->GetSizeY()));
			const float Scale = FMath::Min(DrawSize.X / SourceW, DrawSize.Y / SourceH);
			const FVector2D FitSize(SourceW * Scale, SourceH * Scale);
			DrawPos += (DrawSize - FitSize) * 0.5f;
			DrawSize = FitSize;
		}

		FSlateBrush IconBrush;
		IconBrush.SetResourceObject(IconTex);
		IconBrush.ImageSize = FVector2D(IconTex->GetSizeX(), IconTex->GetSizeY());
		IconBrush.DrawAs = ESlateBrushDrawType::Image;
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			++L,
			AllottedGeometry.ToPaintGeometry(FVector2f(DrawSize), FSlateLayoutTransform(FVector2f(DrawPos))),
			&IconBrush,
			ESlateDrawEffect::None,
			GridStyle ? GridStyle->ItemIconTint : FLinearColor::White);
	};

	// Items
	for (int32 i = 0; i < Bag->Items.Num(); ++i)
	{
		const FYIBagItem& It = Bag->Items[i];
		const bool bLocked = Owner && Owner->IsItemIndexLockedForUI(i);
		const FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
		const FVector2D P = FVector2D(It.Pos) * LocalCell;
		const FVector2D S = FVector2D(Eff) * LocalCell;

		UYIItemDefinition* Def = It.Item.Definition.IsValid() ? It.Item.Definition.Get() : It.Item.Definition.LoadSynchronous();
		UTexture2D* IconTex = bShowItemIcons && Def ? (Def->Icon.IsValid() ? Def->Icon.Get() : Def->Icon.LoadSynchronous()) : nullptr;

		if (GridStyle && GridStyle->ItemFill.bEnabled)
		{
			YI_DrawBrushSlot(OutDrawElements, L, AllottedGeometry, P, S, GridStyle->ItemFill, Box);
		}
		else
		{
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				++L,
				AllottedGeometry.ToPaintGeometry(FVector2f(S), FSlateLayoutTransform(FVector2f(P))),
				Box,
				ESlateDrawEffect::None,
				FLinearColor(1.f, 1.f, 1.f, 0.08f));
		}

		DrawItemIcon(IconTex, P, S);

		if (GridStyle && GridStyle->ItemFrame.bEnabled)
		{
			YI_DrawBrushSlot(OutDrawElements, L, AllottedGeometry, P, S, GridStyle->ItemFrame, Box);
		}
		else
		{
			YI_DrawRectOutline(OutDrawElements, L, AllottedGeometry, P, S, GridStyle ? GridStyle->ItemBorderColor : FLinearColor(0.8f, 0.8f, 0.8f, 0.65f), GridLineThickness);
		}

		if (It.Item.Count > 1)
		{
			const FString CountStr = FString::FromInt(It.Item.Count);
			const int32 StackFontSize = GridStyle ? GridStyle->StackCountFontSize : 10;
			const FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle("Bold", StackFontSize);
			const FVector2D TextSize = FVector2D(40.f, 14.f);
			const FVector2D TP = P + S - TextSize - FVector2D(2.f, 2.f);
			FSlateDrawElement::MakeText(
				OutDrawElements,
				++L,
				AllottedGeometry.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(FVector2f(TP))),
				FText::FromString(CountStr),
				Font,
				ESlateDrawEffect::None,
				GridStyle ? GridStyle->StackCountColor : FLinearColor::White);
		}

		if (bLocked)
		{
			if (GridStyle && GridStyle->LockedItemOverlay.bEnabled)
			{
				YI_DrawBrushSlot(OutDrawElements, L, AllottedGeometry, P, S, GridStyle->LockedItemOverlay, Box);
			}
			else
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					++L,
					AllottedGeometry.ToPaintGeometry(FVector2f(S), FSlateLayoutTransform(FVector2f(P))),
					Box,
					ESlateDrawEffect::None,
					FLinearColor(0.16f, 0.42f, 0.9f, 0.24f));
			}

			const FText LockText = GridStyle ? GridStyle->LockedLabel : NSLOCTEXT("YOLOInventory", "Grid_LockedBadge", "EQUIPPED");
			const int32 LockFontSize = GridStyle ? GridStyle->LockedLabelFontSize : 9;
			const FSlateFontInfo LockFont = FCoreStyle::GetDefaultFontStyle("Bold", LockFontSize);
			const FVector2D LockTextSize(96.f, 14.f);
			const FVector2D LockPos = P + FVector2D(2.f, 2.f);
			FSlateDrawElement::MakeText(
				OutDrawElements,
				++L,
				AllottedGeometry.ToPaintGeometry(FVector2f(LockTextSize), FSlateLayoutTransform(FVector2f(LockPos))),
				LockText,
				LockFont,
				ESlateDrawEffect::None,
				GridStyle ? GridStyle->LockedLabelColor : FLinearColor(0.95f, 0.98f, 1.f, 0.95f));
		}
	}
	// Hover highlight: only when not actively dragging a ghost, to avoid visual conflict
	if (!bGhostActive && bEnableCellHover)
	{
		if (bWholeItemHover && HoveredItemIndex != INDEX_NONE)
		{
			const FVector2D P = FVector2D(HoveredItemTopLeft) * LocalCell;
			const FVector2D S = FVector2D(HoveredItemSize) * LocalCell;
			if (GridStyle && GridStyle->HoveredItemOverlay.bEnabled)
			{
				YI_DrawBrushSlot(OutDrawElements, L, AllottedGeometry, P, S, GridStyle->HoveredItemOverlay, Box);
			}
			else
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					++L,
					AllottedGeometry.ToPaintGeometry(FVector2f(S), FSlateLayoutTransform(FVector2f(P))),
					Box,
					ESlateDrawEffect::None,
					FLinearColor(0.1f, 0.8f, 0.2f, 0.12f));
			}
			// Re-draw item icon above the highlight so it stays visible
			if (Bag->Items.IsValidIndex(HoveredItemIndex))
			{
				const FYIBagItem& HoverItem = Bag->Items[HoveredItemIndex];
				UYIItemDefinition* HoverDef = HoverItem.Item.Definition.IsValid() ? HoverItem.Item.Definition.Get() : HoverItem.Item.Definition.LoadSynchronous();
				UTexture2D* HoverIcon = bShowItemIcons && HoverDef ? (HoverDef->Icon.IsValid() ? HoverDef->Icon.Get() : HoverDef->Icon.LoadSynchronous()) : nullptr;
				DrawItemIcon(HoverIcon, P, S);
			}
		}
		else
		{
			const FVector2D P = FVector2D(HoverCell) * LocalCell;
			if (GridStyle && GridStyle->HoveredCellOverlay.bEnabled)
			{
				YI_DrawBrushSlot(OutDrawElements, L, AllottedGeometry, P, LocalCell, GridStyle->HoveredCellOverlay, Box);
			}
			else
			{
				FSlateDrawElement::MakeBox(
					OutDrawElements,
					++L,
					AllottedGeometry.ToPaintGeometry(FVector2f(LocalCell), FSlateLayoutTransform(FVector2f(P))),
					Box,
					ESlateDrawEffect::None,
					FLinearColor(0.1f, 0.6f, 1.f, 0.08f));
			}
		}
	}
	// Selected cell cursor (thicker outline)
	if (SelectedCell.X >= 0 && SelectedCell.Y >= 0)
	{
		const FVector2D P = FVector2D(SelectedCell) * LocalCell;
		if (GridStyle && GridStyle->SelectedCellOverlay.bEnabled)
		{
			YI_DrawBrushSlot(OutDrawElements, L, AllottedGeometry, P, LocalCell, GridStyle->SelectedCellOverlay, Box);
		}
		YI_DrawRectOutline(
			OutDrawElements,
			L,
			AllottedGeometry,
			P,
			LocalCell,
			GridStyle ? GridStyle->SelectedCellOutlineColor : FLinearColor(0.2f, 0.6f, 1.f, 0.9f),
			GridStyle ? GridStyle->SelectedCellOutlineThickness : 2.5f);
	}
	// Ghost visual (click-to-drag without holding)
	if (bGhostActive)
	{
		// Always draw the footprint highlight so users can see valid/invalid placement while dragging,
		// even when a global ghost renderer is used.
		const FVector2D FootP = FVector2D(GhostTopLeft) * LocalCell;
		const FVector2D FootS = FVector2D(GhostFootprint) * LocalCell;
		const FYIGridStyleBrushSlot* GhostSlot = nullptr;
		if (GridStyle)
		{
			GhostSlot = bGhostPlacementValid ? &GridStyle->GhostPlacementValidOverlay : &GridStyle->GhostPlacementInvalidOverlay;
		}
		if (GhostSlot && GhostSlot->bEnabled)
		{
			YI_DrawBrushSlot(OutDrawElements, L, AllottedGeometry, FootP, FootS, *GhostSlot, Box);
		}
		else
		{
			const FLinearColor GhostTint = bGhostPlacementValid ? FLinearColor(0.2f, 0.8f, 0.2f, 0.18f) : FLinearColor(0.8f, 0.2f, 0.2f, 0.18f);
			FSlateDrawElement::MakeBox(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(FVector2f(FootS), FSlateLayoutTransform(FVector2f(FootP))), Box, ESlateDrawEffect::None, GhostTint);
		}

		// Only draw the ghost icon locally if we are not using a global overlay ghost.
		if (!bUseGlobalDragGhost)
		{
			const FVector2D P = GhostCursorLocal - (GhostSize * 0.5f);
			const FSlateBrush* BrushToUse = bGhostHasIcon ? &GhostBrush : FAppStyle::Get().GetBrush("WhiteBrush");
			const FLinearColor Tint = bGhostHasIcon
				? (GridStyle ? GridStyle->GhostIconTint : FLinearColor(1.f, 1.f, 1.f, 0.9f))
				: FLinearColor(1.f, 1.f, 1.f, 0.18f);
			FSlateDrawElement::MakeBox(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(FVector2f(GhostSize), FSlateLayoutTransform(FVector2f(P))), BrushToUse, ESlateDrawEffect::None, Tint);
			// Outline for visibility
			YI_DrawRectOutline(
				OutDrawElements,
				L,
				AllottedGeometry,
				P,
				GhostSize,
				GridStyle ? GridStyle->GhostOutlineColor : FLinearColor(0.2f, 0.8f, 1.f, 0.6f),
				GridStyle ? GridStyle->GhostOutlineThickness : 1.5f);
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
		bGhostOutOfBounds = false;
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
	// bounds
	bGhostOutOfBounds = (Candidate.X < 0 || Candidate.Y < 0 || Candidate.X + GhostFootprint.X > Bag->GridSize.X || Candidate.Y + GhostFootprint.Y > Bag->GridSize.Y);
	bGhostPlacementValid = EvaluateGhostPlacement(Candidate, Overlap);
	GhostOverlapIndex = Overlap;
	if (LastGhostTopLeft != GhostTopLeft || bLastGhostValid != bGhostPlacementValid || bLastGhostOutOfBounds != bGhostOutOfBounds)
	{
		LastGhostTopLeft = GhostTopLeft;
		bLastGhostValid = bGhostPlacementValid;
		bLastGhostOutOfBounds = bGhostOutOfBounds;
		if (OnGhostPlacementChanged.IsBound())
		{
			OnGhostPlacementChanged.Execute(GhostTopLeft, bGhostPlacementValid, bGhostOutOfBounds);
		}
	}
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
		const int32 ClickedItemIndex = GetItemIndexAtCell(DropCell);
		const bool bClickedLockedItem = (ClickedItemIndex != INDEX_NONE && OwnerWidget.IsValid() && OwnerWidget.Pin()->IsItemIndexLockedForUI(ClickedItemIndex));
		// Update selection to reflect the drop target if mouse selection is enabled
		if (bEnableMouseSelection)
		{
			SelectedCell = bClickedLockedItem ? FIntPoint(-1, -1) : DropCell;
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
		if (bClickedLockedItem)
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
