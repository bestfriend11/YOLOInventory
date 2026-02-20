#include "TradingScreenWidget.h"
#include "UILayerSubsystem.h"
#include "InventoryScreenWidget.h"
#include "InventoryDragOverlayUserWidget.h"
#include "Engine/Engine.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "YITradeSessionActor.h"
#include "YIItemRegistrySubsystem.h"

namespace YITradingScreenWidgetPrivate
{
	static FYIItemInstance MakeItemInstanceByCode(int64 Code, int32 Count)
	{
		FYIItemInstance Item;
		Item.Count = Count;
		if (Code == 0)
		{
			return Item;
		}
		if (UEngine* Engine = GEngine)
		{
			if (UYIItemRegistrySubsystem* Registry = Engine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
			{
				Item.Definition = Registry->GetByCode(Code);
			}
		}
		return Item;
	}
}

void UTradingScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// Ensure both grids exist and are independent
	if (LeftGrid && SharedTooltip) { LeftGrid->SetBoundTooltipWidget(SharedTooltip); }
	if (RightGrid && SharedTooltip) { RightGrid->SetBoundTooltipWidget(SharedTooltip); }
    if (LeftGrid) { LeftGrid->OnCellSelected.AddDynamic(this, &UTradingScreenWidget::OnLeftCellSelected); }
    if (RightGrid) { RightGrid->OnCellSelected.AddDynamic(this, &UTradingScreenWidget::OnRightCellSelected); }
	if (LeftGrid) LeftGrid->RefreshBoundTooltip();
	if (RightGrid) RightGrid->RefreshBoundTooltip();
    // Always render drag ghosts/highlights inside each grid to keep drag/drop simple
    if (LeftGrid) LeftGrid->SetUseGlobalDragGhost(false);
    if (RightGrid) RightGrid->SetUseGlobalDragGhost(false);
    // If a legacy overlay exists in the UMG layout, collapse it to avoid duplicate drawing/overhead
    if (DragOverlay) { DragOverlay->SetVisibility(ESlateVisibility::Collapsed); }
    FocusedGrid = LeftGrid ? LeftGrid : RightGrid;
    // Auto push this screen to layered UI stack
    RequestPush(true);
}

void UTradingScreenWidget::NativeDestruct()
{
    if (LeftGrid) { LeftGrid->OnCellSelected.RemoveAll(this); }
    if (RightGrid) { RightGrid->OnCellSelected.RemoveAll(this); }
    if (Session)
    {
        Session->OnOffersUpdated.RemoveAll(this);
        Session->OnTradeCommitted.RemoveAll(this);
        Session->OnTradeCancelled.RemoveAll(this);
        Session->OnTradeFailed.RemoveAll(this);
        Session->OnInventoriesUpdated.RemoveAll(this);
        Session = nullptr;
    }
    Super::NativeDestruct();
}

void UTradingScreenWidget::SetBags(UYIInventoryBag* InLeftBag, UYIInventoryBag* InRightBag)
{
    if (LeftGrid) { LeftGrid->SetBag(InLeftBag); LeftGrid->RefreshBoundTooltip(); }
    if (RightGrid) { RightGrid->SetBag(InRightBag); RightGrid->RefreshBoundTooltip(); }
}

bool UTradingScreenWidget::TransferSelection(bool bLeftToRight, int32 Count, int32& OutDestIndex)
{
    OutDestIndex = INDEX_NONE;
    UInventoryGridWidget* Src = bLeftToRight ? LeftGrid : RightGrid;
    UInventoryGridWidget* Dst = bLeftToRight ? RightGrid : LeftGrid;
    if (!Src || !Dst) return false;
    return Src->TransferSelectedItemTo(Dst, Count, OutDestIndex);
}

void UTradingScreenWidget::SetSession(AYITradeSessionActor* InSession, UYIInventoryBag* LocalPlayerBag, UYIInventoryBag* OtherPartyBag)
{
    // Unbind any existing session
    if (Session)
    {
        Session->OnOffersUpdated.RemoveAll(this);
        Session->OnTradeCommitted.RemoveAll(this);
        Session->OnTradeCancelled.RemoveAll(this);
        Session->OnTradeFailed.RemoveAll(this);
        Session->OnInventoriesUpdated.RemoveAll(this);
    }

    Session = InSession;
    LocalBag = LocalPlayerBag;
    OtherBag = OtherPartyBag;

    if (!Session)
    {
        SetBags(nullptr, nullptr);
        return;
    }

    // Bind events (owning client)
    Session->OnOffersUpdated.AddDynamic(this, &UTradingScreenWidget::HandleOffersUpdated);
    Session->OnTradeCommitted.AddDynamic(this, &UTradingScreenWidget::HandleTradeCommitted);
    Session->OnTradeCancelled.AddDynamic(this, &UTradingScreenWidget::HandleTradeCancelled);
    Session->OnTradeFailed.AddDynamic(this, &UTradingScreenWidget::HandleTradeFailed);
    Session->OnInventoriesUpdated.AddDynamic(this, &UTradingScreenWidget::HandleOffersUpdated);

    RefreshOffers();
}

void UTradingScreenWidget::HandleOffersUpdated()
{
    RefreshOffers();
}

void UTradingScreenWidget::HandleTradeCommitted()
{
    // Auto-close; designer can override in BP if desired
    HandleCancel();
}

void UTradingScreenWidget::HandleTradeCancelled()
{
    HandleCancel();
}

void UTradingScreenWidget::HandleTradeFailed()
{
    // Refresh offers so UI reflects rollback
    RefreshOffers();
}

UYIInventoryBag* UTradingScreenWidget::BuildMirrorFromOffer(const FYITradeOffer& Offer)
{
    UYIInventoryBag* Bag = NewObject<UYIInventoryBag>(this);
    if (!Bag) return nullptr;
    Bag->GridSize = FIntPoint(10, 6); // compact display; designer can restyle
    for (const FYINetBagItem& Net : Offer.Items)
    {
        if (Net.Code == 0 || Net.Count <= 0) continue;
        FYIBagItem Item;
        Item.Item = YITradingScreenWidgetPrivate::MakeItemInstanceByCode(Net.Code, Net.Count);
        if (Net.InstanceId.IsValid())
        {
            Item.Item.InstanceId = Net.InstanceId;
        }
        if (Net.StackId.IsValid())
        {
            Item.Item.StackId = Net.StackId;
        }
        Item.Item.CustomStackKey = Net.CustomStackKey;
        Item.Item.ContainedBagId = Net.ContainedBagId;
        Item.Pos = Net.Pos;
        Item.Size = Net.Size;
        Bag->Items.Add(Item);
    }
    return Bag;
}

UYIInventoryBag* UTradingScreenWidget::BuildMirrorFromInventory(const TArray<FYINetBagItem>& View, FIntPoint GridSize)
{
    UYIInventoryBag* Bag = NewObject<UYIInventoryBag>(this);
    if (!Bag) return nullptr;
    Bag->GridSize = GridSize;
    for (const FYINetBagItem& Net : View)
    {
        if (Net.Code == 0 || Net.Count <= 0) continue;
        FYIBagItem Item;
        Item.Item = YITradingScreenWidgetPrivate::MakeItemInstanceByCode(Net.Code, Net.Count);
        if (Net.InstanceId.IsValid())
        {
            Item.Item.InstanceId = Net.InstanceId;
        }
        if (Net.StackId.IsValid())
        {
            Item.Item.StackId = Net.StackId;
        }
        Item.Item.CustomStackKey = Net.CustomStackKey;
        Item.Item.ContainedBagId = Net.ContainedBagId;
        Item.Pos = Net.Pos;
        Item.Size = Net.Size;
        Bag->Items.Add(Item);
    }
    return Bag;
}

void UTradingScreenWidget::RefreshOffers()
{
    if (!Session)
    {
        SetBags(nullptr, nullptr);
        return;
    }

    // Resolve local side using owning player state
    APlayerState* MyPS = nullptr;
    if (APlayerController* PC = GetOwningPlayer())
    {
        MyPS = PC->PlayerState;
    }
    if (!MyPS)
    {
        MyPS = GetOwningPlayerState();
    }
    ETradeSide LocalSide = Session->GetSideForPlayer(MyPS);
    const ETradeSide OtherSide = (LocalSide == ETradeSide::SideA) ? ETradeSide::SideB : ETradeSide::SideA;

    // Left: show our live bag if provided, otherwise mirror our replicated view
    if (!LocalBag)
    {
        if (LeftMirrorBag)
        {
            LeftMirrorBag->MarkAsGarbage();
            LeftMirrorBag = nullptr;
        }
        LeftMirrorBag = BuildMirrorFromInventory(Session->GetInventoryView(LocalSide), Session->GetInventorySize(LocalSide));
        LocalBag = LeftMirrorBag;
    }
    if (LeftGrid)
    {
        LeftGrid->SetBag(LocalBag);
        LeftGrid->SetTradeContext(Session, LocalSide);
        LeftGrid->SetAllowSelfMove(LocalBag != nullptr);
        LeftGrid->RefreshBoundTooltip();
    }

    // Right: prefer the other party's live bag if provided; otherwise mirror their full inventory snapshot
    if (RightMirrorBag)
    {
        RightMirrorBag->MarkAsGarbage();
        RightMirrorBag = nullptr;
    }

    if (OtherBag)
    {
        if (RightGrid)
        {
            RightGrid->SetBag(OtherBag);
            RightGrid->SetTradeContext(Session, OtherSide);
            RightGrid->SetAllowSelfMove(false);
            RightGrid->RefreshBoundTooltip();
        }
    }
    else
    {
        const TArray<FYINetBagItem> OtherView = Session->GetInventoryView(OtherSide);
        RightMirrorBag = BuildMirrorFromInventory(OtherView, Session->GetInventorySize(OtherSide));
        if (RightGrid)
        {
            RightGrid->SetBag(RightMirrorBag);
            RightGrid->SetTradeContext(Session, OtherSide);
            RightGrid->SetAllowSelfMove(false);
            RightGrid->RefreshBoundTooltip();
        }
    }
}

void UTradingScreenWidget::SetFocusedGrid(UInventoryGridWidget* NewFocused)
{
    FocusedGrid = NewFocused ? NewFocused : FocusedGrid;
}

void UTradingScreenWidget::OnLeftCellSelected(FIntPoint NewCell)
{
    if (LeftGrid) { LeftGrid->RefreshBoundTooltip(); }
}

void UTradingScreenWidget::OnRightCellSelected(FIntPoint NewCell)
{
    if (RightGrid) { RightGrid->RefreshBoundTooltip(); }
}

bool UTradingScreenWidget::HandleMoveUp()
{
    if (FocusedGrid) { FocusedGrid->MoveSelectionUp(); return true; }
    return false;
}
bool UTradingScreenWidget::HandleMoveDown()
{
    if (FocusedGrid) { FocusedGrid->MoveSelectionDown(); return true; }
    return false;
}
bool UTradingScreenWidget::HandleMoveLeft()
{
    if (FocusedGrid == RightGrid && LeftGrid) { SetFocusedGrid(LeftGrid); return true; }
    if (FocusedGrid) { FocusedGrid->MoveSelectionLeft(); return true; }
    return false;
}
bool UTradingScreenWidget::HandleMoveRight()
{
    if (FocusedGrid == LeftGrid && RightGrid) { SetFocusedGrid(RightGrid); return true; }
    if (FocusedGrid) { FocusedGrid->MoveSelectionRight(); return true; }
    return false;
}
bool UTradingScreenWidget::HandleConfirm()
{
    // optional: open action menu for focused grid if present (reusing InventoryScreenWidget pattern)
    return false;
}
bool UTradingScreenWidget::HandleCancel()
{
    // Let Blueprint close the screen by default
    return false;
}
bool UTradingScreenWidget::HandleOpenMenu()
{
    return HandleConfirm();
}
