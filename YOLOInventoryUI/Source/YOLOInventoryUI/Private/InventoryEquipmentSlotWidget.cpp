#include "InventoryEquipmentSlotWidget.h"

#include "InventoryGridWidget.h"
#include "YIEquipmentComponent.h"
#include "YIInventoryComponent.h"
#include "YIItemDefinition.h"
#include "YIItemSchemaResolver.h"
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

namespace
{
	static bool YIEquipmentSlot_BeginPostUnequipDrag(UYIInventoryComponent* InventoryComponent, UYIInventoryBag* Bag, int32 ItemIndex, const UWorld* World)
	{
		if (!Bag || ItemIndex == INDEX_NONE)
		{
			return false;
		}

		// Client detached-drag currently removes the item from the authoritative bag before the local drag flow is finalized.
		// That can desync/destroy items when the drag is not consumed. Use non-destructive grid drag semantics on clients.
		if (InventoryComponent && InventoryComponent->GetOwner() && !InventoryComponent->GetOwner()->HasAuthority())
		{
			return UInventoryGridWidget::BeginDragFromBagItem(Bag, ItemIndex, World);
		}

		return UInventoryGridWidget::BeginDetachedDragFromBagItem(Bag, ItemIndex, World);
	}
}

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
	if (InventoryComponent == InInventoryComponent)
	{
		return;
	}
	UnbindInventoryEvents();
	InventoryComponent = InInventoryComponent;
	BindInventoryEvents();
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
	BindInventoryEvents();
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

void UInventoryEquipmentSlotWidget::BindInventoryEvents()
{
	if (bBoundInventoryEvents || !InventoryComponent)
	{
		return;
	}
	InventoryComponent->OnBagOpened.AddDynamic(this, &UInventoryEquipmentSlotWidget::HandleInventoryBagOpened);
	bBoundInventoryEvents = true;
}

void UInventoryEquipmentSlotWidget::UnbindInventoryEvents()
{
	if (!bBoundInventoryEvents || !InventoryComponent)
	{
		return;
	}
	InventoryComponent->OnBagOpened.RemoveDynamic(this, &UInventoryEquipmentSlotWidget::HandleInventoryBagOpened);
	bBoundInventoryEvents = false;
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
			LabelText = YIItemSchema::GetDisplayName(Definition);
			const TSoftObjectPtr<UTexture2D> EffectiveIcon = YIItemSchema::GetIcon(Definition);
			CachedIcon = EffectiveIcon.IsValid() ? EffectiveIcon.Get() : EffectiveIcon.LoadSynchronous();
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
		IconBrush.ImageSize = FVector2D(CachedIcon->GetSizeX(), CachedIcon->GetSizeY());
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
	UYIInventoryBag* UnusedBag = nullptr;
	int32 UnusedIndex = INDEX_NONE;
	return TryUnequipToInventoryResolved(UnusedBag, UnusedIndex);
}

bool UInventoryEquipmentSlotWidget::TryUnequipToInventoryResolved(UYIInventoryBag*& OutBag, int32& OutItemIndex)
{
	OutBag = nullptr;
	OutItemIndex = INDEX_NONE;

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

	const bool bSuccess = EquipmentComponent->UnequipToInventoryAndResolveItem(InventoryComponent, SlotTag, OutBag, OutItemIndex);
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

		if (!ResolveComponents() || !EquipmentComponent || !InventoryComponent || !SlotTag.IsValid())
		{
			return FReply::Handled();
		}

		FYIItemInstanceNet IgnoredItem;
		if (!EquipmentComponent->GetEquippedItem(SlotTag, IgnoredItem))
		{
			return FReply::Handled();
		}

		UYIInventoryBag* AddedBag = nullptr;
		int32 AddedIndex = INDEX_NONE;
		// Capture identity before unequip so client RPC flow can resume a detached drag after replication.
		PendingDetachedUnequipItemInstanceId = IgnoredItem.InstanceId;
		bPendingDetachedUnequipDrag = PendingDetachedUnequipItemInstanceId.IsValid();
		const bool bUnequipped = TryUnequipToInventoryResolved(AddedBag, AddedIndex);
		if (!bUnequipped)
		{
			bPendingDetachedUnequipDrag = false;
			PendingDetachedUnequipItemInstanceId.Invalidate();
			return FReply::Handled();
		}

		// For non-authority RPC flow, result index may not be available immediately.
		if (!AddedBag || AddedIndex == INDEX_NONE)
		{
			// Wait for owner-only bag mirror update; HandleInventoryBagOpened will start detached drag.
			return FReply::Handled();
		}

		bPendingDetachedUnequipDrag = false;
		PendingDetachedUnequipItemInstanceId.Invalidate();
		YIEquipmentSlot_BeginPostUnequipDrag(InventoryComponent, AddedBag, AddedIndex, GetWorld());
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
		if (bPendingDetachedUnequipDrag && EquipmentComponent)
		{
			FYIItemInstanceNet CurrentItem;
			// If slot was refilled before we consumed the pending drag, cancel the pending state.
			if (EquipmentComponent->GetEquippedItem(SlotTag, CurrentItem))
			{
				bPendingDetachedUnequipDrag = false;
				PendingDetachedUnequipItemInstanceId.Invalidate();
			}
		}
		UpdateVisualState(false);
	}
}

void UInventoryEquipmentSlotWidget::HandleInventoryBagOpened(UYIInventoryBag* Bag)
{
	TryConsumePendingDetachedUnequipDrag(Bag);
}

void UInventoryEquipmentSlotWidget::TryConsumePendingDetachedUnequipDrag(UYIInventoryBag* CandidateBag)
{
	if (!bPendingDetachedUnequipDrag || !PendingDetachedUnequipItemInstanceId.IsValid() || !CandidateBag)
	{
		return;
	}
	if (UInventoryGridWidget::IsItemDragActive(GetWorld()))
	{
		return;
	}

	for (int32 Index = 0; Index < CandidateBag->Items.Num(); ++Index)
	{
		if (CandidateBag->Items[Index].Item.InstanceId != PendingDetachedUnequipItemInstanceId)
		{
			continue;
		}

		if (YIEquipmentSlot_BeginPostUnequipDrag(InventoryComponent, CandidateBag, Index, GetWorld()))
		{
			bPendingDetachedUnequipDrag = false;
			PendingDetachedUnequipItemInstanceId.Invalidate();
		}
		return;
	}
}

TSharedRef<SWidget> UInventoryEquipmentSlotWidget::RebuildWidget()
{
	RootBorder =
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(EmptyTint)
		.Padding(FMargin(3.f))
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
			.FillHeight(1.f)
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SBox)
				.MinDesiredWidth(IconSize.X)
				.MinDesiredHeight(IconSize.Y)
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
	UnbindInventoryEvents();
	RootBorder.Reset();
	IconWidget.Reset();
	LabelWidget.Reset();
}
