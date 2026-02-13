#pragma once
#include "CoreMinimal.h"
#include "WidgetScreen.h"
#include "InventoryGridWidget.h"
#include "Widgets/InventoryTooltipView.h"
#include "InventoryScreenWidget.generated.h"

/**
 * UInventoryScreenWidget
 *
 * Composite UMG screen that composes the inventory grid, tooltip and (optionally) an action menu.
 * - Designed to be the primary in-game inventory screen; blueprintable hooks let game logic respond to Use/Drop/Combine.
 * - Designers: create a UMG widget with named children `Grid`, `Tooltip`, and optionally `ActionMenu` and bind them.
 */
UCLASS(meta = (DisplayName = "YOLO Inventory Screen"))
class YOLOINVENTORY_API UInventoryScreenWidget : public UWidgetScreen
{
	GENERATED_BODY()
public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//virtual FReply NativeOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

	/** Blueprint-safe getters for bound child widgets (designer convenience). */
	UFUNCTION(BlueprintCallable, Category="Inventory")
	UInventoryGridWidget* GetGrid() const { return Grid; }
	UFUNCTION(BlueprintCallable, Category="Inventory")
	UInventoryTooltipView* GetTooltip() const { return Tooltip; }
	UFUNCTION(BlueprintCallable, Category="Inventory")
	class UInventoryActionMenuWidget* GetActionMenu() const { return ActionMenu; }

	UFUNCTION(BlueprintCallable, Category="Inventory|Equipment")
	void BindEquipmentSlotWidgets();

protected:
	// Bind these to matching named widgets in your UMG widget blueprint
	/** The runtime grid widget that shows items and selection. */
	UPROPERTY(meta = (BindWidget))
	UInventoryGridWidget* Grid;

	/** The tooltip widget that will receive data for the selected cell. */
	UPROPERTY(meta = (BindWidget))
	UInventoryTooltipView* Tooltip;

	/** Optional action menu widget that displays available actions when an item is selected. */
	UPROPERTY(meta = (BindWidget))
	class UInventoryActionMenuWidget* ActionMenu;

	// Enhanced Input assets and runtime binding (assign these in the UMG widget or Blueprint Instance)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	class UInputMappingContext* InputMappingContext = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	class UInputAction* IA_MoveUp = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	class UInputAction* IA_MoveDown = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	class UInputAction* IA_MoveLeft = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	class UInputAction* IA_MoveRight = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	class UInputAction* IA_Confirm = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	class UInputAction* IA_Cancel = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input")
	class UInputAction* IA_OpenMenu = nullptr;

	/** Whether to register Enhanced Input mappings automatically when constructed (if mapping context present). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input", meta=(ToolTip="Automatically register Enhanced Input mapping context on construct"))
	bool bAutoRegisterEnhancedInput = true;

	/** If set to true, Enhanced Input will be used instead of NativeOnKeyDown for navigation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input", meta=(ToolTip="Use Enhanced Input actions instead of hard-coded keys"))
	bool bUseEnhancedInput = true;

	/** Auto-bind any UInventoryEquipmentSlotWidget children to the owning pawn's inventory/equipment components. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Equipment")
	bool bAutoBindEquipmentSlots = true;

	/** Debounce window (seconds) to ignore duplicate confirm/open events fired in quick succession (prevents immediate open+confirm when same key is bound for both). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Input", meta=(ToolTip="Seconds to ignore duplicate Confirm/Open events fired quickly"))
	float ConfirmOpenDebounceSeconds = 0.20f;

protected:
	// last time (real seconds) a confirm/open was handled; used to debounce repeated triggers
	float LastConfirmHandledTime = -FLT_MAX;

	/** Register Enhanced Input mappings and bind actions. Safe to call at runtime. */
	UFUNCTION(BlueprintCallable, Category="Input")
	void BindEnhancedInput();

	/** Unregister Enhanced Input mappings and unbind actions. */
	UFUNCTION(BlueprintCallable, Category="Input")
	void UnbindEnhancedInput();

	// Input handlers (Enhanced Input binding)
	void OnMoveUpAction(const struct FInputActionValue& Value);
	void OnMoveDownAction(const struct FInputActionValue& Value);
	void OnMoveLeftAction(const struct FInputActionValue& Value);
	void OnMoveRightAction(const struct FInputActionValue& Value);
	void OnConfirmAction(const struct FInputActionValue& Value);
	void OnCancelAction(const struct FInputActionValue& Value);
	void OnOpenMenuAction(const struct FInputActionValue& Value);

	// Handlers used by the UILayerSubsystem when this screen is asked to consume input. Return true if handled.
	virtual bool HandleMoveUp() override;
	virtual bool HandleMoveDown() override;
	virtual bool HandleMoveLeft() override;
	virtual bool HandleMoveRight() override;
	virtual bool HandleConfirm() override;
	virtual bool HandleCancel() override;
	virtual bool HandleOpenMenu() override;

	/** Determine available actions for the item at Index. Returns display text and corresponding Action IDs. */
	void EvaluateActionsForIndex(int32 Index, TArray<FText>& OutActions, TArray<int32>& OutActionIds) const;

	/** Called when an action is chosen from the action menu. ActionId matches the ids returned by EvaluateActionsForIndex. */
	UFUNCTION()
	void OnActionChosen(int32 ActionId);

	// Blueprint hooks so game code can react to user-driven item actions
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void OnItemUse(int32 BagIndex);
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void OnItemDropped(int32 BagIndex);
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void OnItemCombined(int32 SourceIndex, int32 TargetIndex);
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void OnItemSold(int32 BagIndex);
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void OnItemEquip(int32 BagIndex, bool bSuccess);

	// Multicast delegate alternative to the implementable event
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryItemSold, int32, BagIndex);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemEquipped, int32, BagIndex, bool, bSuccess);
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryItemSold OnItemSoldEvent;
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryItemEquipped OnItemEquippedEvent;

	/** Transfer the currently selected item in the Grid to Dest's grid (BP-friendly helper). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool TransferSelectionTo(UInventoryGridWidget* Dest, int32 Count, int32& OutDestIndex);
};
