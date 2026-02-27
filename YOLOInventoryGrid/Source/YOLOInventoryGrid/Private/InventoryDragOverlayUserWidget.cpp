#include "InventoryDragOverlayUserWidget.h"
#include "InventoryGridWidget.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Slate/SlateBrushAsset.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/SlateRect.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "YIItemSchemaResolver.h"
#include "YOLOInventorySettings.h"
#include "YIInventoryGridStyleAsset.h"

static UTexture2D* YI_Overlay_TryResolveItemIconNoLoad(const FYIItemInstance& Item)
{
	UYIItemDefinition* Def = Item.Definition.Get();
	if (!Def)
	{
		return nullptr;
	}

	const TSoftObjectPtr<UTexture2D> EffectiveIcon = YIItemSchema::GetIcon(Def);
	return EffectiveIcon.Get();
}

UInventoryDragOverlayUserWidget::UInventoryDragOverlayUserWidget(const FObjectInitializer& OI)
	: Super(OI)
{
	bIsVolatile = true; // We draw at cursor; no need to cache

}

void UInventoryDragOverlayUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	FYIBagItem DragItem; UYIInventoryBag* SrcBag = nullptr;
	bShouldDraw = UInventoryGridWidget::GetActiveDraggedItem(DragItem, SrcBag, GetWorld());
	// Invalidate so we repaint; this widget is cheap
	InvalidateLayoutAndVolatility();
}

int32 UInventoryDragOverlayUserWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
	LayerId += FMath::Max(0, OverlayLayerBias);

	// Convert desktop cursor to viewport space first; this removes window-origin offsets
	// in PIE windowed mode and keeps drag ghost aligned after viewport resize/move.
	const FVector2D CursorDesktop = FSlateApplication::Get().GetCursorPos();
	FVector2D CursorViewportPixels = FVector2D::ZeroVector;
	FVector2D CursorViewportLocal = FVector2D::ZeroVector;
	USlateBlueprintLibrary::AbsoluteToViewport(this, CursorDesktop, CursorViewportPixels, CursorViewportLocal);
	CachedCursorLocal = CursorViewportLocal;
	CachedCursorSS = AllottedGeometry.LocalToAbsolute(CachedCursorLocal);

	// Build a debug string for on-screen display every frame
	const UYOLOInventorySettings& Settings = UYOLOInventorySettings::Get();
	const bool bShowDebug = Settings.bShowDebug
		&& Settings.bEnableDebugPipeline
		&& Settings.bDebugOutputToScreen
		&& Settings.IsDebugChannelEnabled(EYIDebugChannel::Grid);
	const bool bSnapDragVisualsToGrid = Settings.bSnapDragVisualsToGrid;
	FYIBagItem LiveDragItem; UYIInventoryBag* LiveSourceBag = nullptr;
	const bool bHasLiveDrag = UInventoryGridWidget::GetActiveDraggedItem(LiveDragItem, LiveSourceBag, GetWorld());
	FIntPoint LiveAnchorOffset = FIntPoint::ZeroValue;
	if (bHasLiveDrag)
	{
		UInventoryGridWidget::GetActiveDraggedItemAnchor(LiveAnchorOffset, GetWorld());
	}
	bShouldDraw = bHasLiveDrag;
	FString Debug;
	auto V2 = [](const FVector2D& V){ return FString::Printf(TEXT("(%.1f, %.1f)"), V.X, V.Y); };
	auto V2i = [](const FIntPoint& P){ return FString::Printf(TEXT("(%d, %d)"), P.X, P.Y); };
	auto DescribeItem = [V2i](const FYIBagItem& Item)
	{
		FString Name = TEXT("<none>");
		if (!Item.Item.Definition.IsNull())
		{
			if (const UYIItemDefinition* Def = Item.Item.Definition.Get())
			{
				Name = YIItemSchema::GetDisplayName(Def).ToString();
			}
			else
			{
				Name = Item.Item.Definition.ToSoftObjectPath().GetAssetName();
			}
		}
		return FString::Printf(TEXT("%s x%d Size%s Rotated:%s StackKey:%lld"),
			*Name,
			Item.Item.Count,
			*V2i(Item.Size),
			Item.Item.bRotated ? TEXT("Y") : TEXT("N"),
			Item.Item.CustomStackKey);
	};

	if (bShowDebug)
	{
		Debug += FString::Printf(TEXT("bShouldDraw: %s\n"), bShouldDraw ? TEXT("true") : TEXT("false"));
		Debug += FString::Printf(TEXT("CachedCursorSS: %s\n"), *V2(CachedCursorSS));
		Debug += FString::Printf(TEXT("Allotted LocalSize: %s\n"), *V2(AllottedGeometry.GetLocalSize()));
		if (bHasLiveDrag)
		{
			const FString BagName = LiveSourceBag && !LiveSourceBag->DisplayName.IsEmpty()
				? LiveSourceBag->DisplayName.ToString()
				: (LiveSourceBag ? LiveSourceBag->GetName() : TEXT("<none>"));
			Debug += FString::Printf(TEXT("Dragging: %s (SourceBag: %s)\n"), *DescribeItem(LiveDragItem), *BagName);
		}
	}

	if (!bShouldDraw)
	{
		if (bShowDebug && GEngine)
		{
			static const int32 MsgKey = 0x99110001;
			GEngine->AddOnScreenDebugMessage(MsgKey, 0.f, FColor::Yellow, Debug);
		}
		return LayerId;
	}

	// Resolve hovered/source grids first so global ghost can match in-grid ghost size/placement.
	UInventoryGridWidget* HoveredGrid = nullptr;
	UInventoryGridWidget* SourceGrid = nullptr;
	FString HoverPickInfo;
	UInventoryGridWidget::ForEachRegisteredGrid([&](UInventoryGridWidget* Grid)
	{
		if (!Grid || !Grid->Bag)
		{
			return;
		}

		if (!HoveredGrid)
		{
			const FGeometry GridGeomT = Grid->GetCachedGeometry();
			const FSlateRect AbsRect = GridGeomT.GetLayoutBoundingRect();
			const bool bHit = AbsRect.ContainsPoint(CachedCursorSS);
			if (bShowDebug)
			{
				HoverPickInfo += FString::Printf(TEXT("Grid %s AbsRect: (%.1f,%.1f)-(%.1f,%.1f) hit:%s\n"), *Grid->GetName(), AbsRect.Left, AbsRect.Top, AbsRect.Right, AbsRect.Bottom, bHit ? TEXT("Y") : TEXT("N"));
			}
			if (bHit)
			{
				HoveredGrid = Grid;
			}
		}

		if (!SourceGrid && LiveSourceBag && Grid->Bag == LiveSourceBag)
		{
			SourceGrid = Grid;
		}
	});

	UInventoryGridWidget* GhostScaleGrid = HoveredGrid ? HoveredGrid : SourceGrid;
	const UYIInventoryGridStyleAsset* GhostStyle = GhostScaleGrid ? GhostScaleGrid->GetResolvedGridStyleAsset() : nullptr;
	float GhostCellPx = (GhostScaleGrid && GhostScaleGrid->GetCellPixelSize() > 1.f)
		? GhostScaleGrid->GetCellPixelSize()
		: FMath::Max(1.f, FallbackGhostSize.X);
	FIntPoint GhostFootprint = LiveDragItem.Size;
	if (GhostScaleGrid && GhostScaleGrid->Bag)
	{
		GhostFootprint = GhostScaleGrid->Bag->GetEffectiveSize(LiveDragItem.Size);
	}
	else if (LiveSourceBag)
	{
		GhostFootprint = LiveSourceBag->GetEffectiveSize(LiveDragItem.Size);
	}
	GhostFootprint.X = FMath::Max(1, GhostFootprint.X);
	GhostFootprint.Y = FMath::Max(1, GhostFootprint.Y);

	// Cursor local to this overlay (resolved from desktop/viewport sources above).
	const FVector2D Local = CachedCursorLocal;
	// Prefer dragged item icon; fallback to simple white brush if icon is missing.
	const UTexture2D* DragIconTexture = nullptr;
	if (bHasLiveDrag)
	{
		DragIconTexture = YI_Overlay_TryResolveItemIconNoLoad(LiveDragItem.Item);
	}

	const FVector2D GhostSize = FVector2D(GhostFootprint) * GhostCellPx;
	const FVector2D AnchorPixelOffset(
		(static_cast<float>(LiveAnchorOffset.X) + 0.5f) * GhostCellPx,
		(static_cast<float>(LiveAnchorOffset.Y) + 0.5f) * GhostCellPx);
	const FIntPoint CursorCell(
		FMath::FloorToInt(Local.X / GhostCellPx),
		FMath::FloorToInt(Local.Y / GhostCellPx));
	const FIntPoint GhostTopLeftCell = CursorCell - LiveAnchorOffset;
	const FVector2D P = bSnapDragVisualsToGrid
		? FVector2D(GhostTopLeftCell) * GhostCellPx
		: (Local - AnchorPixelOffset);
	const FLinearColor Tint = DragIconTexture
		? (GhostStyle ? GhostStyle->GhostIconTint : FLinearColor(1.f, 1.f, 1.f, 0.90f))
		: FLinearColor(1.f, 1.f, 1.f, 0.18f);
	const FSlateBrush* GhostBrush = FAppStyle::Get().GetBrush("WhiteBrush");
	FSlateBrush IconBrush;
	if (DragIconTexture)
	{
		IconBrush.SetResourceObject(const_cast<UTexture2D*>(DragIconTexture));
		IconBrush.ImageSize = FVector2D(DragIconTexture->GetSizeX(), DragIconTexture->GetSizeY());
		IconBrush.DrawAs = ESlateBrushDrawType::Image;
		GhostBrush = &IconBrush;
	}

	if (bShowDebug)
	{
		Debug += FString::Printf(TEXT("Local(cursor in overlay): %s\n"), *V2(Local));
		Debug += FString::Printf(TEXT("GhostSize: %s  P(top-left): %s\n"), *V2(GhostSize), *V2(P));
		Debug += FString::Printf(TEXT("CursorCell: %s  Anchor: %s  GhostTopLeftCell: %s  Snap:%s\n"), *V2i(CursorCell), *V2i(LiveAnchorOffset), *V2i(GhostTopLeftCell), bSnapDragVisualsToGrid ? TEXT("Y") : TEXT("N"));
		Debug += FString::Printf(TEXT("GhostFootprint(cells): %s  CellPx: %.2f\n"), *V2i(GhostFootprint), GhostCellPx);
		Debug += FString::Printf(TEXT("ScaleGrid: %s (hover=%s source=%s)\n"),
			GhostScaleGrid ? *GhostScaleGrid->GetName() : TEXT("<none>"),
			HoveredGrid ? *HoveredGrid->GetName() : TEXT("<none>"),
			SourceGrid ? *SourceGrid->GetName() : TEXT("<none>"));
		Debug += FString::Printf(TEXT("LayerId(start): %d\n"), LayerId);
	}

	// Footprint highlight under cursor on hovered grid (draw first so drag ghost stays visually on top).
	auto DrawFootprintForGrid = [&](UInventoryGridWidget* Grid)
	{
		if (!Grid || !Grid->Bag) return;
		// Use arranged geometry for this frame and bounds-check against local size to avoid offset issues
		const FGeometry GridGeom = Grid->GetCachedGeometry();
		const FVector2D GridLocalFromScreen = GridGeom.AbsoluteToLocal(CachedCursorSS);
		const float CellPx = Grid->GetCellPixelSize();
		const FVector2D VisualSize = Grid->Bag ? FVector2D(Grid->Bag->GridSize) * CellPx : FVector2D::ZeroVector;
		const bool bInside = (VisualSize.X > 0.f && VisualSize.Y > 0.f && GridLocalFromScreen.X >= 0 && GridLocalFromScreen.Y >= 0 && GridLocalFromScreen.X < VisualSize.X && GridLocalFromScreen.Y < VisualSize.Y);

		if (bShowDebug)
		{
			Debug += FString::Printf(TEXT("-- Grid %s --\n"), *Grid->GetName());
			Debug += FString::Printf(TEXT("GridLocalFromScreen: %s  VisualSize(px): %s  Inside:%s\n"), *V2(GridLocalFromScreen), *V2(VisualSize), bInside?TEXT("Y"):TEXT("N"));
		}
		if (!bInside) return;
		// Compute candidate footprint using the same anchor-cell logic as the grid widget.
		const FIntPoint Foot = Grid->Bag->GetEffectiveSize(LiveDragItem.Size);
		const FVector2D FootS_Grid = FVector2D(Foot) * CellPx;
		const FIntPoint CursorCellGrid(
			FMath::FloorToInt(GridLocalFromScreen.X / CellPx),
			FMath::FloorToInt(GridLocalFromScreen.Y / CellPx));
		const FVector2D AnchorPixelOffsetGrid(
			(static_cast<float>(LiveAnchorOffset.X) + 0.5f) * CellPx,
			(static_cast<float>(LiveAnchorOffset.Y) + 0.5f) * CellPx);
		const FVector2D UnsnappedTopLeftGrid = GridLocalFromScreen - AnchorPixelOffsetGrid;
		const FIntPoint DesiredTopLeft = bSnapDragVisualsToGrid
			? (CursorCellGrid - LiveAnchorOffset)
			: FIntPoint(
				FMath::FloorToInt(UnsnappedTopLeftGrid.X / CellPx),
				FMath::FloorToInt(UnsnappedTopLeftGrid.Y / CellPx));

		int32 IgnoreIndex = INDEX_NONE;
		if (LiveSourceBag == Grid->Bag && LiveDragItem.Item.InstanceId.IsValid())
		{
			Grid->Bag->FindItemIndexByInstanceIdFast(LiveDragItem.Item.InstanceId, IgnoreIndex);
		}
		int32 OverlapIdx = INDEX_NONE;
		const bool bCan = Grid->Bag->FindSingleOverlapAt(DesiredTopLeft, LiveDragItem.Size, IgnoreIndex, OverlapIdx);

		const FIntPoint GridSize = Grid->Bag->GridSize;
		// Convert grid-local footprint rect to overlay local
		const FVector2D FootP_Grid = bSnapDragVisualsToGrid
			? FVector2D(DesiredTopLeft) * CellPx
			: UnsnappedTopLeftGrid;
		const FVector2D FootP_Abs = GridGeom.LocalToAbsolute(FootP_Grid);
		const FVector2D FootP_Overlay = AllottedGeometry.AbsoluteToLocal(FootP_Abs);

		if (bShowDebug)
		{
			Debug += FString::Printf(TEXT("DragItem.Size: %s  Foot(effective): %s  CellPx: %.2f\n"), *V2i(LiveDragItem.Size), *V2i(Foot), CellPx);
			Debug += FString::Printf(TEXT("CursorCell: %s  Anchor: %s  DesiredTopLeft: %s  GridSize: %s\n"), *V2i(CursorCellGrid), *V2i(LiveAnchorOffset), *V2i(DesiredTopLeft), *V2i(GridSize));
			Debug += FString::Printf(TEXT("IgnoreIndex: %d  OverlapIdx: %d\n"), IgnoreIndex, OverlapIdx);
			Debug += FString::Printf(TEXT("bCanPlace: %s\n"), bCan?TEXT("true"):TEXT("false"));
			Debug += FString::Printf(TEXT("FootP_Grid: %s  FootS_Grid: %s  FootP_Abs: %s  FootP_Overlay: %s\n"), *V2(FootP_Grid), *V2(FootS_Grid), *V2(FootP_Abs), *V2(FootP_Overlay));
		}

		const UYIInventoryGridStyleAsset* GridStyle = Grid->GetResolvedGridStyleAsset();
		const FYIGridStyleBrushSlot* OverlaySlot = GridStyle ? (bCan ? &GridStyle->GhostPlacementValidOverlay : &GridStyle->GhostPlacementInvalidOverlay) : nullptr;
		if (OverlaySlot && OverlaySlot->bEnabled)
		{
			const bool bHasBrushData = OverlaySlot->Brush.DrawAs != ESlateBrushDrawType::NoDrawType;
			const FSlateBrush* OverlayBrush = bHasBrushData ? &OverlaySlot->Brush : FAppStyle::Get().GetBrush("WhiteBrush");
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				++LayerId,
				AllottedGeometry.ToPaintGeometry(FVector2f(FootS_Grid), FSlateLayoutTransform(FVector2f(FootP_Overlay))),
				OverlayBrush,
				ESlateDrawEffect::None,
				OverlaySlot->Tint);
		}
		else
		{
			const FLinearColor GhostTint = bCan ? FLinearColor(0.2f, 0.8f, 0.2f, 0.18f) : FLinearColor(0.8f, 0.2f, 0.2f, 0.18f);
			FSlateDrawElement::MakeBox(OutDrawElements, ++LayerId, AllottedGeometry.ToPaintGeometry(FVector2f(FootS_Grid), FSlateLayoutTransform(FVector2f(FootP_Overlay))), FAppStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, GhostTint);
		}

		if (bShowDebug)
		{
			Debug += FString::Printf(TEXT("LayerId(after footprint): %d\n"), LayerId);
		}
	};

	// Draw placement shadow only on hovered grid (same as in-grid behavior).
	if (HoveredGrid)
	{
		DrawFootprintForGrid(HoveredGrid);
	}

	FSlateDrawElement::MakeBox(OutDrawElements, ++LayerId, AllottedGeometry.ToPaintGeometry(FVector2f(GhostSize), FSlateLayoutTransform(FVector2f(P))), GhostBrush, ESlateDrawEffect::None, Tint);
	TArray<FVector2D> Seg;
	Seg.Add(P);
	Seg.Add(P + FVector2D(GhostSize.X, 0));
	Seg.Add(P + FVector2D(GhostSize.X, GhostSize.Y));
	Seg.Add(P + FVector2D(0, GhostSize.Y));
	Seg.Add(P);
	FSlateDrawElement::MakeLines(
		OutDrawElements,
		++LayerId,
		AllottedGeometry.ToPaintGeometry(),
		Seg,
		ESlateDrawEffect::None,
		GhostStyle ? GhostStyle->GhostOutlineColor : FLinearColor(0.2f, 0.8f, 1.f, 0.6f),
		true,
		GhostStyle ? GhostStyle->GhostOutlineThickness : 1.5f);

	if (bShowDebug)
	{
		Debug += FString::Printf(TEXT("LayerId(after ghost): %d\n"), LayerId);
		Debug += HoverPickInfo;
		Debug += FString::Printf(TEXT("HoveredGrid: %s\n"), HoveredGrid ? *HoveredGrid->GetName() : TEXT("<none>"));
	}

	// Emit the debug string and a concise global drag summary so we can see what is being dragged
	if (bShowDebug && GEngine)
	{
		static const int32 MsgKey = 0x99110001;
		GEngine->AddOnScreenDebugMessage(MsgKey, 0.f, FColor::Yellow, Debug);
		if (bHasLiveDrag)
		{
			static const int32 DragMsgKey = 0x99110002;
			GEngine->AddOnScreenDebugMessage(DragMsgKey, 0.f, FColor::Cyan, FString::Printf(TEXT("Dragging: %s"), *DescribeItem(LiveDragItem)));
		}
	}
	return LayerId;
}
