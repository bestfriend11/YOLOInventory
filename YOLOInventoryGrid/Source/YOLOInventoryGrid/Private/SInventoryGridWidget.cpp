#include "SInventoryGridWidget.h"
#include "InventoryGridWidget.h"
#include "YIInventoryBag.h"
#include "YIInventoryTypes.h" // for YI_GetRarityColor
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Rendering/DrawElements.h"
#include "Engine/Texture2D.h"
#include "YIItemDefinition.h"
#include "YIItemSchemaResolver.h"
#include "Framework/Application/SlateApplication.h"
#include "YIInventoryGridStyleAsset.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

static void YI_DrawBrushSlot(
	FSlateWindowElementList& OutDrawElements,
	int32& LayerId,
	const FGeometry& AllottedGeometry,
	const FVector2D& Position,
	const FVector2D& Size,
	const FYIGridStyleBrushSlot& Slot,
	const FSlateBrush* FallbackBrush,
	const FSlateBrush* BrushOverride = nullptr)
{
	if (!Slot.bEnabled || Size.X <= 0.f || Size.Y <= 0.f)
	{
		return;
	}

	const bool bHasBrushData = Slot.Brush.DrawAs != ESlateBrushDrawType::NoDrawType;
	const FSlateBrush* Brush = BrushOverride ? BrushOverride : (bHasBrushData ? &Slot.Brush : FallbackBrush);
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

static UTexture2D* YI_Grid_TryResolveItemIconNoLoad(const FYIItemInstance& Item)
{
	UYIItemDefinition* Def = Item.Definition.Get();
	if (!Def && Item.Definition.ToSoftObjectPath().IsValid())
	{
		Def = Item.Definition.LoadSynchronous();
	}
	if (!Def)
	{
		return nullptr;
	}

	const TSoftObjectPtr<UTexture2D> EffectiveIcon = YIItemSchema::GetIcon(Def);
	if (UTexture2D* LoadedIcon = EffectiveIcon.Get())
	{
		return LoadedIcon;
	}

	if (EffectiveIcon.ToSoftObjectPath().IsValid())
	{
		return EffectiveIcon.LoadSynchronous();
	}

	return nullptr;
}

const FSlateBrush* SInventoryGridWidget::ResolveBrushForStyleSlot(
	const UYIInventoryGridStyleAsset* GridStyle,
	const FYIGridStyleBrushSlot& Slot,
	int32 SlotKey,
	float HoverAmount,
	float SelectedAmount,
	float InvalidAmount,
	float MarqueeAmount) const
{
	if (!GridStyle || !Slot.bEnabled)
	{
		return nullptr;
	}

	const bool bHasBrushData = Slot.Brush.DrawAs != ESlateBrushDrawType::NoDrawType;
	if (!bHasBrushData || !GridStyle->bAutoDriveThemeMaterialParameters)
	{
		return &Slot.Brush;
	}

	UMaterialInterface* SourceMaterial = Cast<UMaterialInterface>(Slot.Brush.GetResourceObject());
	if (!SourceMaterial)
	{
		return &Slot.Brush;
	}

	TStrongObjectPtr<UMaterialInstanceDynamic>& MIDPtr = StyleSlotMIDs.FindOrAdd(SlotKey);
	TWeakObjectPtr<UMaterialInterface>& CachedSourceMaterial = StyleSlotSourceMaterials.FindOrAdd(SlotKey);
	if (!MIDPtr.IsValid() || CachedSourceMaterial.Get() != SourceMaterial)
	{
		MIDPtr = TStrongObjectPtr<UMaterialInstanceDynamic>(UMaterialInstanceDynamic::Create(SourceMaterial, GetTransientPackage()));
		CachedSourceMaterial = SourceMaterial;
		FSlateBrush& DynamicBrush = StyleSlotBrushCache.FindOrAdd(SlotKey);
		DynamicBrush = Slot.Brush;
		DynamicBrush.SetResourceObject(MIDPtr.Get());
	}

	UMaterialInstanceDynamic* MID = MIDPtr.Get();
	if (!MID)
	{
		return &Slot.Brush;
	}

	const float WorldTime = OwnerWidget.IsValid() && OwnerWidget.Pin()->GetWorld()
		? OwnerWidget.Pin()->GetWorld()->GetRealTimeSeconds()
		: FPlatformTime::Seconds();

	if (!GridStyle->ThemeBlendParameterName.IsNone())
	{
		MID->SetScalarParameterValue(GridStyle->ThemeBlendParameterName, GridStyle->ThemeBlend);
	}
	if (!GridStyle->TimeParameterName.IsNone())
	{
		MID->SetScalarParameterValue(GridStyle->TimeParameterName, WorldTime);
	}
	if (!GridStyle->HoverAmountParameterName.IsNone())
	{
		MID->SetScalarParameterValue(GridStyle->HoverAmountParameterName, HoverAmount);
	}
	if (!GridStyle->SelectedAmountParameterName.IsNone())
	{
		MID->SetScalarParameterValue(GridStyle->SelectedAmountParameterName, SelectedAmount);
	}
	if (!GridStyle->InvalidAmountParameterName.IsNone())
	{
		MID->SetScalarParameterValue(GridStyle->InvalidAmountParameterName, InvalidAmount);
	}
	if (!GridStyle->MarqueeAmountParameterName.IsNone())
	{
		MID->SetScalarParameterValue(GridStyle->MarqueeAmountParameterName, MarqueeAmount);
	}
	if (GridStyle->bSetSlotStateIdParameter && !GridStyle->SlotStateIdParameterName.IsNone())
	{
		MID->SetScalarParameterValue(GridStyle->SlotStateIdParameterName, static_cast<float>(SlotKey));
	}

	const FSlateBrush* DynamicBrushPtr = StyleSlotBrushCache.Find(SlotKey);
	return DynamicBrushPtr ? DynamicBrushPtr : &Slot.Brush;
}

void SInventoryGridWidget::RebuildOccupancy()
{
	CellToItemIndex.Reset();
}

int32 SInventoryGridWidget::GetItemIndexAtCell(const FIntPoint& Cell) const
{
	return Bag.IsValid() ? Bag->GetItemIndexAtCellFast(Cell) : INDEX_NONE;
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
	if (GridSize.X <= 0 || GridSize.Y <= 0)
	{
		if (HoveredItemIndex != INDEX_NONE)
		{
			HoveredItemIndex = INDEX_NONE;
			HoveredItemTopLeft = FIntPoint(-1, -1);
			HoveredItemSize = FIntPoint::ZeroValue;
			if (OnHoveredItemChanged.IsBound()) { OnHoveredItemChanged.Execute(HoveredItemIndex); }
		}
		return;
	}

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

	int32 Idx = GetItemIndexAtCell(HoverCell);
	if (Idx != INDEX_NONE && !Bag->Items.IsValidIndex(Idx))
	{
		Idx = INDEX_NONE;
	}

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
	const int32 SlotCellFill = 1;
	const int32 SlotOuterBorder = 2;
	const int32 SlotItemFill = 3;
	const int32 SlotItemFrame = 4;
	const int32 SlotHoveredCell = 5;
	const int32 SlotHoveredItem = 6;
	const int32 SlotSelectedCell = 7;
	const int32 SlotLockedItem = 8;
	const int32 SlotGhostValid = 9;
	const int32 SlotGhostInvalid = 10;

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
		const FSlateBrush* CellFillBrush = ResolveBrushForStyleSlot(GridStyle, GridStyle->CellFill, SlotCellFill, 0.f, 0.f, 0.f, 0.f);
		YI_DrawBrushSlot(OutDrawElements, L, AllottedGeometry, FVector2D::ZeroVector, SizePix, GridStyle->CellFill, Box, CellFillBrush);
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
		const FSlateBrush* OuterBorderBrush = ResolveBrushForStyleSlot(GridStyle, GridStyle->OuterBorder, SlotOuterBorder, 0.f, 0.f, 0.f, 0.f);
		YI_DrawBrushSlot(OutDrawElements, L, AllottedGeometry, FVector2D::ZeroVector, SizePix, GridStyle->OuterBorder, Box, OuterBorderBrush);
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

	const FSlateBrush* ItemFillBrush = (GridStyle && GridStyle->ItemFill.bEnabled)
		? ResolveBrushForStyleSlot(GridStyle, GridStyle->ItemFill, SlotItemFill, 0.f, 0.f, 0.f, 0.f)
		: nullptr;
	const FSlateBrush* ItemFrameBrush = (GridStyle && GridStyle->ItemFrame.bEnabled)
		? ResolveBrushForStyleSlot(GridStyle, GridStyle->ItemFrame, SlotItemFrame, 0.f, 0.f, 0.f, 0.f)
		: nullptr;
	const FSlateBrush* LockedItemBrush = (GridStyle && GridStyle->LockedItemOverlay.bEnabled)
		? ResolveBrushForStyleSlot(GridStyle, GridStyle->LockedItemOverlay, SlotLockedItem, 0.15f, 0.35f, 0.f, 0.f)
		: nullptr;

	// Items
	for (int32 i = 0; i < Bag->Items.Num(); ++i)
	{
		const FYIBagItem& It = Bag->Items[i];
		const bool bLocked = Owner && Owner->IsItemIndexLockedForUI(i);
		const FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
		const FVector2D P = FVector2D(It.Pos) * LocalCell;
		const FVector2D S = FVector2D(Eff) * LocalCell;

		UTexture2D* IconTex = bShowItemIcons ? YI_Grid_TryResolveItemIconNoLoad(It.Item) : nullptr;

		if (GridStyle && GridStyle->ItemFill.bEnabled)
		{
			YI_DrawBrushSlot(OutDrawElements, L, AllottedGeometry, P, S, GridStyle->ItemFill, Box, ItemFillBrush);
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
			YI_DrawBrushSlot(OutDrawElements, L, AllottedGeometry, P, S, GridStyle->ItemFrame, Box, ItemFrameBrush);
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
				YI_DrawBrushSlot(OutDrawElements, L, AllottedGeometry, P, S, GridStyle->LockedItemOverlay, Box, LockedItemBrush);
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
				const FSlateBrush* HoveredItemBrush = ResolveBrushForStyleSlot(GridStyle, GridStyle->HoveredItemOverlay, SlotHoveredItem, 1.f, 0.f, 0.f, 1.f);
				YI_DrawBrushSlot(OutDrawElements, L, AllottedGeometry, P, S, GridStyle->HoveredItemOverlay, Box, HoveredItemBrush);
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
				if (CachedHoveredIconInstanceId != HoverItem.Item.InstanceId)
				{
					CachedHoveredIconInstanceId = HoverItem.Item.InstanceId;
					CachedHoveredIconTexture = bShowItemIcons ? YI_Grid_TryResolveItemIconNoLoad(HoverItem.Item) : nullptr;
				}
				UTexture2D* HoverIcon = CachedHoveredIconTexture.Get();
				DrawItemIcon(HoverIcon, P, S);
			}
		}
		else
		{
			const FVector2D P = FVector2D(HoverCell) * LocalCell;
			if (GridStyle && GridStyle->HoveredCellOverlay.bEnabled)
			{
				const FSlateBrush* HoveredCellBrush = ResolveBrushForStyleSlot(GridStyle, GridStyle->HoveredCellOverlay, SlotHoveredCell, 1.f, 0.f, 0.f, 0.f);
				YI_DrawBrushSlot(OutDrawElements, L, AllottedGeometry, P, LocalCell, GridStyle->HoveredCellOverlay, Box, HoveredCellBrush);
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
			const FSlateBrush* SelectedCellBrush = ResolveBrushForStyleSlot(GridStyle, GridStyle->SelectedCellOverlay, SlotSelectedCell, 0.f, 1.f, 0.f, 0.f);
			YI_DrawBrushSlot(OutDrawElements, L, AllottedGeometry, P, LocalCell, GridStyle->SelectedCellOverlay, Box, SelectedCellBrush);
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
		// When a global drag overlay is active, it owns both ghost icon and placement highlight
		// so the local grid does not draw a second highlight on top.
		if (!bUseGlobalDragGhost)
		{
			const FVector2D FootP = FVector2D(GhostTopLeft) * LocalCell;
			const FVector2D FootS = FVector2D(GhostFootprint) * LocalCell;
			const FYIGridStyleBrushSlot* GhostSlot = nullptr;
			if (GridStyle)
			{
				GhostSlot = bGhostPlacementValid ? &GridStyle->GhostPlacementValidOverlay : &GridStyle->GhostPlacementInvalidOverlay;
			}
			if (GhostSlot && GhostSlot->bEnabled)
			{
				const int32 GhostSlotKey = bGhostPlacementValid ? SlotGhostValid : SlotGhostInvalid;
				const FSlateBrush* GhostStateBrush = ResolveBrushForStyleSlot(
					GridStyle,
					*GhostSlot,
					GhostSlotKey,
					bGhostPlacementValid ? 1.f : 0.f,
					0.f,
					bGhostPlacementValid ? 0.f : 1.f,
					1.f);
				YI_DrawBrushSlot(OutDrawElements, L, AllottedGeometry, FootP, FootS, *GhostSlot, Box, GhostStateBrush);
			}
			else
			{
				const FLinearColor GhostTint = bGhostPlacementValid ? FLinearColor(0.2f, 0.8f, 0.2f, 0.18f) : FLinearColor(0.8f, 0.2f, 0.2f, 0.18f);
				FSlateDrawElement::MakeBox(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(FVector2f(FootS), FSlateLayoutTransform(FVector2f(FootP))), Box, ESlateDrawEffect::None, GhostTint);
			}

			const FVector2D P = FVector2D(GhostTopLeft) * LocalCell;
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
		GhostAnchorCellOffset = FIntPoint::ZeroValue;
		CachedGhostIconInstanceId.Invalidate();
		CachedGhostIconTexture.Reset();
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
		UInventoryGridWidget::GetActiveDraggedItemAnchor(GhostAnchorCellOffset, ContextWorld);
		const FIntPoint Eff = Bag.IsValid() ? Bag->GetEffectiveSize(DragItem.Size) : DragItem.Size;
		if (Eff != GhostFootprint)
		{
			GhostFootprint = Eff;
			GhostSize = FVector2D(GhostFootprint) * CellSize;
		}
		if (CachedGhostIconInstanceId != DragItem.Item.InstanceId)
		{
			CachedGhostIconInstanceId = DragItem.Item.InstanceId;
			CachedGhostIconTexture = YI_Grid_TryResolveItemIconNoLoad(DragItem.Item);
		}
		UTexture2D* Icon = CachedGhostIconTexture.Get();
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
		// Non-destructive drag keeps the source item in the bag while dragging.
		// Ignore that source item for ghost overlap/highlight evaluation so the drag preview behaves as if the item was picked up locally.
		if (Bag.IsValid() && SrcBag == Bag && DragItem.Item.InstanceId.IsValid())
		{
			int32 FoundIndex = INDEX_NONE;
			if (Bag->FindItemIndexByInstanceIdFast(DragItem.Item.InstanceId, FoundIndex))
			{
				GhostIgnoreIndex = FoundIndex;
			}
		}
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
	if (LocalCell.X <= UE_SMALL_NUMBER || LocalCell.Y <= UE_SMALL_NUMBER)
	{
		bGhostPlacementValid = false;
		GhostOverlapIndex = INDEX_NONE;
		bGhostOutOfBounds = true;
		return;
	}

	// Compute top-left by anchoring cursor-cell to the picked cell inside the dragged item.
	const FIntPoint CursorCell(
		FMath::FloorToInt(LocalCursor.X / LocalCell.X),
		FMath::FloorToInt(LocalCursor.Y / LocalCell.Y));
	const FIntPoint Candidate = CursorCell - GhostAnchorCellOffset;
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
			const int32 Idx = Bag->GetItemIndexAtCellFast(FIntPoint(GX, GY));
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
					CachedGhostIconInstanceId = Item.Item.InstanceId;
					CachedGhostIconTexture = YI_Grid_TryResolveItemIconNoLoad(Item.Item);
					UTexture2D* Icon = CachedGhostIconTexture.Get();
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
					UInventoryGridWidget::GetActiveDraggedItemAnchor(GhostAnchorCellOffset, ContextWorld);
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

