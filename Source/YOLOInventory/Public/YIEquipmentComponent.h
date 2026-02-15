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

	/** If false, slot cannot be used until unlocked at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	bool bUnlocked = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FYIEquipmentChangedEvent, FGameplayTag, SlotTag, FYIItemInstanceNet, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FYIEquipmentResultEvent, bool, bSuccess, FGameplayTag, SlotTag, FString, Message);

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

	/** Replicated equipped entries keyed by SlotTag. */
	UPROPERTY(ReplicatedUsing=OnRep_EquippedItems, BlueprintReadOnly, Category="Equipment")
	TArray<FYIEquippedItemEntry> EquippedItems;

	UPROPERTY(BlueprintAssignable, Category="Equipment|Events")
	FYIEquipmentChangedEvent OnEquipmentChanged;

	UPROPERTY(BlueprintAssignable, Category="Equipment|Events")
	FYIEquipmentResultEvent OnEquipmentOperationResult;

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

	int32 NextEquipGroupId = 1;
};
