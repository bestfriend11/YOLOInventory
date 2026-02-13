#include "YIEquipmentComponent.h"

#include "Net/UnrealNetwork.h"
#include "YIInventoryBag.h"
#include "YIInventoryComponent.h"
#include "YIItemDefinition.h"
#include "Engine/Engine.h"

DEFINE_LOG_CATEGORY_STATIC(LogYIEquipment, Log, All);

namespace YIEquipmentPrivate
{
	static FYIItemInstanceNet FullToNet(const FYIItemInstance& Full)
	{
		FYIItemInstanceNet Out;
		Out.Definition = Full.Definition;
		Out.Count = Full.Count;
		Out.CustomStackKey = Full.CustomStackKey;
		Out.bRotated = Full.bRotated;
		Out.Affixes = Full.Affixes;
		Out.Attributes.Reset();
		for (const TPair<FName, float>& KV : Full.Attributes)
		{
			FYIAttributeKV OutKV;
			OutKV.Name = KV.Key;
			OutKV.Value = KV.Value;
			Out.Attributes.Add(OutKV);
		}
		return Out;
	}

	static FYIItemInstance NetToFull(const FYIItemInstanceNet& Net)
	{
		FYIItemInstance Out;
		Out.Definition = Net.Definition;
		Out.Count = Net.Count;
		Out.CustomStackKey = Net.CustomStackKey;
		Out.bRotated = Net.bRotated;
		Out.Affixes = Net.Affixes;
		Out.Attributes.Reset();
		for (const FYIAttributeKV& KV : Net.Attributes)
		{
			Out.Attributes.Add(KV.Name, KV.Value);
		}
		return Out;
	}
}

UYIEquipmentComponent::UYIEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UYIEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UYIEquipmentComponent, EquippedItems);
}

void UYIEquipmentComponent::OnRep_EquippedItems()
{
	for (const FYIEquippedItemEntry& Entry : EquippedItems)
	{
		OnEquipmentChanged.Broadcast(Entry.SlotTag, Entry.Item);
	}
}

int32 UYIEquipmentComponent::FindEntryIndex(FGameplayTag SlotTag) const
{
	return EquippedItems.IndexOfByPredicate([SlotTag](const FYIEquippedItemEntry& Entry)
	{
		return Entry.SlotTag == SlotTag;
	});
}

bool UYIEquipmentComponent::IsAllowedSlot(FGameplayTag SlotTag) const
{
	if (!SlotTag.IsValid())
	{
		return false;
	}
	if (AllowedEquipSlots.Num() == 0)
	{
		return true;
	}
	return AllowedEquipSlots.HasTagExact(SlotTag);
}

FGameplayTag UYIEquipmentComponent::ResolveSlotTagFromDefinition(const UYIItemDefinition* Definition) const
{
	if (!Definition)
	{
		return FGameplayTag();
	}

	TArray<FGameplayTag> ItemTags;
	Definition->Tags.GetGameplayTagArray(ItemTags);
	if (Definition->ItemType.IsValid())
	{
		ItemTags.Add(Definition->ItemType);
	}

	for (const FGameplayTag& Tag : ItemTags)
	{
		const FString TagString = Tag.ToString();
		if (TagString.StartsWith(EquipSlotTagPrefix))
		{
			return Tag;
		}
	}

	return FGameplayTag();
}

bool UYIEquipmentComponent::DoesDefinitionSupportSlot(const UYIItemDefinition* Definition, FGameplayTag SlotTag) const
{
	if (!Definition || !SlotTag.IsValid())
	{
		return false;
	}

	const FGameplayTag ResolvedSlot = ResolveSlotTagFromDefinition(Definition);
	if (ResolvedSlot == SlotTag)
	{
		return true;
	}

	TArray<FGameplayTag> ItemTags;
	Definition->Tags.GetGameplayTagArray(ItemTags);
	if (Definition->ItemType.IsValid())
	{
		ItemTags.Add(Definition->ItemType);
	}

	for (const FGameplayTag& Tag : ItemTags)
	{
		if (Tag == SlotTag)
		{
			return true;
		}
	}

	return false;
}

void UYIEquipmentComponent::EmitEquipmentMessage(const FString& Message, const FColor& Color) const
{
	UE_LOG(LogYIEquipment, Log, TEXT("%s"), *Message);
	if (bDebugEquipment && GEngine)
	{
		const float Duration = bPinDebugMessages ? 20.0f : 4.0f;
		const uint32 Hash = GetTypeHash(Message);
		const uint64 Key = 0x5949455100000000ULL | static_cast<uint64>(Hash); // "YIEQ"
		GEngine->AddOnScreenDebugMessage(Key, Duration, Color, Message);
	}
}

void UYIEquipmentComponent::BroadcastResult(bool bSuccess, FGameplayTag SlotTag, const FString& Message)
{
	OnEquipmentOperationResult.Broadcast(bSuccess, SlotTag, Message);
	EmitEquipmentMessage(Message, bSuccess ? FColor::Green : FColor::Red);
}

bool UYIEquipmentComponent::GetEquippedItem(FGameplayTag SlotTag, FYIItemInstanceNet& OutItem) const
{
	const int32 Index = FindEntryIndex(SlotTag);
	if (Index == INDEX_NONE)
	{
		return false;
	}
	OutItem = EquippedItems[Index].Item;
	return true;
}

void UYIEquipmentComponent::GetPersistedEquipment(TArray<FYIEquippedItemEntry>& OutEntries) const
{
	OutEntries = EquippedItems;
}

void UYIEquipmentComponent::LoadPersistedEquipment(const TArray<FYIEquippedItemEntry>& InEntries)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	EquippedItems = InEntries;
	for (const FYIEquippedItemEntry& Entry : EquippedItems)
	{
		OnEquipmentChanged.Broadcast(Entry.SlotTag, Entry.Item);
	}
	EmitEquipmentMessage(FString::Printf(TEXT("Loaded %d persisted equipped entries."), EquippedItems.Num()), FColor::Cyan);
}

bool UYIEquipmentComponent::ValidateEquipmentSetup(TArray<FString>& OutBlockingIssues, TArray<FString>& OutWarnings) const
{
	OutBlockingIssues.Reset();
	OutWarnings.Reset();

	TSet<FGameplayTag> SeenSlots;
	for (const FYIEquippedItemEntry& Entry : EquippedItems)
	{
		if (!Entry.SlotTag.IsValid())
		{
			OutBlockingIssues.Add(TEXT("Equipment entry has invalid SlotTag."));
			continue;
		}

		if (SeenSlots.Contains(Entry.SlotTag))
		{
			OutBlockingIssues.Add(FString::Printf(TEXT("Duplicate equipped slot '%s'."), *Entry.SlotTag.ToString()));
		}
		SeenSlots.Add(Entry.SlotTag);

		if (AllowedEquipSlots.Num() > 0 && !AllowedEquipSlots.HasTagExact(Entry.SlotTag))
		{
			OutWarnings.Add(FString::Printf(TEXT("Equipped slot '%s' is outside AllowedEquipSlots."), *Entry.SlotTag.ToString()));
		}

		if (Entry.Item.Definition.ToSoftObjectPath().IsNull())
		{
			OutWarnings.Add(FString::Printf(TEXT("Equipped slot '%s' has no item definition."), *Entry.SlotTag.ToString()));
		}
	}

	return OutBlockingIssues.Num() == 0;
}

bool UYIEquipmentComponent::EquipFromInventory(UYIInventoryComponent* SourceInventory, int32 SourceIndex, FGameplayTag RequestedSlotTag)
{
	if (!GetOwner())
	{
		return false;
	}
	if (!GetOwner()->HasAuthority())
	{
		ServerEquipFromInventory(SourceInventory, SourceIndex, RequestedSlotTag);
		return true;
	}

	FString Message;
	const bool bSuccess = EquipFromInventoryInternal(SourceInventory, SourceIndex, RequestedSlotTag, Message);
	BroadcastResult(bSuccess, RequestedSlotTag, Message);
	return bSuccess;
}

void UYIEquipmentComponent::ServerEquipFromInventory_Implementation(UYIInventoryComponent* SourceInventory, int32 SourceIndex, FGameplayTag RequestedSlotTag)
{
	if (!SourceInventory && GetOwner())
	{
		SourceInventory = GetOwner()->FindComponentByClass<UYIInventoryComponent>();
	}
	EquipFromInventory(SourceInventory, SourceIndex, RequestedSlotTag);
}

bool UYIEquipmentComponent::UnequipToInventory(UYIInventoryComponent* DestInventory, FGameplayTag SlotTag)
{
	if (!GetOwner())
	{
		return false;
	}
	if (!GetOwner()->HasAuthority())
	{
		ServerUnequipToInventory(DestInventory, SlotTag);
		return true;
	}

	FString Message;
	const bool bSuccess = UnequipToInventoryInternal(DestInventory, SlotTag, Message);
	BroadcastResult(bSuccess, SlotTag, Message);
	return bSuccess;
}

void UYIEquipmentComponent::ServerUnequipToInventory_Implementation(UYIInventoryComponent* DestInventory, FGameplayTag SlotTag)
{
	if (!DestInventory && GetOwner())
	{
		DestInventory = GetOwner()->FindComponentByClass<UYIInventoryComponent>();
	}
	UnequipToInventory(DestInventory, SlotTag);
}

bool UYIEquipmentComponent::EquipFromInventoryInternal(UYIInventoryComponent* SourceInventory, int32 SourceIndex, FGameplayTag RequestedSlotTag, FString& OutMessage)
{
	if (!SourceInventory || SourceInventory->GetOwner() != GetOwner())
	{
		OutMessage = TEXT("Equip failed: Source inventory is missing or not owned by this actor.");
		return false;
	}

	UYIInventoryBag* SourceBag = SourceInventory->EquippedBag;
	if (!SourceBag)
	{
		OutMessage = TEXT("Equip failed: Source inventory has no active bag.");
		return false;
	}
	if (!SourceBag->Items.IsValidIndex(SourceIndex))
	{
		OutMessage = FString::Printf(TEXT("Equip failed: Source index %d is invalid."), SourceIndex);
		return false;
	}

	const FYIBagItem SourceBagItem = SourceBag->Items[SourceIndex];
	UYIItemDefinition* Definition = SourceBagItem.Item.Definition.IsValid()
		? SourceBagItem.Item.Definition.Get()
		: SourceBagItem.Item.Definition.LoadSynchronous();
	if (!Definition)
	{
		OutMessage = TEXT("Equip failed: Item definition could not be loaded.");
		return false;
	}

	FGameplayTag SlotTag = RequestedSlotTag;
	if (!SlotTag.IsValid())
	{
		SlotTag = ResolveSlotTagFromDefinition(Definition);
	}
	if (!IsAllowedSlot(SlotTag))
	{
		OutMessage = FString::Printf(TEXT("Equip failed: Slot tag '%s' is invalid or not allowed."), *SlotTag.ToString());
		return false;
	}
	if (!DoesDefinitionSupportSlot(Definition, SlotTag))
	{
		OutMessage = FString::Printf(
			TEXT("Equip failed: Item '%s' does not support slot '%s'. Add a matching Equip.Slot.* gameplay tag on item definition."),
			*Definition->GetName(),
			*SlotTag.ToString());
		return false;
	}

	const int32 ExistingIndex = FindEntryIndex(SlotTag);
	const FYIItemInstanceNet SourceNet = YIEquipmentPrivate::FullToNet(SourceBagItem.Item);

	if (!SourceInventory->RemoveItem(SourceIndex))
	{
		OutMessage = TEXT("Equip failed: Could not remove source item from bag.");
		return false;
	}

	if (ExistingIndex != INDEX_NONE)
	{
		FYIBagItem ReturnToBag;
		ReturnToBag.Item = YIEquipmentPrivate::NetToFull(EquippedItems[ExistingIndex].Item);
		if (UYIItemDefinition* ExistingDef = ReturnToBag.Item.Definition.IsValid()
			? ReturnToBag.Item.Definition.Get()
			: ReturnToBag.Item.Definition.LoadSynchronous())
		{
			const FIntPoint BaseSize = ExistingDef->DefaultSize;
			ReturnToBag.Size = ReturnToBag.Item.bRotated ? FIntPoint(BaseSize.Y, BaseSize.X) : BaseSize;
		}
		else
		{
			ReturnToBag.Size = FIntPoint(1, 1);
		}

		if (SourceBag->AddBagItem(ReturnToBag) == INDEX_NONE)
		{
			// Best-effort rollback: put source item back.
			const int32 RollbackIndex = SourceBag->AddBagItem(SourceBagItem);
			if (RollbackIndex == INDEX_NONE)
			{
				EmitEquipmentMessage(TEXT("Equip rollback failed: source item could not be restored to bag."), FColor::Red);
			}
			OutMessage = TEXT("Equip failed: Could not move previously equipped item back to bag.");
			return false;
		}
	}

	FYIEquippedItemEntry NewEntry;
	NewEntry.SlotTag = SlotTag;
	NewEntry.Item = SourceNet;

	if (ExistingIndex != INDEX_NONE)
	{
		EquippedItems[ExistingIndex] = NewEntry;
	}
	else
	{
		EquippedItems.Add(NewEntry);
	}

	OnEquipmentChanged.Broadcast(SlotTag, SourceNet);
	OutMessage = FString::Printf(TEXT("Equipped '%s' into slot '%s'."), *Definition->GetName(), *SlotTag.ToString());
	return true;
}

bool UYIEquipmentComponent::UnequipToInventoryInternal(UYIInventoryComponent* DestInventory, FGameplayTag SlotTag, FString& OutMessage)
{
	if (!SlotTag.IsValid())
	{
		OutMessage = TEXT("Unequip failed: Slot tag is invalid.");
		return false;
	}
	if (!DestInventory || DestInventory->GetOwner() != GetOwner())
	{
		OutMessage = TEXT("Unequip failed: Destination inventory is missing or not owned by this actor.");
		return false;
	}

	const int32 EntryIndex = FindEntryIndex(SlotTag);
	if (EntryIndex == INDEX_NONE)
	{
		OutMessage = FString::Printf(TEXT("Unequip failed: No equipped item in slot '%s'."), *SlotTag.ToString());
		return false;
	}

	UYIInventoryBag* DestBag = DestInventory->EquippedBag;
	if (!DestBag)
	{
		if (DestInventory->Bags.Num() > 0)
		{
			DestBag = DestInventory->Bags[0];
			DestInventory->OpenBag(DestBag);
		}
	}
	if (!DestBag)
	{
		OutMessage = TEXT("Unequip failed: Destination inventory has no active bag.");
		return false;
	}

	FYIBagItem ToInventory;
	ToInventory.Item = YIEquipmentPrivate::NetToFull(EquippedItems[EntryIndex].Item);
	if (UYIItemDefinition* Def = ToInventory.Item.Definition.IsValid()
		? ToInventory.Item.Definition.Get()
		: ToInventory.Item.Definition.LoadSynchronous())
	{
		const FIntPoint BaseSize = Def->DefaultSize;
		ToInventory.Size = ToInventory.Item.bRotated ? FIntPoint(BaseSize.Y, BaseSize.X) : BaseSize;
	}
	else
	{
		ToInventory.Size = FIntPoint(1, 1);
	}

	if (DestBag->AddBagItem(ToInventory) == INDEX_NONE)
	{
		OutMessage = TEXT("Unequip failed: Destination bag has no room for the item.");
		return false;
	}

	const FYIItemInstanceNet UnequippedItem = EquippedItems[EntryIndex].Item;
	EquippedItems.RemoveAt(EntryIndex);
	OnEquipmentChanged.Broadcast(SlotTag, UnequippedItem);
	OutMessage = FString::Printf(TEXT("Unequipped slot '%s' to bag '%s'."), *SlotTag.ToString(), *DestBag->GetName());
	return true;
}
