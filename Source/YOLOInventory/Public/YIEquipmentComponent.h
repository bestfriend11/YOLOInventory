#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "YIItemPickup.h"
#include "YIEquipmentComponent.generated.h"

class UYIInventoryComponent;
class UYIInventoryBag;
class UYIItemDefinition;
class UYIEquipmentSchemaAsset;
class USoundBase;

UENUM(BlueprintType)
enum class EYIEquipInventoryBehavior : uint8
{
	/** Move equipped item out of inventory and store it only in equipment slots. */
	MoveToEquipment UMETA(DisplayName = "Move Item To Equipment"),

	/** Keep item in inventory, lock it there, and mirror it in equipment slots until unequipped. */
	KeepInInventoryLocked UMETA(DisplayName = "Keep Item In Inventory (Locked)")
};

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIEquippedItemEntry
{
	GENERATED_BODY()

	/** Replicated slot id where the item is equipped (for example Equip.Slot.Chest). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment", meta=(ToolTip="Replicated slot tag occupied by this equipped entry (example: Equip.Slot.Chest)."))
	FGameplayTag SlotTag;

	/** Replicated lightweight item payload used for equipment state sync. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment", meta=(ToolTip="Replicated equipped item payload. Contains definition, affixes and serialized attributes."))
	FYIItemInstanceNet Item;

	/** Same value across all slot entries occupied by one equipped item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment", meta=(ToolTip="Replicated group id shared by all slots occupied by one equipped item (multi-slot equips)."))
	int32 EquipGroupId = 0;

	/** True when equipped entry mirrors an item that remains in inventory and is locked. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment", meta=(ToolTip="Replicated. True when equip mode keeps the item in inventory as locked and mirrors it in equipment."))
	bool bInventoryLocked = false;

	/** Source bag id used when bInventoryLocked is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment", meta=(ToolTip="Replicated source bag id used to unlock/reconcile the source item when unequipping locked inventory entries."))
	FGuid SourceBagId;

	/** Source item identity used when bInventoryLocked is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment", meta=(ToolTip="Replicated source item key used to find the locked item inside source bag."))
	int64 SourceCustomStackKey = 0;

	/** Source item code fallback for lock resolution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment", meta=(ToolTip="Replicated fallback item code used for source item lock resolution."))
	int64 SourceItemCode = 0;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIEquipmentSlotDefinition
{
	GENERATED_BODY()

	/** Slot tag designers use to target this equipment slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment", meta=(ToolTip="Replicated slot tag definition (example: Equip.Slot.WeaponMain)."))
	FGameplayTag SlotTag;

	/** Optional item type/tag filter for this slot. Empty means no extra filter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment", meta=(ToolTip="Replicated slot filter. Equipped item must match at least one tag when this list is not empty."))
	FGameplayTagContainer AcceptedItemTags;

	/** Maximum equip cost this slot can hold (for example Chest slot capacity 6). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment", meta=(ClampMin="1", ToolTip="Replicated slot capacity. ItemDefinition.EquipSlotCost must be <= this value."))
	int32 Capacity = 1;

	/** If false, slot cannot be used until unlocked at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment", meta=(ToolTip="Replicated unlock flag. Locked slots reject equip attempts until SetSlotUnlocked(true)."))
	bool bUnlocked = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FYIEquipmentChangedEvent, FGameplayTag, SlotTag, FYIItemInstanceNet, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FYIEquipmentResultEvent, bool, bSuccess, FGameplayTag, SlotTag, FString, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FYIItemEquippedEvent, FGameplayTag, SlotTag, FYIItemInstanceNet, Item, UYIItemDefinition*, ItemDefinition);

/**
 * Server-authoritative equipment container keyed by slot gameplay tags.
 * Designed to keep equip/unequip flow deterministic and simple for designers.
 */
UCLASS(ClassGroup=(Inventory), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class YOLOINVENTORY_API UYIEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UYIEquipmentComponent();

	/** Optional allow-list of equip slot tags (e.g. Equip.Slot.WeaponMain, Equip.Slot.Spellbook.Primary). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Rules", meta=(ToolTip="Server authoritative rule. If empty, slot validation falls back to SlotDefinitions or project settings prefix."))
	FGameplayTagContainer AllowedEquipSlots;

	/** Controls whether equipped items are removed from bag or kept there as locked references. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Rules", meta=(ToolTip="Server authoritative equip behavior.\nMoveToEquipment: remove item from bag while equipped.\nKeepInInventoryLocked: keep item in bag and lock it until unequipped."))
	EYIEquipInventoryBehavior EquipInventoryBehavior = EYIEquipInventoryBehavior::MoveToEquipment;

	/** Auto-resolve item equip slot from item tags that start with this prefix. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Rules", meta=(ToolTip="Server authoritative prefix used when RequestedSlotTag is empty. Example: Equip.Slot."))
	FString EquipSlotTagPrefix = TEXT("Equip.Slot.");

	/** Designer-authored slot table. If provided, this becomes the primary slot source. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="Equipment|Rules", meta=(ToolTip="Replicated slot definitions used for runtime validation, filtering, capacity and lock state."))
	TArray<FYIEquipmentSlotDefinition> SlotDefinitions;

	/** Optional schema asset shared across pawns. Layout stays in UMG; this asset defines slot rules. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Schema", meta=(ToolTip="Optional shared schema asset. Applied on server via ApplyEquipmentSchema or BeginPlay auto-apply."))
	TSoftObjectPtr<UYIEquipmentSchemaAsset> EquipmentSchemaAsset;

	/** If true, apply schema on BeginPlay when local fields are empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Schema", meta=(ToolTip="Server-only behavior. On BeginPlay, applies EquipmentSchemaAsset when local definitions are empty."))
	bool bApplySchemaOnBeginPlay = true;

	/** Prints equipment operation messages on screen/log. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Debug", meta=(ToolTip="If true, equipment operations write debug messages to log and on-screen debug text."))
	bool bDebugEquipment = true;

	/** Keep debug messages pinned long enough for debugging sessions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Debug", meta=(ToolTip="If true, debug messages stay longer on screen."))
	bool bPinDebugMessages = true;

	/** If true, plays item-based equip SFX when an equip succeeds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Audio", meta=(ToolTip="When true, successful equip plays sound resolved from InventoryComponent.ItemSFXLibrary (Equip event, fallback Drop)."))
	bool bPlayEquipSound = true;

	/** Replicated equipped entries keyed by SlotTag. */
	UPROPERTY(ReplicatedUsing=OnRep_EquippedItems, BlueprintReadOnly, Category="Equipment", meta=(ToolTip="Replicated equipped state. Use OnEquipmentChanged for per-slot UI refresh when this changes."))
	TArray<FYIEquippedItemEntry> EquippedItems;

	/** Replication-friendly state event. Fires on server during changes and on clients when EquippedItems replicates. */
	UPROPERTY(BlueprintAssignable, Category="Equipment|Events", meta=(ToolTip="State event. Fired for each affected slot when equip/unequip changes equipment state.\nServer: immediate on operation.\nClients: from OnRep after replication."))
	FYIEquipmentChangedEvent OnEquipmentChanged;

	/** Operation result event (success/failure + message). Authority-side feedback. */
	UPROPERTY(BlueprintAssignable, Category="Equipment|Events", meta=(ToolTip="Operation feedback event. Fired when EquipFromInventory/UnequipToInventory finishes.\nContains success flag, requested slot and readable message.\nAuthority-driven feedback."))
	FYIEquipmentResultEvent OnEquipmentOperationResult;

	/** Fired when an equip succeeds (server and owning client). */
	UPROPERTY(BlueprintAssignable, Category="Equipment|Events", meta=(ToolTip="Equip-success event for moment feedback (SFX/VFX/animations).\nFires on server and owning client via RPC.\nNot fired for unequip."))
	FYIItemEquippedEvent OnItemEquipped;

	/** Equip item from inventory's active bag index into the requested slot (or auto slot if empty). */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Equipment|Net", meta=(ToolTip="Equip item by source bag index.\nArgs:\n- SourceInventory: owner inventory component.\n- SourceIndex: index in SourceInventory active bag.\n- RequestedSlotTag: optional target slot; empty = auto resolve from item tags.\nNetwork:\n- Client call sends server RPC.\n- Server performs validation, mutates replicated state, and broadcasts events."))
	bool EquipFromInventory(UYIInventoryComponent* SourceInventory, int32 SourceIndex, FGameplayTag RequestedSlotTag);

	UFUNCTION(Server, Reliable)
	void ServerEquipFromInventory(UYIInventoryComponent* SourceInventory, int32 SourceIndex, FGameplayTag RequestedSlotTag);

	/** Unequip slot back into destination inventory active bag. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Equipment|Net", meta=(ToolTip="Unequip item from slot back into destination inventory active bag.\nArgs:\n- DestInventory: owner inventory component that receives item.\n- SlotTag: equipped slot to remove.\nNetwork:\n- Client call sends server RPC.\n- Server mutates replicated state and broadcasts events."))
	bool UnequipToInventory(UYIInventoryComponent* DestInventory, FGameplayTag SlotTag);
	/** Authority helper: unequip and return the exact bag/index that now contains the item. */
	bool UnequipToInventoryAndResolveItem(UYIInventoryComponent* DestInventory, FGameplayTag SlotTag, UYIInventoryBag*& OutBag, int32& OutItemIndex);

	UFUNCTION(Server, Reliable)
	void ServerUnequipToInventory(UYIInventoryComponent* DestInventory, FGameplayTag SlotTag);

	UFUNCTION(Client, Unreliable)
	void ClientNotifyItemEquipped(FGameplayTag SlotTag, FYIItemInstanceNet Item);

	/** Get equipped item for a slot. */
	UFUNCTION(BlueprintPure, Category="YOLOInventory|Equipment", meta=(ToolTip="Returns true when slot currently has equipped item.\nArgs:\n- SlotTag: queried slot.\n- OutItem: populated equipped item net payload when found."))
	bool GetEquippedItem(FGameplayTag SlotTag, FYIItemInstanceNet& OutItem) const;

	/** Persistence helper: snapshot all equipped entries. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Equipment", meta=(ToolTip="Copies current replicated EquippedItems into OutEntries for save/persistence pipelines."))
	void GetPersistedEquipment(TArray<FYIEquippedItemEntry>& OutEntries) const;

	/** Persistence helper: replace equipped entries on authority and broadcast changes. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Equipment", BlueprintAuthorityOnly, meta=(ToolTip="Authority-only load helper.\nReplaces replicated EquippedItems with InEntries and broadcasts OnEquipmentChanged for refresh."))
	void LoadPersistedEquipment(const TArray<FYIEquippedItemEntry>& InEntries);

	/** Validate equipped entries and slot configuration. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Equipment|Diagnostics", meta=(ToolTip="Runs setup validation.\nOutBlockingIssues: hard problems that break equip flow.\nOutWarnings: soft issues/mismatches.\nReturns true when no blocking issues were found."))
	bool ValidateEquipmentSetup(TArray<FString>& OutBlockingIssues, TArray<FString>& OutWarnings) const;

	UFUNCTION(BlueprintPure, Category="YOLOInventory|Equipment", meta=(ToolTip="Returns whether SlotDefinitions marks this slot as unlocked.\nIf slot has no definition, defaults to true."))
	bool IsSlotUnlocked(FGameplayTag SlotTag) const;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Equipment|Rules", meta=(ToolTip="Authority-only slot lock toggle.\nArgs:\n- SlotTag: slot to modify.\n- bUnlocked: true to allow equips, false to reject equips.\nReturns false if slot definition is missing or caller is not authority."))
	bool SetSlotUnlocked(FGameplayTag SlotTag, bool bUnlocked);

	/** Applies schema rules to this component (server authority). */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Equipment|Schema", BlueprintAuthorityOnly, meta=(ToolTip="Authority-only schema import.\nArgs:\n- bOverwriteExisting: true replaces existing local fields, false fills when empty.\nReturns true when component data changed."))
	bool ApplyEquipmentSchema(bool bOverwriteExisting = false);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_EquippedItems();

private:
	bool EquipFromInventoryInternal(UYIInventoryComponent* SourceInventory, int32 SourceIndex, FGameplayTag RequestedSlotTag, FString& OutMessage);
	bool UnequipToInventoryInternal(UYIInventoryComponent* DestInventory, FGameplayTag SlotTag, FString& OutMessage, UYIInventoryBag** OutBag = nullptr, int32* OutItemIndex = nullptr);

	int32 FindEntryIndex(FGameplayTag SlotTag) const;
	int32 FindSlotDefinitionIndex(FGameplayTag SlotTag) const;
	bool IsAllowedSlot(FGameplayTag SlotTag) const;
	FGameplayTag ResolveSlotTagFromDefinition(const UYIItemDefinition* Definition) const;
	bool DoesDefinitionSupportSlot(const UYIItemDefinition* Definition, FGameplayTag SlotTag) const;
	bool DoesDefinitionPassSlotFilter(const UYIItemDefinition* Definition, FGameplayTag SlotTag) const;
	int32 ResolveOrCreateEquipGroupId();
	int32 GetEntryGroupIdForIndex(int32 EntryIndex) const;

	void EmitEquipmentMessage(const FString& Message, const FColor& Color) const;
	void BroadcastResult(bool bSuccess, FGameplayTag SlotTag, const FString& Message);
	void HandleItemEquippedFeedback(FGameplayTag SlotTag, const FYIItemInstanceNet& Item, UYIItemDefinition* Definition);
	USoundBase* ResolveEquipSound(UYIItemDefinition* Definition) const;
	void PlayEquipSound(UYIItemDefinition* Definition) const;

	int32 NextEquipGroupId = 1;
};
