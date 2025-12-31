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
#include "InventoryActionMenuWidget.h"


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
			if (Grid && Grid->Bag) { Grid->Bag->RotateItem(ItemIdx); }
			break;
		case 2: // Drop
			if (Grid && Grid->Bag) { Grid->Bag->RemoveItem(ItemIdx); OnItemDropped(ItemIdx); }
			break;
		case 3: // Combine
			if (Grid && Grid->Bag)
			{
				int32 Target = Grid->Bag->FindExistingStackIndexForItem(Grid->Bag->Items[ItemIdx]);
				if (Target != INDEX_NONE && Target != ItemIdx) { Grid->Bag->CombineStacks(Target, ItemIdx); OnItemCombined(ItemIdx, Target); }
			}
			break;
	case 4: // Sell
		// Let Blueprint handle pricing and removal; fire the Blueprint hook / multicast event
		OnItemSold(ItemIdx);
		OnItemSoldEvent.Broadcast(ItemIdx);
		break;
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

	if (Grid && Tooltip)
	{
		Grid->SetBoundTooltipWidget(Tooltip);
		// Prime tooltip for current selection
		Grid->RefreshBoundTooltip();
	}

	if (ActionMenu)
	{
		ActionMenu->HideActions();
		ActionMenu->OnActionSelected.AddDynamic(this, &UInventoryScreenWidget::OnActionChosen);
	}

	// Setup Enhanced Input if requested
	if (bUseEnhancedInput && bAutoRegisterEnhancedInput)
	{
		BindEnhancedInput();
	}

	// Auto push to the UI stack so the inventory receives focus when shown
	RequestPush(true);
}

void UInventoryScreenWidget::NativeDestruct()
{
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
	// by default do nothing; Blueprint can override to close the screen
}

void UInventoryScreenWidget::OnOpenMenuAction(const FInputActionValue& Value)
{
	// Alias to Confirm for convenience
	OnConfirmAction(Value);
}