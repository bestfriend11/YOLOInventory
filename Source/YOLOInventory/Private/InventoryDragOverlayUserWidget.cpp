#include "InventoryDragOverlayUserWidget.h"
#include "InventoryGridWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Slate/SlateBrushAsset.h"
#include "Rendering/DrawElements.h"
#include "Framework/Application/SlateApplication.h"
#include "Layout/SlateRect.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "YOLOInventorySettings.h"


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
	if (bShouldDraw && FSlateApplication::Get().GetActiveTopLevelWindow())
	{
		CachedCursorSS = FSlateApplication::Get().GetCursorPos() - FSlateApplication::Get().GetActiveTopLevelWindow()->GetPositionInScreen();
	}
	// Invalidate so we repaint; this widget is cheap
	InvalidateLayoutAndVolatility();
}

int32 UInventoryDragOverlayUserWidget::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	// Build a debug string for on-screen display every frame
	const bool bShowDebug = UYOLOInventorySettings::Get().bShowDebug;
	FYIBagItem LiveDragItem; UYIInventoryBag* LiveSourceBag = nullptr;
	const bool bHasLiveDrag = UInventoryGridWidget::GetActiveDraggedItem(LiveDragItem, LiveSourceBag, GetWorld());
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
				Name = !Def->DisplayName.IsEmpty() ? Def->DisplayName.ToString() : Def->GetName();
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

	// Compute overlay-local cursor each paint using current geometry to avoid window position/DPI offsets
	const FVector2D Local = AllottedGeometry.AbsoluteToLocal(CachedCursorSS);
	// Prefer dragged item icon; fallback to simple white brush if icon is missing.
	const UTexture2D* DragIconTexture = nullptr;
	if (bHasLiveDrag)
	{
		if (UYIItemDefinition* Def = LiveDragItem.Item.Definition.IsValid()
			? LiveDragItem.Item.Definition.Get()
			: LiveDragItem.Item.Definition.LoadSynchronous())
		{
			DragIconTexture = Def->Icon.IsValid() ? Def->Icon.Get() : Def->Icon.LoadSynchronous();
		}
	}

	const FVector2D GhostSize = FallbackGhostSize;
	const FVector2D P = Local - (GhostSize * 0.5f);
	const FLinearColor Tint(1.f, 1.f, 1.f, 0.85f);
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
		Debug += FString::Printf(TEXT("LayerId(start): %d\n"), LayerId);
	}

	FSlateDrawElement::MakeBox(OutDrawElements, ++LayerId, AllottedGeometry.ToPaintGeometry(FVector2f(GhostSize), FSlateLayoutTransform(FVector2f(P))), GhostBrush, ESlateDrawEffect::None, Tint);
	TArray<FVector2D> Seg;
	Seg.Add(P);
	Seg.Add(P + FVector2D(GhostSize.X, 0));
	Seg.Add(P + FVector2D(GhostSize.X, GhostSize.Y));
	Seg.Add(P + FVector2D(0, GhostSize.Y));
	Seg.Add(P);
	FSlateDrawElement::MakeLines(OutDrawElements, ++LayerId, AllottedGeometry.ToPaintGeometry(), Seg, ESlateDrawEffect::None, FLinearColor(0.2f,0.8f,1.f,0.6f), true, 1.5f);

	if (bShowDebug)
	{
		Debug += FString::Printf(TEXT("LayerId(after ghost): %d\n"), LayerId);
	}

	// Determine hovered grid by absolute rect containment (pick first match)
	UInventoryGridWidget* HoveredGrid = nullptr;
	FString HoverPickInfo;
	UInventoryGridWidget::ForEachRegisteredGrid([&](UInventoryGridWidget* Grid)
	{
		if (HoveredGrid || !Grid || !Grid->Bag) return; // already found
		const FGeometry GridGeomT = Grid->GetCachedGeometry();
		const FSlateRect AbsRect = GridGeomT.GetLayoutBoundingRect();
		const bool bHit = AbsRect.ContainsPoint(CachedCursorSS);
		HoverPickInfo += FString::Printf(TEXT("Grid %s AbsRect: (%.1f,%.1f)-(%.1f,%.1f) hit:%s\n"), *Grid->GetName(), AbsRect.Left, AbsRect.Top, AbsRect.Right, AbsRect.Bottom, bHit?TEXT("Y"):TEXT("N"));
		if (bHit)
		{
			HoveredGrid = Grid;
		}
	});
	if (bShowDebug)
	{
		Debug += HoverPickInfo;
		Debug += FString::Printf(TEXT("HoveredGrid: %s\n"), HoveredGrid ? *HoveredGrid->GetName() : TEXT("<none>"));
	}
	if (!HoveredGrid)
	{
		if (bShowDebug && GEngine)
		{
			static const int32 MsgKey = 0x99110001;
			GEngine->AddOnScreenDebugMessage(MsgKey, 0.f, FColor::Yellow, Debug);
		}
		return LayerId;
	}
	// Footprint highlight under cursor on hovered grid
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
		// Compute candidate footprint anchored at cursor center, clamped inside grid
		FYIBagItem DragItem; UYIInventoryBag* SrcBag=nullptr; UInventoryGridWidget::GetActiveDraggedItem(DragItem, SrcBag, GetWorld());
		const FIntPoint Foot = Grid->Bag->GetEffectiveSize(DragItem.Size);
		const FVector2D HalfFootPx = FVector2D(Foot) * CellPx * 0.5f;
		// Determine the cell that would center under the cursor
		const FVector2D CursorCellF = GridLocalFromScreen / CellPx; // fractional cell coords
		FIntPoint DesiredTopLeft(
			FMath::FloorToInt(CursorCellF.X - (Foot.X * 0.5f)),
			FMath::FloorToInt(CursorCellF.Y - (Foot.Y * 0.5f))
		);
		// Clamp TopLeft so the footprint stays entirely inside the grid
		const FIntPoint GridSize = Grid->Bag->GridSize;
		DesiredTopLeft.X = FMath::Clamp(DesiredTopLeft.X, 0, FMath::Max(0, GridSize.X - Foot.X));
		DesiredTopLeft.Y = FMath::Clamp(DesiredTopLeft.Y, 0, FMath::Max(0, GridSize.Y - Foot.Y));
		// Evaluate placement at clamped position
		const bool bCan = Grid->Bag->CanPlaceAt(DesiredTopLeft, DragItem.Size);
		// Convert grid-local footprint rect to overlay local
		const FVector2D FootP_Grid = FVector2D(DesiredTopLeft) * CellPx;
		const FVector2D FootS_Grid = FVector2D(Foot) * CellPx;
		const FVector2D FootP_Abs = GridGeom.LocalToAbsolute(FootP_Grid);
		const FVector2D FootP_Overlay = AllottedGeometry.AbsoluteToLocal(FootP_Abs);

		if (bShowDebug)
		{
			Debug += FString::Printf(TEXT("DragItem.Size: %s  Foot(effective): %s  CellPx: %.2f\n"), *V2i(DragItem.Size), *V2i(Foot), CellPx);
			Debug += FString::Printf(TEXT("CursorCellF: %s  DesiredTopLeft(clamped): %s  GridSize: %s\n"), *V2(CursorCellF), *V2i(DesiredTopLeft), *V2i(GridSize));
			Debug += FString::Printf(TEXT("bCanPlace: %s\n"), bCan?TEXT("true"):TEXT("false"));
			Debug += FString::Printf(TEXT("FootP_Grid: %s  FootS_Grid: %s  FootP_Abs: %s  FootP_Overlay: %s\n"), *V2(FootP_Grid), *V2(FootS_Grid), *V2(FootP_Abs), *V2(FootP_Overlay));
		}

		const FLinearColor GhostTint = bCan ? FLinearColor(0.2f,0.8f,0.2f,0.18f) : FLinearColor(0.8f,0.2f,0.2f,0.18f);
		FSlateDrawElement::MakeBox(OutDrawElements, ++LayerId, AllottedGeometry.ToPaintGeometry(FVector2f(FootS_Grid), FSlateLayoutTransform(FVector2f(FootP_Overlay))), FAppStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, GhostTint);

		if (bShowDebug)
		{
			Debug += FString::Printf(TEXT("LayerId(after footprint): %d\n"), LayerId);
		}
	};

	// Draw only for hovered grid
	DrawFootprintForGrid(HoveredGrid);

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
