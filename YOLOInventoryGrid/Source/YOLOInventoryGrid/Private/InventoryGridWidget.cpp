#include "InventoryGridWidget.h"
#include "SInventoryGridWidget.h"
#include "YIInventoryGridFeatureAdapter.h"
#include "YIInventoryBag.h"
#include "YIInventoryComponent.h"
#include "YIInventoryBlueprintLibrary.h"
#include "InventoryUtils.h"
#include "YIItemDefinition.h"
#include "YIItemInstanceFragmentAccess.h"
#include "AbilitySystemComponent.h"
#include "YIRequirement.h"
#include "Kismet/GameplayStatics.h"
#include "YIItemNetTypes.h"
#include "YIItemSFXLibrary.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "YIInventoryGridStyleAsset.h"
#include "YOLOInventorySettings.h"

// Global drag state used to track click-to-pickup drags across grids
TSet<TWeakObjectPtr<UInventoryGridWidget>> UInventoryGridWidget::GRegisteredGrids;

static struct FInventoryGlobalDrag
{
	TWeakObjectPtr<UInventoryGridWidget> SourceGrid;
	int32 SourceIndex = INDEX_NONE; // original index at pickup time (may be invalid after removal)
	FIntPoint SourcePos = FIntPoint(-1,-1);
	FYIBagItem Item;
	bool bRemovedFromSource = false; // true if we removed the item from its bag when drag started
	bool bActive = false;
	bool bFromExchange = false;
	TWeakObjectPtr<UGameInstance> DragGI;
	void Reset() { SourceGrid = nullptr; SourceIndex = INDEX_NONE; SourcePos = FIntPoint(-1,-1); Item = FYIBagItem(); bRemovedFromSource = false; bActive = false; bFromExchange = false; DragGI.Reset(); }
} GInventoryDrag;

namespace
{
	struct FYIActionMenuShowParams
	{
		TArray<FText> Actions;
		TArray<int32> ActionIds;
	};

	struct FYIInventoryTooltipUpdateParams
	{
		FYITooltipData Data;
	};

	static bool YI_ShowActionMenu(UUserWidget* MenuWidget, const TArray<FText>& Actions, const TArray<int32>& ActionIds)
	{
		if (!MenuWidget)
		{
			return false;
		}

		if (UFunction* Fn = MenuWidget->FindFunction(TEXT("ShowActions")))
		{
			FYIActionMenuShowParams Params;
			Params.Actions = Actions;
			Params.ActionIds = ActionIds;
			MenuWidget->ProcessEvent(Fn, &Params);
			return true;
		}

		return false;
	}

	static bool YI_UpdateTooltipWidget(UUserWidget* TooltipWidget, const FYITooltipData& Data)
	{
		if (!TooltipWidget)
		{
			return false;
		}

		if (UFunction* Fn = TooltipWidget->FindFunction(TEXT("SetTooltipData")))
		{
			FYIInventoryTooltipUpdateParams Params;
			Params.Data = Data;
			TooltipWidget->ProcessEvent(Fn, &Params);
			return true;
		}

		if (UFunction* Fn = TooltipWidget->FindFunction(TEXT("OnTooltipDataUpdated")))
		{
			FYIInventoryTooltipUpdateParams Params;
			Params.Data = Data;
			TooltipWidget->ProcessEvent(Fn, &Params);
			return true;
		}

		return false;
	}

	static void YI_ClearTooltipWidget(UUserWidget* TooltipWidget)
	{
		if (!TooltipWidget)
		{
			return;
		}

		if (UFunction* Fn = TooltipWidget->FindFunction(TEXT("ClearTooltip")))
		{
			TooltipWidget->ProcessEvent(Fn, nullptr);
			return;
		}

		if (UFunction* Fn = TooltipWidget->FindFunction(TEXT("OnTooltipCleared")))
		{
			TooltipWidget->ProcessEvent(Fn, nullptr);
		}
	}
}

static void YI_HandleInventoryDragWorldCleanup(UWorld* World, bool /*bSessionEnded*/, bool /*bCleanupResources*/)
{
	if (!GInventoryDrag.bActive)
	{
		return;
	}

	if (!GInventoryDrag.DragGI.IsValid())
	{
		GInventoryDrag.Reset();
		return;
	}

	if (World && World->GetGameInstance() == GInventoryDrag.DragGI.Get())
	{
		GInventoryDrag.Reset();
	}
}

static void YI_RegisterInventoryDragCleanup()
{
	static bool bRegistered = false;
	if (!bRegistered)
	{
		FWorldDelegates::OnWorldCleanup.AddStatic(&YI_HandleInventoryDragWorldCleanup);
		bRegistered = true;
	}
}

static bool YI_IsGlobalDragValid(const UWorld* ContextWorld = nullptr)
{
	if (!GInventoryDrag.bActive)
	{
		return false;
	}

	if (!GInventoryDrag.DragGI.IsValid())
	{
		GInventoryDrag.Reset();
		return false;
	}

	if (ContextWorld && ContextWorld->GetGameInstance() != GInventoryDrag.DragGI.Get())
	{
		// Stale drag from a previous PIE run: no live source grid remains.
		if (!GInventoryDrag.SourceGrid.IsValid())
		{
			GInventoryDrag.Reset();
		}
		return false;
	}

	return true;
}

static FYIItemInstanceNet MakeNetItem(const FYIItemInstance& Item)
{
	FYIItemInstanceNet Net;
	Net.Definition = Item.Definition;
	Net.Count = Item.Count;
	Net.InstanceId = Item.InstanceId;
	Net.StackId = Item.StackId;
	Net.CustomStackKey = Item.CustomStackKey;
	Net.ContainedBagId = Item.ContainedBagId;
	Net.bRotated = Item.bRotated;
	YIItemInstanceFragments::ExportNetFragmentPayload(Item, Net.Fragments);
	return Net;
}

static const UYIItemSFXLibrary* ResolveSFXLibrary(const UInventoryGridWidget* Grid)
{
	if (!Grid)
	{
		return nullptr;
	}
	if (Grid->ItemSFXLibrary.IsValid())
	{
		return Grid->ItemSFXLibrary.Get();
	}
	if (Grid->ItemSFXLibrary.ToSoftObjectPath().IsValid())
	{
		return Grid->ItemSFXLibrary.LoadSynchronous();
	}
	if (Grid->Bag)
	{
		if (const UYIInventoryComponent* OwnerComp = Grid->Bag->GetTypedOuter<UYIInventoryComponent>())
		{
			if (OwnerComp->ItemSFXLibrary.IsValid())
			{
				return OwnerComp->ItemSFXLibrary.Get();
			}
			if (OwnerComp->ItemSFXLibrary.ToSoftObjectPath().IsValid())
			{
				return OwnerComp->ItemSFXLibrary.LoadSynchronous();
			}
		}
	}
	return nullptr;
}

static UYIItemDefinition* ResolveItemDefForSFX(const FYIItemInstance& Item, bool bAllowLoad)
{
	UYIItemDefinition* Def = Item.Definition.Get();
	if (!Def && bAllowLoad && Item.Definition.ToSoftObjectPath().IsValid())
	{
		Def = Item.Definition.LoadSynchronous();
	}
	return Def;
}

static USoundBase* ResolveItemSoundForEvent(const UInventoryGridWidget* Grid, const FYIItemInstance& Item, EYIItemSFXEvent Event)
{
	const UYIItemSFXLibrary* Library = ResolveSFXLibrary(Grid);
	UYIItemDefinition* Def = ResolveItemDefForSFX(Item, Grid ? Grid->bLoadDefinitionForSFX : false);
	return UYIInventoryBlueprintLibrary::ResolveItemSFXSound(Def, Library, Event);
}

// Check if placing a footprint at Pos would overlap at most one other item (ignoring SourceIdx). Returns that item index or INDEX_NONE.
static bool FindSingleOverlap(const UYIInventoryBag* Bag, int32 SourceIdx, const FIntPoint& Pos, const FIntPoint& Footprint, int32& OutOverlapIdx)
{
	OutOverlapIdx = INDEX_NONE;
	if (!Bag) return false;
	UYIInventoryComponent* OwnerComp = Bag->GetTypedOuter<UYIInventoryComponent>();
	// bounds check
	if (Pos.X < 0 || Pos.Y < 0 || Pos.X + Footprint.X > Bag->GridSize.X || Pos.Y + Footprint.Y > Bag->GridSize.Y) return false;
	for (int32 i=0;i<Bag->Items.Num();++i)
	{
		if (i == SourceIdx) continue;
		const FYIBagItem& It = Bag->Items[i];
		const FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
		if (RectsOverlap(Pos, Footprint, It.Pos, Eff))
		{
			if (OutOverlapIdx == INDEX_NONE) { OutOverlapIdx = i; }
			else if (OutOverlapIdx != i) { OutOverlapIdx = INDEX_NONE; return false; }
		}
	}
	return true;
}

// Helper: get item index at an arbitrary cell in a bag (returns INDEX_NONE)
static int32 GetItemIndexAtCell(const UYIInventoryBag* Bag, const FIntPoint& Cell)
{
	if (!Bag) return INDEX_NONE;
	for (int32 i = 0; i < Bag->Items.Num(); ++i)
	{
		const auto& It = Bag->Items[i];
		if (It.Pos.X < 0 || It.Pos.Y < 0) continue;
		FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
		if (Cell.X >= It.Pos.X && Cell.Y >= It.Pos.Y && Cell.X < It.Pos.X + Eff.X && Cell.Y < It.Pos.Y + Eff.Y)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void UInventoryGridWidget::OnWidgetRebuilt()
{
	Super::OnWidgetRebuilt();
	YI_RegisterInventoryDragCleanup();
	GRegisteredGrids.Add(this);
	RebindInventoryContextDelegates();
	RefreshBagFromBinding();
}

void UInventoryGridWidget::BeginDestroy()
{
	if (GInventoryDrag.SourceGrid.Get() == this)
	{
		GInventoryDrag.Reset();
	}
	else if (GInventoryDrag.bActive && !GInventoryDrag.DragGI.IsValid())
	{
		GInventoryDrag.Reset();
	}
	GRegisteredGrids.Remove(this);
	Super::BeginDestroy();
}

void UInventoryGridWidget::ForEachRegisteredGrid(TFunctionRef<void(UInventoryGridWidget*)> Callback)
{
	UGameInstance* TargetGI = GInventoryDrag.DragGI.Get();
	for (auto It = GRegisteredGrids.CreateIterator(); It; ++It)
	{
		if (UInventoryGridWidget* Grid = It->Get())
		{
			if (!TargetGI || (Grid->GetWorld() && Grid->GetWorld()->GetGameInstance() == TargetGI))
			{
				Callback(Grid);
			}
		}
	}
}

TSharedRef<SWidget> UInventoryGridWidget::RebuildWidget()
{
	// Hook Slate callbacks so Slate can notify the owning UWidget about hover/selection changes
	MySlateWidget = SNew(SInventoryGridWidget)
		.OwnerWidget(this)
		.Bag(Bag)
		.CellPixelSize(CellPixelSize)
		.bEnableCellHover(bEnableCellHover)
		.bEnableMouseSelection(bEnableMouseSelection)
		.OnHoveredItemChanged(SInventoryGridWidget::FOnHoveredItemChanged::CreateLambda([WeakThis = TWeakObjectPtr<UInventoryGridWidget>(this)](int32 Idx)
		{
			if (WeakThis.IsValid()) { WeakThis->HandleHoverChanged(Idx); }
		}))
		.OnHoveredCellChanged(SInventoryGridWidget::FOnHoveredCellChanged::CreateLambda([WeakThis = TWeakObjectPtr<UInventoryGridWidget>(this)](const FIntPoint& Cell)
		{
			if (WeakThis.IsValid()) { WeakThis->HandleHoverCellChanged(Cell); }
		}))
		.OnGhostPlacementChanged(SInventoryGridWidget::FOnGhostPlacementChanged::CreateLambda([WeakThis = TWeakObjectPtr<UInventoryGridWidget>(this)](const FIntPoint& Cell, bool bValid, bool bOutOfBounds)
		{
			if (WeakThis.IsValid()) { WeakThis->HandleGhostPlacementChanged(Cell, bValid, bOutOfBounds); }
		}))
		.OnSelectedCellChanged(SInventoryGridWidget::FOnSelectedCellChanged::CreateLambda([WeakThis = TWeakObjectPtr<UInventoryGridWidget>(this)](const FIntPoint& Cell)
		{
			if (WeakThis.IsValid()) { WeakThis->HandleSelectionChanged(Cell); }
		}))
		.OnCellClicked(SInventoryGridWidget::FOnCellClicked::CreateLambda([WeakThis = TWeakObjectPtr<UInventoryGridWidget>(this)](const FIntPoint& Cell)
		{
			if (WeakThis.IsValid()) { WeakThis->HandleCellClicked(Cell); }
		}));
	// Apply wrap/ghost settings
	if (MySlateWidget.IsValid()) { MySlateWidget->SetWrapNavigation(bWrapNavigation); MySlateWidget->SetUseGlobalDragGhost(bUseGlobalDragGhost); MySlateWidget->SetCellHoverEnabled(bEnableCellHover); MySlateWidget->SetMouseSelectionEnabled(bEnableMouseSelection); MySlateWidget->Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);}
	return MySlateWidget.ToSharedRef();
}

void UInventoryGridWidget::SetWrapNavigation(bool bEnable)
{
	bWrapNavigation = bEnable;
	if (MySlateWidget.IsValid()) MySlateWidget->SetWrapNavigation(bWrapNavigation);
}

void UInventoryGridWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	RebindInventoryContextDelegates();
	RefreshBagFromBinding();
	if (MySlateWidget)
	{
		MySlateWidget->SetBag(Bag);
		MySlateWidget->SetCellPixelSize(CellPixelSize);
		MySlateWidget->SetWrapNavigation(bWrapNavigation);
		MySlateWidget->SetUseGlobalDragGhost(bUseGlobalDragGhost);
		MySlateWidget->SetCellHoverEnabled(bEnableCellHover);
		MySlateWidget->SetMouseSelectionEnabled(bEnableMouseSelection);
		MySlateWidget->Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);
		MySlateWidget->Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);
	}

	// Bind to bag change events so tooltips are updated when items change
	if (CachedBag != Bag)
	{
		if (CachedBag)
		{
			CachedBag->OnChanged.Remove(BagChangedHandle);
		}
		if (Bag)
		{
			BagChangedHandle = Bag->OnChanged.AddLambda([this]() { OnBagChanged(); });
		}
		CachedBag = Bag;
	}

	// Ensure tooltip reflects the current selection
	UpdateBoundTooltip();
}

FVector2D UInventoryGridWidget::GetDesiredSize() const
{
	if (MySlateWidget.IsValid()) return MySlateWidget->GetDesiredSize();
	if (Bag) return FVector2D(Bag->GridSize) * FVector2D(CellPixelSize, CellPixelSize);
	return FVector2D::Zero();
}

void UInventoryGridWidget::SetUseGlobalDragGhost(bool bEnable)
{
	bUseGlobalDragGhost = bEnable; 
	if (MySlateWidget.IsValid())
		MySlateWidget->Invalidate(EInvalidateWidgetReason::Paint);
}

void UInventoryGridWidget::SetEnableCellHover(bool bEnable)
{
	bEnableCellHover = bEnable;
	if (MySlateWidget.IsValid())
	{
		MySlateWidget->SetCellHoverEnabled(bEnableCellHover);
		MySlateWidget->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void UInventoryGridWidget::SetEnableMouseSelection(bool bEnable)
{
	bEnableMouseSelection = bEnable;
	if (MySlateWidget.IsValid())
	{
		MySlateWidget->SetMouseSelectionEnabled(bEnableMouseSelection);
		MySlateWidget->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void UInventoryGridWidget::SetTooltipRequirementContext(UAbilitySystemComponent* InASC, int32 InXP, const FGameplayTagContainer& InOwnedTags)
{
	RequirementAbilitySystem = InASC;
	RequirementXP = InXP;
	RequirementOwnedTags = InOwnedTags;
	UpdateBoundTooltip();
}

void UInventoryGridWidget::SetTooltipPreviewAttributes(const TMap<FName,float>& InAttributes)
{
	RequirementPreviewAttributes = InAttributes;
	UpdateBoundTooltip();
}

bool UInventoryGridWidget::MoveSelection(FIntPoint Delta)
{
	if (!MySlateWidget.IsValid()) return false;
	// Let Slate change selection and notify via the slate callback which calls HandleSelectionChanged
	return MySlateWidget->MoveSelection(Delta);
}

bool UInventoryGridWidget::MoveSelectionUp()
{
	return MoveSelection(FIntPoint(0, -1));
}

bool UInventoryGridWidget::MoveSelectionDown()
{
	return MoveSelection(FIntPoint(0, 1));
}

bool UInventoryGridWidget::MoveSelectionLeft()
{
	return MoveSelection(FIntPoint(-1, 0));
}

bool UInventoryGridWidget::MoveSelectionRight()
{
	return MoveSelection(FIntPoint(1, 0));
}

void UInventoryGridWidget::SetSelectedCell(FIntPoint Cell)
{
	const int32 ItemIndex = (Bag && Cell.X >= 0 && Cell.Y >= 0) ? GetItemIndexAtCell(Bag, Cell) : INDEX_NONE;
	if (ItemIndex != INDEX_NONE && IsItemIndexLockedForUI(ItemIndex))
	{
		Cell = FIntPoint(-1, -1);
	}

	if (MySlateWidget.IsValid())
	{
		MySlateWidget->SetSelectedCell(Cell);
		// Slate may not trigger the callback when set programmatically; ensure we process the change here
		HandleSelectionChanged(Cell);
	}
}

bool UInventoryGridWidget::GetSelectedCellTooltipData(FYITooltipData& OutData, const FYIRequirementContext& RequirementContext) const
{
	if (!Bag) return false;
	if (SelectedCell.X < 0 || SelectedCell.Y < 0) return false;
	for (int32 i = 0; i < Bag->Items.Num(); ++i)
	{
		const auto& It = Bag->Items[i];
		if (It.Pos.X < 0 || It.Pos.Y < 0) continue;
		FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
		if (SelectedCell.X >= It.Pos.X && SelectedCell.Y >= It.Pos.Y && SelectedCell.X < It.Pos.X + Eff.X && SelectedCell.Y < It.Pos.Y + Eff.Y)
		{
			return UYIInventoryBlueprintLibrary::GetItemTooltipData(Bag, i, OutData, RequirementContext);
		}
	}
	return false;
}

int32 UInventoryGridWidget::GetSelectedItemIndex() const
{
	if (!Bag) return INDEX_NONE;
	if (SelectedCell.X < 0 || SelectedCell.Y < 0) return INDEX_NONE;
	for (int32 i = 0; i < Bag->Items.Num(); ++i)
	{
		const auto& It = Bag->Items[i];
		if (It.Pos.X < 0 || It.Pos.Y < 0) continue;
		FIntPoint Eff = Bag->GetEffectiveSize(It.Size);
		if (SelectedCell.X >= It.Pos.X && SelectedCell.Y >= It.Pos.Y && SelectedCell.X < It.Pos.X + Eff.X && SelectedCell.Y < It.Pos.Y + Eff.Y)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

// Called when the bound bag changes (items added/removed/modified)
void UInventoryGridWidget::OnBagChanged()
{
	// Bag changed (add/remove/etc.) — ensure selection/hover/tooltips stay correct
	if (MySlateWidget.IsValid())
	{ MySlateWidget->RefreshFromBag(); }
	// Recompute selection-related state by re-broadcasting the current selection
	HandleSelectionChanged(SelectedCell);
	UpdateBoundTooltip();
}

void UInventoryGridWidget::HandleSelectionChanged(const FIntPoint& NewCell)
{
	SelectedCell = NewCell;
	// Determine item index now selected
	int32 ItemIndex = GetSelectedItemIndex();
	if (ItemIndex != INDEX_NONE && IsItemIndexLockedForUI(ItemIndex))
	{
		SelectedCell = FIntPoint(-1, -1);
		ItemIndex = INDEX_NONE;
		if (MySlateWidget.IsValid())
		{
			MySlateWidget->SetSelectedCell(SelectedCell);
		}
	}
	// Legacy simple cell selected event
	OnCellSelected.Broadcast(SelectedCell);
	// New detailed event
	OnSelectionChanged.Broadcast(SelectedCell, ItemIndex);
	// Update tooltip to reflect new selection
	UpdateBoundTooltip();

	// Optionally request UI to show action menu
	OnActionMenuRequested.Broadcast(ItemIndex);
	if (bAutoOpenActionMenuOnSelect && BoundActionMenu && ItemIndex != INDEX_NONE)
	{
		RequestOpenActionMenu(BoundActionMenu);
	}
}

void UInventoryGridWidget::SetGridStyleOverride(UYIInventoryGridStyleAsset* InStyle)
{
	GridStyleOverride = InStyle;
	if (MySlateWidget.IsValid())
	{
		MySlateWidget->Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);
	}
}

UYIInventoryGridStyleAsset* UInventoryGridWidget::GetResolvedGridStyleAsset() const
{
	auto ResolveTypedStyle = [](const TSoftObjectPtr<UYIInventoryGridStyleAsset>& StylePtr) -> UYIInventoryGridStyleAsset*
		{
			if (StylePtr.IsValid())
			{
				return StylePtr.Get();
			}
			if (StylePtr.ToSoftObjectPath().IsValid())
			{
				return StylePtr.LoadSynchronous();
			}
			return nullptr;
		};

	auto ResolveUntypedStyle = [](const TSoftObjectPtr<UObject>& StylePtr) -> UYIInventoryGridStyleAsset*
		{
			UObject* LoadedObject = nullptr;
			if (StylePtr.IsValid())
			{
				LoadedObject = StylePtr.Get();
			}
			else if (StylePtr.ToSoftObjectPath().IsValid())
			{
				LoadedObject = StylePtr.LoadSynchronous();
			}
			return Cast<UYIInventoryGridStyleAsset>(LoadedObject);
		};

	if (UYIInventoryGridStyleAsset* WidgetStyle = ResolveTypedStyle(GridStyleOverride))
	{
		return WidgetStyle;
	}

	if (bUseBagStyleAsset && Bag)
	{
		if (UYIInventoryGridStyleAsset* BagStyle = ResolveUntypedStyle(Bag->GridStyleAsset))
		{
			return BagStyle;
		}
	}

	return ResolveUntypedStyle(UYOLOInventorySettings::Get().DefaultGridStyle);
}

bool UInventoryGridWidget::GetAvailableActionsForSelectedItem(TArray<FText>& OutActions, TArray<int32>& OutActionIds) const
{
	OutActions.Reset(); OutActionIds.Reset();
	if (!Bag) return false;
	int32 Index = GetSelectedItemIndex();
	if (Index == INDEX_NONE) return false;
	if (IsItemIndexLockedForUI(Index)) return false;

	// Default actions
	OutActions.Add(NSLOCTEXT("YOLOInventory", "UseAction", "Use")); OutActionIds.Add(ACTION_USE);
	if (Bag->bAllowRotation) { OutActions.Add(NSLOCTEXT("YOLOInventory", "RotateAction", "Rotate")); OutActionIds.Add(ACTION_ROTATE); }
	OutActions.Add(NSLOCTEXT("YOLOInventory", "DropAction", "Drop")); OutActionIds.Add(ACTION_DROP);
	int32 Found = Bag->FindExistingStackIndexForItem(Bag->Items[Index]);
	if (Found != INDEX_NONE && Found != Index) { OutActions.Add(NSLOCTEXT("YOLOInventory", "CombineAction", "Combine")); OutActionIds.Add(ACTION_COMBINE); }
	OutActions.Add(NSLOCTEXT("YOLOInventory", "SellAction", "Sell")); OutActionIds.Add(ACTION_SELL);

	// Extra actions: transfer, inspect, equip/grab
	OutActions.Add(NSLOCTEXT("YOLOInventory", "TransferAction", "Transfer")); OutActionIds.Add(ACTION_TRANSFER);
	OutActions.Add(NSLOCTEXT("YOLOInventory", "InspectAction", "Inspect")); OutActionIds.Add(ACTION_INSPECT);
	OutActions.Add(NSLOCTEXT("YOLOInventory", "EquipAction", "Equip")); OutActionIds.Add(ACTION_EQUIP);
	OutActions.Add(NSLOCTEXT("YOLOInventory", "GrabAction", "Grab")); OutActionIds.Add(ACTION_GRAB);

	return true;
}

bool UInventoryGridWidget::RequestOpenActionMenu(UUserWidget* Menu)
{
	if (!Menu) return false;
	TArray<FText> Actions; TArray<int32> ActionIds;
	if (!GetAvailableActionsForSelectedItem(Actions, ActionIds)) return false;
	return YI_ShowActionMenu(Menu, Actions, ActionIds);
}

void UInventoryGridWidget::HandleHoverChanged(int32 HoveredIndex)
{
	const int32 PrevHovered = HoveredItemIndexCached;
	// Broadcast hover state to Blueprint listeners
	OnItemHoverChanged.Broadcast(HoveredIndex);
	const bool bHasItem = HoveredIndex != INDEX_NONE;
	OnHoverSlotChanged.Broadcast(HoveredIndex, bHasItem);
	HoveredItemIndexCached = HoveredIndex;

	if (PrevHovered != HoveredIndex)
	{
		if (IsHoverSoundEnabled() && bHasItem && Bag && Bag->Items.IsValidIndex(HoveredIndex))
		{
			if (USoundBase* ItemSound = ResolveItemSoundForEvent(this, Bag->Items[HoveredIndex].Item, EYIItemSFXEvent::HoverItem))
			{
				UGameplayStatics::PlaySound2D(this, ItemSound);
			}
			else if (HoverSlotSound)
			{
				UGameplayStatics::PlaySound2D(this, HoverSlotSound);
			}
		}
	}
	UpdateBoundTooltip();
	// Optionally refresh tooltip if there is no explicit selection but we want hover to drive tooltip (kept to selection-only for now)
}

void UInventoryGridWidget::HandleHoverCellChanged(const FIntPoint& NewCell)
{
	const FIntPoint PrevCell = HoveredCellCached;
	HoveredCellCached = NewCell;

	if (!bEnableCellHover || PrevCell == NewCell)
	{
		return;
	}

	if (YI_IsGlobalDragValid(GetWorld()))
	{
		return;
	}

	if (!Bag)
	{
		return;
	}

	const int32 HoverIdx = GetItemIndexAtCell(Bag, NewCell);
	// Only play empty-slot hover sounds when we are hovering an empty cell.
	if (IsHoverSoundEnabled() && HoverIdx == INDEX_NONE && HoverEmptySound)
	{
		UGameplayStatics::PlaySound2D(this, HoverEmptySound);
	}
}

void UInventoryGridWidget::HandleGhostPlacementChanged(const FIntPoint& TopLeftCell, bool bValid, bool bOutOfBounds)
{
	(void)TopLeftCell;
	// Only play ghost highlight SFX when dragging.
	if (!YI_IsGlobalDragValid(GetWorld()))
	{
		return;
	}
	if (!IsDragHoverSoundEnabled())
	{
		return;
	}
	if (bOutOfBounds && !bPlayDragHoverOutOfBounds)
	{
		return;
	}

	if (bValid)
	{
		if (USoundBase* ItemSound = ResolveItemSoundForEvent(this, GInventoryDrag.Item.Item, EYIItemSFXEvent::HoverItem))
		{
			UGameplayStatics::PlaySound2D(this, ItemSound);
		}
		else if (DragHoverSound)
		{
			UGameplayStatics::PlaySound2D(this, DragHoverSound);
		}
		else if (HoverEmptySound)
		{
			UGameplayStatics::PlaySound2D(this, HoverEmptySound);
		}
	}
	else
	{
		if (USoundBase* ItemSound = ResolveItemSoundForEvent(this, GInventoryDrag.Item.Item, EYIItemSFXEvent::InvalidMove))
		{
			UGameplayStatics::PlaySound2D(this, ItemSound);
		}
		else if (DragHoverInvalidSound)
		{
			UGameplayStatics::PlaySound2D(this, DragHoverInvalidSound);
		}
		else if (IsInvalidSoundEnabled() && InvalidMoveSound)
		{
			UGameplayStatics::PlaySound2D(this, InvalidMoveSound);
		}
	}
}

void UInventoryGridWidget::SetFeatureAdapter(UYIInventoryGridFeatureAdapter* InAdapter)
{
	FeatureAdapter = InAdapter;
	if (FeatureAdapter)
	{
		FeatureAdapter->OnAssignedToGrid(this);
	}
}

bool UInventoryGridWidget::IsInventorySoundEnabled() const
{
	if (!bEnableInventorySounds)
	{
		return false;
	}
	if (Bag)
	{
		if (const UYIInventoryComponent* OwnerComp = Bag->GetTypedOuter<UYIInventoryComponent>())
		{
			return OwnerComp->bEnableInventorySounds;
		}
	}
	return true;
}

bool UInventoryGridWidget::IsHoverSoundEnabled() const
{
	return IsInventorySoundEnabled() && bEnableHoverSounds;
}

bool UInventoryGridWidget::IsDragSoundEnabled() const
{
	return IsInventorySoundEnabled() && bEnableDragSounds;
}

bool UInventoryGridWidget::IsDragHoverSoundEnabled() const
{
	return IsInventorySoundEnabled() && bEnableDragHoverSounds;
}

bool UInventoryGridWidget::IsInvalidSoundEnabled() const
{
	return IsInventorySoundEnabled() && bEnableInvalidMoveSounds;
}

bool UInventoryGridWidget::BeginDragFromCell(FIntPoint Cell)
{
	if (!Bag) return false;
	int32 Idx = GetItemIndexAtCell(Bag, Cell);
	if (Idx == INDEX_NONE) return false;
	if (IsItemIndexLockedForUI(Idx))
	{
		return false;
	}
	// Only allow drag within this game instance (prevents cross-PIE bleed)
	if (UWorld* World = GetWorld())
	{
		GInventoryDrag.DragGI = World->GetGameInstance();
	}
	GInventoryDrag.SourceGrid = this;
	GInventoryDrag.SourceIndex = Idx;
	GInventoryDrag.Item = Bag->Items[Idx];
	GInventoryDrag.SourcePos = GInventoryDrag.Item.Pos;
	GInventoryDrag.bRemovedFromSource = false; // keep item in bag until drop is confirmed (prevents other clients seeing it vanish mid-drag)
	GInventoryDrag.bActive = true;
	GInventoryDrag.bFromExchange = false;
	// Notify listeners
	OnItemDragStarted.Broadcast(this, Idx);
	if (IsDragSoundEnabled())
	{
		if (USoundBase* ItemSound = ResolveItemSoundForEvent(this, GInventoryDrag.Item.Item, EYIItemSFXEvent::DragStart))
		{
			UGameplayStatics::PlaySound2D(this, ItemSound);
		}
		else if (DragStartSound)
		{
			UGameplayStatics::PlaySound2D(this, DragStartSound);
		}
	}
	// Clear selection so UI reflects pickup
	SetSelectedCell(FIntPoint(-1, -1));
	UpdateBoundTooltip();
	return true;
}

bool UInventoryGridWidget::BeginDragFromSelectedCell()
{
	if (SelectedCell.X < 0 || SelectedCell.Y < 0) return false;
	return BeginDragFromCell(SelectedCell);
}

bool UInventoryGridWidget::DropDraggedItemAtCell(FIntPoint Cell)
{
	if (!YI_IsGlobalDragValid(GetWorld())) return false;
	if (!Bag) return false;
	UYIInventoryComponent* OwnerComp = Bag->GetTypedOuter<UYIInventoryComponent>();
	const bool bHasOwnerComp = (OwnerComp != nullptr);
	const bool bOwnerCompHasAuthority = bHasOwnerComp && OwnerComp->GetOwner() && OwnerComp->GetOwner()->HasAuthority();
	const auto TryOwnerMoveItem = [OwnerComp, this](int32 Index, const FIntPoint& NewPos) -> bool
	{
		if (!OwnerComp || !Bag || !Bag->Items.IsValidIndex(Index))
		{
			return false;
		}
		const FYIBagItem& ItemToMove = Bag->Items[Index];
		if (ItemToMove.Item.InstanceId.IsValid() && Bag->BagId.IsValid())
		{
			FYIInventoryMoveItemRequest Request;
			Request.ItemRef.Bag.BagId = Bag->BagId;
			Request.ItemRef.Item.ItemInstanceId = ItemToMove.Item.InstanceId;
			Request.TargetCell = NewPos;
			Request.bUseExactCell = false;
			Request.ExpectedSourceBagRevision = Bag->RuntimeRevision;
			return OwnerComp->RequestMoveItem(Request).bRequestAccepted;
		}
		// Fallback only for primary bag / legacy edge cases
		if (OwnerComp->GetBag() == Bag)
		{
			return OwnerComp->MoveItem(Index, NewPos);
		}
		return false;
	};
	const auto TryOwnerRemoveItem = [OwnerComp, this](int32 Index) -> bool
	{
		if (!OwnerComp || !Bag || !Bag->Items.IsValidIndex(Index))
		{
			return false;
		}
		const FYIBagItem& ItemToRemove = Bag->Items[Index];
		if (ItemToRemove.Item.InstanceId.IsValid() && Bag->BagId.IsValid())
		{
			FYIInventoryRemoveItemRequest Request;
			Request.ItemRef.Bag.BagId = Bag->BagId;
			Request.ItemRef.Item.ItemInstanceId = ItemToRemove.Item.InstanceId;
			Request.ExpectedSourceBagRevision = Bag->RuntimeRevision;
			return OwnerComp->RequestRemoveItem(Request).bRequestAccepted;
		}
		if (OwnerComp->GetBag() == Bag)
		{
			return OwnerComp->RemoveItem(Index);
		}
		return false;
	};
	auto PlayDropSound = [this]()
	{
		if (!IsDragSoundEnabled())
		{
			return;
		}
		if (USoundBase* ItemSound = ResolveItemSoundForEvent(this, GInventoryDrag.Item.Item, EYIItemSFXEvent::Drop))
		{
			UGameplayStatics::PlaySound2D(this, ItemSound);
		}
		else if (DropSound)
		{
			UGameplayStatics::PlaySound2D(this, DropSound);
		}
	};
	auto PlayInvalidMoveSound = [this]()
	{
		if (!IsInvalidSoundEnabled())
		{
			return;
		}
		if (USoundBase* ItemSound = ResolveItemSoundForEvent(this, GInventoryDrag.Item.Item, EYIItemSFXEvent::InvalidMove))
		{
			UGameplayStatics::PlaySound2D(this, ItemSound);
		}
		else if (InvalidMoveSound)
		{
			UGameplayStatics::PlaySound2D(this, InvalidMoveSound);
		}
	};

	const auto HasLockedOverlapAtCell = [this, Cell]() -> bool
	{
		if (!Bag)
		{
			return false;
		}

		const FYIBagItem ToPlace = GInventoryDrag.Item;
		const FIntPoint Footprint = Bag->GetEffectiveSize(ToPlace.Size);
		for (int32 ItemIndex = 0; ItemIndex < Bag->Items.Num(); ++ItemIndex)
		{
	if (GInventoryDrag.SourceGrid.Get() == this && ItemIndex == GInventoryDrag.SourceIndex)
			{
				continue;
			}

			const FYIBagItem& ExistingItem = Bag->Items[ItemIndex];
			const FIntPoint ExistingSize = Bag->GetEffectiveSize(ExistingItem.Size);
			if (RectsOverlap(Cell, Footprint, ExistingItem.Pos, ExistingSize) && IsItemIndexLockedForUI(ItemIndex))
			{
				return true;
			}
		}
		return false;
	};
	if (HasLockedOverlapAtCell())
	{
		OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
		PlayInvalidMoveSound();
		return false;
	}

	// Same bag: if this drag originated here and we removed from source, we are placing an unattached item now
	if (GInventoryDrag.SourceGrid.Get() == this)
	{
		// Client same-bag drops should use server-authoritative exact-cell move/swap and avoid local mutation/self-collision desync.
		if (bHasOwnerComp && !bOwnerCompHasAuthority && !GInventoryDrag.bRemovedFromSource && GInventoryDrag.SourceIndex != INDEX_NONE &&
			Bag->Items.IsValidIndex(GInventoryDrag.SourceIndex))
		{
			const FYIBagItem& SourceItem = Bag->Items[GInventoryDrag.SourceIndex];
			if (Bag->BagId.IsValid() && SourceItem.Item.InstanceId.IsValid())
			{
				FYIInventoryMoveItemRequest Request;
				Request.ItemRef.Bag.BagId = Bag->BagId;
				Request.ItemRef.Item.ItemInstanceId = SourceItem.Item.InstanceId;
				Request.TargetCell = Cell;
				Request.bUseExactCell = true;
				Request.bAllowSingleOverlapSwap = true;
				Request.ExpectedSourceBagRevision = Bag->RuntimeRevision;
				const bool bRequested = OwnerComp->RequestMoveItem(Request).bRequestAccepted;
				OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, bRequested);
				if (!bRequested)
				{
					PlayInvalidMoveSound();
					return false;
				}
				PlayDropSound();
				GInventoryDrag.Reset();
				UpdateBoundTooltip();
				return true;
			}
		}

		if (!bAllowSelfMove)
		{
			OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
			PlayInvalidMoveSound();
			return false;
		}
		// When pickup removed the item, SourceIndex is INDEX_NONE and the bag no longer contains it. Treat as add-at-cell or swap.
		if (GInventoryDrag.bRemovedFromSource && GInventoryDrag.SourceIndex == INDEX_NONE)
		{
			FYIBagItem ToPlace = GInventoryDrag.Item; ToPlace.Pos = Cell;
			// Enforce exact placement at the highlighted cell; do not allow AddBagItem to relocate to first-fit
			if (Bag->CanPlaceAt(Cell, ToPlace.Size))
			{
				// Temporarily disable auto-merge to enforce exact placement at target cell
				bool bSavedAutoMerge = Bag->bAutoMergeOnAdd; Bag->bAutoMergeOnAdd = false;
				int32 NewIdx = Bag->AddBagItem(ToPlace);
				Bag->bAutoMergeOnAdd = bSavedAutoMerge;
				if (NewIdx != INDEX_NONE)
				{
					OnItemDropped.Broadcast(this, INDEX_NONE, Cell, true);
					// Drag ends because no victim was displaced
					GInventoryDrag.Reset();
					UpdateBoundTooltip();
					return true;
				}
			}
			// If we can't place exactly at Cell, try displacing a single overlapped item and continue dragging that victim
			int32 Victim = INDEX_NONE; const FIntPoint Foot = Bag->GetEffectiveSize(ToPlace.Size);
			if (!FindSingleOverlap(Bag, INDEX_NONE, Cell, Foot, Victim) || Victim == INDEX_NONE)
			{
				OnItemDropped.Broadcast(this, INDEX_NONE, Cell, false);
				PlayInvalidMoveSound();
				return false;
			}
			// Displace victim: remove it from the bag and continue dragging it (no swap/backfill)
			FYIBagItem SavedVictim = Bag->Items[Victim];
			if (!Bag->RemoveItem(Victim))
			{
				OnItemDropped.Broadcast(this, INDEX_NONE, Cell, false);
				PlayInvalidMoveSound();
				return false;
			}
			// Place dragged item at Cell (exact)
			int32 NewIdx = INDEX_NONE;
			if (Bag->CanPlaceAt(Cell, ToPlace.Size))
			{
				NewIdx = Bag->AddBagItem(ToPlace);
			}
			if (NewIdx == INDEX_NONE)
			{
				// Rollback: reinsert victim to its original spot
				SavedVictim.Pos = SavedVictim.Pos; // unchanged
				Bag->AddBagItem(SavedVictim);
				OnItemDropped.Broadcast(this, INDEX_NONE, Cell, false);
				PlayInvalidMoveSound();
				return false;
			}
			OnItemDropped.Broadcast(this, INDEX_NONE, Cell, true);
			// Continue dragging the displaced item (victim) as UNATTACHED (no lingering visual)
			GInventoryDrag.SourceGrid = this;
			GInventoryDrag.SourceIndex = INDEX_NONE;
			GInventoryDrag.Item = SavedVictim;
			GInventoryDrag.bRemovedFromSource = true;
			GInventoryDrag.bFromExchange = true;
			GInventoryDrag.SourcePos = SavedVictim.Pos;
			GInventoryDrag.bActive = true;
			OnItemDragStarted.Broadcast(this, INDEX_NONE);
			UpdateBoundTooltip();
			return true;
		}
		if (!((bHasOwnerComp ? TryOwnerMoveItem(GInventoryDrag.SourceIndex, Cell) : Bag->MoveItem(GInventoryDrag.SourceIndex, Cell))))
		{
			// Non-authority clients should not attempt local in-place swaps; server-authoritative path above handles exact-cell move/swap.
			if (bHasOwnerComp && !bOwnerCompHasAuthority)
			{
				OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
				PlayInvalidMoveSound();
				return false;
			}
			// Allow displacing a single overlapped item if the footprint only hits that one
			const FYIBagItem& Src = Bag->Items[GInventoryDrag.SourceIndex];
			const FIntPoint Foot = Bag->GetEffectiveSize(Src.Size);
			int32 Victim = INDEX_NONE;
			if (!FindSingleOverlap(Bag, GInventoryDrag.SourceIndex, Cell, Foot, Victim) || Victim == INDEX_NONE)
			{
				OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
				PlayInvalidMoveSound();
				return false;
			}
			// Remove victim, adjust source index if needed, then attempt move again
			FYIBagItem SavedVictim = Bag->Items[Victim];
			if (bHasOwnerComp) TryOwnerRemoveItem(Victim); else Bag->RemoveItem(Victim);
			if (Victim < GInventoryDrag.SourceIndex)
			{
				GInventoryDrag.SourceIndex -= 1;
			}
			if (!((bHasOwnerComp ? TryOwnerMoveItem(GInventoryDrag.SourceIndex, Cell) : Bag->MoveItem(GInventoryDrag.SourceIndex, Cell))))
			{
			// Failed even after clearing victim; restore it in-place to avoid merge/stack side-effects
			Bag->Items.Insert(SavedVictim, Victim);
			Bag->MarkPackageDirty(); Bag->OnChanged.Broadcast();
				OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
				PlayInvalidMoveSound();
				return false;
			}
			// Start a new drag with the displaced item
			GInventoryDrag.SourceGrid = nullptr;
			GInventoryDrag.SourceIndex = INDEX_NONE;
			GInventoryDrag.Item = SavedVictim;
			GInventoryDrag.bActive = true;
			OnItemDragStarted.Broadcast(this, INDEX_NONE);
		}
		OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, true);
		PlayDropSound();
		if (GInventoryDrag.SourceGrid.IsValid() || GInventoryDrag.SourceIndex != INDEX_NONE)
		{
			GInventoryDrag.Reset();
		}
		UpdateBoundTooltip();
		return true;
	}

	// Cross-bag: perform an atomic swap (drag item takes victim's slot, victim becomes active drag or drops)
	FYIBagItem ToPlace = GInventoryDrag.Item;
	ToPlace.Pos = Cell;

	// Optional feature adapters can intercept cross-grid drops (trade/shop/etc) before core bag transfer logic.
	if (UInventoryGridWidget* SourceGrid = GInventoryDrag.SourceGrid.Get())
	{
		auto TryAdapterDrop = [&](UYIInventoryGridFeatureAdapter* Adapter) -> TOptional<bool>
		{
			if (!Adapter)
			{
				return {};
			}
			const EYIInventoryGridExternalOpResult Result = Adapter->TryHandleCrossGridDrop(
				this,
				SourceGrid,
				GInventoryDrag.SourceIndex,
				GInventoryDrag.Item,
				Cell);
			if (Result == EYIInventoryGridExternalOpResult::NotHandled)
			{
				return {};
			}
			const bool bSuccess = (Result == EYIInventoryGridExternalOpResult::HandledSucceeded);
			OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, bSuccess);
			if (bSuccess)
			{
				PlayDropSound();
				GInventoryDrag.Reset();
				SourceGrid->RefreshBoundTooltip();
				RefreshBoundTooltip();
			}
			else
			{
				PlayInvalidMoveSound();
			}
			return bSuccess;
		};

		if (const TOptional<bool> DestHandled = TryAdapterDrop(FeatureAdapter))
		{
			return DestHandled.GetValue();
		}
		if (SourceGrid != this)
		{
			if (const TOptional<bool> SrcHandled = TryAdapterDrop(SourceGrid->FeatureAdapter))
			{
				return SrcHandled.GetValue();
			}
		}
	}

	UYIItemDefinition* DraggedDef = ToPlace.Item.Definition.IsValid()
		? ToPlace.Item.Definition.Get()
		: ToPlace.Item.Definition.LoadSynchronous();
	if (!DraggedDef || !Bag->CanAcceptItemDefinition(DraggedDef))
	{
		OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
		PlayInvalidMoveSound();
		return false;
	}

	// Inventory-owned cross-bag transfer/swap path (server-authoritative, explicit bag-targeted RPC).
	if (UInventoryGridWidget* DragSourceGrid = GInventoryDrag.SourceGrid.Get())
	{
		UYIInventoryBag* SourceBag = DragSourceGrid->Bag;
		UYIInventoryComponent* DestOwnerComp = Bag ? Bag->GetTypedOuter<UYIInventoryComponent>() : nullptr;
		UYIInventoryComponent* SourceOwnerComp = SourceBag ? SourceBag->GetTypedOuter<UYIInventoryComponent>() : nullptr;
		if (DestOwnerComp && DestOwnerComp == SourceOwnerComp &&
			SourceBag && SourceBag != Bag &&
			!GInventoryDrag.bRemovedFromSource &&
			GInventoryDrag.SourceIndex != INDEX_NONE &&
			SourceBag->Items.IsValidIndex(GInventoryDrag.SourceIndex) &&
			SourceBag->BagId.IsValid() && Bag->BagId.IsValid())
		{
			const FYIBagItem& SourceItem = SourceBag->Items[GInventoryDrag.SourceIndex];
			if (SourceItem.Item.InstanceId.IsValid())
			{
				FYIInventoryTransferItemRequest Request;
				Request.ItemRef.Bag.BagId = SourceBag->BagId;
				Request.ItemRef.Item.ItemInstanceId = SourceItem.Item.InstanceId;
				Request.DestBagId = Bag->BagId;
				Request.bUseExactCell = true;
				Request.DestCell = Cell;
				Request.Count = 0;
				Request.bAllowSingleOverlapSwap = true;
				Request.ExpectedSourceBagRevision = SourceBag->RuntimeRevision;
				Request.ExpectedDestBagRevision = Bag->RuntimeRevision;
				const bool bTransferred = DestOwnerComp->RequestTransferItem(Request).bRequestAccepted;
				OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, bTransferred);
				if (!bTransferred)
				{
					PlayInvalidMoveSound();
					return false;
				}

				PlayDropSound();
				DragSourceGrid->RefreshBoundTooltip();
				RefreshBoundTooltip();
				OnItemTransferred.Broadcast(DragSourceGrid, GInventoryDrag.SourceIndex, INDEX_NONE);
				if (DragSourceGrid != this)
				{
					DragSourceGrid->OnItemTransferred.Broadcast(DragSourceGrid, GInventoryDrag.SourceIndex, INDEX_NONE);
				}
				GInventoryDrag.Reset();
				return true;
			}
		}
	}
	
	// If we are not authoritative, do not mutate bags directly for cross-bag operations.
	if (OwnerComp && OwnerComp->GetOwner() && !OwnerComp->GetOwner()->HasAuthority())
	{
		OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
		PlayInvalidMoveSound();
		return false;
	}
	
	// Check if placement at Cell is possible or if we need a victim swap
	int32 VictimIdx = INDEX_NONE;
	const FIntPoint Foot = Bag->GetEffectiveSize(ToPlace.Size);
	
	// First try to place without a victim
	if (Bag->CanPlaceAt(Cell, ToPlace.Size))
	{
		// Clean placement: just add and remove from source
		// Temporarily disable auto-merge to avoid merging dragged item into existing stacks during cross-bag direct placement
		int32 NewIdx; { bool bSavedAutoMerge = Bag->bAutoMergeOnAdd; Bag->bAutoMergeOnAdd = false; NewIdx = Bag->AddBagItem(ToPlace); Bag->bAutoMergeOnAdd = bSavedAutoMerge; }
		if (NewIdx != INDEX_NONE)
		{
			UInventoryGridWidget* DragSourceGrid = GInventoryDrag.SourceGrid.Get();
			if (DragSourceGrid && DragSourceGrid->Bag)
{
// If item was already removed at pickup, skip removing now
				if (GInventoryDrag.bRemovedFromSource || GInventoryDrag.SourceIndex == INDEX_NONE || DragSourceGrid->Bag->RemoveItem(GInventoryDrag.SourceIndex))
				{
					OnItemTransferred.Broadcast(DragSourceGrid, GInventoryDrag.SourceIndex, NewIdx);
					if (DragSourceGrid != this) DragSourceGrid->OnItemTransferred.Broadcast(DragSourceGrid, GInventoryDrag.SourceIndex, NewIdx);
					DragSourceGrid->RefreshBoundTooltip();
					RefreshBoundTooltip();
					OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, true);
					PlayDropSound();
					GInventoryDrag.Reset();
					return true;
				}
				else
				{
					// Remove from source failed; undo the add
					Bag->RemoveItem(NewIdx);
					OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
					PlayInvalidMoveSound();
					GInventoryDrag.Reset();
					return false;
				}
			}
			else
			{
				// Unattached drag (picked up from world) â€” accept the placement
				UE_LOG(LogTemp, Warning, TEXT("Inventory: Placed unattached dragged item into bag at index %d."), NewIdx);
				RefreshBoundTooltip();
				OnItemDropped.Broadcast(this, INDEX_NONE, Cell, true);
				PlayDropSound();
				GInventoryDrag.Reset();
				return true;
			}
		}
		// If we get here with NewIdx == INDEX_NONE, fall through to victim logic
	}
	
	// Placement failed or AddBagItem returned INDEX_NONE; try victim swap
	if (!FindSingleOverlap(Bag, INDEX_NONE, Cell, Foot, VictimIdx) || VictimIdx == INDEX_NONE)
	{
		OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
		PlayInvalidMoveSound();
		return false;
	}

	// We have a victim. Now perform an ATOMIC swap across bags:
	// 1. Save both items
	FYIBagItem SavedDragged = GInventoryDrag.Item;
	FYIBagItem SavedVictim = Bag->Items[VictimIdx];
	const FIntPoint SavedVictimPos = SavedVictim.Pos;

	auto RestoreVictim = [this, &SavedVictim, SavedVictimPos]() -> bool
	{
		FYIBagItem VictimToRestore = SavedVictim;
		VictimToRestore.Pos = SavedVictimPos;
		if (!Bag->CanPlaceAt(VictimToRestore.Pos, VictimToRestore.Size))
		{
			return false;
		}
		const bool bSavedAutoMerge = Bag->bAutoMergeOnAdd;
		Bag->bAutoMergeOnAdd = false;
		const int32 RestoreIdx = Bag->AddBagItem(VictimToRestore);
		Bag->bAutoMergeOnAdd = bSavedAutoMerge;
		return RestoreIdx != INDEX_NONE;
	};

	// Remove victim first so destination constraints are re-evaluated through AddBagItem.
	if (!Bag->RemoveItem(VictimIdx))
	{
		OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
		PlayInvalidMoveSound();
		return false;
	}

	ToPlace.Pos = Cell;
	SavedDragged.Pos = Cell;
	if (!Bag->CanPlaceAt(Cell, ToPlace.Size))
	{
		RestoreVictim();
		OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
		PlayInvalidMoveSound();
		return false;
	}

	const bool bSavedAutoMerge = Bag->bAutoMergeOnAdd;
	Bag->bAutoMergeOnAdd = false;
	const int32 DraggedDestIndex = Bag->AddBagItem(ToPlace);
	Bag->bAutoMergeOnAdd = bSavedAutoMerge;
	if (DraggedDestIndex == INDEX_NONE)
	{
		RestoreVictim();
		OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
		PlayInvalidMoveSound();
		return false;
	}

	// Remove the original dragged item from source bag only after destination placement succeeds.
	UInventoryGridWidget* DragSourceGrid = GInventoryDrag.SourceGrid.Get();
	if (DragSourceGrid && DragSourceGrid->Bag)
	{
		UYIInventoryBag* SourceBag = DragSourceGrid->Bag;
		const int32 SourceIdx = GInventoryDrag.SourceIndex;
		if (!GInventoryDrag.bRemovedFromSource && SourceIdx != INDEX_NONE)
		{
			const bool bRemoved = SourceBag->RemoveItem(SourceIdx);
			if (!bRemoved)
			{
				Bag->RemoveItem(DraggedDestIndex);
				RestoreVictim();
				OnItemDropped.Broadcast(this, SourceIdx, Cell, false);
				PlayInvalidMoveSound();
				GInventoryDrag.Reset();
				return false;
			}

			OnItemTransferred.Broadcast(DragSourceGrid, SourceIdx, DraggedDestIndex);
			if (DragSourceGrid != this)
			{
				DragSourceGrid->OnItemTransferred.Broadcast(DragSourceGrid, SourceIdx, DraggedDestIndex);
			}
			DragSourceGrid->RefreshBoundTooltip();
		}
	}
	
	// 5. Set victim as active drag for the next placement
	RefreshBoundTooltip();
	OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, true);
	PlayDropSound();

	// Update drag state: the victim is now at VictimIdx in this bag (linked, not unattached)
	// If BeginDragFromCell is called again, it will just pick up the victim from its new location
	GInventoryDrag.SourceGrid = this;
	GInventoryDrag.SourceIndex = INDEX_NONE;
	GInventoryDrag.Item = SavedVictim;
	GInventoryDrag.bRemovedFromSource = true; // victim no longer exists in any bag after being displaced
	GInventoryDrag.bFromExchange = true;
	GInventoryDrag.SourcePos = SavedVictim.Pos;
	GInventoryDrag.bActive = true;
	OnItemDragStarted.Broadcast(this, INDEX_NONE);
	return true;
}

void UInventoryGridWidget::CancelDrag()
{
	if (GInventoryDrag.bActive)
	{
		if (!YI_IsGlobalDragValid(GetWorld()))
		{
			if (!GInventoryDrag.SourceGrid.IsValid() || !GInventoryDrag.DragGI.IsValid())
			{
				GInventoryDrag.Reset();
			}
			return;
		}

		if (IsDragSoundEnabled())
		{
			if (USoundBase* ItemSound = ResolveItemSoundForEvent(this, GInventoryDrag.Item.Item, EYIItemSFXEvent::Cancel))
			{
				UGameplayStatics::PlaySound2D(this, ItemSound);
			}
			else if (CancelDragSound)
			{
				UGameplayStatics::PlaySound2D(this, CancelDragSound);
			}
		}

		bool bDroppedToWorld = false;
		// If we removed the item from its source bag at pickup, try to restore it at its original position
		UInventoryGridWidget* DragSourceGrid = GInventoryDrag.SourceGrid.Get();
		if (GInventoryDrag.bRemovedFromSource && DragSourceGrid && DragSourceGrid->Bag)
		{
			UYIInventoryBag* SrcBag = DragSourceGrid->Bag;
			FYIBagItem Restore = GInventoryDrag.Item;

			if (GInventoryDrag.bFromExchange)
			{
				// Exchange drag: try original victim cell, then any fit, else drop to world
				bool bPlaced = false;
				if (SrcBag->CanPlaceAt(GInventoryDrag.SourcePos, Restore.Size))
				{
					Restore.Pos = GInventoryDrag.SourcePos;
					bPlaced = (SrcBag->AddBagItem(Restore) != INDEX_NONE);
				}
				if (!bPlaced)
				{
					FIntPoint FitPos;
					if (SrcBag->FindFirstFit(Restore.Size, FitPos))
					{
						Restore.Pos = FitPos;
						bPlaced = (SrcBag->AddBagItem(Restore) != INDEX_NONE);
					}
				}
				if (!bPlaced)
				{
					if (UYIInventoryComponent* OwnerComp = SrcBag->GetTypedOuter<UYIInventoryComponent>())
					{
						if (AActor* Owner = OwnerComp->GetOwner())
						{
							const FVector SpawnLoc = Owner->GetActorLocation() + Owner->GetActorForwardVector() * 80.f;
							const FTransform SpawnTransform(Owner->GetActorRotation(), SpawnLoc);
							if (DragSourceGrid->FeatureAdapter &&
								DragSourceGrid->FeatureAdapter->TrySpawnWorldDropFromInstance(this, Restore.Item, SpawnTransform))
							{
								bDroppedToWorld = true;
							}
						}
					}
				}
			}
			else
			{
				Restore.Pos = GInventoryDrag.SourcePos;
				// Try to add back; if it fails (blocked now), try to add anywhere to avoid loss
				if (SrcBag->AddBagItem(Restore) == INDEX_NONE)
				{
					FIntPoint FitPos;
					if (SrcBag->FindFirstFit(Restore.Size, FitPos))
					{
						Restore.Pos = FitPos;
						SrcBag->AddBagItem(Restore);
					}
				}
			}
		}
		OnItemDragCancelled.Broadcast(GInventoryDrag.SourceGrid.Get(), GInventoryDrag.Item.Item, bDroppedToWorld);
		GInventoryDrag.Reset();
	}
}

bool UInventoryGridWidget::IsItemDragActive(const UWorld* ContextWorld)
{
	return YI_IsGlobalDragValid(ContextWorld);
}

bool UInventoryGridWidget::GetActiveDraggedItem(FYIBagItem& OutItem, UYIInventoryBag*& OutSourceBag, const UWorld* ContextWorld)
{
	if (!YI_IsGlobalDragValid(ContextWorld))
	{
		OutSourceBag = nullptr;
		return false;
	}
	OutItem = GInventoryDrag.Item;
	UInventoryGridWidget* SourceGrid = GInventoryDrag.SourceGrid.Get();
	OutSourceBag = SourceGrid ? SourceGrid->Bag : nullptr;
	return true;
}

bool UInventoryGridWidget::TryEquipActiveDraggedItem(UObject* EquipmentContextObject, FGameplayTag RequestedSlotTag)
{
	if (!EquipmentContextObject)
	{
		return false;
	}

	if (UWorld* World = EquipmentContextObject->GetWorld())
	{
		if (!YI_IsGlobalDragValid(World))
		{
			return false;
		}
	}
	else if (!YI_IsGlobalDragValid(nullptr))
	{
		return false;
	}

	UInventoryGridWidget* SourceGrid = GInventoryDrag.SourceGrid.Get();
	if (!SourceGrid || !SourceGrid->Bag)
	{
		return false;
	}

	UYIInventoryComponent* SourceInventory = SourceGrid->Bag->GetTypedOuter<UYIInventoryComponent>();
	if (!SourceInventory)
	{
		return false;
	}

	int32 SourceIndexToEquip = GInventoryDrag.SourceIndex;
	int32 TempInsertedIndex = INDEX_NONE;

	// Detached drags (unequip flow) carry an item that is currently not inside the bag.
	// Reinsert temporarily so normal equipment validation/path can consume it.
	if (SourceIndexToEquip == INDEX_NONE && GInventoryDrag.bRemovedFromSource)
	{
		FYIBagItem RestoreItem = GInventoryDrag.Item;
		RestoreItem.Pos = GInventoryDrag.SourcePos;

		const bool bSavedAutoMerge = SourceGrid->Bag->bAutoMergeOnAdd;
		SourceGrid->Bag->bAutoMergeOnAdd = false;
		TempInsertedIndex = SourceGrid->Bag->AddBagItem(RestoreItem);
		SourceGrid->Bag->bAutoMergeOnAdd = bSavedAutoMerge;
		if (TempInsertedIndex == INDEX_NONE)
		{
			return false;
		}

		SourceIndexToEquip = TempInsertedIndex;
	}

	if (SourceIndexToEquip == INDEX_NONE)
	{
		return false;
	}

	UYIInventoryGridFeatureAdapter* Adapter = SourceGrid->GetFeatureAdapter();
	if (!Adapter)
	{
		if (TempInsertedIndex != INDEX_NONE)
		{
			SourceGrid->Bag->RemoveItem(TempInsertedIndex);
			SourceGrid->RefreshBoundTooltip();
		}
		return false;
	}

	const bool bEquipped = Adapter->TryEquipItemFromInventory(EquipmentContextObject, SourceInventory, SourceIndexToEquip, RequestedSlotTag);
	if (bEquipped)
	{
		SourceGrid->RefreshBoundTooltip();
		GInventoryDrag.Reset();
		return true;
	}

	// Equip failed: if we reinserted a detached drag item, remove it again and keep drag active.
	if (TempInsertedIndex != INDEX_NONE)
	{
		bool bRemovedRollback = false;
		if (SourceInventory->GetOwner() && SourceInventory->GetOwner()->HasAuthority() && SourceInventory->GetBag() == SourceGrid->Bag)
		{
			bRemovedRollback = SourceInventory->RemoveItem(TempInsertedIndex);
		}
		if (!bRemovedRollback)
		{
			SourceGrid->Bag->RemoveItem(TempInsertedIndex);
		}
		SourceGrid->RefreshBoundTooltip();
	}
	return false;
}

UInventoryGridWidget* UInventoryGridWidget::FindRegisteredGridForBag(UYIInventoryBag* InBag, const UWorld* ContextWorld)
{
	if (!InBag)
	{
		return nullptr;
	}

	for (auto It = GRegisteredGrids.CreateIterator(); It; ++It)
	{
		UInventoryGridWidget* Grid = It->Get();
		if (!Grid)
		{
			It.RemoveCurrent();
			continue;
		}
		if (ContextWorld && Grid->GetWorld() && Grid->GetWorld()->GetGameInstance() != ContextWorld->GetGameInstance())
		{
			continue;
		}
		if (Grid->Bag == InBag)
		{
			return Grid;
		}
	}

	return nullptr;
}

bool UInventoryGridWidget::BeginDragFromBagItem(UYIInventoryBag* InBag, int32 ItemIndex, const UWorld* ContextWorld)
{
	if (!InBag || !InBag->Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	UInventoryGridWidget* Grid = FindRegisteredGridForBag(InBag, ContextWorld);
	if (!Grid)
	{
		return false;
	}

	return Grid->BeginDragFromCell(InBag->Items[ItemIndex].Pos);
}

bool UInventoryGridWidget::BeginDetachedDragFromBagItem(UYIInventoryBag* InBag, int32 ItemIndex, const UWorld* ContextWorld)
{
	if (!InBag || !InBag->Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	UInventoryGridWidget* Grid = FindRegisteredGridForBag(InBag, ContextWorld);
	if (!Grid)
	{
		return false;
	}

	if (Grid->IsItemIndexLockedForUI(ItemIndex))
	{
		return false;
	}

	const FYIBagItem DraggedItem = InBag->Items[ItemIndex];
	const FIntPoint DragSourcePos = DraggedItem.Pos;

	bool bRemovedFromBag = false;
	if (UYIInventoryComponent* OwnerComp = InBag->GetTypedOuter<UYIInventoryComponent>())
	{
		// Prefer explicit bag-targeted server-authoritative mutation for inventory-owned bags.
		if (InBag->BagId.IsValid() && DraggedItem.Item.InstanceId.IsValid())
		{
			bRemovedFromBag = OwnerComp->RemoveItemFromBag(InBag->BagId, DraggedItem.Item.InstanceId);
		}
		else if (OwnerComp->GetOwner() && OwnerComp->GetOwner()->HasAuthority() && OwnerComp->GetBag() == InBag)
		{
			bRemovedFromBag = OwnerComp->RemoveItem(ItemIndex);
		}
	}
	if (!bRemovedFromBag)
	{
		bRemovedFromBag = InBag->RemoveItem(ItemIndex);
	}
	if (!bRemovedFromBag)
	{
		return false;
	}

	// Only allow drag within this game instance (prevents cross-PIE bleed).
	if (UWorld* World = Grid->GetWorld())
	{
		GInventoryDrag.DragGI = World->GetGameInstance();
	}
	GInventoryDrag.SourceGrid = Grid;
	GInventoryDrag.SourceIndex = INDEX_NONE;
	GInventoryDrag.SourcePos = DragSourcePos;
	GInventoryDrag.Item = DraggedItem;
	GInventoryDrag.bRemovedFromSource = true;
	GInventoryDrag.bActive = true;
	GInventoryDrag.bFromExchange = false;

	Grid->OnItemDragStarted.Broadcast(Grid, INDEX_NONE);
	Grid->SetSelectedCell(FIntPoint(-1, -1));
	Grid->RefreshBoundTooltip();
	return true;
}

bool UInventoryGridWidget::IsItemIndexLockedForUI(int32 ItemIndex) const
{
	if (!Bag || !Bag->Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	const UYIInventoryComponent* OwnerInventory = BoundInventoryComponent
		? BoundInventoryComponent.Get()
		: Bag->GetTypedOuter<UYIInventoryComponent>();
	return OwnerInventory && OwnerInventory->IsBagItemLocked(Bag, ItemIndex);
}

void UInventoryGridWidget::HandleCellClicked(const FIntPoint& Cell)
{
	// If a drag is active, attempt drop; otherwise start a drag from the clicked cell
	if (YI_IsGlobalDragValid(GetWorld()))
	{
		// Attempt drop on this cell
		DropDraggedItemAtCell(Cell);
		return;
	}
	// No active drag: click only updates selection; dragging starts on drag detect
}

void UInventoryGridWidget::UpdateBoundTooltip()
{
	FYITooltipData Data;
	FYIRequirementContext Ctx;
	if (RequirementAbilitySystem.IsValid()) { Ctx.AbilitySystem = RequirementAbilitySystem; }
	Ctx.OwnedTags = RequirementOwnedTags;
	Ctx.XP = RequirementXP;
	Ctx.PreviewAttributes = RequirementPreviewAttributes;

	bool bGot = GetSelectedCellTooltipData(Data, Ctx);
	if (!bGot && Bag && HoveredItemIndexCached != INDEX_NONE)
	{
		bGot = UYIInventoryBlueprintLibrary::GetItemTooltipData(Bag, HoveredItemIndexCached, Data, Ctx);
	}

	if (bGot)
	{
		if (BoundTooltipWidget)
		{
			YI_UpdateTooltipWidget(BoundTooltipWidget, Data);
		}
		OnTooltipDataUpdated.Broadcast(Data);
	}
	else
	{
		// Clear tooltip by sending empty data
		if (BoundTooltipWidget)
		{
			YI_ClearTooltipWidget(BoundTooltipWidget);
		}
		OnTooltipCleared.Broadcast();
	}
}

void UInventoryGridWidget::RefreshBoundTooltip()
{
	UpdateBoundTooltip();
}

bool UInventoryGridWidget::TransferSelectedItemTo(UInventoryGridWidget* Other, int32 Count, int32& OutDestIndex)
{
	OutDestIndex = INDEX_NONE;
	if (!Other || !Bag || !Other->Bag) return false;
	int32 SourceIndex = GetSelectedItemIndex();
	if (SourceIndex == INDEX_NONE) return false;

	// Feature adapters can route transfer actions (trade/shop/etc) before core bag transfer logic.
	auto TryAdapterTransfer = [&](UYIInventoryGridFeatureAdapter* Adapter) -> TOptional<bool>
	{
		if (!Adapter)
		{
			return {};
		}
		const EYIInventoryGridExternalOpResult Result = Adapter->TryHandleTransferSelectedTo(this, Other, SourceIndex, Count, OutDestIndex);
		if (Result == EYIInventoryGridExternalOpResult::NotHandled)
		{
			return {};
		}
		const bool bSuccess = (Result == EYIInventoryGridExternalOpResult::HandledSucceeded);
		if (bSuccess)
		{
			UpdateBoundTooltip();
			Other->RefreshBoundTooltip();
			OnItemTransferred.Broadcast(this, SourceIndex, OutDestIndex);
			Other->OnItemTransferred.Broadcast(this, SourceIndex, OutDestIndex);
		}
		return bSuccess;
	};
	if (const TOptional<bool> Handled = TryAdapterTransfer(FeatureAdapter))
	{
		return Handled.GetValue();
	}
	if (Other != this)
	{
		if (const TOptional<bool> OtherHandled = TryAdapterTransfer(Other->FeatureAdapter))
		{
			return OtherHandled.GetValue();
		}
	}

	// Non-trade: only allow direct transfer on authority.
	UYIInventoryComponent* OwnerComp = Bag->GetTypedOuter<UYIInventoryComponent>();
	UYIInventoryComponent* OtherOwnerComp = Other->Bag->GetTypedOuter<UYIInventoryComponent>();

	// Prefer explicit bag-targeted component API when both bags belong to the same inventory component.
	if (OwnerComp && OwnerComp == OtherOwnerComp &&
		Bag->BagId.IsValid() && Other->Bag->BagId.IsValid() &&
		Bag->Items.IsValidIndex(SourceIndex) && Bag->Items[SourceIndex].Item.InstanceId.IsValid())
	{
		FYIInventoryTransferItemRequest Request;
		Request.ItemRef.Bag.BagId = Bag->BagId;
		Request.ItemRef.Item.ItemInstanceId = Bag->Items[SourceIndex].Item.InstanceId;
		Request.DestBagId = Other->Bag->BagId;
		Request.Count = Count;
		Request.ExpectedSourceBagRevision = Bag->RuntimeRevision;
		Request.ExpectedDestBagRevision = Other->Bag->RuntimeRevision;
		const bool bTransferred = OwnerComp->RequestTransferItem(Request).bRequestAccepted;
		if (bTransferred)
		{
			UpdateBoundTooltip();
			Other->RefreshBoundTooltip();
			OnItemTransferred.Broadcast(this, SourceIndex, OutDestIndex);
			Other->OnItemTransferred.Broadcast(this, SourceIndex, OutDestIndex);
		}
		return bTransferred;
	}

	if (OwnerComp && OwnerComp->GetOwner() && !OwnerComp->GetOwner()->HasAuthority())
	{
		return false;
	}

	const bool b = UYIInventoryBlueprintLibrary::TransferItemBetweenBags(Bag, Other->Bag, SourceIndex, Count, OutDestIndex);
	if (b)
	{
		// Refresh our tooltip and the other grid's tooltip
		UpdateBoundTooltip();
		Other->RefreshBoundTooltip();
		// Notify grid-level listeners
		OnItemTransferred.Broadcast(this, SourceIndex, OutDestIndex);
		Other->OnItemTransferred.Broadcast(this, SourceIndex, OutDestIndex);
	}
	return b;
}

void UInventoryGridWidget::SetBoundTooltipWidget(UUserWidget* Widget)
{
	BoundTooltipWidget = Widget;
	UpdateBoundTooltip();
}

void UInventoryGridWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	if (CachedBoundInventoryComponent.IsValid())
	{
		CachedBoundInventoryComponent->OnBagOpened.RemoveDynamic(this, &UInventoryGridWidget::HandleInventoryBagOpened);
		CachedBoundInventoryComponent->OnBagClosed.RemoveDynamic(this, &UInventoryGridWidget::HandleInventoryBagClosed);
		CachedBoundInventoryComponent.Reset();
	}

	// Unregister bag change delegate
	if (CachedBag)
	{
		if (BagChangedHandle.IsValid())
		{
			CachedBag->OnChanged.Remove(BagChangedHandle);
			BagChangedHandle = FDelegateHandle();
		}
		CachedBag = nullptr;
	}

	// Ensure the underlying Slate widget is removed from parent and released
	if (MySlateWidget.IsValid())
	{
		MySlateWidget.Reset();
	}

	// Clear any bound tooltip pointer so it cannot keep Slate/UWidget references alive
	BoundTooltipWidget = nullptr;
}

void UInventoryGridWidget::SetBag(UYIInventoryBag* InBag)
{
	Bag = InBag;
	// Rebind Slate to the new bag immediately
	if (MySlateWidget.IsValid())
	{
		MySlateWidget->SetBag(Bag);
		MySlateWidget->RefreshFromBag();
	}
	// Rebind bag-changed delegate like SynchronizeProperties
	if (CachedBag != Bag)
	{
		if (CachedBag)
		{
			if (BagChangedHandle.IsValid())
			{
				CachedBag->OnChanged.Remove(BagChangedHandle);
				BagChangedHandle = FDelegateHandle();
			}
		}
		if (Bag)
		{
			BagChangedHandle = Bag->OnChanged.AddLambda([this]() { OnBagChanged(); });
		}
		CachedBag = Bag;
	}
	// Ensure tooltip reflects current selection and new bag
	UpdateBoundTooltip();
}

void UInventoryGridWidget::SetBagBindingById(UYIInventoryComponent* InInventoryComponent, const FGuid& InBagId)
{
	BoundInventoryComponent = InInventoryComponent;
	BoundBagId = InBagId;
	BoundBagRoleTag = FGameplayTag();
	bBindToActiveBagContext = false;
	BoundActiveContextTag = FGameplayTag();
	RebindInventoryContextDelegates();
	RefreshBagFromBinding();
}

void UInventoryGridWidget::SetBagBindingByRole(UYIInventoryComponent* InInventoryComponent, FGameplayTag InBagRoleTag)
{
	BoundInventoryComponent = InInventoryComponent;
	BoundBagId.Invalidate();
	BoundBagRoleTag = InBagRoleTag;
	bBindToActiveBagContext = false;
	BoundActiveContextTag = FGameplayTag();
	RebindInventoryContextDelegates();
	RefreshBagFromBinding();
}

void UInventoryGridWidget::SetBagBindingToActiveContext(UYIInventoryComponent* InInventoryComponent, FGameplayTag InContextTag)
{
	BoundInventoryComponent = InInventoryComponent;
	BoundBagId.Invalidate();
	BoundBagRoleTag = FGameplayTag();
	bBindToActiveBagContext = true;
	BoundActiveContextTag = InContextTag;
	RebindInventoryContextDelegates();
	RefreshBagFromBinding();
}

void UInventoryGridWidget::ClearBagBinding()
{
	BoundBagId.Invalidate();
	BoundBagRoleTag = FGameplayTag();
	bBindToActiveBagContext = false;
	BoundActiveContextTag = FGameplayTag();
	BoundInventoryComponent = nullptr;
	RebindInventoryContextDelegates();
}

void UInventoryGridWidget::SetBindingInventoryComponent(UYIInventoryComponent* InInventoryComponent)
{
	BoundInventoryComponent = InInventoryComponent;
	RebindInventoryContextDelegates();
	RefreshBagFromBinding();
}

void UInventoryGridWidget::RebindInventoryContextDelegates()
{
	if (CachedBoundInventoryComponent.IsValid())
	{
		CachedBoundInventoryComponent->OnBagOpened.RemoveDynamic(this, &UInventoryGridWidget::HandleInventoryBagOpened);
		CachedBoundInventoryComponent->OnBagClosed.RemoveDynamic(this, &UInventoryGridWidget::HandleInventoryBagClosed);
		CachedBoundInventoryComponent.Reset();
	}

	if (BoundInventoryComponent)
	{
		BoundInventoryComponent->OnBagOpened.AddDynamic(this, &UInventoryGridWidget::HandleInventoryBagOpened);
		BoundInventoryComponent->OnBagClosed.AddDynamic(this, &UInventoryGridWidget::HandleInventoryBagClosed);
		CachedBoundInventoryComponent = BoundInventoryComponent;
	}
}

void UInventoryGridWidget::RefreshBagFromBinding()
{
	if (!BoundInventoryComponent)
	{
		return;
	}

	UYIInventoryBag* ResolvedBag = nullptr;
	if (bBindToActiveBagContext)
	{
		ResolvedBag = BoundActiveContextTag.IsValid()
			? BoundInventoryComponent->GetActiveContextBag(BoundActiveContextTag)
			: BoundInventoryComponent->GetBagById(BoundInventoryComponent->GetActiveBagId());
		if (!ResolvedBag && !BoundActiveContextTag.IsValid())
		{
			ResolvedBag = BoundInventoryComponent->GetBag();
		}
	}
	else if (BoundBagId.IsValid())
	{
		ResolvedBag = BoundInventoryComponent->GetBagById(BoundBagId);
	}
	else if (BoundBagRoleTag.IsValid())
	{
		ResolvedBag = BoundInventoryComponent->GetBagByRoleTag(BoundBagRoleTag);
	}

	if (ResolvedBag != Bag)
	{
		SetBag(ResolvedBag);
	}
}

void UInventoryGridWidget::HandleInventoryBagOpened(UYIInventoryBag* InBag)
{
	if (!InBag)
	{
		return;
	}
	if (bBindToActiveBagContext)
	{
		RefreshBagFromBinding();
		return;
	}
	if (BoundBagId.IsValid())
	{
		if (InBag->BagId == BoundBagId)
		{
			SetBag(InBag);
		}
		return;
	}
	if (BoundBagRoleTag.IsValid() && InBag->BagRoleTag.IsValid() && InBag->BagRoleTag.MatchesTag(BoundBagRoleTag))
	{
		SetBag(InBag);
	}
}

void UInventoryGridWidget::HandleInventoryBagClosed(UYIInventoryBag* InBag)
{
	if (!InBag)
	{
		return;
	}

	if (Bag == InBag)
	{
		if (bBindToActiveBagContext)
		{
			RefreshBagFromBinding();
		}
		else
		{
			SetBag(nullptr);
		}
	}
}



