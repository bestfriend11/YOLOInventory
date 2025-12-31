#pragma once
#include "CoreMinimal.h"
#include "WidgetScreen.h"
#include "InventoryActionMenuWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryActionSelected, int32, ActionId);

/**
 * UInventoryActionMenuWidget
 *
 * Small action menu used at runtime to present contextual item actions (Use/Drop/Combine/etc.).
 * - Designers can style the list in UMG and call ShowActions to populate it with localized texts and stable ids.
 * - When opened the menu behaves as a modal top-level screen that receives input via the UILayerSubsystem.
 */
UCLASS(meta=(DisplayName="YOLO Inventory Action Menu"))
class YOLOINVENTORY_API UInventoryActionMenuWidget : public UWidgetScreen
{
	GENERATED_BODY()
public:
	UInventoryActionMenuWidget(const FObjectInitializer& Obj);

	/** Populate and display a list of actions. ActionIds must align 1:1 with InActions and are returned when selection is confirmed. */
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Show contextual actions and associated stable ActionIds"))
	void ShowActions(const TArray<FText>& InActions, const TArray<int32>& InActionIds);

	/** Hide the menu (useful when cancelling). */
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Hide the action menu"))
	void HideActions();

	/** Move highlight to the next action (wraps). */
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Move highlight to the next action"))
	void NextAction();

	/** Move highlight to the previous action (wraps). */
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Move highlight to the previous action"))
	void PrevAction();

	/** Confirm the currently highlighted action and broadcast OnActionSelected(ActionId). */
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Confirm the selected action"))
	void ConfirmSelection();

    /** Set the currently selected action index (clamped). Useful for per-button wiring in Blueprint. */
    UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Set the selected action index (clamped)"))
    void SetSelectedIndex(int32 Index);

    /** Confirm the action at Index (broadcasts the associated ActionId and hides the menu). */
    UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Confirm selection at Index (convenience)"))
    void ConfirmSelectionAt(int32 Index);
	FOnInventoryActionSelected OnActionSelected;

	// Getters for UI binding
	UFUNCTION(BlueprintCallable, Category="Inventory")
	const TArray<FText>& GetActions() const { return Actions; }
	UFUNCTION(BlueprintCallable, Category="Inventory")
	int32 GetSelectedIndex() const { return SelectedIndex; }

protected:
	// Input handlers invoked by UILayerSubsystem. Return true if consumed.
	virtual bool HandleMoveUp() override;
	virtual bool HandleMoveDown() override;
	virtual bool HandleConfirm() override;
	virtual bool HandleCancel() override;
	// Legacy fallback for non-EnhancedInput setups: route keys to Handle* methods
	virtual FReply NativeOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	// When gaining stack focus we make sure input focus is set to this widget
	virtual void OnStackFocusGained(bool bModal) override;
	virtual void OnStackFocusLost() override;

private:
	// Internal backing arrays; designers should not modify these directly at runtime.
	TArray<FText> Actions;
	TArray<int32> ActionIds;
	int32 SelectedIndex = -1;
};