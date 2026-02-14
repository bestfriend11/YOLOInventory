#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "GameplayTagContainer.h"
#include "YIItemPickup.h"
#include "InventoryEquipmentSlotWidget.generated.h"

class UYIEquipmentComponent;
class UYIInventoryComponent;
class UTexture2D;
class SBorder;
class SImage;
class STextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEquipmentSlotActionResult, bool, bSuccess, FGameplayTag, SlotTag, FString, Message);

/**
 * Typed equipment slot widget for runtime UIs.
 * Supports:
 * - Dropping currently dragged inventory item into this slot.
 * - Unequipping slot item back to inventory (right-click).
 */
UCLASS(meta=(DisplayName="YOLO Equipment Slot"))
class YOLOINVENTORY_API UInventoryEquipmentSlotWidget : public UWidget
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	FText SlotDisplayName;

	/** If true, tries to auto-resolve components from owning pawn/player if not assigned manually. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	bool bAutoResolveComponents = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	bool bAllowRightClickUnequip = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance", meta=(ClampMin="8.0", ClampMax="256.0"))
	FVector2D IconSize = FVector2D(56.f, 56.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance")
	FLinearColor EmptyTint = FLinearColor(0.16f, 0.16f, 0.16f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance")
	FLinearColor FilledTint = FLinearColor(0.10f, 0.22f, 0.15f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Appearance")
	FLinearColor InvalidDropTint = FLinearColor(0.30f, 0.10f, 0.10f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	TObjectPtr<UYIEquipmentComponent> EquipmentComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	TObjectPtr<UYIInventoryComponent> InventoryComponent = nullptr;

	UPROPERTY(BlueprintAssignable, Category="Equipment|Events")
	FOnEquipmentSlotActionResult OnSlotActionResult;

	UFUNCTION(BlueprintCallable, Category="Equipment")
	void SetEquipmentComponent(UYIEquipmentComponent* InEquipmentComponent);

	UFUNCTION(BlueprintCallable, Category="Equipment")
	void SetInventoryComponent(UYIInventoryComponent* InInventoryComponent);

	UFUNCTION(BlueprintPure, Category="Equipment")
	UYIEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

	UFUNCTION(BlueprintPure, Category="Equipment")
	UYIInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	UFUNCTION(BlueprintCallable, Category="Equipment")
	void RefreshSlot();

	UFUNCTION(BlueprintCallable, Category="Equipment")
	bool TryEquipFromActiveDrag();

	UFUNCTION(BlueprintCallable, Category="Equipment")
	bool TryUnequipToInventory();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	bool ResolveComponents();
	void BindEquipmentEvents();
	void UnbindEquipmentEvents();
	void UpdateVisualState(bool bForceInvalidTint = false);
	void BroadcastResult(bool bSuccess, const FString& Message);

	FReply HandleMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent);

	UFUNCTION()
	void HandleEquipmentChanged(FGameplayTag ChangedSlotTag, FYIItemInstanceNet Item);

	TSharedPtr<SBorder> RootBorder;
	TSharedPtr<SImage> IconWidget;
	TSharedPtr<STextBlock> LabelWidget;
	FSlateBrush IconBrush;
	TObjectPtr<UTexture2D> CachedIcon = nullptr;
	bool bBoundEquipmentEvents = false;
};
