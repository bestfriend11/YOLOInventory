#include "InventoryActionMenuWidget.h"


UInventoryActionMenuWidget::UInventoryActionMenuWidget(const FObjectInitializer& Obj)
	: Super(Obj)
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UInventoryActionMenuWidget::ShowActions(const TArray<FText>& InActions, const TArray<int32>& InActionIds)
{
	Actions = InActions;
	ActionIds = InActionIds;
	SelectedIndex = Actions.Num() > 0 ? 0 : -1;
	SetVisibility(ESlateVisibility::Visible);
	// Push onto UI stack so it becomes top-most and receives input
	RequestPush(true);
	// Let UMG update visuals via Blueprint
}

void UInventoryActionMenuWidget::HideActions()
{
	// Pop from stack (safe if not present)
	RequestPop();
	SetVisibility(ESlateVisibility::Collapsed);
	Actions.Empty(); ActionIds.Empty(); SelectedIndex = -1;
}

void UInventoryActionMenuWidget::NextAction()
{
	if (Actions.Num() == 0) return;
	SelectedIndex = (SelectedIndex + 1) % Actions.Num();
}

void UInventoryActionMenuWidget::PrevAction()
{
	if (Actions.Num() == 0) return;
	SelectedIndex = (SelectedIndex - 1 + Actions.Num()) % Actions.Num();
}

void UInventoryActionMenuWidget::ConfirmSelection()
{
	if (SelectedIndex >= 0 && SelectedIndex < ActionIds.Num())
	{
		OnActionSelected.Broadcast(ActionIds[SelectedIndex]);
	}
	HideActions();
}

void UInventoryActionMenuWidget::SetSelectedIndex(int32 Index)
{
	if (Actions.Num() == 0) { SelectedIndex = -1; return; }
	// Clamp to valid range
	SelectedIndex = FMath::Clamp(Index, 0, Actions.Num() - 1);
	// Let UMG update visuals via Blueprint bindings (GetSelectedIndex/GetActions)
}

void UInventoryActionMenuWidget::ConfirmSelectionAt(int32 Index)
{
	if (Index >= 0 && Index < ActionIds.Num())
	{
		SelectedIndex = Index;
		OnActionSelected.Broadcast(ActionIds[SelectedIndex]);
	}
	HideActions();
}

// Input handlers used by UILayerSubsystem
bool UInventoryActionMenuWidget::HandleMoveUp()
{
	PrevAction();
	return true;
}

bool UInventoryActionMenuWidget::HandleMoveDown()
{
	NextAction();
	return true;
}

bool UInventoryActionMenuWidget::HandleConfirm()
{
	ConfirmSelection();
	return true;
}

bool UInventoryActionMenuWidget::HandleCancel()
{
	HideActions();
	return true;
}

// Legacy fallback for users who still rely on NativeOnKeyDown; route to the Handle* API
FReply UInventoryActionMenuWidget::NativeOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
	{
		HandleMoveUp();
		return FReply::Handled();
	}
	if (Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
	{
		HandleMoveDown();
		return FReply::Handled();
	}
	if (Key == EKeys::Enter || Key == EKeys::Virtual_Accept || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		HandleConfirm();
		return FReply::Handled();
	}
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		HandleCancel();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(MyGeometry, InKeyEvent);
}

void UInventoryActionMenuWidget::OnStackFocusGained(bool bModal)
{
	// Give keyboard/gamepad focus to this widget when it becomes top-most
	SetUserFocus(GetOwningPlayer());
}

void UInventoryActionMenuWidget::OnStackFocusLost()
{
	// Optionally release focus
	// Keep the menu visible until explicitly hidden
	// SetUserFocus(nullptr); // not necessary
}