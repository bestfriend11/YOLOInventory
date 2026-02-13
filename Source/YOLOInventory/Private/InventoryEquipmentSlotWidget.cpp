#include "InventoryEquipmentSlotWidget.h"

#include "InventoryGridWidget.h"
#include "YIEquipmentComponent.h"
#include "YIInventoryComponent.h"
#include "YIItemDefinition.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "InputCoreTypes.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void UInventoryEquipmentSlotWidget::SetEquipmentComponent(UYIEquipmentComponent* InEquipmentComponent)
{
	if (EquipmentComponent == InEquipmentComponent)
	{
		return;
	}

	UnbindEquipmentEvents();
	EquipmentComponent = InEquipmentComponent;
	BindEquipmentEvents();
	RefreshSlot();
}

void UInventoryEquipmentSlotWidget::SetInventoryComponent(UYIInventoryComponent* InInventoryComponent)
{
	InventoryComponent = InInventoryComponent;
	RefreshSlot();
}

bool UInventoryEquipmentSlotWidget::ResolveComponents()
{
	if (EquipmentComponent && InventoryComponent)
	{
		return true;
	}

	if (!bAutoResolveComponents)
	{
		return EquipmentComponent != nullptr;
	}

	APawn* Pawn = nullptr;
	if (EquipmentComponent && EquipmentComponent->GetOwner())
	{
		Pawn = Cast<APawn>(EquipmentComponent->GetOwner());
	}

	if (!Pawn && InventoryComponent && InventoryComponent->GetOwner())
	{
		Pawn = Cast<APawn>(InventoryComponent->GetOwner());
	}

	if (!Pawn)
	{
		if (UWorld* World = GetWorld())
		{
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0))
			{
				Pawn = PC->GetPawn();
			}
		}
	}

	if (Pawn)
	{
		if (!EquipmentComponent)
		{
			EquipmentComponent = Pawn->FindComponentByClass<UYIEquipmentComponent>();
		}
		if (!InventoryComponent)
		{
			InventoryComponent = Pawn->FindComponentByClass<UYIInventoryComponent>();
		}
	}

	BindEquipmentEvents();
	return EquipmentComponent != nullptr;
}

void UInventoryEquipmentSlotWidget::BindEquipmentEvents()
{
	if (bBoundEquipmentEvents || !EquipmentComponent)
	{
		return;
	}
	EquipmentComponent->OnEquipmentChanged.AddDynamic(this, &UInventoryEquipmentSlotWidget::HandleEquipmentChanged);
	bBoundEquipmentEvents = true;
}

void UInventoryEquipmentSlotWidget::UnbindEquipmentEvents()
{
	if (!bBoundEquipmentEvents || !EquipmentComponent)
	{
		return;
	}
	EquipmentComponent->OnEquipmentChanged.RemoveDynamic(this, &UInventoryEquipmentSlotWidget::HandleEquipmentChanged);
	bBoundEquipmentEvents = false;
}

void UInventoryEquipmentSlotWidget::BroadcastResult(bool bSuccess, const FString& Message)
{
	OnSlotActionResult.Broadcast(bSuccess, SlotTag, Message);
}

void UInventoryEquipmentSlotWidget::UpdateVisualState(bool bForceInvalidTint)
{
	if (!RootBorder.IsValid() || !LabelWidget.IsValid() || !IconWidget.IsValid())
	{
		return;
	}

	FLinearColor BackgroundColor = EmptyTint;
	FText LabelText = SlotDisplayName.IsEmpty() ? FText::FromString(SlotTag.ToString()) : SlotDisplayName;
	CachedIcon = nullptr;

	FYIItemInstanceNet EquippedItem;
	if (ResolveComponents() && EquipmentComponent && SlotTag.IsValid() && EquipmentComponent->GetEquippedItem(SlotTag, EquippedItem))
	{
		BackgroundColor = FilledTint;
		if (UYIItemDefinition* Definition = EquippedItem.Definition.IsValid() ? EquippedItem.Definition.Get() : EquippedItem.Definition.LoadSynchronous())
		{
			LabelText = Definition->DisplayName.IsEmpty() ? FText::FromString(Definition->GetName()) : Definition->DisplayName;
			CachedIcon = Definition->Icon.IsValid() ? Definition->Icon.Get() : Definition->Icon.LoadSynchronous();
		}
	}

	if (bForceInvalidTint)
	{
		BackgroundColor = InvalidDropTint;
	}

	RootBorder->SetBorderBackgroundColor(BackgroundColor);
	LabelWidget->SetText(LabelText);

	if (CachedIcon)
	{
		IconBrush.SetResourceObject(CachedIcon);
		IconBrush.ImageSize = IconSize;
		IconBrush.DrawAs = ESlateBrushDrawType::Image;
		IconWidget->SetImage(&IconBrush);
		IconWidget->SetVisibility(EVisibility::Visible);
	}
	else
	{
		IconWidget->SetImage(nullptr);
		IconWidget->SetVisibility(EVisibility::Collapsed);
	}
}

void UInventoryEquipmentSlotWidget::RefreshSlot()
{
	UpdateVisualState(false);
}

bool UInventoryEquipmentSlotWidget::TryEquipFromActiveDrag()
{
	if (!ResolveComponents() || !EquipmentComponent || !SlotTag.IsValid())
	{
		BroadcastResult(false, TEXT("Equip slot is not configured."));
		UpdateVisualState(true);
		return false;
	}

	const bool bSuccess = UInventoryGridWidget::TryEquipActiveDraggedItem(EquipmentComponent, SlotTag);
	if (!bSuccess)
	{
		BroadcastResult(false, TEXT("Drag equip failed. Ensure dragged item matches this slot tag."));
		UpdateVisualState(true);
		return false;
	}

	BroadcastResult(true, TEXT("Item equipped from drag."));
	UpdateVisualState(false);
	return true;
}

bool UInventoryEquipmentSlotWidget::TryUnequipToInventory()
{
	if (!ResolveComponents() || !EquipmentComponent || !SlotTag.IsValid())
	{
		BroadcastResult(false, TEXT("Unequip failed: slot is not configured."));
		UpdateVisualState(true);
		return false;
	}

	if (!InventoryComponent && EquipmentComponent->GetOwner())
	{
		InventoryComponent = EquipmentComponent->GetOwner()->FindComponentByClass<UYIInventoryComponent>();
	}

	if (!InventoryComponent)
	{
		BroadcastResult(false, TEXT("Unequip failed: no inventory component found."));
		UpdateVisualState(true);
		return false;
	}

	const bool bSuccess = EquipmentComponent->UnequipToInventory(InventoryComponent, SlotTag);
	BroadcastResult(bSuccess, bSuccess ? TEXT("Item unequipped to inventory.") : TEXT("Unequip failed."));
	UpdateVisualState(!bSuccess);
	return bSuccess;
}

FReply UInventoryEquipmentSlotWidget::HandleMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& MouseEvent)
{
	(void)Geometry;

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (UInventoryGridWidget::IsItemDragActive(GetWorld()))
		{
			return TryEquipFromActiveDrag() ? FReply::Handled() : FReply::Unhandled();
		}
		return FReply::Handled();
	}

	if (bAllowRightClickUnequip && MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		return TryUnequipToInventory() ? FReply::Handled() : FReply::Unhandled();
	}

	return FReply::Unhandled();
}

void UInventoryEquipmentSlotWidget::HandleEquipmentChanged(FGameplayTag ChangedSlotTag, FYIItemInstanceNet Item)
{
	(void)Item;
	if (!SlotTag.IsValid() || ChangedSlotTag == SlotTag)
	{
		UpdateVisualState(false);
	}
}

TSharedRef<SWidget> UInventoryEquipmentSlotWidget::RebuildWidget()
{
	RootBorder =
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(EmptyTint)
		.Padding(FMargin(6.f))
		.OnMouseButtonDown(BIND_UOBJECT_DELEGATE(FPointerEventHandler, HandleMouseButtonDown))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.f, 0.f, 0.f, 4.f)
			[
				SAssignNew(LabelWidget, STextBlock)
				.Text(SlotDisplayName.IsEmpty() ? FText::FromString(SlotTag.ToString()) : SlotDisplayName)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(IconSize.X)
				.HeightOverride(IconSize.Y)
				[
					SAssignNew(IconWidget, SImage)
				]
			]
		];

	UpdateVisualState(false);
	return RootBorder.ToSharedRef();
}

void UInventoryEquipmentSlotWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	UpdateVisualState(false);
}

void UInventoryEquipmentSlotWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	UnbindEquipmentEvents();
	RootBorder.Reset();
	IconWidget.Reset();
	LabelWidget.Reset();
}
