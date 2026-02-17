#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YIInventoryBag.h"
#include "UObject/SoftObjectPtr.h"
#include "YIInventoryComponent.generated.h"

class UYIInventoryBag;
class UInventoryScreenWidget;
class UTradingScreenWidget;
class UShopScreenWidget;
class AYITradeSessionActor;
class UYIShopComponent;

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYINetBagDescriptor
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid BagId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGameplayTag BagRoleTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FIntPoint GridSize = FIntPoint(0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 ItemCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid ParentBagId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid ParentItemInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bIsNestedContainer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bIsActive = false;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYILockedBagItemRef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid BagId;

	/** Primary runtime identity for lock tracking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid ItemInstanceId;

	/** Legacy fallback identity (content hash). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int64 CustomStackKey = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int64 Code = 0;
};

UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class YOLOINVENTORY_API UYIInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UYIInventoryComponent();

	// Currently equipped (open) bag
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory", meta=(ToolTip="Runtime active bag for this inventory component.\nServer is authoritative; owning client receives mirrored active context."))
	TObjectPtr<UYIInventoryBag> EquippedBag;

	// All bags owned by this component
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory", meta=(ToolTip="Runtime bags owned by this component. Mutate on server for authoritative gameplay state."))
	TArray<TObjectPtr<UYIInventoryBag>> Bags;

	// Events
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBagOpened, UYIInventoryBag*, Bag);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBagClosed, UYIInventoryBag*, Bag);

	UPROPERTY(BlueprintAssignable, Category="Inventory", meta=(ToolTip="Fires when a bag becomes active/open on this instance."))
	FOnBagOpened OnBagOpened;

	UPROPERTY(BlueprintAssignable, Category="Inventory", meta=(ToolTip="Fires when an active bag is closed on this instance."))
	FOnBagClosed OnBagClosed;

	// Create a new bag (owned by this component)
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Create a runtime bag owned by this component.\nArgs:\n- BagName: display/name identifier.\n- GridSize: columns/rows.\nReturns created bag or null."))
	UYIInventoryBag* CreateBag(FName BagName, FIntPoint GridSize);

	// Open/close bag (fire events)
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Set active bag context and broadcast OnBagOpened."))
	void OpenBag(UYIInventoryBag* Bag);

	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Close active bag context and broadcast OnBagClosed for Bag."))
	void CloseBag(UYIInventoryBag* Bag);

	// Quick accessors
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Get currently active bag pointer for this instance."))
	UYIInventoryBag* GetBag() const;

	UFUNCTION(BlueprintPure, Category="Inventory", meta=(ToolTip="Find bag by BagId. Returns null when missing."))
	UYIInventoryBag* GetBagById(const FGuid& BagId) const;

	UFUNCTION(BlueprintPure, Category="Inventory", meta=(ToolTip="Find first bag by role tag."))
	UYIInventoryBag* GetBagByRoleTag(FGameplayTag BagRoleTag) const;

	UFUNCTION(BlueprintPure, Category="Inventory", meta=(ToolTip="Find bag by display/name id."))
	UYIInventoryBag* GetBagByDisplayName(FName BagName) const;

	UFUNCTION(BlueprintPure, Category="Inventory", meta=(ToolTip="Replicated active bag id for owner UI context wiring."))
	FGuid GetActiveBagId() const { return ActiveBagId; }

	UFUNCTION(BlueprintPure, Category="Inventory", meta=(ToolTip="Replicated active spellbook bag id for owner UI context wiring."))
	FGuid GetActiveSpellbookBagId() const { return ActiveSpellbookBagId; }

	UFUNCTION(BlueprintPure, Category="Inventory", meta=(ToolTip="Resolve active spellbook bag pointer from replicated spellbook context."))
	UYIInventoryBag* GetActiveSpellbookBag() const;

	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Set active bag by BagId. Returns true on success."))
	bool SetActiveBagById(const FGuid& InBagId);

	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Set active bag by role tag. Returns true on success."))
	bool SetActiveBagByRoleTag(FGameplayTag InBagRoleTag);

	/** Open nested container bag from item index in active bag. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Containers", meta=(ToolTip="Open nested container bag from item in current active bag. Server-authoritative in multiplayer."))
	bool OpenContainedBagAtIndex(int32 ItemIndex);

	/** Navigate back to parent bag when active bag is nested. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Containers", meta=(ToolTip="Open parent bag for current nested active bag. Returns false if current bag has no parent."))
	bool OpenParentBag();

	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Set active spellbook bag by BagId. Returns true on success."))
	bool SetActiveSpellbookBagById(const FGuid& InBagId);

	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Set active spellbook bag by role tag. Returns true on success."))
	bool SetActiveSpellbookBagByRoleTag(FGameplayTag InBagRoleTag);

	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Copy replicated bag descriptors for owner UI listing."))
	void GetReplicatedBagDescriptors(TArray<FYINetBagDescriptor>& OutDescriptors) const;

	// Add an item to a bag; returns success
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Add item definition to target bag.\nServer-authoritative mutation helper."))
	bool AddItemToBag(UYIInventoryBag* Bag, TSoftObjectPtr<class UYIItemDefinition> ItemDef, int32 Count = 1);

	// Remove a bag owned by this component
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Remove bag from this component. Returns false if invalid/not owned/in use."))
	bool RemoveBag(UYIInventoryBag* Bag);

	/** Runtime lock used by keep-in-inventory equip mode. Locked items cannot be moved/rotated/removed. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Equipment", meta=(ToolTip="Lock/unlock item by bag+index.\nLocked items cannot be moved/rotated/removed."))
	bool SetBagItemLocked(UYIInventoryBag* Bag, int32 ItemIndex, bool bLocked);

	UFUNCTION(BlueprintCallable, Category="Inventory|Equipment", meta=(ToolTip="Lock/unlock item by identity reference (BagId + CustomStackKey + Code fallback). Prefer SetBagItemLockedByInstanceRef for deterministic identity."))
	bool SetBagItemLockedByRef(const FGuid& BagId, int64 CustomStackKey, int64 Code, bool bLocked);

	UFUNCTION(BlueprintCallable, Category="Inventory|Equipment", meta=(ToolTip="Lock/unlock item by deterministic runtime identity.\nArgs: BagId + ItemInstanceId (primary), with optional CustomStackKey/Code fallback for legacy items."))
	bool SetBagItemLockedByInstanceRef(const FGuid& BagId, const FGuid& ItemInstanceId, int64 CustomStackKey, int64 Code, bool bLocked);

	UFUNCTION(BlueprintPure, Category="Inventory|Equipment", meta=(ToolTip="Returns true when specified item is currently lock-protected."))
	bool IsBagItemLocked(UYIInventoryBag* Bag, int32 ItemIndex) const;

	/** Server-only: push current bag state into net mirror to replicate to owning client. */
	void SyncNetState();

	// --------- Authority-safe inventory mutations (RPC-backed) ----------
	UFUNCTION(BlueprintCallable, Category="Inventory|Net", meta=(ToolTip="Move item within active bag.\nClient calls forward to ServerMoveItem RPC."))
	bool MoveItem(int32 Index, FIntPoint NewPos);
	UFUNCTION(Server, Reliable)
	void ServerMoveItem(int32 Index, FIntPoint NewPos);

	UFUNCTION(BlueprintCallable, Category="Inventory|Net", meta=(ToolTip="Rotate item in active bag.\nClient calls forward to ServerRotateItem RPC."))
	bool RotateItem(int32 Index);
	UFUNCTION(Server, Reliable)
	void ServerRotateItem(int32 Index);

	/** Add an already-built bag item (e.g., from drag/drop). */
	UFUNCTION(BlueprintCallable, Category="Inventory|Net", meta=(ToolTip="Add an already-built bag item into active bag.\nClient calls forward to ServerAddBagItem RPC.\nReturns inserted index or INDEX_NONE."))
	int32 AddBagItem(const FYIBagItem& Item);
	UFUNCTION(Server, Reliable)
	void ServerAddBagItem(const struct FYIItemInstanceNet& NetItem, FIntPoint Pos, FIntPoint Size);

	UFUNCTION(BlueprintCallable, Category="Inventory|Net", meta=(ToolTip="Remove item by index from active bag.\nClient calls forward to ServerRemoveItem RPC."))
	bool RemoveItem(int32 Index);
	UFUNCTION(Server, Reliable)
	void ServerRemoveItem(int32 Index);

	/** Drop an item instance to the world (server authoritative). */
	UFUNCTION(BlueprintCallable, Category="Inventory|Net", meta=(ToolTip="Drop item payload to world on server.\nClient calls forward to ServerDropItemToWorld RPC."))
	bool DropItemToWorld(const struct FYIItemInstanceNet& NetItem, const FTransform& SpawnTransform);
	UFUNCTION(Server, Reliable)
	void ServerDropItemToWorld(const struct FYIItemInstanceNet& NetItem, const FTransform& SpawnTransform);

	UFUNCTION(Server, Reliable)
	void ServerSetActiveBagById(const FGuid& InBagId);

	UFUNCTION(Server, Reliable)
	void ServerSetActiveSpellbookBagById(const FGuid& InBagId);

	UFUNCTION(Server, Reliable)
	void ServerOpenContainedBagByInstance(const FGuid& ParentBagId, const FGuid& ParentItemInstanceId);

	UFUNCTION(Server, Reliable)
	void ServerOpenParentBag(const FGuid& ChildBagId);

	/** Soft class references so designers can assign widgets once and call the helpers below. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI", meta=(ToolTip="Inventory screen widget class used by OpenInventoryScreen on owning client."))
	TSoftClassPtr<UInventoryScreenWidget> InventoryScreenClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI", meta=(ToolTip="Trade screen widget class used by OpenTradeScreen on owning client."))
	TSoftClassPtr<UTradingScreenWidget> TradingScreenClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI", meta=(ToolTip="Shop screen widget class used by OpenShopScreen on owning client."))
	TSoftClassPtr<UShopScreenWidget> ShopScreenClass;

	/** Optional per-inventory SFX library for item-driven UI sounds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio", meta=(ToolTip="Optional per-inventory SFX library for item-driven UI sounds"))
	TSoftObjectPtr<class UYIItemSFXLibrary> ItemSFXLibrary;

	/** Master toggle for all inventory UI sounds (drag/hover/drop/etc). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio", meta=(ToolTip="Master toggle for inventory UI sounds"))
	bool bEnableInventorySounds = true;

	/** Debug: print on-screen messages for inventory actions (add/move/drop/transfer). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Debug", meta=(ToolTip="Enable runtime debug prints for inventory actions."))
	bool bDebugInventoryActions = false;

	// -------- Inventory action delegates (designer-friendly) ----------
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventoryItemAdded, UYIInventoryBag*, Bag, int32, Index, FYIBagItem, Item);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventoryItemRemoved, UYIInventoryBag*, Bag, int32, Index, FYIBagItem, Item);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventoryItemMoved, UYIInventoryBag*, Bag, int32, Index, FIntPoint, NewPos);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemRotated, UYIInventoryBag*, Bag, int32, Index);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnInventoryItemTransferred, UYIInventoryBag*, Source, UYIInventoryBag*, Dest, int32, SourceIndex, int32, DestIndex);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemDroppedToWorld, const FYIItemInstanceNet&, Item, const FTransform&, SpawnTransform);

	UPROPERTY(BlueprintAssignable, Category="Inventory|Events", meta=(ToolTip="Fires when item is added to bag on this instance."))
	FOnInventoryItemAdded OnInventoryItemAdded;
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events", meta=(ToolTip="Fires when item is removed from bag on this instance."))
	FOnInventoryItemRemoved OnInventoryItemRemoved;
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events", meta=(ToolTip="Fires when item position changes in bag on this instance."))
	FOnInventoryItemMoved OnInventoryItemMoved;
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events", meta=(ToolTip="Fires when item is rotated in bag on this instance."))
	FOnInventoryItemRotated OnInventoryItemRotated;
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events", meta=(ToolTip="Fires when item transfers between bags."))
	FOnInventoryItemTransferred OnInventoryItemTransferred;
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events", meta=(ToolTip="Fires when item payload is dropped/spawned to world."))
	FOnInventoryItemDroppedToWorld OnInventoryItemDroppedToWorld;

	/** Open the inventory screen for the owning local player. Creates if needed, sets the current bag, adds to viewport. */
	UFUNCTION(BlueprintCallable, Category="UI", meta=(ToolTip="Owning-client helper.\nOpen inventory screen and bind to current active bag context."))
	UInventoryScreenWidget* OpenInventoryScreen();

	/** Close and remove the inventory screen if it is open. */
	UFUNCTION(BlueprintCallable, Category="UI", meta=(ToolTip="Owning-client helper. Close inventory screen if open."))
	void CloseInventoryScreen();

	/** Open a trading screen for an existing session (client-side). LocalBag can be left null to auto-use GetBag(). */
	UFUNCTION(BlueprintCallable, Category="UI", meta=(ToolTip="Owning-client helper to open trade screen for Session.\nLocalBag optional; null auto-resolves from active bag."))
	UTradingScreenWidget* OpenTradeScreen(AYITradeSessionActor* Session, UYIInventoryBag* LocalBag = nullptr);

	/** Close the trading screen if it is open. */
	UFUNCTION(BlueprintCallable, Category="UI", meta=(ToolTip="Owning-client helper. Close trade screen if open."))
	void CloseTradeScreen();

	/** Open a shop screen for a shop component (client-side). */
	UFUNCTION(BlueprintCallable, Category="UI", meta=(ToolTip="Owning-client helper. Open shop UI using provided replicated stock snapshot."))
	UShopScreenWidget* OpenShopScreen(UYIShopComponent* Shop, UYIInventoryBag* LocalBag, const TArray<FYINetBagItem>& Stock, FIntPoint StockSize);

	/** Update the shop screen if it is already open (no-op if closed). */
	UFUNCTION(BlueprintCallable, Category="UI", meta=(ToolTip="Owning-client helper. Refresh currently open shop UI snapshot."))
	void UpdateShopScreen(UYIShopComponent* Shop, UYIInventoryBag* LocalBag, const TArray<FYINetBagItem>& Stock, FIntPoint StockSize);

	/** Close the shop screen if it is open. */
	UFUNCTION(BlueprintCallable, Category="UI", meta=(ToolTip="Owning-client helper. Close shop screen if open."))
	void CloseShopScreen();

	/** Close all inventory-related screens (inventory/trade/shop). */
	UFUNCTION(BlueprintCallable, Category="UI", meta=(ToolTip="Owning-client helper. Close all inventory/trade/shop screens."))
	void CloseAllScreens();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Owner-only replicated minimal bag state for UI; rebuilt into a transient bag on clients.
	UPROPERTY(ReplicatedUsing=OnRep_NetBag, Transient)
	TArray<FYINetBagItem> NetBagItems;
	UPROPERTY(Replicated, Transient)
	FIntPoint NetBagGridSize = FIntPoint(0, 0);
	UPROPERTY(ReplicatedUsing=OnRep_NetBagDescriptors, Transient)
	TArray<FYINetBagDescriptor> NetBagDescriptors;
	UPROPERTY(ReplicatedUsing=OnRep_ActiveBagContexts, Transient)
	FGuid ActiveBagId;
	UPROPERTY(ReplicatedUsing=OnRep_ActiveBagContexts, Transient)
	FGuid ActiveSpellbookBagId;

	UFUNCTION()
	void OnRep_NetBag();
	UFUNCTION()
	void OnRep_NetBagDescriptors();
	UFUNCTION()
	void OnRep_ActiveBagContexts();
	UFUNCTION()
	void OnRep_LockedBagItems();

	UPROPERTY(ReplicatedUsing=OnRep_LockedBagItems, Transient)
	TArray<FYILockedBagItemRef> LockedBagItems;

	/** Client-only preview bag built from NetBagItems (not authoritative). */
	UPROPERTY(Transient)
	TObjectPtr<UYIInventoryBag> ClientPreviewBag = nullptr;

private:
	FDelegateHandle BagChangedHandle;
	UYIInventoryBag* BagEventSource = nullptr;
	TWeakObjectPtr<UInventoryScreenWidget> ActiveInventoryScreen;
	TWeakObjectPtr<UTradingScreenWidget> ActiveTradeScreen;
	TWeakObjectPtr<UShopScreenWidget> ActiveShopScreen;

	// Cleanup delegate when component is destroyed
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	virtual void BeginPlay() override;

	/** Create a runtime bag instance from a template asset (layout + items). */
	UYIInventoryBag* CloneBagTemplate(const UYIInventoryBag* TemplateBag);

	/** Returns true if the bag is an asset/template (outer is package/public). */
	bool IsTemplateBag(const UYIInventoryBag* Bag) const;

	UFUNCTION()
	void HandleBagItemAdded(int32 Index, FYIBagItem Item);
	UFUNCTION()
	void HandleBagItemRemoved(int32 Index, FYIBagItem Item);
	UFUNCTION()
	void HandleBagItemMoved(int32 Index, FIntPoint NewPos);
	UFUNCTION()
	void HandleBagItemRotated(int32 Index);
	UFUNCTION()
	void HandleBagItemTransferred(UYIInventoryBag* Src, UYIInventoryBag* Dest, int32 SrcIdx, int32 DestIdx);

	bool GetBagItemIdentity(const UYIInventoryBag* Bag, int32 ItemIndex, FYILockedBagItemRef& OutIdentity) const;
	bool IsBagItemLockedByIdentity(const FYILockedBagItemRef& Identity) const;
	bool FindItemIndexByInstanceId(const UYIInventoryBag* Bag, const FGuid& InstanceId, int32& OutIndex) const;
	bool FindContainerParentForBag(const FGuid& ChildBagId, FGuid& OutParentBagId, FGuid& OutParentItemInstanceId) const;
	bool IsBagDescendantOf(const FGuid& CandidateBagId, const FGuid& PotentialAncestorBagId) const;
	UYIInventoryBag* EnsureContainedBagForItem(FYIBagItem& InOutItem, const UYIInventoryBag* ParentBag);
	bool TryOpenContainedBagInternal(UYIInventoryBag* ParentBag, int32 ItemIndex);
};
