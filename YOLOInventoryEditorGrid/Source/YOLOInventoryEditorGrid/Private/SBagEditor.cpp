#include "SBagEditor.h"
#include "YIInventoryBag.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SConstraintCanvas.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Styling/SlateIconFinder.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "YIItemDefinition.h"
#include "YIItemSchemaResolver.h"
#include "YIInventoryBlueprintLibrary.h"

SBagEditor::~SBagEditor()
{
	if (Bag.IsValid() && BagChangedHandle.IsValid()) { Bag->OnChanged.Remove(BagChangedHandle); }
}

void SBagEditor::RequestSort()
{
	if (!Bag.IsValid() || !SelectedSort.IsValid()) return;
	const FString& Key = *SelectedSort;

	// Store original items to restore if packing fails
	TArray<FYIBagItem> OriginalItems = Bag->Items;

	Bag->Modify();
	// Sort items by criteria
	Bag->Items.StableSort([&](const FYIBagItem& A, const FYIBagItem& B) {
		auto GetRarity = [](const FYIBagItem& It) { const UYIItemDefinition* Def = It.Item.Definition.LoadSynchronous(); return 0; /* rarity TBD */ };
		auto GetSize = [](const FYIBagItem& It) { return It.Size.X * It.Size.Y; };
		auto GetStack = [](const FYIBagItem& It) { return It.Item.Count; };
		auto GetPrice = [](const FYIBagItem& It) { const UYIItemDefinition* Def = It.Item.Definition.LoadSynchronous(); return 0; /* price TBD */ };
		int32 LA = 0, LB = 0;
		if (Key == TEXT("Rarity")) { LA = GetRarity(A); LB = GetRarity(B); }
		else if (Key == TEXT("Size")) { LA = GetSize(A); LB = GetSize(B); }
		else if (Key == TEXT("Stack")) { LA = GetStack(A); LB = GetStack(B); }
		else if (Key == TEXT("Price")) { LA = GetPrice(A); LB = GetPrice(B); }
		return bSortAscending ? (LA < LB) : (LA > LB);
		});

	// Clear positions and repack visually
	for (auto& Item : Bag->Items) { Item.Pos = FIntPoint(-1, -1); }

	bool bAllFit = true;
	for (int32 i = 0; i < Bag->Items.Num(); ++i)
	{
		FYIBagItem& Item = Bag->Items[i];
		FIntPoint FitPos;
		if (Bag->FindFirstFit(Item.Size, FitPos))
		{
			Item.Pos = FitPos;
		}
		else
		{
			bAllFit = false;
			break;
		}
	}

	if (!bAllFit)
	{
		// Restore original layout if packing failed
		Bag->Items = OriginalItems;
		FNotificationInfo Info(NSLOCTEXT("YOLOInventory", "SortPackFailed", "Sort failed - not enough space to repack items"));
		Info.ExpireDuration = 3.f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}

	Bag->OnChanged.Broadcast();
}
FIntPoint SBagEditor::ToCell(const FVector2D& LocalPos) const
{
	return FIntPoint(FMath::FloorToInt(LocalPos.X / CellSize.X), FMath::FloorToInt(LocalPos.Y / CellSize.Y));
}

int32 SBagEditor::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 L = LayerId;
	if (!Bag.IsValid()) return L;
	const FSlateBrush* Box = FAppStyle::Get().GetBrush("WhiteBrush");
	const FVector2D SizePix = FVector2D(Bag->GridSize) * CellSize;
	// Cell background
	FSlateDrawElement::MakeBox(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(FVector2f(SizePix), FSlateLayoutTransform()), Box, ESlateDrawEffect::None, Bag->CellBgColor);
	// Pixel-aligned grid lines drawn as independent segments (no joins)
	const float Half = 0.5f;
	const float MaxX = SizePix.X - Half;
	const float MaxY = SizePix.Y - Half;
	// Vertical lines
	for (int x = 0; x <= Bag->GridSize.X; ++x)
	{
		const float X = x * CellSize.X + Half;
		TArray<FVector2D> Seg = { FVector2D(X,Half), FVector2D(X,MaxY) };
		FSlateDrawElement::MakeLines(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(), Seg, ESlateDrawEffect::None, Bag->GridLineColor, false, FMath::Max(1.f, Bag->GridThickness));
	}
	// Horizontal lines
	for (int y = 0; y <= Bag->GridSize.Y; ++y)
	{
		const float Y = y * CellSize.Y + Half;
		TArray<FVector2D> Seg = { FVector2D(Half,Y), FVector2D(MaxX,Y) };
		FSlateDrawElement::MakeLines(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(), Seg, ESlateDrawEffect::None, Bag->GridLineColor, false, FMath::Max(1.f, Bag->GridThickness));
	}
	// Outer rect (pixel aligned)
	TArray<FVector2D> Outer = { FVector2D(Half,Half), FVector2D(MaxX,Half), FVector2D(MaxX,MaxY), FVector2D(Half,MaxY), FVector2D(Half,Half) };
	FSlateDrawElement::MakeLines(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(), Outer, ESlateDrawEffect::None, Bag->OuterLineColor, false, Bag->GridThickness * 1.5f);
	// Items
	for (int32 i = 0; i < Bag->Items.Num(); ++i)
	{
		const auto& It = Bag->Items[i];
		FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
		FVector2D P = ToPixel(It.Pos);
		FVector2D S = FVector2D(Eff) * CellSize;
		// Color by item or stack rarity if available
		FLinearColor Base = FLinearColor(0.1f, 0.9f, 0.3f, 0.20f);
		if (It.Item.Definition.IsValid()) { if (const UYIItemDefinition* Def = It.Item.Definition.Get()) { /* tint TBD */ } }
		FLinearColor Fill = (i == SelectedIndex) ? FLinearColor(0.2f, 0.6f, 1.f, 0.20f) : (i == HoveredIndex && Bag->bEnableHoverHighlight ? FLinearColor(1.f, 1.f, 1.f, 0.10f) : Base);
		FSlateDrawElement::MakeBox(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(FVector2f(S), FSlateLayoutTransform(FVector2f(P))), Box, ESlateDrawEffect::None, Fill);
		// Selection/hover outline
		TArray<FVector2D> ItemRect = { P, P + FVector2D(S.X,0), P + S, P + FVector2D(0,S.Y), P };
		FLinearColor Outline = (i == SelectedIndex) ? FLinearColor(0.2f, 0.6f, 1.f, 0.9f) : (i == HoveredIndex ? FLinearColor(1.f, 1.f, 1.f, 0.6f) : FLinearColor(0, 0, 0, 0));
		if (Outline.A > 0)
		{
			FSlateDrawElement::MakeLines(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(), ItemRect, ESlateDrawEffect::None, Outline, true, 1.5f);
		}
		// Item icon from asset
		if (Bag->bEnableThumbnails && It.Item.Definition.IsValid())
		{
			const UYIItemDefinition* Def = It.Item.Definition.Get();
			const FSlateBrush* Icon = nullptr;
			if (Def)
			{
				const TSoftObjectPtr<UTexture2D> EffectiveIcon = YIItemSchema::GetIcon(Def);
				UTexture2D* IconTex = EffectiveIcon.IsValid() ? EffectiveIcon.Get() : EffectiveIcon.LoadSynchronous();
				if (IconTex)
				{
					// Create a dynamic brush using the texture name
					static TMap<UTexture2D*, TSharedPtr<FSlateDynamicImageBrush>> BrushCache;
					if (!BrushCache.Contains(IconTex))
					{
						TSharedPtr<FSlateDynamicImageBrush> NewBrush = MakeShared<FSlateDynamicImageBrush>(IconTex->GetFName(), FVector2D(IconTex->GetSizeX(), IconTex->GetSizeY()));
						NewBrush->SetResourceObject(IconTex);
						BrushCache.Add(IconTex, NewBrush);
					}
					Icon = BrushCache[IconTex].Get();
				}
			}
			if (!Icon) { Icon = FSlateIconFinder::FindIconBrushForClass(UYIItemDefinition::StaticClass()); }

			const float Pad = 2.f;
			FVector2D MaxIconSize = S - FVector2D(Pad * 2.f, Pad * 2.f);
			MaxIconSize.X = FMath::Max(1.f, MaxIconSize.X);
			MaxIconSize.Y = FMath::Max(1.f, MaxIconSize.Y);
			FVector2D IconSize = MaxIconSize;
			if (Icon && Icon->ImageSize.X > KINDA_SMALL_NUMBER && Icon->ImageSize.Y > KINDA_SMALL_NUMBER)
			{
				const float TexAspect = Icon->ImageSize.X / Icon->ImageSize.Y;
				const float SlotAspect = MaxIconSize.X / MaxIconSize.Y;
				if (TexAspect > SlotAspect)
				{
					IconSize = FVector2D(MaxIconSize.X, MaxIconSize.X / FMath::Max(TexAspect, KINDA_SMALL_NUMBER));
				}
				else
				{
					IconSize = FVector2D(MaxIconSize.Y * TexAspect, MaxIconSize.Y);
				}
			}
			FVector2D IconPos = P + (S - IconSize) * 0.5f;
			FSlateDrawElement::MakeBox(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(FVector2f(IconSize), FSlateLayoutTransform(FVector2f(IconPos))), Icon, ESlateDrawEffect::None, FLinearColor::White);
		}
		// Count badge top-right with background
		FSlateFontInfo Font = FAppStyle::Get().GetFontStyle("NormalFont");
		FString CountStr = FString::FromInt(FMath::Max(1, It.Item.Count));
		FVector2D BadgeSize = FVector2D(14, 14);
		FVector2D BadgePos = P + FVector2D(S.X - BadgeSize.X - 2, 2);
		FSlateDrawElement::MakeBox(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(FVector2f(BadgeSize), FSlateLayoutTransform(FVector2f(BadgePos))), Box, ESlateDrawEffect::None, FLinearColor(0, 0, 0, 0.6f));
		FVector2D TextPos = BadgePos + FVector2D(3, -1);
		FSlateDrawElement::MakeText(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(FVector2f(1, 1), FSlateLayoutTransform(FVector2f(TextPos))), CountStr, Font, ESlateDrawEffect::None, FLinearColor::Yellow);
	}
	// Drag-drop preview
	if (bPreviewActive)
	{
		FVector2D P = ToPixel(PreviewCell);
		FVector2D S = FVector2D(PreviewSize) * CellSize;
		FLinearColor PC = bPreviewOk ? FLinearColor(0.f, 1.f, 0.f, 0.25f) : FLinearColor(1.f, 0.f, 0.f, 0.25f);
		FSlateDrawElement::MakeBox(OutDrawElements, ++L, AllottedGeometry.ToPaintGeometry(FVector2f(S), FSlateLayoutTransform(FVector2f(P))), Box, ESlateDrawEffect::None, PC);
	}
	return L;
}

FReply SBagEditor::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Bag.IsValid())
	{
		const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		for (int32 i = Bag->Items.Num() - 1; i >= 0; --i)
		{
			const auto& It = Bag->Items[i];
			FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
			FVector2D ItemPos = ToPixel(It.Pos);
			FVector2D ItemSize = FVector2D(Eff) * CellSize;
			if (FSlateRect(ItemPos, ItemPos + ItemSize).ContainsPoint(Local))
			{
				SelectedIndex = i;
				OnSelectionChanged.ExecuteIfBound(SelectedIndex);
				DraggingIndex = i;
				DragOffset = Local - ItemPos;
				return FReply::Handled().CaptureMouse(AsShared());
			}
		}
		if (SelectedIndex != INDEX_NONE)
		{
			SelectedIndex = INDEX_NONE;
			OnSelectionChanged.ExecuteIfBound(SelectedIndex);
		}
	}
	return FReply::Unhandled();

	if (!Bag.IsValid()) return FReply::Unhandled();
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		const FIntPoint Cell = ToCell(Local);
		// hit test items
		for (int32 i = 0; i < Bag->Items.Num(); ++i)
		{
			const auto& It = Bag->Items[i];
			FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
			FVector2D ItemPos = ToPixel(It.Pos);
			FVector2D ItemSize = FVector2D(Eff) * CellSize;
			if (Cell.X >= It.Pos.X && Cell.Y >= It.Pos.Y && Cell.X < It.Pos.X + Eff.X && Cell.Y < It.Pos.Y + Eff.Y)
			{
				// small rotate hit region at top-left 16x16
				if (Bag->bAllowRotation && Local.X >= ItemPos.X && Local.X <= ItemPos.X + 16 && Local.Y >= ItemPos.Y && Local.Y <= ItemPos.Y + 16)
				{
					Bag->RotateItem(i);
					return FReply::Handled();
				}
				SelectedIndex = i; DraggingIndex = i; DragOffset = Local - ItemPos; DragStartPos = It.Pos;
				return FReply::Handled().CaptureMouse(AsShared());
			}
		}
	}
	return FReply::Unhandled();
}

FReply SBagEditor::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	bPreviewActive = false; UpdateTooltipVisibility();
	auto Op = DragDropEvent.GetOperation();
	if (Bag.IsValid() && Op.IsValid() && Op->IsOfType<FAssetDragDropOp>())
	{
		auto AssetOp = StaticCastSharedPtr<FAssetDragDropOp>(Op);
		for (const FAssetData& AD : AssetOp->GetAssets())
		{
			if (AD.GetClass()->IsChildOf(UYIItemDefinition::StaticClass()))
			{
				const FVector2D Local = MyGeometry.AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
				const FIntPoint Cell = ToCell(Local);
				UYIItemDefinition* Def = Cast<UYIItemDefinition>(AD.GetAsset());
				FIntPoint Size = Def ? YIItemSchema::GetDefaultSize(Def) : FIntPoint(1, 1);
				// Alt modifier forces new stack even if merge is possible
				const bool bForceNewStack = DragDropEvent.IsAltDown();
				// If hovering an item and it matches type with stacking capability, stack into it (unless Alt forces new stack)
				int32 Hit = HitIndexAtRelease(Cell);
				if (!bForceNewStack && Hit != INDEX_NONE && Def)
				{
					int32 Existing = Bag->FindExistingStackIndex(Def);
					if (Existing == Hit)
					{
						// Merge 1 unit if allowed
						FYIBagItem& Target = Bag->Items[Hit];
						UYIItemDefinition* DefT = Target.Item.Definition.IsValid() ? Target.Item.Definition.Get() : Target.Item.Definition.LoadSynchronous();
						if (DefT && DefT == Def && YIItemSchema::IsStackingEnabled(DefT))
						{
							Target.Item.Count = FMath::Clamp(Target.Item.Count + 1, 1, YIItemSchema::GetMaxStackCount(DefT));
							Bag->MarkPackageDirty();
							Bag->OnChanged.Broadcast();
							return FReply::Handled();
						}
					}
				}
				// Otherwise, place a new stack at cell, fallback to first-fit
				FYIBagItem NewItem; NewItem.Item.Definition = TSoftObjectPtr<UYIItemDefinition>(AD.ToSoftObjectPath()); NewItem.Size = Size; NewItem.Pos = Cell; NewItem.Item.Count = 1;
				// Temporarily disable auto-merge when Alt is held to force new stack
				bool SavedAutoMerge = Bag->bAutoMergeOnAdd;
				if (bForceNewStack) { Bag->bAutoMergeOnAdd = false; }
				int32 NewIndex = Bag->AddBagItem(NewItem);
				if (bForceNewStack) { Bag->bAutoMergeOnAdd = SavedAutoMerge; }
				if (NewIndex == INDEX_NONE)
				{
					FIntPoint FitPos; if (Bag->FindFirstFit(NewItem.Size, FitPos)) { NewItem.Pos = FitPos; if (bForceNewStack) { Bag->bAutoMergeOnAdd = false; } Bag->AddBagItem(NewItem); Bag->bAutoMergeOnAdd = SavedAutoMerge; }
				}
				return FReply::Handled();
			}
		}
	}
	// Ignore custom palette drags here (stack entries), not inventory assets
	bPreviewActive = false; UpdateTooltipVisibility();
	return FReply::Unhandled();
}

FReply SBagEditor::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Update hover index and preview cell when not dragging
	if (Bag.IsValid() && DraggingIndex == INDEX_NONE)
	{
		const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		const FIntPoint Cell = ToCell(Local);
		PreviewCell = Cell; // Update for info bar

		int32 NewHover = INDEX_NONE;
		for (int32 i = 0; i < Bag->Items.Num(); ++i)
		{
			const auto& It = Bag->Items[i];
			FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
			if (Cell.X >= It.Pos.X && Cell.Y >= It.Pos.Y && Cell.X < It.Pos.X + Eff.X && Cell.Y < It.Pos.Y + Eff.Y) { NewHover = i; break; }
		}
		if (NewHover != HoveredIndex) { HoveredIndex = NewHover; UpdateTooltipVisibility(); }
	}

	if (!Bag.IsValid()) return FReply::Unhandled();
	if (DraggingIndex != INDEX_NONE)
	{
		const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
		const FVector2D PosPix = Local - DragOffset;
		FIntPoint Cell = ToCell(PosPix);
		Bag->MoveItem(DraggingIndex, Cell);
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void SBagEditor::OnMouseLeave(const FPointerEvent& MouseEvent)
{
    SCompoundWidget::OnMouseLeave(MouseEvent);
    if (HoveredIndex != INDEX_NONE)
    {
        HoveredIndex = INDEX_NONE;
        UpdateTooltipVisibility();
        Invalidate(EInvalidateWidgetReason::PaintAndVolatility);
    }
}

void SBagEditor::UpdateTooltipVisibility()
{
    if (!DynamicToolTip.IsValid()) { return; }
    if (!Bag.IsValid()) { SetToolTip(nullptr); return; }
    if (HoveredIndex == INDEX_NONE || !Bag->bShowCellTooltips)
    {
        SetToolTip(nullptr);
    }
    else
    {
        SetToolTip(DynamicToolTip);
    }
}

FReply SBagEditor::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
    auto Op = DragDropEvent.GetOperation();
    if (Bag.IsValid() && Op.IsValid() && Op->IsOfType<FAssetDragDropOp>())
    {
        // ensure tooltip state is correct during drag
        UpdateTooltipVisibility();

        auto AssetOp = StaticCastSharedPtr<FAssetDragDropOp>(Op);
        // find first inventory asset
        for (const FAssetData& AD : AssetOp->GetAssets())
        {
            if (AD.GetClass()->IsChildOf(UYIItemDefinition::StaticClass()))
            {
                const FVector2D Local = MyGeometry.AbsoluteToLocal(DragDropEvent.GetScreenSpacePosition());
                PreviewCell = ToCell(Local);
                // Load the asset to get its default size
                UYIItemDefinition* Def = Cast<UYIItemDefinition>(AD.GetAsset());
                FIntPoint ItemSize = Def ? YIItemSchema::GetDefaultSize(Def) : FIntPoint(1, 1);
                PreviewSize = Bag->GetEffectiveSize(ItemSize);
                bPreviewActive = true;
                bPreviewOk = Bag->CanPlaceAtWithScale(PreviewCell, ItemSize);
                return FReply::Handled();
            }
        }
    }
    bPreviewActive = false;
    UpdateTooltipVisibility();
    return FReply::Unhandled();
}

FReply SBagEditor::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!Bag.IsValid()) return FReply::Unhandled();
	const FVector2D Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FIntPoint Cell = ToCell(Local);

	// Context menu on right click
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		int32 RightHit = INDEX_NONE;
		for (int32 i = 0; i < Bag->Items.Num(); ++i)
		{
			const auto& It = Bag->Items[i];
			FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
			if (Cell.X >= It.Pos.X && Cell.Y >= It.Pos.Y && Cell.X < It.Pos.X + Eff.X && Cell.Y < It.Pos.Y + Eff.Y) { RightHit = i; break; }
		}
		ShowItemContextMenu(MyGeometry, MouseEvent, RightHit);
		return FReply::Handled();
	}

	// Finalize drag gesture (non-transactional)
	if (DraggingIndex != INDEX_NONE && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D PosPix = Local - DragOffset;
		FIntPoint DropCell = ToCell(PosPix);
		if (Bag.IsValid())
		{
			// Try normal move first (not the click-transaction path)
			if (!Bag->MoveItem(DraggingIndex, DropCell))
			{
				int32 TargetIdx = HitIndexAtRelease(DropCell);
				if (TargetIdx != INDEX_NONE && TargetIdx != DraggingIndex)
				{
					// Try combine stacks: merge dragging into target
					if (!Bag->CombineStacks(TargetIdx, DraggingIndex))
					{
						// Try swap positions dungeon-siege style
						FYIBagItem A = Bag->Items[DraggingIndex];
						FYIBagItem B = Bag->Items[TargetIdx];
						FIntPoint PosA = A.Pos; FIntPoint PosB = B.Pos;
						// Temporarily remove both and validate swap
						Bag->Items.RemoveAt(FMath::Max(DraggingIndex, TargetIdx));
						Bag->Items.RemoveAt(FMath::Min(DraggingIndex, TargetIdx));
						bool bCanPlaceA = Bag->CanPlaceAt(PosB, A.Size);
						bool bCanPlaceB = Bag->CanPlaceAt(PosA, B.Size);
						// Reinsert originals
						Bag->Items.Insert(A, FMath::Min(DraggingIndex, TargetIdx));
						Bag->Items.Insert(B, FMath::Max(DraggingIndex, TargetIdx));
						if (bCanPlaceA && bCanPlaceB)
						{
							Bag->Items[DraggingIndex].Pos = PosB;
							Bag->Items[TargetIdx].Pos = PosA;
							Bag->MarkPackageDirty();
							Bag->OnChanged.Broadcast();
						}
					}
				}
			}
		}
		DraggingIndex = INDEX_NONE; return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

// Removed transactional OnKeyDown; click-to-pick disabled for now
/*FReply SBagEditor::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Delete && Bag.IsValid())
	{
		if (SelectedIndex != INDEX_NONE && Bag->Items.IsValidIndex(SelectedIndex))
		{
			Bag->RemoveItem(SelectedIndex);
			SelectedIndex = INDEX_NONE;
			return FReply::Handled();
		}
	}

	if (bTransactionActive && InKeyEvent.GetKey() == EKeys::Escape && Bag.IsValid())
	{
		// Revert picked and displaced items to their original positions
		if (PickedIndex != INDEX_NONE)
		{
			Bag->Items[PickedIndex].Pos = PickedOriginalPos;
		}
		if (DisplacedIndex != INDEX_NONE)
		{
			Bag->Items[DisplacedIndex].Pos = DisplacedOriginalPos;
		}
		Bag->MarkPackageDirty();
		Bag->OnChanged.Broadcast();
		bTransactionActive = false;
		PickedIndex = INDEX_NONE;
		DisplacedIndex = INDEX_NONE;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

void SBagEditor::ShowItemContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, int32 HitIndex)
{
	FMenuBuilder Menu(true, nullptr);
	if (Bag.IsValid())
	{
		if (HitIndex != INDEX_NONE)
		{
			Menu.AddMenuEntry(NSLOCTEXT("YOLOInventory", "Rotate", "Rotate"), FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, HitIndex]() { Bag->RotateItem(HitIndex); })));
			Menu.AddMenuEntry(NSLOCTEXT("YOLOInventory", "Duplicate", "Duplicate"), FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, HitIndex]() { FYIBagItem Copy = Bag->Items[HitIndex]; Copy.Pos += FIntPoint(1, 0); Bag->AddItem(Copy); })));
			Menu.AddMenuEntry(NSLOCTEXT("YOLOInventory", "Delete", "Delete"), FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, HitIndex]() { Bag->RemoveItem(HitIndex); })));
		}
		Menu.AddMenuEntry(NSLOCTEXT("YOLOInventory", "Add1x1", "Add 1x1"), FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this]() { FYIBagItem N; N.Size = FIntPoint(1, 1); Bag->AddItem(N); })));
	}
	FSlateApplication::Get().PushMenu(AsShared(), FWidgetPath(), Menu.MakeWidget(), MouseEvent.GetScreenSpacePosition(), FPopupTransitionEffect::ContextMenu);
}

int32 SBagEditor::HitIndexAtRelease(const FIntPoint& Cell) const
{
	if (!Bag.IsValid()) return INDEX_NONE;
	for (int32 i = Bag->Items.Num()-1; i >= 0; --i)
	{
		const auto& It = Bag->Items[i];
		if (It.Pos.X < 0 || It.Pos.Y < 0) continue; // skip temp-unplaced
		FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
		if (Cell.X >= It.Pos.X && Cell.Y >= It.Pos.Y && Cell.X < It.Pos.X + Eff.X && Cell.Y < It.Pos.Y + Eff.Y) { return i; }
	}
	return INDEX_NONE;
}

void SBagEditor::Construct(const FArguments& InArgs)
{
	Bag = InArgs._Bag;
	OnSelectionChanged = InArgs._OnSelectionChanged;
	if (Bag.IsValid()) { CellSize = FVector2D(Bag->CellPixelSize, Bag->CellPixelSize); }

	if (Bag.IsValid()) { BagChangedHandle = Bag->OnChanged.AddLambda([this]() { Invalidate(EInvalidateWidgetReason::PaintAndVolatility); UpdateTooltipVisibility(); }); }
	// Dynamic tooltip reflecting hovered item or cell
	SetToolTip(SNew(SToolTip)
		.Text_Lambda([this]() -> FText {
			if (!Bag.IsValid()) return FText();
			if (HoveredIndex != INDEX_NONE && HoveredIndex < Bag->Items.Num())
			{
				const auto& It = Bag->Items[HoveredIndex];
				FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
				return Bag->bShowCellTooltips ? FText::Format(NSLOCTEXT("YOLOInventory", "ItemTip", "Item at {0},{1} size {2}x{3} (eff {4}x{5}) Count {6}"), It.Pos.X, It.Pos.Y, It.Size.X, It.Size.Y, Eff.X, Eff.Y, It.Count) : FText();
			}
			return Bag->bShowCellTooltips ? NSLOCTEXT("YOLOInventory", "CellTip", "Move items by dragging. Right-click for more options.") : FText();
			})
	);


	ChildSlot
		[
			SNew(SBorder)
				.Padding(6)
				[
					SNew(SHorizontalBox)
						// Left info panel
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)[
							SNew(SBox)
								.WidthOverride(260.f)
								[
									SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight()[
											SNew(SBorder)
												.BorderImage(FAppStyle::Get().GetBrush("DetailsView.CategoryTop"))
												.Padding(FMargin(6))
												[
													SNew(STextBlock)
														.Font(FAppStyle::Get().GetFontStyle("NormalFont"))
														.ColorAndOpacity(FLinearColor(0.9f, 0.9f, 1.f))
														.Text_Lambda([this]() {
														FIntPoint C = PreviewCell;
														FString Info = FString::Printf(TEXT("Cell: (%d,%d)\nHoverIndex: %d\n%s"), C.X, C.Y, HoveredIndex, (HoveredIndex == INDEX_NONE ? TEXT("Empty") : TEXT("Occupied")));
														return FText::FromString(Info);
															})
												]
										]
								]
						]
						// Right main panel
						+ SHorizontalBox::Slot().FillWidth(1.f)[
							SNew(SVerticalBox)
								// Sorting + Cell size row
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)[
									SNew(SHorizontalBox)
										+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)[SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "SortBy", "Sort By:"))]
										+ SHorizontalBox::Slot().AutoWidth().Padding(2)[
											SNew(SComboBox<TSharedPtr<FString>>)
												.OptionsSource(&SortOptions)
												.OnGenerateWidget_Lambda([](TSharedPtr<FString> In) { return SNew(STextBlock).Text(FText::FromString(*In)); })
												.OnSelectionChanged_Lambda([this](TSharedPtr<FString> In, ESelectInfo::Type) { SelectedSort = In; RequestSort(); })
												[SNew(STextBlock).Text_Lambda([this]() { return SelectedSort.IsValid() ? FText::FromString(*SelectedSort) : NSLOCTEXT("YOLOInventory", "Choose", "Choose"); })]
										]
										+ SHorizontalBox::Slot().AutoWidth().Padding(2).VAlign(VAlign_Center)[
											SNew(SCheckBox)
												.OnCheckStateChanged_Lambda([this](ECheckBoxState) { bSortAscending = !bSortAscending; RequestSort(); })
												.Content()[SNew(STextBlock).Text_Lambda([this]() { return bSortAscending ? NSLOCTEXT("YOLOInventory", "Asc", "Asc") : NSLOCTEXT("YOLOInventory", "Desc", "Desc"); })]
										]
											+ SHorizontalBox::Slot().AutoWidth().Padding(16, 0, 4, 0).VAlign(VAlign_Center)[SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "CellSize", "Cell:"))]
											+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2)[
												SNew(SSlider)
													.MinValue(8.f).MaxValue(128.f)
													.Value_Lambda([this]() { return Bag.IsValid() ? Bag->CellPixelSize : 32.f; })
													.OnValueChanged_Lambda([this](float V) { if (Bag.IsValid()) { Bag->CellPixelSize = V; CellSize = FVector2D(V, V); Invalidate(EInvalidateWidgetReason::PaintAndVolatility); } })
											]
								]
								// Minify bar
								+ SVerticalBox::Slot().AutoHeight()[
									SNew(SHorizontalBox)
										+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Minify", "Minify"))]
										+ SHorizontalBox::Slot().FillWidth(1.f)[
											SNew(SSlider)
												.MinValue(0.1f).MaxValue(1.0f)
												.Value_Lambda([this]() { return Bag.IsValid() ? Bag->MinifyScale : 1.0f; })
												.OnValueChanged_Lambda([this](float V) { if (Bag.IsValid()) { TArray<FYIBagItem> Dropped; Bag->ApplyMinifyScale(V, Dropped); if (Dropped.Num() > 0) { FNotificationInfo Info(NSLOCTEXT("YOLOInventory", "DroppedOnMinify", "Some items were dropped due to minify")); Info.ExpireDuration = 3.f; FSlateNotificationManager::Get().AddNotification(Info); } } })
										]
								]
									// Grid area
									+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)[
										SNew(SBox)
											.HeightOverride(500.f)
											[
												SNew(SScrollBox)
													+ SScrollBox::Slot()[
														SNew(SOverlay)
															+ SOverlay::Slot()[
																SNew(SBorder)
																	.BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.05f))
																	[
																		SNew(STextBlock).Text_Lambda([this]() { return Bag.IsValid() ? FText::Format(NSLOCTEXT("YOLOInventory", "BagGridInfo", "Grid {0}x{1}"), Bag->GridSize.X, Bag->GridSize.Y) : FText::FromString(TEXT("No Bag")); })
																	]
															]
															+ SOverlay::Slot()[
																SNew(SConstraintCanvas)
																	+ SConstraintCanvas::Slot()
																	.Offset_Lambda([this]() { return FVector2D::ZeroVector; })
																	.AutoSize(true)
																	[SNew(SBox).WidthOverride_Lambda([this]() { return Bag.IsValid() ? Bag->GridSize.X * CellSize.X : 256.f; }).HeightOverride_Lambda([this]() { return Bag.IsValid() ? Bag->GridSize.Y * CellSize.Y : 256.f; })]
																	+ SConstraintCanvas::Slot()
																	.Alignment(FVector2D(0, 0))
																	.Anchors(FAnchors(0, 0, 0, 0))
																	.Offset(FMargin(0))
																	[
																		SNew(SOverlay)
																			+ SOverlay::Slot()[
																				SNew(SBox)
																					.WidthOverride_Lambda([this]() { return Bag.IsValid() ? Bag->GridSize.X * CellSize.X : 256.f; })
																					.HeightOverride_Lambda([this]() { return Bag.IsValid() ? Bag->GridSize.Y * CellSize.Y : 256.f; })
																					[
																						// Items canvas
																						SNew(SConstraintCanvas)
																							+ SConstraintCanvas::Slot()
																							.Alignment(FVector2D(0, 0))
																							.Anchors(FAnchors(0, 0))
																							.Offset_Lambda([this]() { return FMargin(0); })
																							[
																								SNew(SOverlay)
																							]
																					]
																			]
																	]
															]
													]
											]
									]
						]
				]
];
}
*/

// Reintroduced active definitions (click-to-pick disabled)
void SBagEditor::ShowItemContextMenu(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, int32 HitIndex)
{
	FMenuBuilder Menu(true, nullptr);
	if (Bag.IsValid())
	{
		if (HitIndex != INDEX_NONE)
		{
			Menu.AddMenuEntry(NSLOCTEXT("YOLOInventory", "Rotate", "Rotate"), FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, HitIndex]() { Bag->RotateItem(HitIndex); })));
			Menu.AddMenuEntry(NSLOCTEXT("YOLOInventory", "Duplicate", "Duplicate"), FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, HitIndex]() { FYIBagItem Copy = Bag->Items[HitIndex]; Copy.Pos += FIntPoint(1, 0); Bag->AddBagItem(Copy); })));
			Menu.AddMenuEntry(NSLOCTEXT("YOLOInventory", "Delete", "Delete"), FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this, HitIndex]() { Bag->RemoveItem(HitIndex); })));
		}
		Menu.AddMenuEntry(NSLOCTEXT("YOLOInventory", "Add1x1", "Add 1x1"), FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateLambda([this]() { FYIBagItem N; N.Size = FIntPoint(1, 1); Bag->AddBagItem(N); })));
	}
	FSlateApplication::Get().PushMenu(AsShared(), FWidgetPath(), Menu.MakeWidget(), MouseEvent.GetScreenSpacePosition(), FPopupTransitionEffect::ContextMenu);
}

int32 SBagEditor::HitIndexAtRelease(const FIntPoint& Cell) const
{
	if (!Bag.IsValid()) return INDEX_NONE;
	for (int32 i = Bag->Items.Num() - 1; i >= 0; --i)
	{
		const auto& It = Bag->Items[i];
		if (It.Pos.X < 0 || It.Pos.Y < 0) continue; // skip temp-unplaced
		FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
		if (Cell.X >= It.Pos.X && Cell.Y >= It.Pos.Y && Cell.X < It.Pos.X + Eff.X && Cell.Y < It.Pos.Y + Eff.Y) { return i; }
	}
	return INDEX_NONE;
}

void SBagEditor::Construct(const FArguments& InArgs)
{
	Bag = InArgs._Bag;
	OnSelectionChanged = InArgs._OnSelectionChanged;
	if (Bag.IsValid()) { CellSize = FVector2D(Bag->CellPixelSize, Bag->CellPixelSize); }

	if (Bag.IsValid()) { BagChangedHandle = Bag->OnChanged.AddLambda([this]() { Invalidate(EInvalidateWidgetReason::PaintAndVolatility); UpdateTooltipVisibility(); }); }
	// Dynamic tooltip reflecting hovered item or cell (rarity-colored title + affix lines)
	DynamicToolTip = SNew(SToolTip)
		.Content()
		[
			SNew(SBox).WidthOverride(420.f)
				[
					SNew(SVerticalBox)
						// Title (rarity colored)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
								.Font(FAppStyle::Get().GetFontStyle("NormalFont"))
								.Text_Lambda([this]() -> FText {
								if (!Bag.IsValid()) return FText();
								if (HoveredIndex != INDEX_NONE && HoveredIndex < Bag->Items.Num())
								{
									FYITooltipData Data;
									if (UYIInventoryBlueprintLibrary::GetItemTooltipData(Bag.Get(), HoveredIndex, Data)) { return Data.Title; }
								}
								return FText();
									})
								.ColorAndOpacity_Lambda([this]() -> FSlateColor {
								if (!Bag.IsValid()) return FSlateColor(FLinearColor::White);
								if (HoveredIndex != INDEX_NONE && HoveredIndex < Bag->Items.Num())
								{
									FYITooltipData Data;
									if (UYIInventoryBlueprintLibrary::GetItemTooltipData(Bag.Get(), HoveredIndex, Data)) { return FSlateColor(Data.RarityColor); }
								}
								return FSlateColor(FLinearColor::White);
									})
						]
					// Description
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 6, 0, 2)
						[
							SNew(STextBlock)
								.AutoWrapText(true)
								.Text_Lambda([this]() -> FText {
								if (!Bag.IsValid()) return FText();
								if (HoveredIndex != INDEX_NONE && HoveredIndex < Bag->Items.Num())
								{
									FYITooltipData Data;
									if (UYIInventoryBlueprintLibrary::GetItemTooltipData(Bag.Get(), HoveredIndex, Data) && !Data.Description.IsEmpty()) { return Data.Description; }
									// fallback info
									const auto& It = Bag->Items[HoveredIndex];
									FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
									return Bag->bShowCellTooltips ? FText::Format(NSLOCTEXT("YOLOInventory", "ItemTip", "Item at {0},{1} size {2}x{3} (eff {4}x{5}) Count {6}"), It.Pos.X, It.Pos.Y, It.Size.X, It.Size.Y, Eff.X, Eff.Y, It.Item.Count) : FText();
								}
								return FText();
									})
						]
					// Affix lines (single block for simplicity)
					+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
								.AutoWrapText(true)
								.Text_Lambda([this]() -> FText {
								if (!Bag.IsValid()) return FText();
								if (HoveredIndex != INDEX_NONE && HoveredIndex < Bag->Items.Num())
								{
									FYITooltipData Data;
									if (UYIInventoryBlueprintLibrary::GetItemTooltipData(Bag.Get(), HoveredIndex, Data))
									{
										FString Combined;
										for (const FText& L : Data.AffixLines) { if (!Combined.IsEmpty()) Combined += TEXT("\n"); Combined += L.ToString(); }
										return FText::FromString(Combined);
									}
								}
								return FText();
									})
								.ColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.95f, 0.8f, 1.f)))
						]
				]
		];
	// Initially hide tooltip (we only show when hovering an item)
	SetToolTip(nullptr);

	ChildSlot
		[
			SNew(SBorder)
				.Padding(6)
				[
					SNew(SHorizontalBox)
						// Left info panel
						+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)[
							SNew(SBox)
								.WidthOverride(260.f)
								[
									SNew(SVerticalBox)
										+ SVerticalBox::Slot().AutoHeight()[
											SNew(SBorder)
												.BorderImage(FAppStyle::Get().GetBrush("DetailsView.CategoryTop"))
												.Padding(FMargin(6))
												[
													SNew(STextBlock)
														.Font(FAppStyle::Get().GetFontStyle("NormalFont"))
														.ColorAndOpacity(FLinearColor(0.9f, 0.9f, 1.f))
														.Text_Lambda([this]() {
														FIntPoint C = PreviewCell;
														FString Info = FString::Printf(TEXT("Cell: (%d,%d)\nHoverIndex: %d\n%s"), C.X, C.Y, HoveredIndex, (HoveredIndex == INDEX_NONE ? TEXT("Empty") : TEXT("Occupied")));
														return FText::FromString(Info);
															})
												]
										]
								]
						]
						// Right main panel
						+ SHorizontalBox::Slot().FillWidth(1.f)[
							SNew(SVerticalBox)
								// Sorting + Cell size row
								+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 4)[
									SNew(SHorizontalBox)
										+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2)[SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "SortBy", "Sort By:"))]
										+ SHorizontalBox::Slot().AutoWidth().Padding(2)[
											SNew(SComboBox<TSharedPtr<FString>>)
												.OptionsSource(&SortOptions)
												.OnGenerateWidget_Lambda([](TSharedPtr<FString> In) { return SNew(STextBlock).Text(FText::FromString(*In)); })
												.OnSelectionChanged_Lambda([this](TSharedPtr<FString> In, ESelectInfo::Type) { SelectedSort = In; RequestSort(); })
												[SNew(STextBlock).Text_Lambda([this]() { return SelectedSort.IsValid() ? FText::FromString(*SelectedSort) : NSLOCTEXT("YOLOInventory", "Choose", "Choose"); })]
										]
										+ SHorizontalBox::Slot().AutoWidth().Padding(2).VAlign(VAlign_Center)[
											SNew(SCheckBox)
												.OnCheckStateChanged_Lambda([this](ECheckBoxState) { bSortAscending = !bSortAscending; RequestSort(); })
												.Content()[SNew(STextBlock).Text_Lambda([this]() { return bSortAscending ? NSLOCTEXT("YOLOInventory", "Asc", "Asc") : NSLOCTEXT("YOLOInventory", "Desc", "Desc"); })]
										]
											+ SHorizontalBox::Slot().AutoWidth().Padding(16, 0, 4, 0).VAlign(VAlign_Center)[SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "CellSize", "Cell:"))]
											+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2)[
												SNew(SSlider)
													.MinValue(8.f).MaxValue(128.f)
													.Value_Lambda([this]() { return Bag.IsValid() ? Bag->CellPixelSize : 32.f; })
													.OnValueChanged_Lambda([this](float V) { if (Bag.IsValid()) { Bag->CellPixelSize = V; CellSize = FVector2D(V, V); Invalidate(EInvalidateWidgetReason::PaintAndVolatility); } })
											]
								]
								// Minify bar
								+ SVerticalBox::Slot().AutoHeight()[
									SNew(SHorizontalBox)
										+ SHorizontalBox::Slot().AutoWidth()[SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "Minify", "Minify"))]
										+ SHorizontalBox::Slot().FillWidth(1.f)[
											SNew(SSlider)
												.MinValue(0.1f).MaxValue(1.0f)
												.Value_Lambda([this]() { return Bag.IsValid() ? Bag->MinifyScale : 1.0f; })
												.OnValueChanged_Lambda([this](float V) { if (Bag.IsValid()) { TArray<FYIBagItem> Dropped; Bag->ApplyMinifyScale(V, Dropped); if (Dropped.Num() > 0) { FNotificationInfo Info(NSLOCTEXT("YOLOInventory", "DroppedOnMinify", "Some items were dropped due to minify")); Info.ExpireDuration = 3.f; FSlateNotificationManager::Get().AddNotification(Info); } } })
										]
								]
									// Grid area (placeholder: real paint happens in OnPaint)
									+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)[
										SNew(SBox)
											.HeightOverride(500.f)
											[
												SNew(STextBlock).Text(NSLOCTEXT("YOLOInventory", "GridPlaceholder", "Inventory Grid"))
											]
									]
						]
				]
		];
}
