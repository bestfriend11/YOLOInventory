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
	FIntPoint AnchorCellOffset = FIntPoint::ZeroValue;
	FYIBagItem Item;
	bool bRemovedFromSource = false; // true if we removed the item from its bag when drag started
	bool bActive = false;
	bool bFromExchange = false;
	TWeakObjectPtr<UGameInstance> DragGI;
	void Reset() { SourceGrid = nullptr; SourceIndex = INDEX_NONE; SourcePos = FIntPoint(-1,-1); AnchorCellOffset = FIntPoint::ZeroValue; Item = FYIBagItem(); bRemovedFromSource = false; bActive = false; bFromExchange = false; DragGI.Reset(); }
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

static bool YI_DoesGlobalDragTouchGridOrBag(const UInventoryGridWidget* Grid, const UYIInventoryBag* Bag)
{
	if (!GInventoryDrag.bActive)
	{
		return false;
	}

	if (GInventoryDrag.SourceGrid.Get() == Grid)
	{
		return true;
	}

	const UInventoryGridWidget* SourceGrid = GInventoryDrag.SourceGrid.Get();
	return Bag && SourceGrid && SourceGrid->Bag == Bag;
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
	if (!Bag) return false;
	return Bag->FindSingleOverlapAt(Pos, Footprint, SourceIdx, OutOverlapIdx);
}

// Helper: get item index at an arbitrary cell in a bag (returns INDEX_NONE)
static int32 GetItemIndexAtCell(const UYIInventoryBag* Bag, const FIntPoint& Cell)
{
	return Bag ? Bag->GetItemIndexAtCellFast(Cell) : INDEX_NONE;
}

static int32 YI_AddBagItemExactNoMerge(UYIInventoryBag* Bag, const FYIBagItem& ItemAtExactPos)
{
	if (!Bag)
	{
		return INDEX_NONE;
	}
	if (!Bag->CanPlaceAt(ItemAtExactPos.Pos, ItemAtExactPos.Size))
	{
		return INDEX_NONE;
	}

	const bool bSavedAutoMerge = Bag->bAutoMergeOnAdd;
	Bag->bAutoMergeOnAdd = false;
	const int32 NewIndex = Bag->AddBagItem(ItemAtExactPos);
	Bag->bAutoMergeOnAdd = bSavedAutoMerge;
	return NewIndex;
}

static bool YI_ShouldContinueDraggingSwappedItem()
{
	return UYOLOInventorySettings::Get().bContinueDraggingSwappedItem;
}

static bool YI_ContinueDraggingLinkedBagItem(UInventoryGridWidget* Grid, int32 ItemIndex)
{
	if (!Grid || !Grid->Bag || !Grid->Bag->Items.IsValidIndex(ItemIndex))
	{
		return false;
	}

	GInventoryDrag.SourceGrid = Grid;
	GInventoryDrag.SourceIndex = ItemIndex;
	GInventoryDrag.Item = Grid->Bag->Items[ItemIndex];
	GInventoryDrag.SourcePos = GInventoryDrag.Item.Pos;
	GInventoryDrag.AnchorCellOffset = FIntPoint::ZeroValue;
	GInventoryDrag.bRemovedFromSource = false;
	GInventoryDrag.bFromExchange = false;
	GInventoryDrag.bActive = true;
	Grid->OnItemDragStarted.Broadcast(Grid, ItemIndex);
	return true;
}

static bool YI_TryLocalAtomicSameBagMoveOrSwap(UYIInventoryBag* Bag, int32 SourceIndex, const FIntPoint& DestCell)
{
	if (!Bag || !Bag->Items.IsValidIndex(SourceIndex))
	{
		return false;
	}

	const FYIBagItem SourceItemCopy = Bag->Items[SourceIndex];
	if (Bag->MoveItem(SourceIndex, DestCell))
	{
		return true;
	}

	int32 VictimIndex = INDEX_NONE;
	if (!Bag->FindSingleOverlapAt(DestCell, SourceItemCopy.Size, SourceIndex, VictimIndex) ||
		VictimIndex == INDEX_NONE ||
		!Bag->Items.IsValidIndex(VictimIndex))
	{
		return false;
	}

	const FYIBagItem VictimItemCopy = Bag->Items[VictimIndex];
	const FIntPoint SourceOriginalPos = SourceItemCopy.Pos;
	const FIntPoint VictimOriginalPos = VictimItemCopy.Pos;

	auto RestoreVictim = [Bag, &VictimItemCopy, VictimOriginalPos]() -> bool
	{
		FYIBagItem RestoreItem = VictimItemCopy;
		RestoreItem.Pos = VictimOriginalPos;
		return YI_AddBagItemExactNoMerge(Bag, RestoreItem) != INDEX_NONE;
	};

	if (!Bag->RemoveItem(VictimIndex))
	{
		return false;
	}

	int32 SourceIndexAfterVictimRemove = INDEX_NONE;
	if (SourceItemCopy.Item.InstanceId.IsValid())
	{
		Bag->FindItemIndexByInstanceIdFast(SourceItemCopy.Item.InstanceId, SourceIndexAfterVictimRemove);
	}
	else
	{
		const int32 AdjustedIndex = SourceIndex - ((VictimIndex < SourceIndex) ? 1 : 0);
		if (Bag->Items.IsValidIndex(AdjustedIndex))
		{
			SourceIndexAfterVictimRemove = AdjustedIndex;
		}
	}

	if (!Bag->Items.IsValidIndex(SourceIndexAfterVictimRemove) || !Bag->RemoveItem(SourceIndexAfterVictimRemove))
	{
		RestoreVictim();
		return false;
	}

	FYIBagItem PlacedSource = SourceItemCopy;
	PlacedSource.Pos = DestCell;
	const int32 NewSourceIdx = YI_AddBagItemExactNoMerge(Bag, PlacedSource);
	if (NewSourceIdx == INDEX_NONE)
	{
		FYIBagItem RestoreSource = SourceItemCopy;
		RestoreSource.Pos = SourceOriginalPos;
		YI_AddBagItemExactNoMerge(Bag, RestoreSource);
		RestoreVictim();
		return false;
	}

	FYIBagItem PlacedVictim = VictimItemCopy;
	PlacedVictim.Pos = SourceOriginalPos;
	const int32 NewVictimIdx = YI_AddBagItemExactNoMerge(Bag, PlacedVictim);
	if (NewVictimIdx == INDEX_NONE)
	{
		Bag->RemoveItem(NewSourceIdx);
		FYIBagItem RestoreSource = SourceItemCopy;
		RestoreSource.Pos = SourceOriginalPos;
		YI_AddBagItemExactNoMerge(Bag, RestoreSource);
		RestoreVictim();
		return false;
	}

	return true;
}

static IYIInventoryGridAdapterInterface* YI_ResolveGridFeatureAdapter(UInventoryGridWidget* Grid)
{
	if (!Grid)
	{
		return nullptr;
	}

	UObject* AdapterObject = Grid->GetFeatureAdapter();
	if (!AdapterObject || !AdapterObject->GetClass()->ImplementsInterface(UYIInventoryGridAdapterInterface::StaticClass()))
	{
		return nullptr;
	}

	return Cast<IYIInventoryGridAdapterInterface>(AdapterObject);
}

static TOptional<bool> YI_TryFeatureTransferRequest(const FYIInventoryGridTransferRequest& Request, int32& OutDestIndex)
{
	auto TryAdapter = [&](IYIInventoryGridAdapterInterface* Adapter) -> TOptional<bool>
	{
		if (!Adapter)
		{
			return {};
		}

		const EYIInventoryGridExternalOpResult Result = Adapter->TryHandleTransferRequest(Request, OutDestIndex);
		if (Result == EYIInventoryGridExternalOpResult::NotHandled)
		{
			return {};
		}

		return Result == EYIInventoryGridExternalOpResult::HandledSucceeded;
	};

	IYIInventoryGridAdapterInterface* DestAdapter = YI_ResolveGridFeatureAdapter(Request.DestGrid);
	if (const TOptional<bool> DestHandled = TryAdapter(DestAdapter))
	{
		return DestHandled;
	}

	IYIInventoryGridAdapterInterface* SourceAdapter = YI_ResolveGridFeatureAdapter(Request.SourceGrid);
	if (SourceAdapter && SourceAdapter != DestAdapter)
	{
		if (const TOptional<bool> SourceHandled = TryAdapter(SourceAdapter))
		{
			return SourceHandled;
		}
	}

	return {};
}

static bool YI_TryCoreInventoryTransferRequest(const FYIInventoryGridTransferRequest& Request, int32& OutDestIndex)
{
	if (!Request.SourceGrid || !Request.DestGrid || !Request.SourceGrid->Bag || !Request.DestGrid->Bag)
	{
		return false;
	}

	UYIInventoryBag* SourceBag = Request.SourceGrid->Bag;
	UYIInventoryBag* DestBag = Request.DestGrid->Bag;
	UYIInventoryComponent* SourceOwnerComp = SourceBag->GetTypedOuter<UYIInventoryComponent>();
	UYIInventoryComponent* DestOwnerComp = DestBag->GetTypedOuter<UYIInventoryComponent>();

	if (SourceOwnerComp && SourceOwnerComp == DestOwnerComp &&
		SourceBag->BagId.IsValid() && DestBag->BagId.IsValid() &&
		SourceBag->Items.IsValidIndex(Request.SourceIndex))
	{
		const FYIBagItem& SourceItem = SourceBag->Items[Request.SourceIndex];
		if (SourceItem.Item.InstanceId.IsValid())
		{
			FYIInventoryTransferItemRequest TransferRequest;
			TransferRequest.ItemRef.Bag.BagId = SourceBag->BagId;
			TransferRequest.ItemRef.Item.ItemInstanceId = SourceItem.Item.InstanceId;
			TransferRequest.DestBagId = DestBag->BagId;
			TransferRequest.bUseExactCell = Request.bHasDestCell;
			TransferRequest.DestCell = Request.DestCell;
			TransferRequest.Count = Request.Count;
			TransferRequest.bAllowSingleOverlapSwap = Request.bHasDestCell;
			TransferRequest.ExpectedSourceBagRevision = SourceBag->RuntimeRevision;
			TransferRequest.ExpectedDestBagRevision = DestBag->RuntimeRevision;
			return SourceOwnerComp->RequestTransferItem(TransferRequest).bRequestAccepted;
		}
	}

	if (SourceOwnerComp && SourceOwnerComp->GetOwner() && !SourceOwnerComp->GetOwner()->HasAuthority())
	{
		return false;
	}

	if (!Request.bHasDestCell)
	{
		return UYIInventoryBlueprintLibrary::TransferItemBetweenBags(
			SourceBag,
			DestBag,
			Request.SourceIndex,
			Request.Count,
			OutDestIndex);
	}

	return false;
}

void UInventoryGridWidget::OnWidgetRebuilt()
{
	Super::OnWidgetRebuilt();
	YI_RegisterInventoryDragCleanup();
	// Only register for drag operations if not in an editor world
	if (UWorld* World = GetWorld())
	{
		if (!World->IsEditorWorld())
		{
			GRegisteredGrids.Add(this);
		}
	}
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
			// Skip editor world grids
			if (UWorld* World = Grid->GetWorld())
			{
				if (World->IsEditorWorld())
				{
					continue;
				}
			}
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

IYIInventoryGridAdapterInterface* UInventoryGridWidget::ResolveFeatureAdapterInterface() const
{
	if (!FeatureAdapter)
	{
		return nullptr;
	}
	if (!FeatureAdapter->GetClass()->ImplementsInterface(UYIInventoryGridAdapterInterface::StaticClass()))
	{
		return nullptr;
	}
	return Cast<IYIInventoryGridAdapterInterface>(FeatureAdapter);
}

void UInventoryGridWidget::SetFeatureAdapter(UObject* InAdapter)
{
	if (InAdapter && !InAdapter->GetClass()->ImplementsInterface(UYIInventoryGridAdapterInterface::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryGridWidget: Ignoring feature adapter '%s' because it does not implement YIInventoryGridAdapterInterface."), *GetNameSafe(InAdapter));
		return;
	}
	FeatureAdapter = InAdapter;
	if (IYIInventoryGridAdapterInterface* Adapter = ResolveFeatureAdapterInterface())
	{
		Adapter->OnAssignedToGrid(this);
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
	
	// Prevent drag operations in editor worlds (preview instances)
	if (UWorld* World = GetWorld())
	{
		if (World->IsEditorWorld())
		{
			return false;
		}
		GInventoryDrag.DragGI = World->GetGameInstance();
	}
	
	GInventoryDrag.SourceGrid = this;
	GInventoryDrag.SourceIndex = Idx;
	GInventoryDrag.Item = Bag->Items[Idx];
	GInventoryDrag.SourcePos = GInventoryDrag.Item.Pos;
	{
		const FIntPoint Eff = Bag->GetEffectiveSize(GInventoryDrag.Item.Size);
		const FIntPoint RawAnchor = Cell - GInventoryDrag.Item.Pos;
		GInventoryDrag.AnchorCellOffset.X = FMath::Clamp(RawAnchor.X, 0, FMath::Max(0, Eff.X - 1));
		GInventoryDrag.AnchorCellOffset.Y = FMath::Clamp(RawAnchor.Y, 0, FMath::Max(0, Eff.Y - 1));
	}
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
		TArray<int32> OverlapIndices;
		const int32 IgnoreIndex = (GInventoryDrag.SourceGrid.Get() == this) ? GInventoryDrag.SourceIndex : INDEX_NONE;
		Bag->GetOverlappingItemIndicesAt(Cell, Footprint, IgnoreIndex, OverlapIndices);
		for (int32 ItemIndex : OverlapIndices)
		{
			if (IsItemIndexLockedForUI(ItemIndex))
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
		if (!bAllowSelfMove)
		{
			OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
			PlayInvalidMoveSound();
			return false;
		}

		// Bound runtime inventories should use the exact-cell authoritative path for both simple moves and swaps.
		if (!GInventoryDrag.bRemovedFromSource &&
			GInventoryDrag.SourceIndex != INDEX_NONE &&
			Bag->Items.IsValidIndex(GInventoryDrag.SourceIndex))
		{
			const FYIBagItem SourceItem = Bag->Items[GInventoryDrag.SourceIndex];
			FGuid SwappedVictimInstanceId;
			if (YI_ShouldContinueDraggingSwappedItem())
			{
				int32 OverlapIndex = INDEX_NONE;
				if (FindSingleOverlap(Bag, GInventoryDrag.SourceIndex, Cell, SourceItem.Size, OverlapIndex) &&
					Bag->Items.IsValidIndex(OverlapIndex))
				{
					SwappedVictimInstanceId = Bag->Items[OverlapIndex].Item.InstanceId;
				}
			}
			bool bMoveAccepted = false;
			if (bHasOwnerComp && Bag->BagId.IsValid() && SourceItem.Item.InstanceId.IsValid())
			{
				FYIInventoryMoveItemRequest Request;
				Request.ItemRef.Bag.BagId = Bag->BagId;
				Request.ItemRef.Item.ItemInstanceId = SourceItem.Item.InstanceId;
				Request.TargetCell = Cell;
				Request.bUseExactCell = true;
				Request.bAllowSingleOverlapSwap = true;
				Request.ExpectedSourceBagRevision = INDEX_NONE;
				bMoveAccepted = OwnerComp->RequestMoveItem(Request).bRequestAccepted;
				if (bMoveAccepted && !bOwnerCompHasAuthority)
				{
					// Predict the same atomic exact-cell move/swap locally so the moved item stays interactive until replication lands.
					YI_TryLocalAtomicSameBagMoveOrSwap(Bag, GInventoryDrag.SourceIndex, Cell);
				}
			}
			else
			{
				bMoveAccepted = YI_TryLocalAtomicSameBagMoveOrSwap(Bag, GInventoryDrag.SourceIndex, Cell);
			}

			OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, bMoveAccepted);
			if (!bMoveAccepted)
			{
				PlayInvalidMoveSound();
				return false;
			}

			PlayDropSound();
			if (SwappedVictimInstanceId.IsValid())
			{
				int32 VictimIndexAfterSwap = INDEX_NONE;
				if (Bag->FindItemIndexByInstanceIdFast(SwappedVictimInstanceId, VictimIndexAfterSwap) &&
					YI_ContinueDraggingLinkedBagItem(this, VictimIndexAfterSwap))
				{
					UpdateBoundTooltip();
					return true;
				}
			}

			GInventoryDrag.Reset();
			UpdateBoundTooltip();
			return true;
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
			if (YI_ShouldContinueDraggingSwappedItem())
			{
				// Continue dragging the displaced item (victim) as unattached.
				GInventoryDrag.SourceGrid = this;
				GInventoryDrag.SourceIndex = INDEX_NONE;
				GInventoryDrag.Item = SavedVictim;
				GInventoryDrag.AnchorCellOffset = FIntPoint::ZeroValue;
				GInventoryDrag.bRemovedFromSource = true;
				GInventoryDrag.bFromExchange = true;
				GInventoryDrag.SourcePos = SavedVictim.Pos;
				GInventoryDrag.bActive = true;
				OnItemDragStarted.Broadcast(this, INDEX_NONE);
			}
			else
			{
				GInventoryDrag.Reset();
			}
			UpdateBoundTooltip();
			return true;
		}
		// Any remaining state here is stale or detached and should not fall back to the old shuffle path.
		OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, false);
		PlayInvalidMoveSound();
		return false;
	}

	// Cross-bag: perform an atomic swap (drag item takes victim's slot, victim becomes active drag or drops)
	FYIBagItem ToPlace = GInventoryDrag.Item;
	ToPlace.Pos = Cell;
	FYIInventoryGridTransferRequest TransferRequest;
	TransferRequest.SourceGrid = GInventoryDrag.SourceGrid.Get();
	TransferRequest.DestGrid = this;
	TransferRequest.SourceIndex = GInventoryDrag.SourceIndex;
	TransferRequest.Item = GInventoryDrag.Item;
	TransferRequest.Count = 0;
	TransferRequest.bIsDragDrop = true;
	TransferRequest.bHasDestCell = true;
	TransferRequest.DestCell = Cell;

	// Optional feature adapters can intercept cross-grid drops (trade/shop/etc) before core bag transfer logic.
	if (UInventoryGridWidget* SourceGrid = GInventoryDrag.SourceGrid.Get())
	{
		int32 AdapterDestIndex = INDEX_NONE;
		if (const TOptional<bool> FeatureHandled = YI_TryFeatureTransferRequest(TransferRequest, AdapterDestIndex))
		{
			const bool bSuccess = FeatureHandled.GetValue();
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

	// Standard inventory-component transfer path.
	if (!GInventoryDrag.bRemovedFromSource)
	{
		int32 IgnoredDestIndex = INDEX_NONE;
		if (YI_TryCoreInventoryTransferRequest(TransferRequest, IgnoredDestIndex))
		{
			UInventoryGridWidget* DragSourceGrid = GInventoryDrag.SourceGrid.Get();
			OnItemDropped.Broadcast(this, GInventoryDrag.SourceIndex, Cell, true);
			PlayDropSound();
			if (DragSourceGrid)
			{
				DragSourceGrid->RefreshBoundTooltip();
				OnItemTransferred.Broadcast(DragSourceGrid, GInventoryDrag.SourceIndex, IgnoredDestIndex);
				if (DragSourceGrid != this)
				{
					DragSourceGrid->OnItemTransferred.Broadcast(DragSourceGrid, GInventoryDrag.SourceIndex, IgnoredDestIndex);
				}
			}
			RefreshBoundTooltip();
			GInventoryDrag.Reset();
			return true;
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

	if (YI_ShouldContinueDraggingSwappedItem())
	{
		// Continue dragging the displaced item as unattached after the cross-bag swap.
		GInventoryDrag.SourceGrid = this;
		GInventoryDrag.SourceIndex = INDEX_NONE;
		GInventoryDrag.Item = SavedVictim;
		GInventoryDrag.AnchorCellOffset = FIntPoint::ZeroValue;
		GInventoryDrag.bRemovedFromSource = true;
		GInventoryDrag.bFromExchange = true;
		GInventoryDrag.SourcePos = SavedVictim.Pos;
		GInventoryDrag.bActive = true;
		OnItemDragStarted.Broadcast(this, INDEX_NONE);
	}
	else
	{
		GInventoryDrag.Reset();
	}
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
								if (IYIInventoryGridAdapterInterface* Adapter = DragSourceGrid->ResolveFeatureAdapterInterface())
								{
									if (Adapter->TrySpawnWorldDropFromInstance(this, Restore.Item, SpawnTransform))
									{
										bDroppedToWorld = true;
									}
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

bool UInventoryGridWidget::GetActiveDraggedItemAnchor(FIntPoint& OutAnchorCellOffset, const UWorld* ContextWorld)
{
	if (!YI_IsGlobalDragValid(ContextWorld))
	{
		OutAnchorCellOffset = FIntPoint::ZeroValue;
		return false;
	}

	OutAnchorCellOffset = GInventoryDrag.AnchorCellOffset;
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

	IYIInventoryGridAdapterInterface* Adapter = SourceGrid->ResolveFeatureAdapterInterface();
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
				if (SourceGrid->Bag && SourceGrid->Bag->Items.IsValidIndex(TempInsertedIndex) &&
					SourceGrid->Bag->BagId.IsValid() && SourceGrid->Bag->Items[TempInsertedIndex].Item.InstanceId.IsValid())
				{
					FYIInventoryRemoveItemRequest RemoveRequest;
					RemoveRequest.ItemRef.Bag.BagId = SourceGrid->Bag->BagId;
					RemoveRequest.ItemRef.Item.ItemInstanceId = SourceGrid->Bag->Items[TempInsertedIndex].Item.InstanceId;
					RemoveRequest.ExpectedSourceBagRevision = SourceGrid->Bag->RuntimeRevision;
					bRemovedRollback = SourceInventory->RequestRemoveItem(RemoveRequest).bRequestAccepted;
				}
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
			FYIInventoryRemoveItemRequest Request;
			Request.ItemRef.Bag.BagId = InBag->BagId;
			Request.ItemRef.Item.ItemInstanceId = DraggedItem.Item.InstanceId;
			Request.ExpectedSourceBagRevision = InBag->RuntimeRevision;
			bRemovedFromBag = OwnerComp->RequestRemoveItem(Request).bRequestAccepted;
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
	GInventoryDrag.AnchorCellOffset = FIntPoint::ZeroValue;
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
	if (!OwnerInventory)
	{
		return false;
	}

	FYIInventoryItemRef ItemRef;
	if (!OwnerInventory->GetBagItemCoreRef(Bag, ItemIndex, ItemRef))
	{
		return false;
	}
	return OwnerInventory->IsBagItemLockedByCoreRef(ItemRef);
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

	int32 TooltipItemIndex = INDEX_NONE;
	bool bGot = GetSelectedCellTooltipData(Data, Ctx);
	if (bGot)
	{
		TooltipItemIndex = GetSelectedItemIndex();
	}
	if (!bGot && Bag && HoveredItemIndexCached != INDEX_NONE)
	{
		bGot = UYIInventoryBlueprintLibrary::GetItemTooltipData(Bag, HoveredItemIndexCached, Data, Ctx);
		if (bGot)
		{
			TooltipItemIndex = HoveredItemIndexCached;
		}
	}

	if (bGot)
	{
		if (IYIInventoryGridAdapterInterface* Adapter = ResolveFeatureAdapterInterface())
		{
			Adapter->AugmentTooltipData(this, Bag, TooltipItemIndex, Data);
		}

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

	FYIInventoryGridTransferRequest TransferRequest;
	TransferRequest.SourceGrid = this;
	TransferRequest.DestGrid = Other;
	TransferRequest.SourceIndex = SourceIndex;
	TransferRequest.Item = Bag->Items[SourceIndex];
	TransferRequest.Count = Count;
	TransferRequest.bIsDragDrop = false;
	TransferRequest.bHasDestCell = false;

	// Feature adapters can route transfer actions (trade/shop/etc) before core bag transfer logic.
	if (const TOptional<bool> Handled = YI_TryFeatureTransferRequest(TransferRequest, OutDestIndex))
	{
		if (Handled.GetValue())
		{
			UpdateBoundTooltip();
			Other->RefreshBoundTooltip();
			OnItemTransferred.Broadcast(this, SourceIndex, OutDestIndex);
			Other->OnItemTransferred.Broadcast(this, SourceIndex, OutDestIndex);
		}
		return Handled.GetValue();
	}

	const bool bTransferred = YI_TryCoreInventoryTransferRequest(TransferRequest, OutDestIndex);
	if (bTransferred)
	{
		UpdateBoundTooltip();
		Other->RefreshBoundTooltip();
		OnItemTransferred.Broadcast(this, SourceIndex, OutDestIndex);
		Other->OnItemTransferred.Broadcast(this, SourceIndex, OutDestIndex);
	}
	return bTransferred;
}

void UInventoryGridWidget::SetBoundTooltipWidget(UUserWidget* Widget)
{
	BoundTooltipWidget = Widget;
	UpdateBoundTooltip();
}

void UInventoryGridWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	if (YI_DoesGlobalDragTouchGridOrBag(this, Bag))
	{
		CancelDrag();
	}

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
	if (Bag != InBag && YI_DoesGlobalDragTouchGridOrBag(this, Bag))
	{
		CancelDrag();
	}

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
	if (YI_DoesGlobalDragTouchGridOrBag(this, Bag))
	{
		CancelDrag();
	}

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

	if (YI_DoesGlobalDragTouchGridOrBag(this, InBag))
	{
		CancelDrag();
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



