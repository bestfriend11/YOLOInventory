#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "YIItemPickup.h"
#include "YIEquipmentComponent.generated.h"

class UYIInventoryComponent;
class UYIItemDefinition;
class UYIEquipmentLayoutAsset;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	FYIItemInstanceNet Item;

	/** Same value across all slot entries occupied by one equipped item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	int32 EquipGroupId = 0;

	/** True when equipped entry mirrors an item that remains in inventory and is locked. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	bool bInventoryLocked = false;

	/** Source bag id used when bInventoryLocked is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	FGuid SourceBagId;

	/** Source item identity used when bInventoryLocked is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	int64 SourceCustomStackKey = 0;

	/** Source item code fallback for lock resolution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	int64 SourceItemCode = 0;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIEquipmentSlotDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	FGameplayTag SlotTag;

	/** Optional item type/tag filter for this slot. Empty means no extra filter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	FGameplayTagContainer AcceptedItemTags;

	/** Maximum equip cost this slot can hold (for example Chest slot capacity 6). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment", meta=(ClampMin="1"))
	int32 Capacity = 1;

	/** If false, slot cannot be used until unlocked at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Rules")
	FGameplayTagContainer AllowedEquipSlots;

	/** Controls whether equipped items are removed from bag or kept there as locked references. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Rules")
	EYIEquipInventoryBehavior EquipInventoryBehavior = EYIEquipInventoryBehavior::MoveToEquipment;

	/** Auto-resolve item equip slot from item tags that start with this prefix. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Rules")
	FString EquipSlotTagPrefix = TEXT("Equip.Slot.");

	/** Designer-authored slot table. If provided, this becomes the primary slot source. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category="Equipment|Rules")
	TArray<FYIEquipmentSlotDefinition> SlotDefinitions;

	/** Optional schema asset shared across pawns. Layout stays in UMG; this asset defines slot rules. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Schema")
	TSoftObjectPtr<UYIEquipmentSchemaAsset> EquipmentSchemaAsset;

	/** If true, apply schema on BeginPlay when local fields are empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Schema")
	bool bApplySchemaOnBeginPlay = true;

	/** Optional default runtime panel layout used by UInventoryScreenWidget auto wiring. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|UI")
	TSoftObjectPtr<UYIEquipmentLayoutAsset> DefaultEquipmentLayoutAsset;

	/** Prints equipment operation messages on screen/log. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Debug")
	bool bDebugEquipment = true;

	/** Keep debug messages pinned long enough for debugging sessions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Debug")
	bool bPinDebugMessages = true;

	/** If true, plays item-based equip SFX when an equip succeeds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment|Audio")
	bool bPlayEquipSound = true;

	/** Replicated equipped entries keyed by SlotTag. */
	UPROPERTY(ReplicatedUsing=OnRep_EquippedItems, BlueprintReadOnly, Category="Equipment")
	TArray<FYIEquippedItemEntry> EquippedItems;

	UPROPERTY(BlueprintAssignable, Category="Equipment|Events")
	FYIEquipmentChangedEvent OnEquipmentChanged;

	UPROPERTY(BlueprintAssignable, Category="Equipment|Events")
	FYIEquipmentResultEvent OnEquipmentOperationResult;

	/** Fired when an equip succeeds (server and owning client). */
	UPROPERTY(BlueprintAssignable, Category="Equipment|Events")
	FYIItemEquippedEvent OnItemEquipped;

	/** Equip item from inventory's active bag index into the requested slot (or auto slot if empty). */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Equipment|Net")
	bool EquipFromInventory(UYIInventoryComponent* SourceInventory, int32 SourceIndex, FGameplayTag RequestedSlotTag);

	UFUNCTION(Server, Reliable)
	void ServerEquipFromInventory(UYIInventoryComponent* SourceInventory, int32 SourceIndex, FGameplayTag RequestedSlotTag);

	/** Unequip slot back into destination inventory active bag. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Equipment|Net")
	bool UnequipToInventory(UYIInventoryComponent* DestInventory, FGameplayTag SlotTag);

	UFUNCTION(Server, Reliable)
	void ServerUnequipToInventory(UYIInventoryComponent* DestInventory, FGameplayTag SlotTag);

	UFUNCTION(Client, Unreliable)
	void ClientNotifyItemEquipped(FGameplayTag SlotTag, FYIItemInstanceNet Item);

	/** Get equipped item for a slot. */
	UFUNCTION(BlueprintPure, Category="YOLOInventory|Equipment")
	bool GetEquippedItem(FGameplayTag SlotTag, FYIItemInstanceNet& OutItem) const;

	/** Persistence helper: snapshot all equipped entries. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Equipment")
	void GetPersistedEquipment(TArray<FYIEquippedItemEntry>& OutEntries) const;

	/** Persistence helper: replace equipped entries on authority and broadcast changes. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Equipment", BlueprintAuthorityOnly)
	void LoadPersistedEquipment(const TArray<FYIEquippedItemEntry>& InEntries);

	/** Validate equipped entries and slot configuration. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Equipment|Diagnostics")
	bool ValidateEquipmentSetup(TArray<FString>& OutBlockingIssues, TArray<FString>& OutWarnings) const;

	UFUNCTION(BlueprintPure, Category="YOLOInventory|Equipment")
	bool IsSlotUnlocked(FGameplayTag SlotTag) const;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Equipment|Rules")
	bool SetSlotUnlocked(FGameplayTag SlotTag, bool bUnlocked);

	/** Applies schema rules to this component (server authority). */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Equipment|Schema", BlueprintAuthorityOnly)
	bool ApplyEquipmentSchema(bool bOverwriteExisting = false);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_EquippedItems();

private:
	bool EquipFromInventoryInternal(UYIInventoryComponent* SourceInventory, int32 SourceIndex, FGameplayTag RequestedSlotTag, FString& OutMessage);
	bool UnequipToInventoryInternal(UYIInventoryComponent* DestInventory, FGameplayTag SlotTag, FString& OutMessage);

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
