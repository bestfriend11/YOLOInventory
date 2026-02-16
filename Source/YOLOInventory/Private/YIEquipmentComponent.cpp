#include "YIEquipmentComponent.h"

#include "Net/UnrealNetwork.h"
#include "YIInventoryBag.h"
#include "YIInventoryComponent.h"
#include "YIItemDefinition.h"
#include "YIEquipmentSchemaAsset.h"
#include "YIInventoryBlueprintLibrary.h"
#include "YIItemSFXLibrary.h"
#include "YOLOInventorySettings.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Algo/Unique.h"

DEFINE_LOG_CATEGORY_STATIC(LogYIEquipment, Log, All);

namespace YIEquipmentPrivate
{
	static FYIItemInstanceNet FullToNet(const FYIItemInstance& Full)
	{
		FYIItemInstanceNet Out;
		Out.Definition = Full.Definition;
		Out.Count = Full.Count;
		Out.InstanceId = Full.InstanceId;
		Out.StackId = Full.StackId;
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
		Out.InstanceId = Net.InstanceId.IsValid() ? Net.InstanceId : FGuid::NewGuid();
		Out.StackId = Net.StackId.IsValid() ? Net.StackId : FGuid::NewGuid();
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

void UYIEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority() && bApplySchemaOnBeginPlay)
	{
		ApplyEquipmentSchema(false);
	}
}

void UYIEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UYIEquipmentComponent, EquippedItems);
	DOREPLIFETIME(UYIEquipmentComponent, SlotDefinitions);
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

int32 UYIEquipmentComponent::FindSlotDefinitionIndex(FGameplayTag SlotTag) const
{
	return SlotDefinitions.IndexOfByPredicate([SlotTag](const FYIEquipmentSlotDefinition& Entry)
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
	if (SlotDefinitions.Num() > 0)
	{
		return FindSlotDefinitionIndex(SlotTag) != INDEX_NONE;
	}
	if (AllowedEquipSlots.Num() == 0)
	{
		const UYOLOInventorySettings& Settings = UYOLOInventorySettings::Get();
		if (Settings.bEnableEquipmentSlotTagLocking)
		{
			if (Settings.AllowedEquipmentSlotTags.Num() > 0)
			{
				return Settings.AllowedEquipmentSlotTags.HasTagExact(SlotTag);
			}

			const FString Prefix = Settings.EquipmentSlotTagPrefix;
			if (!Prefix.IsEmpty())
			{
				return SlotTag.ToString().StartsWith(Prefix, ESearchCase::IgnoreCase);
			}
		}
		return true;
	}
	return AllowedEquipSlots.HasTagExact(SlotTag);
}

bool UYIEquipmentComponent::IsSlotUnlocked(FGameplayTag SlotTag) const
{
	if (!SlotTag.IsValid())
	{
		return false;
	}

	const int32 SlotDefIndex = FindSlotDefinitionIndex(SlotTag);
	if (SlotDefIndex != INDEX_NONE)
	{
		return SlotDefinitions[SlotDefIndex].bUnlocked;
	}

	return true;
}

bool UYIEquipmentComponent::SetSlotUnlocked(FGameplayTag SlotTag, bool bUnlocked)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !SlotTag.IsValid())
	{
		return false;
	}

	const int32 SlotDefIndex = FindSlotDefinitionIndex(SlotTag);
	if (SlotDefIndex == INDEX_NONE)
	{
		return false;
	}

	if (SlotDefinitions[SlotDefIndex].bUnlocked == bUnlocked)
	{
		return true;
	}

	SlotDefinitions[SlotDefIndex].bUnlocked = bUnlocked;
	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
	return true;
}

bool UYIEquipmentComponent::ApplyEquipmentSchema(bool bOverwriteExisting)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	UYIEquipmentSchemaAsset* Schema = EquipmentSchemaAsset.IsValid()
		? EquipmentSchemaAsset.Get()
		: EquipmentSchemaAsset.LoadSynchronous();
	if (!Schema)
	{
		return false;
	}

	bool bChanged = false;
	if (bOverwriteExisting || SlotDefinitions.Num() == 0)
	{
		SlotDefinitions = Schema->SlotDefinitions;
		bChanged = true;
	}
	if (bOverwriteExisting || AllowedEquipSlots.Num() == 0)
	{
		AllowedEquipSlots = Schema->AllowedEquipSlots;
		bChanged = true;
	}
	if (bOverwriteExisting || EquipSlotTagPrefix.IsEmpty())
	{
		if (!Schema->EquipSlotTagPrefix.IsEmpty())
		{
			EquipSlotTagPrefix = Schema->EquipSlotTagPrefix;
			bChanged = true;
		}
	}

	if (bChanged && GetOwner())
	{
		GetOwner()->ForceNetUpdate();
	}
	return bChanged;
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

	FString EffectivePrefix = EquipSlotTagPrefix;
	if (EffectivePrefix.IsEmpty())
	{
		EffectivePrefix = UYOLOInventorySettings::Get().EquipmentSlotTagPrefix;
	}

	for (const FGameplayTag& Tag : ItemTags)
	{
		const FString TagString = Tag.ToString();
		if (!EffectivePrefix.IsEmpty() && TagString.StartsWith(EffectivePrefix))
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
	if (ResolvedSlot.IsValid())
	{
		// Support exact slot matches and child slots (e.g. item tag Equip.Slot.Chest matches Equip.Slot.Chest.01).
		if (ResolvedSlot == SlotTag || SlotTag.MatchesTag(ResolvedSlot) || ResolvedSlot.MatchesTag(SlotTag))
		{
			return true;
		}
	}

	TArray<FGameplayTag> ItemTags;
	Definition->Tags.GetGameplayTagArray(ItemTags);
	if (Definition->ItemType.IsValid())
	{
		ItemTags.Add(Definition->ItemType);
	}

	for (const FGameplayTag& Tag : ItemTags)
	{
		if (Tag == SlotTag || SlotTag.MatchesTag(Tag) || Tag.MatchesTag(SlotTag))
		{
			return true;
		}
	}

	return false;
}

bool UYIEquipmentComponent::DoesDefinitionPassSlotFilter(const UYIItemDefinition* Definition, FGameplayTag SlotTag) const
{
	if (!Definition || !SlotTag.IsValid())
	{
		return false;
	}

	const int32 SlotDefIndex = FindSlotDefinitionIndex(SlotTag);
	if (SlotDefIndex == INDEX_NONE || SlotDefinitions[SlotDefIndex].AcceptedItemTags.Num() == 0)
	{
		return true;
	}

	FGameplayTagContainer ItemTags = Definition->Tags;
	if (Definition->ItemType.IsValid())
	{
		ItemTags.AddTag(Definition->ItemType);
	}

	return ItemTags.HasAny(SlotDefinitions[SlotDefIndex].AcceptedItemTags);
}

int32 UYIEquipmentComponent::ResolveOrCreateEquipGroupId()
{
	const int32 NewGroupId = FMath::Max(1, NextEquipGroupId++);
	return NewGroupId;
}

int32 UYIEquipmentComponent::GetEntryGroupIdForIndex(int32 EntryIndex) const
{
	if (!EquippedItems.IsValidIndex(EntryIndex))
	{
		return 0;
	}

	const int32 GroupId = EquippedItems[EntryIndex].EquipGroupId;
	if (GroupId > 0)
	{
		return GroupId;
	}

	// Legacy entries before group support: treat each slot as standalone group.
	return (EntryIndex + 1) * -1;
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

USoundBase* UYIEquipmentComponent::ResolveEquipSound(UYIItemDefinition* Definition) const
{
	if (!Definition || !GetOwner())
	{
		return nullptr;
	}

	const UYIInventoryComponent* InventoryComp = GetOwner()->FindComponentByClass<UYIInventoryComponent>();
	const UYIItemSFXLibrary* Library = nullptr;
	if (InventoryComp)
	{
		if (InventoryComp->ItemSFXLibrary.IsValid())
		{
			Library = InventoryComp->ItemSFXLibrary.Get();
		}
		else if (InventoryComp->ItemSFXLibrary.ToSoftObjectPath().IsValid())
		{
			Library = InventoryComp->ItemSFXLibrary.LoadSynchronous();
		}
	}

	USoundBase* Sound = UYIInventoryBlueprintLibrary::ResolveItemSFXSound(Definition, Library, EYIItemSFXEvent::Equip);
	if (!Sound)
	{
		// Backward compatibility for profiles that only configured "Drop".
		Sound = UYIInventoryBlueprintLibrary::ResolveItemSFXSound(Definition, Library, EYIItemSFXEvent::Drop);
	}
	return Sound;
}

void UYIEquipmentComponent::PlayEquipSound(UYIItemDefinition* Definition) const
{
	if (!bPlayEquipSound || !GetOwner())
	{
		return;
	}

	if (USoundBase* Sound = ResolveEquipSound(Definition))
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, GetOwner()->GetActorLocation());
	}
}

void UYIEquipmentComponent::HandleItemEquippedFeedback(FGameplayTag SlotTag, const FYIItemInstanceNet& Item, UYIItemDefinition* Definition)
{
	OnItemEquipped.Broadcast(SlotTag, Item, Definition);
	PlayEquipSound(Definition);
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
		NextEquipGroupId = FMath::Max(NextEquipGroupId, Entry.EquipGroupId + 1);
	}
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

	if (SlotDefinitions.Num() == 0)
	{
		if (!EquipmentSchemaAsset.IsNull())
		{
			const UYIEquipmentSchemaAsset* Schema = EquipmentSchemaAsset.LoadSynchronous();
			if (!Schema)
			{
				OutWarnings.Add(TEXT("EquipmentSchemaAsset is set but failed to load."));
			}
			else if (Schema->SlotDefinitions.Num() == 0)
			{
				OutWarnings.Add(FString::Printf(TEXT("Equipment schema '%s' has no slot definitions."), *Schema->GetName()));
			}
		}
		else
		{
			OutWarnings.Add(TEXT("No SlotDefinitions and no EquipmentSchemaAsset configured."));
		}
	}

	TSet<FGameplayTag> SeenSlotDefs;
	for (const FYIEquipmentSlotDefinition& SlotDef : SlotDefinitions)
	{
		if (!SlotDef.SlotTag.IsValid())
		{
			OutBlockingIssues.Add(TEXT("SlotDefinitions contains an invalid SlotTag."));
			continue;
		}

		if (SeenSlotDefs.Contains(SlotDef.SlotTag))
		{
			OutBlockingIssues.Add(FString::Printf(TEXT("SlotDefinitions has duplicate slot '%s'."), *SlotDef.SlotTag.ToString()));
		}
		SeenSlotDefs.Add(SlotDef.SlotTag);
	}

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

		if (!IsAllowedSlot(Entry.SlotTag))
		{
			OutWarnings.Add(FString::Printf(TEXT("Equipped slot '%s' is not allowed by current slot rules."), *Entry.SlotTag.ToString()));
		}
		if (!IsSlotUnlocked(Entry.SlotTag))
		{
			OutWarnings.Add(FString::Printf(TEXT("Equipped slot '%s' is currently locked."), *Entry.SlotTag.ToString()));
		}

		if (Entry.Item.Definition.ToSoftObjectPath().IsNull())
		{
			OutWarnings.Add(FString::Printf(TEXT("Equipped slot '%s' has no item definition."), *Entry.SlotTag.ToString()));
		}
		else if (const UYIItemDefinition* Def = Entry.Item.Definition.IsValid() ? Entry.Item.Definition.Get() : Entry.Item.Definition.LoadSynchronous())
		{
			if (!DoesDefinitionSupportSlot(Def, Entry.SlotTag))
			{
				OutWarnings.Add(FString::Printf(TEXT("Equipped item '%s' does not declare support for slot '%s'."), *Def->GetName(), *Entry.SlotTag.ToString()));
			}
			if (!DoesDefinitionPassSlotFilter(Def, Entry.SlotTag))
			{
				OutWarnings.Add(FString::Printf(TEXT("Equipped item '%s' does not satisfy slot '%s' accepted tag filter."), *Def->GetName(), *Entry.SlotTag.ToString()));
			}
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
	const bool bSuccess = UnequipToInventoryInternal(DestInventory, SlotTag, Message, nullptr, nullptr);
	BroadcastResult(bSuccess, SlotTag, Message);
	return bSuccess;
}

bool UYIEquipmentComponent::UnequipToInventoryAndResolveItem(UYIInventoryComponent* DestInventory, FGameplayTag SlotTag, UYIInventoryBag*& OutBag, int32& OutItemIndex)
{
	OutBag = nullptr;
	OutItemIndex = INDEX_NONE;

	if (!GetOwner())
	{
		return false;
	}

	if (!GetOwner()->HasAuthority())
	{
		// Non-authority callers can request unequip, but deterministic source index is only known on authority.
		return UnequipToInventory(DestInventory, SlotTag);
	}

	FString Message;
	UYIInventoryBag* AddedBag = nullptr;
	int32 AddedIndex = INDEX_NONE;
	const bool bSuccess = UnequipToInventoryInternal(DestInventory, SlotTag, Message, &AddedBag, &AddedIndex);
	BroadcastResult(bSuccess, SlotTag, Message);

	if (bSuccess)
	{
		OutBag = AddedBag;
		OutItemIndex = AddedIndex;
	}
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

void UYIEquipmentComponent::ClientNotifyItemEquipped_Implementation(FGameplayTag SlotTag, FYIItemInstanceNet Item)
{
	UYIItemDefinition* Definition = Item.Definition.IsValid() ? Item.Definition.Get() : Item.Definition.LoadSynchronous();
	HandleItemEquippedFeedback(SlotTag, Item, Definition);
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

	FYIBagItem SourceBagItem = SourceBag->Items[SourceIndex];
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
	if (!SlotTag.IsValid())
	{
		OutMessage = FString::Printf(TEXT("Equip failed: Item '%s' has no resolvable equipment slot tag."), *Definition->GetName());
		return false;
	}

	// Allow parent slot tags on item defs (e.g. Equip.Slot.Chest) to resolve to concrete schema slots
	// (e.g. Equip.Slot.Chest.01, Equip.Slot.Chest.02, ...).
	if (!IsAllowedSlot(SlotTag) && SlotDefinitions.Num() > 0)
	{
		for (const FYIEquipmentSlotDefinition& SlotDef : SlotDefinitions)
		{
			if (!SlotDef.SlotTag.IsValid())
			{
				continue;
			}
			if (SlotDef.SlotTag == SlotTag || SlotDef.SlotTag.MatchesTag(SlotTag) || SlotTag.MatchesTag(SlotDef.SlotTag))
			{
				SlotTag = SlotDef.SlotTag;
				break;
			}
		}
	}

	if (!IsAllowedSlot(SlotTag))
	{
		OutMessage = FString::Printf(TEXT("Equip failed: Slot tag '%s' is invalid or not allowed."), *SlotTag.ToString());
		return false;
	}
	if (!IsSlotUnlocked(SlotTag))
	{
		OutMessage = FString::Printf(TEXT("Equip failed: Slot tag '%s' is currently locked."), *SlotTag.ToString());
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
	if (!DoesDefinitionPassSlotFilter(Definition, SlotTag))
	{
		OutMessage = FString::Printf(
			TEXT("Equip failed: Item '%s' does not match slot '%s' accepted item tags."),
			*Definition->GetName(),
			*SlotTag.ToString());
		return false;
	}

	const int32 RequiredEquipCost = FMath::Max(1, Definition->EquipSlotCost);
	const int32 SlotDefIndex = FindSlotDefinitionIndex(SlotTag);
	if (SlotDefIndex != INDEX_NONE)
	{
		const int32 SlotCapacity = FMath::Max(1, SlotDefinitions[SlotDefIndex].Capacity);
		if (RequiredEquipCost > SlotCapacity)
		{
			OutMessage = FString::Printf(
				TEXT("Equip failed: Item '%s' requires slot cost %d but slot '%s' capacity is %d."),
				*Definition->GetName(),
				RequiredEquipCost,
				*SlotTag.ToString(),
				SlotCapacity);
			return false;
		}
	}
	else if (RequiredEquipCost > 1)
	{
		OutMessage = FString::Printf(
			TEXT("Equip failed: Item '%s' requires slot cost %d but slot '%s' has no capacity definition. Add SlotDefinitions to equipment schema/component."),
			*Definition->GetName(),
			RequiredEquipCost,
			*SlotTag.ToString());
		return false;
	}

	TArray<FGameplayTag> TargetSlots;
	TargetSlots.Add(SlotTag);

	for (const FGameplayTag& OccupiedSlot : TargetSlots)
	{
		if (!IsAllowedSlot(OccupiedSlot))
		{
			OutMessage = FString::Printf(TEXT("Equip failed: Occupied slot '%s' is not allowed."), *OccupiedSlot.ToString());
			return false;
		}
		if (!IsSlotUnlocked(OccupiedSlot))
		{
			OutMessage = FString::Printf(TEXT("Equip failed: Occupied slot '%s' is locked."), *OccupiedSlot.ToString());
			return false;
		}
		if (!DoesDefinitionPassSlotFilter(Definition, OccupiedSlot))
		{
			OutMessage = FString::Printf(TEXT("Equip failed: Item '%s' does not satisfy slot '%s' tag filter."), *Definition->GetName(), *OccupiedSlot.ToString());
			return false;
		}
	}
	const bool bKeepInInventoryLocked = (EquipInventoryBehavior == EYIEquipInventoryBehavior::KeepInInventoryLocked);
	if (bKeepInInventoryLocked)
	{
		SourceBag->EnsureBagId();
		if (!SourceInventory->SetBagItemLocked(SourceBag, SourceIndex, true))
		{
			OutMessage = TEXT("Equip failed: Could not lock source inventory item.");
			return false;
		}
		SourceBagItem = SourceBag->Items[SourceIndex];
	}
	else if (!SourceInventory->RemoveItem(SourceIndex))
	{
		OutMessage = TEXT("Equip failed: Could not remove source item from bag.");
		return false;
	}

	TSet<int32> ReplaceGroupIds;
	for (const FGameplayTag& OccupiedSlot : TargetSlots)
	{
		const int32 ExistingIndex = FindEntryIndex(OccupiedSlot);
		if (ExistingIndex != INDEX_NONE)
		{
			ReplaceGroupIds.Add(GetEntryGroupIdForIndex(ExistingIndex));
		}
	}

	TArray<FYIBagItem> ReturnItems;
	TMap<int32, int32> GroupFirstEntryIndex;
	TArray<int32> EntryIndicesToRemove;
	for (int32 EntryIndex = 0; EntryIndex < EquippedItems.Num(); ++EntryIndex)
	{
		const int32 GroupId = GetEntryGroupIdForIndex(EntryIndex);
		if (!ReplaceGroupIds.Contains(GroupId))
		{
			continue;
		}

		EntryIndicesToRemove.Add(EntryIndex);
		if (!GroupFirstEntryIndex.Contains(GroupId))
		{
			GroupFirstEntryIndex.Add(GroupId, EntryIndex);
		}
	}

	for (const TPair<int32, int32>& Pair : GroupFirstEntryIndex)
	{
		const FYIEquippedItemEntry* GroupEntry = EquippedItems.IsValidIndex(Pair.Value) ? &EquippedItems[Pair.Value] : nullptr;
		if (!GroupEntry)
		{
			continue;
		}

		if (GroupEntry->bInventoryLocked && GroupEntry->SourceBagId.IsValid()
			&& (GroupEntry->SourceInstanceId.IsValid() || GroupEntry->SourceCustomStackKey != 0))
		{
			SourceInventory->SetBagItemLockedByInstanceRef(
				GroupEntry->SourceBagId,
				GroupEntry->SourceInstanceId,
				GroupEntry->SourceCustomStackKey,
				GroupEntry->SourceItemCode,
				false);
		}

		if (!bKeepInInventoryLocked && !GroupEntry->bInventoryLocked)
		{
			FYIBagItem ReturnItem;
			ReturnItem.Item = YIEquipmentPrivate::NetToFull(GroupEntry->Item);
			if (UYIItemDefinition* ExistingDef = ReturnItem.Item.Definition.IsValid()
				? ReturnItem.Item.Definition.Get()
				: ReturnItem.Item.Definition.LoadSynchronous())
			{
				const FIntPoint BaseSize = ExistingDef->DefaultSize;
				ReturnItem.Size = ReturnItem.Item.bRotated ? FIntPoint(BaseSize.Y, BaseSize.X) : BaseSize;
			}
			else
			{
				ReturnItem.Size = FIntPoint(1, 1);
			}
			ReturnItems.Add(ReturnItem);
		}
	}

	const TArray<FYIEquippedItemEntry> BackupEntries = EquippedItems;
	EntryIndicesToRemove.Sort([](int32 A, int32 B) { return A > B; });
	for (const int32 RemoveIdx : EntryIndicesToRemove)
	{
		if (EquippedItems.IsValidIndex(RemoveIdx))
		{
			EquippedItems.RemoveAt(RemoveIdx);
		}
	}

	TArray<int32> AddedReturnIndices;
	for (FYIBagItem& ReturnItem : ReturnItems)
	{
		const bool bSavedAutoMerge = SourceBag->bAutoMergeOnAdd;
		SourceBag->bAutoMergeOnAdd = false;
		const int32 AddedIdx = SourceBag->AddBagItem(ReturnItem);
		SourceBag->bAutoMergeOnAdd = bSavedAutoMerge;

		if (AddedIdx == INDEX_NONE)
		{
			for (int32 i = AddedReturnIndices.Num() - 1; i >= 0; --i)
			{
				SourceBag->RemoveItem(AddedReturnIndices[i]);
			}
			EquippedItems = BackupEntries;
			const int32 RollbackIndex = SourceBag->AddBagItem(SourceBagItem);
			if (RollbackIndex == INDEX_NONE)
			{
				EmitEquipmentMessage(TEXT("Equip rollback failed: source item could not be restored to bag."), FColor::Red);
			}
			OutMessage = TEXT("Equip failed: Could not move replaced equipped item(s) back to bag.");
			return false;
		}

		AddedReturnIndices.Add(AddedIdx);
	}

	const FYIItemInstanceNet SourceNet = YIEquipmentPrivate::FullToNet(SourceBagItem.Item);
	const int32 NewGroupId = ResolveOrCreateEquipGroupId();
	for (const FGameplayTag& OccupiedSlot : TargetSlots)
	{
		FYIEquippedItemEntry NewEntry;
		NewEntry.SlotTag = OccupiedSlot;
		NewEntry.Item = SourceNet;
		NewEntry.EquipGroupId = NewGroupId;
		NewEntry.bInventoryLocked = bKeepInInventoryLocked;
		if (bKeepInInventoryLocked)
		{
			NewEntry.SourceBagId = SourceBag->BagId;
			NewEntry.SourceInstanceId = SourceBagItem.Item.InstanceId;
			NewEntry.SourceCustomStackKey = SourceBagItem.Item.CustomStackKey;
			NewEntry.SourceItemCode = Definition->UniqueCode;
		}
		EquippedItems.Add(NewEntry);
		OnEquipmentChanged.Broadcast(OccupiedSlot, SourceNet);
	}

	HandleItemEquippedFeedback(SlotTag, SourceNet, Definition);
	if (GetOwner() && GetOwner()->HasAuthority() && !GetOwner()->HasLocalNetOwner())
	{
		ClientNotifyItemEquipped(SlotTag, SourceNet);
	}

	OutMessage = FString::Printf(TEXT("Equipped '%s' into slot '%s' (cost %d)."), *Definition->GetName(), *SlotTag.ToString(), RequiredEquipCost);
	return true;
}

bool UYIEquipmentComponent::UnequipToInventoryInternal(UYIInventoryComponent* DestInventory, FGameplayTag SlotTag, FString& OutMessage, UYIInventoryBag** OutBag, int32* OutItemIndex)
{
	if (OutBag)
	{
		*OutBag = nullptr;
	}
	if (OutItemIndex)
	{
		*OutItemIndex = INDEX_NONE;
	}

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
	const FYIItemInstanceNet UnequippedItem = EquippedItems[EntryIndex].Item;
	const bool bInventoryLockedGroup = EquippedItems[EntryIndex].bInventoryLocked;
	int32 AddedItemIndex = INDEX_NONE;

	UYIInventoryBag* DestBag = nullptr;
	if (!bInventoryLockedGroup)
	{
		DestBag = DestInventory->EquippedBag;
		if (!DestBag)
		{
			DestBag = DestInventory->GetBag();
			if (DestBag)
			{
				DestInventory->OpenBag(DestBag);
			}
		}
		if (!DestBag)
		{
			OutMessage = TEXT("Unequip failed: Destination inventory has no active bag.");
			return false;
		}

		FYIBagItem ToInventory;
		ToInventory.Item = YIEquipmentPrivate::NetToFull(UnequippedItem);
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

		AddedItemIndex = DestBag->AddBagItem(ToInventory);
		if (AddedItemIndex == INDEX_NONE)
		{
			OutMessage = TEXT("Unequip failed: Destination bag has no room for the item.");
			return false;
		}
	}

	const int32 UnequipGroupId = GetEntryGroupIdForIndex(EntryIndex);

	TArray<FGameplayTag> RemovedSlots;
	TArray<FYIEquippedItemEntry> GroupEntries;
	for (int32 i = EquippedItems.Num() - 1; i >= 0; --i)
	{
		if (GetEntryGroupIdForIndex(i) == UnequipGroupId)
		{
			GroupEntries.Add(EquippedItems[i]);
			RemovedSlots.Add(EquippedItems[i].SlotTag);
			EquippedItems.RemoveAt(i);
		}
	}

	for (const FYIEquippedItemEntry& GroupEntry : GroupEntries)
	{
		if (GroupEntry.bInventoryLocked && GroupEntry.SourceBagId.IsValid()
			&& (GroupEntry.SourceInstanceId.IsValid() || GroupEntry.SourceCustomStackKey != 0))
		{
			DestInventory->SetBagItemLockedByInstanceRef(
				GroupEntry.SourceBagId,
				GroupEntry.SourceInstanceId,
				GroupEntry.SourceCustomStackKey,
				GroupEntry.SourceItemCode,
				false);
		}
	}

	if (bInventoryLockedGroup)
	{
		// Item stayed in inventory and was unlocked; resolve its current runtime location for deterministic drag-start.
		for (const FYIEquippedItemEntry& GroupEntry : GroupEntries)
		{
			if (!GroupEntry.SourceBagId.IsValid() || (!GroupEntry.SourceInstanceId.IsValid() && GroupEntry.SourceCustomStackKey == 0))
			{
				continue;
			}

			UYIInventoryBag* SourceBag = DestInventory->GetBagById(GroupEntry.SourceBagId);
			if (!SourceBag)
			{
				continue;
			}

			const int32 SourceIdx = SourceBag->Items.IndexOfByPredicate([&GroupEntry](const FYIBagItem& Item)
			{
				if (GroupEntry.SourceInstanceId.IsValid() && Item.Item.InstanceId.IsValid())
				{
					return Item.Item.InstanceId == GroupEntry.SourceInstanceId;
				}
				if (GroupEntry.SourceCustomStackKey == 0 || Item.Item.CustomStackKey != GroupEntry.SourceCustomStackKey)
				{
					return false;
				}
				if (GroupEntry.SourceItemCode == 0)
				{
					return true;
				}
				if (UYIItemDefinition* Def = Item.Item.Definition.IsValid()
					? Item.Item.Definition.Get()
					: Item.Item.Definition.LoadSynchronous())
				{
					return Def->UniqueCode == GroupEntry.SourceItemCode;
				}
				return false;
			});

			if (SourceIdx != INDEX_NONE)
			{
				DestBag = SourceBag;
				AddedItemIndex = SourceIdx;
				break;
			}
		}
	}

	if (RemovedSlots.Num() == 0)
	{
		RemovedSlots.Add(SlotTag);
	}
	for (const FGameplayTag& RemovedSlot : RemovedSlots)
	{
		OnEquipmentChanged.Broadcast(RemovedSlot, UnequippedItem);
	}

	if (bInventoryLockedGroup)
	{
		OutMessage = FString::Printf(TEXT("Unequipped %d locked slot(s). Item remained in inventory and was unlocked."), RemovedSlots.Num());
	}
	else
	{
		OutMessage = FString::Printf(TEXT("Unequipped %d slot(s) to bag '%s'."), RemovedSlots.Num(), *DestBag->GetName());
	}

	if (OutBag)
	{
		*OutBag = DestBag;
	}
	if (OutItemIndex)
	{
		*OutItemIndex = AddedItemIndex;
	}
	return true;
}
