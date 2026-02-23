#include "InventoryScreenWidget.h"
#include "InputCoreTypes.h"

// Enhanced Input
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputActionValue.h"
#include "UILayerSubsystem.h"

#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "YIItemSchemaResolver.h"
#include "InventoryActionMenuWidget.h"
#include "YIInventoryComponent.h"
#include "YIEquipmentComponent.h"
#include "InventoryDragOverlayUserWidget.h"
#include "InventoryEquipmentSlotWidget.h"
#include "Components/GridPanel.h"
#include "Components/GridSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/PanelWidget.h"
#include "GameFramework/Pawn.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Widget.h"
#include "TimerManager.h"

namespace YIInventoryScreenPrivate
{
	struct FYIAutoEquipmentSlotEntry
	{
		FGameplayTag SlotTag;
		FText DisplayName;
		int32 SortOrder = 0;
		int32 Row = -1;
		int32 Column = -1;
		int32 RowSpan = 1;
		int32 ColumnSpan = 1;
		FVector2D IconSize = FVector2D(56.f, 56.f);
	};

	template<typename TWidgetType>
	TWidgetType* FindWidgetByNameOrType(UWidgetTree* WidgetTree, const TCHAR* PreferredName, const TSet<UWidget*>& ExcludedWidgets = TSet<UWidget*>())
	{
		if (!WidgetTree)
		{
			return nullptr;
		}

		if (PreferredName && PreferredName[0] != 0)
		{
			if (UWidget* NamedWidget = WidgetTree->FindWidget(FName(PreferredName)))
			{
				if (TWidgetType* TypedNamedWidget = Cast<TWidgetType>(NamedWidget))
				{
					return TypedNamedWidget;
				}
			}
		}

		TArray<UWidget*> AllWidgets;
		WidgetTree->GetAllWidgets(AllWidgets);
		for (UWidget* Widget : AllWidgets)
		{
			if (!Widget || ExcludedWidgets.Contains(Widget))
			{
				continue;
			}
			if (TWidgetType* Typed = Cast<TWidgetType>(Widget))
			{
				return Typed;
			}
		}
		return nullptr;
	}

	static void BuildAutoSlotEntriesFromDefinitions(const UYIEquipmentComponent* EquipmentComp, TArray<FYIAutoEquipmentSlotEntry>& OutSlots)
	{
		OutSlots.Reset();
		if (!EquipmentComp)
		{
			return;
		}

		for (const FYIEquipmentSlotDefinition& SlotDef : EquipmentComp->SlotDefinitions)
		{
			if (!SlotDef.SlotTag.IsValid())
			{
				continue;
			}

			FYIAutoEquipmentSlotEntry Entry;
			Entry.SlotTag = SlotDef.SlotTag;
			const FString SlotName = SlotDef.SlotTag.ToString();
			int32 LastDotIndex = INDEX_NONE;
			if (SlotName.FindLastChar('.', LastDotIndex) && LastDotIndex >= 0 && LastDotIndex + 1 < SlotName.Len())
			{
				Entry.DisplayName = FText::FromString(SlotName.RightChop(LastDotIndex + 1));
			}
			else
			{
				Entry.DisplayName = FText::FromString(SlotName);
			}
			Entry.SortOrder = OutSlots.Num();
			OutSlots.Add(Entry);
		}
	}
}

void UInventoryScreenWidget::BindInventoryBagContexts(UYIInventoryComponent* InInventoryComponent)
{
	if (!InInventoryComponent)
	{
		return;
	}

	if (Grid)
	{
		Grid->SetBindingInventoryComponent(InInventoryComponent);
		Grid->RefreshBoundTooltip();
	}
	if (SecondaryContextGrid)
	{
		SecondaryContextGrid->SetBindingInventoryComponent(InInventoryComponent);
		SecondaryContextGrid->RefreshBoundTooltip();
	}
}

void UInventoryScreenWidget::AutoResolveWidgetReferences()
{
	if (!bAutoResolveWidgetReferences || !WidgetTree)
	{
		return;
	}

	TSet<UWidget*> Excluded;
	if (!Grid)
	{
		Grid = YIInventoryScreenPrivate::FindWidgetByNameOrType<UInventoryGridWidget>(WidgetTree, TEXT("Grid"));
	}
	if (Grid)
	{
		Excluded.Add(Grid);
	}

	if (!SecondaryContextGrid)
	{
		SecondaryContextGrid = YIInventoryScreenPrivate::FindWidgetByNameOrType<UInventoryGridWidget>(WidgetTree, TEXT("SecondaryContextGrid"), Excluded);
	}
	if (!SecondaryContextGrid)
	{
		SecondaryContextGrid = YIInventoryScreenPrivate::FindWidgetByNameOrType<UInventoryGridWidget>(WidgetTree, TEXT("ContextGrid"), Excluded);
	}
	if (!SecondaryContextGrid)
	{
		// Legacy UMG name fallback.
		SecondaryContextGrid = YIInventoryScreenPrivate::FindWidgetByNameOrType<UInventoryGridWidget>(WidgetTree, TEXT("SpellbookGrid"), Excluded);
	}
	if (SecondaryContextGrid)
	{
		Excluded.Add(SecondaryContextGrid);
	}

	if (!Tooltip)
	{
		Tooltip = YIInventoryScreenPrivate::FindWidgetByNameOrType<UInventoryTooltipView>(WidgetTree, TEXT("Tooltip"));
	}
	if (!ActionMenu)
	{
		ActionMenu = YIInventoryScreenPrivate::FindWidgetByNameOrType<UInventoryActionMenuWidget>(WidgetTree, TEXT("ActionMenu"));
	}

	if (!EquipmentSlotsPanel)
	{
		EquipmentSlotsPanel = YIInventoryScreenPrivate::FindWidgetByNameOrType<UGridPanel>(WidgetTree, TEXT("EquipmentSlotsPanel"));
	}
	if (!EquipmentSlotsCanvasPanel)
	{
		EquipmentSlotsCanvasPanel = YIInventoryScreenPrivate::FindWidgetByNameOrType<UCanvasPanel>(WidgetTree, TEXT("EquipmentSlotsCanvasPanel"));
	}
	if (!DragOverlay)
	{
		DragOverlay = YIInventoryScreenPrivate::FindWidgetByNameOrType<UInventoryDragOverlayUserWidget>(WidgetTree, TEXT("DragOverlay"));
	}
}

void UInventoryScreenWidget::EnsureMinimalDefaultLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	if (Grid && Tooltip)
	{
		return;
	}

	if (!WidgetTree->RootWidget)
	{
		UHorizontalBox* DefaultRoot = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InventoryDefaultRoot"));
		WidgetTree->RootWidget = DefaultRoot;
	}

	UPanelWidget* RootPanel = Cast<UPanelWidget>(WidgetTree->RootWidget);
	if (!RootPanel)
	{
		return;
	}

	if (!Grid)
	{
		Grid = WidgetTree->ConstructWidget<UInventoryGridWidget>(UInventoryGridWidget::StaticClass(), TEXT("Grid"));
		if (Grid)
		{
			if (UHorizontalBox* RootHorizontalBox = Cast<UHorizontalBox>(RootPanel))
			{
				if (UHorizontalBoxSlot* HorizontalSlot = RootHorizontalBox->AddChildToHorizontalBox(Grid))
				{
					HorizontalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
					HorizontalSlot->SetPadding(FMargin(8.f));
				}
			}
			else
			{
				RootPanel->AddChild(Grid);
			}
		}
	}

	if (!Tooltip)
	{
		Tooltip = WidgetTree->ConstructWidget<UInventoryTooltipView>(UInventoryTooltipView::StaticClass(), TEXT("Tooltip"));
		if (Tooltip)
		{
			if (UHorizontalBox* RootHorizontalBox = Cast<UHorizontalBox>(RootPanel))
			{
				if (UHorizontalBoxSlot* HorizontalSlot = RootHorizontalBox->AddChildToHorizontalBox(Tooltip))
				{
					HorizontalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
					HorizontalSlot->SetPadding(FMargin(8.f));
				}
			}
			else
			{
				RootPanel->AddChild(Tooltip);
			}
		}
	}
}

void UInventoryScreenWidget::EnsureGlobalDragOverlay()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!bEnableGlobalDragOverlay)
	{
		if (DragOverlay)
		{
			DragOverlay->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (Grid)
		{
			Grid->SetUseGlobalDragGhost(false);
		}
		if (SecondaryContextGrid)
		{
			SecondaryContextGrid->SetUseGlobalDragGhost(false);
		}
		return;
	}

	if (!DragOverlay && bAutoCreateDragOverlay)
	{
		UPanelWidget* RootPanel = Cast<UPanelWidget>(WidgetTree->RootWidget);
		if (RootPanel)
		{
			UClass* OverlayClass = DragOverlayClass ? DragOverlayClass.Get() : UInventoryDragOverlayUserWidget::StaticClass();
			DragOverlay = WidgetTree->ConstructWidget<UInventoryDragOverlayUserWidget>(OverlayClass, TEXT("DragOverlay_Auto"));
			if (DragOverlay)
			{
				if (UCanvasPanel* CanvasRoot = Cast<UCanvasPanel>(RootPanel))
				{
					if (UCanvasPanelSlot* CanvasSlot = CanvasRoot->AddChildToCanvas(DragOverlay))
					{
						CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
						CanvasSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
						CanvasSlot->SetZOrder(9999);
					}
				}
				else
				{
					RootPanel->AddChild(DragOverlay);
				}
			}
		}
	}

	if (DragOverlay)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(DragOverlay->Slot))
		{
			CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			CanvasSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 0.f));
			CanvasSlot->SetZOrder(9999);
		}
		DragOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
		DragOverlay->LeftGrid = Grid;
		DragOverlay->RightGrid = SecondaryContextGrid;
	}

	const bool bHasOverlay = (DragOverlay != nullptr);
	if (Grid)
	{
		Grid->SetUseGlobalDragGhost(bHasOverlay);
	}
	if (SecondaryContextGrid)
	{
		SecondaryContextGrid->SetUseGlobalDragGhost(bHasOverlay);
	}
}

bool UInventoryScreenWidget::ResolveRuntimeComponents(UYIInventoryComponent*& OutInventory, UYIEquipmentComponent*& OutEquipment) const
{
	OutInventory = nullptr;
	OutEquipment = nullptr;

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			OutInventory = Pawn->FindComponentByClass<UYIInventoryComponent>();
			OutEquipment = Pawn->FindComponentByClass<UYIEquipmentComponent>();
		}
	}

	if (!OutInventory && Grid && Grid->Bag)
	{
		OutInventory = Grid->Bag->GetTypedOuter<UYIInventoryComponent>();
		if (OutInventory && OutInventory->GetOwner())
		{
			OutEquipment = OutInventory->GetOwner()->FindComponentByClass<UYIEquipmentComponent>();
		}
	}

	return OutInventory != nullptr || OutEquipment != nullptr;
}

bool UInventoryScreenWidget::AutoWireScreen(bool bRebuildEquipmentPane)
{
	EnsureMinimalDefaultLayout();
	AutoResolveWidgetReferences();
	EnsureGlobalDragOverlay();

	UYIInventoryComponent* InventoryComp = nullptr;
	UYIEquipmentComponent* EquipmentComp = nullptr;
	ResolveRuntimeComponents(InventoryComp, EquipmentComp);

	if (InventoryComp)
	{
		BindInventoryBagContexts(InventoryComp);
	}

	if (Grid && Tooltip)
	{
		Grid->SetBoundTooltipWidget(Tooltip);
		Grid->RefreshBoundTooltip();
	}

	if (bAutoGenerateEquipmentSlotPane && bRebuildEquipmentPane)
	{
		RebuildEquipmentSlotPaneFromLayout();
	}

	if (bAutoBindEquipmentSlots)
	{
		BindEquipmentSlotWidgets();
	}

	if (ActionMenu)
	{
		ActionMenu->HideActions();
		ActionMenu->OnActionSelected.RemoveDynamic(this, &UInventoryScreenWidget::OnActionChosen);
		ActionMenu->OnActionSelected.AddDynamic(this, &UInventoryScreenWidget::OnActionChosen);
	}

	// Consider wiring complete when at least inventory context + main grid are resolved.
	return Grid != nullptr && InventoryComp != nullptr;
}

void UInventoryScreenWidget::StartAutoWireRetry()
{
	if (!bRetryAutoWireUntilReady)
	{
		return;
	}
	if (UWorld* World = GetWorld())
	{
		AutoWireRetryStartTime = World->GetTimeSeconds();
		World->GetTimerManager().ClearTimer(AutoWireRetryTimer);
		World->GetTimerManager().SetTimer(AutoWireRetryTimer, this, &UInventoryScreenWidget::HandleAutoWireRetry, AutoWireRetryInterval, true);
	}
}

void UInventoryScreenWidget::StopAutoWireRetry()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoWireRetryTimer);
	}
}

void UInventoryScreenWidget::HandleAutoWireRetry()
{
	if (AutoWireScreen(false))
	{
		StopAutoWireRetry();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		if (World->GetTimeSeconds() - AutoWireRetryStartTime >= AutoWireRetryTimeout)
		{
			StopAutoWireRetry();
		}
	}
}

void UInventoryScreenWidget::OnActionChosen(int32 ActionId)
{
	int32 ItemIdx = Grid ? Grid->GetSelectedItemIndex() : INDEX_NONE;
	if (ItemIdx == INDEX_NONE) return;

	switch (ActionId)
	{
		case 0: // Use
			OnItemUse(ItemIdx);
			break;
		case 1: // Rotate
			if (Grid && Grid->Bag)
			{
				bool bHandled = false;
				if (UYIInventoryComponent* OwnerComp = Grid->Bag->GetTypedOuter<UYIInventoryComponent>())
				{
					if (Grid->Bag->Items.IsValidIndex(ItemIdx) && Grid->Bag->BagId.IsValid() && Grid->Bag->Items[ItemIdx].Item.InstanceId.IsValid())
					{
						bHandled = OwnerComp->RotateItemInBag(Grid->Bag->BagId, Grid->Bag->Items[ItemIdx].Item.InstanceId);
					}
					else if (OwnerComp->GetBag() == Grid->Bag)
					{
						bHandled = OwnerComp->RotateItem(ItemIdx);
					}
				}
				if (!bHandled)
				{
					Grid->Bag->RotateItem(ItemIdx);
				}
			}
			break;
		case 2: // Drop
			if (Grid && Grid->Bag)
			{
				bool bHandled = false;
				if (UYIInventoryComponent* OwnerComp = Grid->Bag->GetTypedOuter<UYIInventoryComponent>())
				{
					if (Grid->Bag->Items.IsValidIndex(ItemIdx) && Grid->Bag->BagId.IsValid() && Grid->Bag->Items[ItemIdx].Item.InstanceId.IsValid())
					{
						bHandled = OwnerComp->RemoveItemFromBag(Grid->Bag->BagId, Grid->Bag->Items[ItemIdx].Item.InstanceId);
					}
					else if (OwnerComp->GetBag() == Grid->Bag)
					{
						bHandled = OwnerComp->RemoveItem(ItemIdx);
					}
				}
				if (!bHandled)
				{
					bHandled = Grid->Bag->RemoveItem(ItemIdx);
				}
				if (bHandled)
				{
					OnItemDropped(ItemIdx);
				}
			}
			break;
		case 3: // Combine
			if (Grid && Grid->Bag)
			{
				int32 Target = Grid->Bag->Items.IsValidIndex(ItemIdx) ? Grid->Bag->FindExistingStackIndexForItem(Grid->Bag->Items[ItemIdx]) : INDEX_NONE;
				bool bHandled = false;
				if (UYIInventoryComponent* OwnerComp = Grid->Bag->GetTypedOuter<UYIInventoryComponent>())
				{
					if (Grid->Bag->Items.IsValidIndex(ItemIdx) && Grid->Bag->BagId.IsValid() && Grid->Bag->Items[ItemIdx].Item.InstanceId.IsValid())
					{
						bHandled = OwnerComp->CombineItemInBag(Grid->Bag->BagId, Grid->Bag->Items[ItemIdx].Item.InstanceId);
					}
				}
				if (!bHandled && Target != INDEX_NONE && Target != ItemIdx)
				{
					bHandled = Grid->Bag->CombineStacks(Target, ItemIdx);
				}
				if (bHandled && Target != INDEX_NONE && Target != ItemIdx)
				{
					OnItemCombined(ItemIdx, Target);
				}
			}
			break;
	case 4: // Sell
		// Let Blueprint handle pricing and removal; fire the Blueprint hook / multicast event
		OnItemSold(ItemIdx);
		OnItemSoldEvent.Broadcast(ItemIdx);
		break;
	case 6: // Open Container
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			if (APawn* Pawn = PC->GetPawn())
			{
				if (UYIInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UYIInventoryComponent>())
				{
					InventoryComp->OpenContainedBagAtIndex(ItemIdx);
				}
			}
		}
		break;
	}
	case 7: // Equip
	{
		bool bSuccess = false;
		if (Grid && Grid->Bag)
		{
			if (APlayerController* PC = GetOwningPlayer())
			{
				if (APawn* Pawn = PC->GetPawn())
				{
					if (UYIInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UYIInventoryComponent>())
					{
						if (UYIEquipmentComponent* EquipmentComp = Pawn->FindComponentByClass<UYIEquipmentComponent>())
						{
							bSuccess = EquipmentComp->EquipFromInventory(InventoryComp, ItemIdx, FGameplayTag());
						}
					}
				}
			}
		}
		OnItemEquip(ItemIdx, bSuccess);
		OnItemEquippedEvent.Broadcast(ItemIdx, bSuccess);
		break;
	}
	}
	// Refresh tooltip after action
	if (Grid) Grid->RefreshBoundTooltip();
}

void UInventoryScreenWidget::EvaluateActionsForIndex(int32 Index, TArray<FText>& OutActions, TArray<int32>& OutActionIds) const
{
	OutActions.Reset(); OutActionIds.Reset();
	if (!Grid || !Grid->Bag) return;
	if (Index < 0 || Index >= Grid->Bag->Items.Num()) return;

	// We always allow 'Use' so game logic may optionally implement it in BP
	OutActions.Add(NSLOCTEXT("YOLOInventory", "UseAction", "Use")); OutActionIds.Add(0);

	// Rotate if bag allows rotation
	if (Grid->Bag->bAllowRotation) { OutActions.Add(NSLOCTEXT("YOLOInventory", "RotateAction", "Rotate")); OutActionIds.Add(1); }

	// Drop
	OutActions.Add(NSLOCTEXT("YOLOInventory", "DropAction", "Drop")); OutActionIds.Add(2);

	// Open container when this item carries a nested bag (or can become one by definition).
	const FYIBagItem& SelectedItem = Grid->Bag->Items[Index];
	UYIItemDefinition* Definition = SelectedItem.Item.Definition.IsValid()
		? SelectedItem.Item.Definition.Get()
		: SelectedItem.Item.Definition.LoadSynchronous();
	const bool bCanOpenContainer = SelectedItem.Item.ContainedBagId.IsValid() || (Definition && YIItemSchema::IsContainerItem(Definition));
	if (bCanOpenContainer)
	{
		OutActions.Add(NSLOCTEXT("YOLOInventory", "OpenContainerAction", "Open"));
		OutActionIds.Add(6);
	}

	// Combine if there is another existing stack
	int32 Found = Grid->Bag->FindExistingStackIndexForItem(Grid->Bag->Items[Index]);
	if (Found != INDEX_NONE && Found != Index) { OutActions.Add(NSLOCTEXT("YOLOInventory", "CombineAction", "Combine")); OutActionIds.Add(3); }

	// Sell - by default the UI provides a Sell action; game logic (Blueprint) can implement pricing and remove the item
	OutActions.Add(NSLOCTEXT("YOLOInventory", "SellAction", "Sell")); OutActionIds.Add(4);
}

void UInventoryScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// Ensure we can receive keyboard/gamepad input
	if (GetOwningPlayer()) { SetUserFocus(GetOwningPlayer()); }

	const bool bWiredNow = AutoWireScreen(true);
	if (!bWiredNow)
	{
		StartAutoWireRetry();
	}

	// Setup Enhanced Input if requested
	if (bUseEnhancedInput && bAutoRegisterEnhancedInput)
	{
		BindEnhancedInput();
	}

	// Auto push to the UI stack so the inventory receives focus when shown
	RequestPush(true);
}

void UInventoryScreenWidget::BindEquipmentSlotWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	UYIInventoryComponent* InventoryComp = nullptr;
	UYIEquipmentComponent* EquipmentComp = nullptr;
	ResolveRuntimeComponents(InventoryComp, EquipmentComp);

	TArray<UWidget*> Widgets;
	WidgetTree->GetAllWidgets(Widgets);
	for (UWidget* Widget : Widgets)
	{
		UInventoryEquipmentSlotWidget* SlotWidget = Cast<UInventoryEquipmentSlotWidget>(Widget);
		if (!SlotWidget)
		{
			continue;
		}

		if (!SlotWidget->GetEquipmentComponent())
		{
			SlotWidget->SetEquipmentComponent(EquipmentComp);
		}
		if (!SlotWidget->GetInventoryComponent())
		{
			SlotWidget->SetInventoryComponent(InventoryComp);
		}
		SlotWidget->RefreshSlot();
	}
}

void UInventoryScreenWidget::RebuildEquipmentSlotPaneFromLayout()
{
	if (!WidgetTree)
	{
		return;
	}

	AutoResolveWidgetReferences();

	UYIInventoryComponent* InventoryComp = nullptr;
	UYIEquipmentComponent* EquipmentComp = nullptr;
	ResolveRuntimeComponents(InventoryComp, EquipmentComp);

	if (EquipmentSlotsPanel)
	{
		EquipmentSlotsPanel->ClearChildren();
	}
	if (EquipmentSlotsCanvasPanel)
	{
		EquipmentSlotsCanvasPanel->ClearChildren();
	}

	UGridPanel* TargetGridPanel = EquipmentSlotsPanel;
	UCanvasPanel* TargetCanvasPanel = EquipmentSlotsCanvasPanel;
	if (TargetGridPanel && TargetCanvasPanel)
	{
		TargetCanvasPanel = nullptr; // Prefer grid panel when both are present.
	}
	if (!TargetGridPanel && !TargetCanvasPanel)
	{
		return;
	}

	TArray<YIInventoryScreenPrivate::FYIAutoEquipmentSlotEntry> SortedSlots;
	if (bAutoResolveLayoutFromEquipmentComponent)
	{
		YIInventoryScreenPrivate::BuildAutoSlotEntriesFromDefinitions(EquipmentComp, SortedSlots);
	}
	if (SortedSlots.Num() == 0)
	{
		return;
	}

	const int32 AutoColumns = 4;
	const float SlotPadding = 4.f;
	int32 AutoPlacementIndex = 0;

	for (const YIInventoryScreenPrivate::FYIAutoEquipmentSlotEntry& SlotEntry : SortedSlots)
	{
		if (!SlotEntry.SlotTag.IsValid())
		{
			continue;
		}

		int32 Row = SlotEntry.Row;
		int32 Column = SlotEntry.Column;
		if (Row < 0 || Column < 0)
		{
			Row = AutoPlacementIndex / AutoColumns;
			Column = AutoPlacementIndex % AutoColumns;
			++AutoPlacementIndex;
		}

		const FString SafeTagName = SlotEntry.SlotTag.ToString().Replace(TEXT("."), TEXT("_"));
		const FName WidgetName(*FString::Printf(TEXT("EquipSlot_%s"), *SafeTagName));
		UInventoryEquipmentSlotWidget* SlotWidget = WidgetTree->ConstructWidget<UInventoryEquipmentSlotWidget>(
			UInventoryEquipmentSlotWidget::StaticClass(),
			WidgetName);
		if (!SlotWidget)
		{
			continue;
		}

		SlotWidget->SlotTag = SlotEntry.SlotTag;
		SlotWidget->SlotDisplayName = SlotEntry.DisplayName;
		SlotWidget->IconSize = SlotEntry.IconSize;
		SlotWidget->SetInventoryComponent(InventoryComp);
		SlotWidget->SetEquipmentComponent(EquipmentComp);

		if (TargetGridPanel)
		{
			UGridSlot* GridSlot = TargetGridPanel->AddChildToGrid(SlotWidget, Row, Column);
			if (GridSlot)
			{
				GridSlot->SetHorizontalAlignment(HAlign_Fill);
				GridSlot->SetVerticalAlignment(VAlign_Fill);
				GridSlot->SetRowSpan(FMath::Max(1, SlotEntry.RowSpan));
				GridSlot->SetColumnSpan(FMath::Max(1, SlotEntry.ColumnSpan));
				GridSlot->SetPadding(FMargin(SlotPadding));
			}
		}
		else if (TargetCanvasPanel)
		{
			const FVector2D AutoCellSize(96.f, 96.f);
			const FVector2D AutoPos(
				(float)(Column * AutoCellSize.X) + SlotPadding,
				(float)(Row * AutoCellSize.Y) + SlotPadding);

			UCanvasPanelSlot* CanvasSlot = TargetCanvasPanel->AddChildToCanvas(SlotWidget);
			if (CanvasSlot)
			{
				CanvasSlot->SetAutoSize(false);
				CanvasSlot->SetPosition(AutoPos);
				CanvasSlot->SetSize(FVector2D(96.f, 96.f));
			}
		}
	}
}

void UInventoryScreenWidget::NativeDestruct()
{
	StopAutoWireRetry();
	if (ActionMenu)
	{
		ActionMenu->OnActionSelected.RemoveDynamic(this, &UInventoryScreenWidget::OnActionChosen);
	}
	// Clean up Enhanced Input first
	UnbindEnhancedInput();
	Super::NativeDestruct();
}

//FReply UInventoryScreenWidget::NativeOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
//{
//	// Keep as fallback for non-EnhancedInput setups
//	const FKey Key = InKeyEvent.GetKey();
//	if (!Grid) return Super::NativeOnKeyDown(MyGeometry, InKeyEvent);
//
//	// If action menu is visible, forward nav/confirm/cancel keys to it
//	if (ActionMenu && ActionMenu->IsVisible())
//	{
//		if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up) { ActionMenu->PrevAction(); return FReply::Handled(); }
//		if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down) { ActionMenu->NextAction(); return FReply::Handled(); }
//		if (Key == EKeys::Enter || Key == EKeys::Virtual_Accept || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom) { ActionMenu->ConfirmSelection(); return FReply::Handled(); }
//		if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right) { ActionMenu->HideActions(); return FReply::Handled(); }
//	}
//
//	bool bHandled = false;
//	if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up) { bHandled = Grid->MoveSelectionUp(); }
//	else if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down) { bHandled = Grid->MoveSelectionDown(); }
//	else if (Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left) { bHandled = Grid->MoveSelectionLeft(); }
//	else if (Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right) { bHandled = Grid->MoveSelectionRight(); }
//	else if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Virtual_Accept || Key == EKeys::Gamepad_FaceButton_Bottom)
//	{
//		// Open action menu for current selection
//		int32 ItemIdx = Grid->GetSelectedItemIndex();
//		if (ItemIdx != INDEX_NONE && ActionMenu)
//		{
//			TArray<FText> Actions;
//			TArray<int32> ActionIds;
//			EvaluateActionsForIndex(ItemIdx, Actions, ActionIds);
//			if (Actions.Num() > 0)
//			{
//				ActionMenu->ShowActions(Actions, ActionIds);
//				return FReply::Handled();
//			}
//		}
//	}
//
//	if (bHandled)
//	{
//		// selection changed; tooltip / delegates are already handled by grid
//		return FReply::Handled();
//	}
//
//	return Super::NativeOnKeyDown(MyGeometry, InKeyEvent);
//}

bool UInventoryScreenWidget::TransferSelectionTo(UInventoryGridWidget* Dest, int32 Count, int32& OutDestIndex)
{
	OutDestIndex = INDEX_NONE;
	if (!Grid) return false;
	return Grid->TransferSelectedItemTo(Dest, Count, OutDestIndex);
}

void UInventoryScreenWidget::BindEnhancedInput()
{
	if (!GetOwningPlayer() || !bUseEnhancedInput) return;
	APlayerController* PC = GetOwningPlayer();
	// Register mapping context if provided
	if (InputMappingContext)
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				Sub->AddMappingContext(InputMappingContext, 0);
			}
		}
	}

	// Bind actions to EnhancedInputComponent if available
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PC->InputComponent))
	{
		// Route all UI input to the UILayerSubsystem which will forward to the top-most screen
		UUILayerSubsystem* UISub = GetGameInstance() ? GetGameInstance()->GetSubsystem<UUILayerSubsystem>() : nullptr;
		if (UISub)
		{
			if (IA_MoveUp) EIC->BindAction(IA_MoveUp, ETriggerEvent::Triggered, UISub, &UUILayerSubsystem::OnMoveUp);
			if (IA_MoveDown) EIC->BindAction(IA_MoveDown, ETriggerEvent::Triggered, UISub, &UUILayerSubsystem::OnMoveDown);
			if (IA_MoveLeft) EIC->BindAction(IA_MoveLeft, ETriggerEvent::Triggered, UISub, &UUILayerSubsystem::OnMoveLeft);
			if (IA_MoveRight) EIC->BindAction(IA_MoveRight, ETriggerEvent::Triggered, UISub, &UUILayerSubsystem::OnMoveRight);
			if (IA_Confirm) EIC->BindAction(IA_Confirm, ETriggerEvent::Triggered, UISub, &UUILayerSubsystem::OnConfirm);
			if (IA_Cancel) EIC->BindAction(IA_Cancel, ETriggerEvent::Triggered, UISub, &UUILayerSubsystem::OnCancel);
			if (IA_OpenMenu) EIC->BindAction(IA_OpenMenu, ETriggerEvent::Triggered, UISub, &UUILayerSubsystem::OnOpenMenu);
		}
		else
		{
			// fallback to existing binding if the subsystem isn't available
			if (IA_MoveUp) EIC->BindAction(IA_MoveUp, ETriggerEvent::Triggered, this, &UInventoryScreenWidget::OnMoveUpAction);
			if (IA_MoveDown) EIC->BindAction(IA_MoveDown, ETriggerEvent::Triggered, this, &UInventoryScreenWidget::OnMoveDownAction);
			if (IA_MoveLeft) EIC->BindAction(IA_MoveLeft, ETriggerEvent::Triggered, this, &UInventoryScreenWidget::OnMoveLeftAction);
			if (IA_MoveRight) EIC->BindAction(IA_MoveRight, ETriggerEvent::Triggered, this, &UInventoryScreenWidget::OnMoveRightAction);
			if (IA_Confirm) EIC->BindAction(IA_Confirm, ETriggerEvent::Triggered, this, &UInventoryScreenWidget::OnConfirmAction);
			if (IA_Cancel) EIC->BindAction(IA_Cancel, ETriggerEvent::Triggered, this, &UInventoryScreenWidget::OnCancelAction);
			if (IA_OpenMenu) EIC->BindAction(IA_OpenMenu, ETriggerEvent::Triggered, this, &UInventoryScreenWidget::OnOpenMenuAction);
		}
	}
}

void UInventoryScreenWidget::UnbindEnhancedInput()
{
	if (!GetOwningPlayer()) return;
	APlayerController* PC = GetOwningPlayer();
	// Remove mapping context
	if (InputMappingContext)
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				Sub->RemoveMappingContext(InputMappingContext);
			}
		}
	}
	// Note: explicit unbinding of EnhancedInputComponent delegates is not required here - they will be cleaned up with controller
}

// --- UWidgetScreen overrides to let UILayerSubsystem drive top-most input routing ---
bool UInventoryScreenWidget::HandleMoveUp()
{
	if (ActionMenu && ActionMenu->IsVisible()) { ActionMenu->PrevAction(); return true; }
	if (Grid) return Grid->MoveSelectionUp();
	return false;
}

bool UInventoryScreenWidget::HandleMoveDown()
{
	if (ActionMenu && ActionMenu->IsVisible()) { ActionMenu->NextAction(); return true; }
	if (Grid) return Grid->MoveSelectionDown();
	return false;
}

bool UInventoryScreenWidget::HandleMoveLeft()
{
	if (Grid) return Grid->MoveSelectionLeft();
	return false;
}

bool UInventoryScreenWidget::HandleMoveRight()
{
	if (Grid) return Grid->MoveSelectionRight();
	return false;
}

bool UInventoryScreenWidget::HandleConfirm()
{
	// Debounce: ignore repeated confirm/open triggers within a short threshold to avoid "open then immediate confirm" when a single key maps to both
	if (UWorld* W = GetWorld())
	{
		const float Now = W->GetRealTimeSeconds();
		if (Now - LastConfirmHandledTime < ConfirmOpenDebounceSeconds)
		{
			return false; // ignore duplicate within debounce window
		}
		LastConfirmHandledTime = Now;
	}

	if (ActionMenu && ActionMenu->IsVisible()) { ActionMenu->ConfirmSelection(); return true; }
	// open action menu for current selection
	if (!Grid || !ActionMenu) return false;
	int32 ItemIdx = Grid->GetSelectedItemIndex();
	if (ItemIdx == INDEX_NONE) return false;
	TArray<FText> Actions; TArray<int32> ActionIds;
	EvaluateActionsForIndex(ItemIdx, Actions, ActionIds);
	if (Actions.Num() > 0) { ActionMenu->ShowActions(Actions, ActionIds); return true; }
	return false;
}

bool UInventoryScreenWidget::HandleCancel()
{
	if (ActionMenu && ActionMenu->IsVisible()) { ActionMenu->HideActions(); return true; }
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			if (UYIInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UYIInventoryComponent>())
			{
				if (InventoryComp->OpenParentBag())
				{
					return true;
				}
			}
		}
	}
	return false; // default: let caller (e.g., top-level game) handle screen close
}

bool UInventoryScreenWidget::HandleOpenMenu()
{
	return HandleConfirm();
}

void UInventoryScreenWidget::OnMoveUpAction(const FInputActionValue& Value)
{
	if (ActionMenu && ActionMenu->IsVisible()) { ActionMenu->PrevAction(); return; }
	if (Grid) Grid->MoveSelectionUp();
}

void UInventoryScreenWidget::OnMoveDownAction(const FInputActionValue& Value)
{
	if (ActionMenu && ActionMenu->IsVisible()) { ActionMenu->NextAction(); return; }
	if (Grid) Grid->MoveSelectionDown();
}

void UInventoryScreenWidget::OnMoveLeftAction(const FInputActionValue& Value)
{
	if (Grid) Grid->MoveSelectionLeft();
}

void UInventoryScreenWidget::OnMoveRightAction(const FInputActionValue& Value)
{
	if (Grid) Grid->MoveSelectionRight();
}

void UInventoryScreenWidget::OnConfirmAction(const FInputActionValue& Value)
{
	// Debounce: ignore repeated confirm/open triggers within a short threshold to avoid "open then immediate confirm" when a single key maps to both
	if (UWorld* W = GetWorld())
	{
		const float Now = W->GetRealTimeSeconds();
		if (Now - LastConfirmHandledTime < ConfirmOpenDebounceSeconds)
		{
			return; // ignore duplicate within debounce window
		}
		LastConfirmHandledTime = Now;
	}

	if (ActionMenu && ActionMenu->IsVisible()) { ActionMenu->ConfirmSelection(); return; }
	// open action menu for current selection
	if (!Grid || !ActionMenu) return;
	int32 ItemIdx = Grid->GetSelectedItemIndex();
	if (ItemIdx == INDEX_NONE) return;
	TArray<FText> Actions; TArray<int32> ActionIds;
	EvaluateActionsForIndex(ItemIdx, Actions, ActionIds);
	if (Actions.Num() > 0) { ActionMenu->ShowActions(Actions, ActionIds); }
}

void UInventoryScreenWidget::OnCancelAction(const FInputActionValue& Value)
{
	if (ActionMenu && ActionMenu->IsVisible()) { ActionMenu->HideActions(); return; }
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			if (UYIInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UYIInventoryComponent>())
			{
				if (InventoryComp->OpenParentBag())
				{
					return;
				}
			}
		}
	}
	// by default do nothing; Blueprint can override to close the screen
}

void UInventoryScreenWidget::OnOpenMenuAction(const FInputActionValue& Value)
{
	// Alias to Confirm for convenience
	OnConfirmAction(Value);
}
