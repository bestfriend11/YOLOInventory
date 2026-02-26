#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YIInventoryBag.h"
#include "YIInventoryApiTypes.h"
#include "YIInventoryCoreTypes.h"
#include "YIInventoryContextTypes.h"
#include "YIInventoryEventTypes.h"
#include "YIItemNetTypes.h"
#include "YIInventoryReplicationTypes.h"
#include "UObject/SoftObjectPtr.h"
#include "YIInventoryComponent.generated.h"

class UYIInventoryBag;
struct FYIInventoryContainerRuntimeService;
struct FYIInventoryBagContextService;
struct FYIInventoryBootstrapService;
struct FYIInventoryItemIngressService;
struct FYIInventoryIdentityLockService;
struct FYIInventoryEventBridgeService;
struct FYIInventoryMirrorService;
struct FYIInventoryMutationService;

UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class YOLOINVENTORYCONTAINERS_API UYIInventoryComponent : public UActorComponent
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

	UPROPERTY(BlueprintAssignable, Category="Inventory", meta=(ToolTip="Fires when a bag becomes active/open on this instance."))
	FYIOnInventoryBagOpened OnBagOpened;

	UPROPERTY(BlueprintAssignable, Category="Inventory", meta=(ToolTip="Fires when an active bag is closed on this instance."))
	FYIOnInventoryBagClosed OnBagClosed;

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

	UFUNCTION(BlueprintPure, Category="Inventory", meta=(ToolTip="Current runtime revision for a bag (authoritative on server, mirrored for client previews when available). Returns INDEX_NONE when bag is missing."))
	int32 GetBagRuntimeRevisionById(const FGuid& BagId) const;

	UFUNCTION(BlueprintPure, Category="Inventory|Context", meta=(ToolTip="Resolve owner-replicated active bag id for a semantic context tag (for example UI.Context.Secondary)."))
	FGuid GetActiveContextBagId(FGameplayTag ContextTag) const;

	UFUNCTION(BlueprintPure, Category="Inventory|Context", meta=(ToolTip="Resolve runtime bag pointer for a semantic context tag from the owner-replicated active context list."))
	UYIInventoryBag* GetActiveContextBag(FGameplayTag ContextTag) const;

	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Set active bag by BagId. Returns true on success."))
	bool SetActiveBagById(const FGuid& InBagId);

	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Set active bag by role tag. Returns true on success."))
	bool SetActiveBagByRoleTag(FGameplayTag InBagRoleTag);

	/** Open nested container bag from item index in active bag. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Containers", meta=(ToolTip="Open nested container bag from item in current active bag. Server-authoritative in multiplayer."))
	bool OpenContainedBagAtIndex(int32 ItemIndex);

	/** Ensure a runtime nested bag exists for the container item at index in the active bag and return it. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Containers", meta=(ToolTip="Materialize/resolve contained runtime bag for the item at ItemIndex in the current active bag.\nReturns null if item is not a container item or index is invalid.\nServer is authoritative in multiplayer; client calls forward to server only through higher-level flows."))
	UYIInventoryBag* EnsureContainedBagAtIndex(int32 ItemIndex);

	/** Navigate back to parent bag when active bag is nested. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Containers", meta=(ToolTip="Open parent bag for current nested active bag. Returns false if current bag has no parent."))
	bool OpenParentBag();

	UFUNCTION(BlueprintCallable, Category="Inventory|Context", meta=(ToolTip="Set active bag context by semantic context tag + BagId. Returns true on success."))
	bool SetActiveContextBagById(FGameplayTag ContextTag, const FGuid& InBagId);

	UFUNCTION(BlueprintCallable, Category="Inventory|Context", meta=(ToolTip="Set active bag context by semantic context tag + bag role tag. Returns true on success."))
	bool SetActiveContextBagByRoleTag(FGameplayTag ContextTag, FGameplayTag InBagRoleTag);

	UFUNCTION(BlueprintCallable, Category="Inventory|Context", meta=(ToolTip="Clear active bag for a semantic context tag. Returns true when a mapping was removed."))
	bool ClearActiveContextBag(FGameplayTag ContextTag);

	UFUNCTION(BlueprintCallable, Category="Inventory|Context", meta=(ToolTip="Clear all active context mappings that point to the specified bag id. Returns number of mappings removed."))
	int32 ClearActiveContextsForBagId(const FGuid& InBagId);

	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Copy replicated bag descriptors for owner UI listing."))
	void GetReplicatedBagDescriptors(TArray<FYINetBagDescriptor>& OutDescriptors) const;

	// Add an item to a bag; returns success
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Add item definition to target bag.\nServer-authoritative mutation helper."))
	bool AddItemToBag(UYIInventoryBag* Bag, TSoftObjectPtr<class UYIItemDefinition> ItemDef, int32 Count = 1);

	// Remove a bag owned by this component
	UFUNCTION(BlueprintCallable, Category="Inventory", meta=(ToolTip="Remove bag from this component. Returns false if invalid/not owned/in use."))
	bool RemoveBag(UYIInventoryBag* Bag);

	/** Runtime lock used by keep-in-inventory equip mode. Locked items cannot be moved/rotated/removed. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Equipment", meta=(DeprecatedFunction, DeprecationMessage="Use SetBagItemLockedByCoreRef (build ref with UYIInventoryCommandLibrary::BuildItemRef / BuildActiveBagItemRef / BuildContextBagItemRef).", ToolTip="Legacy convenience lock/unlock by bag+index.\nPrefer canonical item refs to avoid index drift bugs."))
	bool SetBagItemLocked(UYIInventoryBag* Bag, int32 ItemIndex, bool bLocked);

	UFUNCTION(BlueprintCallable, Category="Inventory|Equipment", meta=(ToolTip="Lock/unlock item by canonical core identity reference (Bag + Item handles)."))
	bool SetBagItemLockedByCoreRef(const FYIInventoryItemRef& ItemRef, bool bLocked);

	UFUNCTION(BlueprintPure, Category="Inventory|Equipment", meta=(ToolTip="Build canonical core item reference for a bag item index.\nReturns false when bag/index is invalid or identity cannot be resolved."))
	bool GetBagItemCoreRef(UYIInventoryBag* Bag, int32 ItemIndex, FYIInventoryItemRef& OutItemRef) const;

	UFUNCTION(BlueprintPure, Category="Inventory|Equipment", meta=(DeprecatedFunction, DeprecationMessage="Use IsBagItemLockedByCoreRef (build ref with UYIInventoryCommandLibrary helpers).", ToolTip="Legacy convenience lock query by bag+index.\nPrefer canonical item refs to avoid index drift bugs."))
	bool IsBagItemLocked(UYIInventoryBag* Bag, int32 ItemIndex) const;

	UFUNCTION(BlueprintPure, Category="Inventory|Equipment", meta=(ToolTip="Returns true when specified canonical item ref is currently lock-protected."))
	bool IsBagItemLockedByCoreRef(const FYIInventoryItemRef& ItemRef) const;

	/** Server-only: push current bag state into net mirror to replicate to owning client. */
	void SyncNetState();

	// --------- Authority-safe inventory mutations (RPC-backed) ----------
	UFUNCTION(BlueprintCallable, Category="Inventory|Net", meta=(DeprecatedFunction, DeprecationMessage="Use explicit bag-targeted APIs (MoveItemInBag/MoveItemInBagAtCell) or UYIInventoryCommandLibrary::MoveItemByRef.", ToolTip="Legacy convenience move in active bag by index.\nPrefer canonical item refs / bag-targeted commands to avoid index drift bugs."))
	bool MoveItem(int32 Index, FIntPoint NewPos);
	UFUNCTION(Server, Reliable)
	void ServerMoveItem(int32 Index, FIntPoint NewPos);

	/** Move a specific item in a specific bag (explicit target, safer for multi-grid/context UIs). */
	UFUNCTION(BlueprintCallable, Category="Inventory|Net", meta=(ToolTip="Move item by explicit bag + item instance identity.\nArgs:\n- BagId: target bag.\n- ItemInstanceId: runtime item identity.\n- NewPos: destination cell.\nClient calls forward to ServerMoveItemInBag RPC."))
	bool MoveItemInBag(const FGuid& BagId, const FGuid& ItemInstanceId, FIntPoint NewPos);
	UFUNCTION(Server, Reliable)
	void ServerMoveItemInBag(const FGuid& BagId, const FGuid& ItemInstanceId, FIntPoint NewPos);

	/** Move/swap an item within an explicit bag to an exact cell (optional single-overlap swap, server-authoritative). */
	UFUNCTION(BlueprintCallable, Category="Inventory|Net", meta=(ToolTip="Move item to an exact cell in an explicit bag.\nIf direct move fails and bAllowSingleOverlapSwap is true, server may perform an atomic single-overlap swap.\nClient calls forward to ServerMoveItemInBagAtCell RPC."))
	bool MoveItemInBagAtCell(const FGuid& BagId, const FGuid& ItemInstanceId, FIntPoint DestCell, bool bAllowSingleOverlapSwap = true);
	UFUNCTION(Server, Reliable)
	void ServerMoveItemInBagAtCell(const FGuid& BagId, const FGuid& ItemInstanceId, FIntPoint DestCell, bool bAllowSingleOverlapSwap);

	UFUNCTION(BlueprintCallable, Category="Inventory|Net", meta=(DeprecatedFunction, DeprecationMessage="Use RotateItemInBag or UYIInventoryCommandLibrary::RotateItemByRef.", ToolTip="Legacy convenience rotate in active bag by index.\nPrefer canonical item refs / bag-targeted commands to avoid index drift bugs."))
	bool RotateItem(int32 Index);
	UFUNCTION(Server, Reliable)
	void ServerRotateItem(int32 Index);

	/** Rotate a specific item in a specific bag (explicit target, safer for multi-grid/context UIs). */
	UFUNCTION(BlueprintCallable, Category="Inventory|Net", meta=(ToolTip="Rotate item by explicit bag + item instance identity.\nClient calls forward to ServerRotateItemInBag RPC."))
	bool RotateItemInBag(const FGuid& BagId, const FGuid& ItemInstanceId);
	UFUNCTION(Server, Reliable)
	void ServerRotateItemInBag(const FGuid& BagId, const FGuid& ItemInstanceId);

	/** Add an already-built bag item (e.g., from drag/drop). */
	UFUNCTION(BlueprintCallable, Category="Inventory|Net", meta=(DeprecatedFunction, DeprecationMessage="UI/internal convenience. Prefer AddItemToBag (definitions) or explicit transfer commands. For runtime item identity-safe flows, use bag-targeted commands.", ToolTip="Legacy convenience add of a prebuilt bag item into the active bag.\nPrimarily used by internal UI drag flows."))
	int32 AddBagItem(const FYIBagItem& Item);
	UFUNCTION(Server, Reliable)
	void ServerAddBagItem(const struct FYIItemInstanceNet& NetItem, FIntPoint Pos, FIntPoint Size);

	UFUNCTION(BlueprintCallable, Category="Inventory|Net", meta=(DeprecatedFunction, DeprecationMessage="Use RemoveItemFromBag or UYIInventoryCommandLibrary::RemoveItemByRef.", ToolTip="Legacy convenience remove from active bag by index.\nPrefer canonical item refs / bag-targeted commands to avoid index drift bugs."))
	bool RemoveItem(int32 Index);
	UFUNCTION(Server, Reliable)
	void ServerRemoveItem(int32 Index);

	/** Remove a specific item from a specific bag (explicit target, safer for multi-grid/context UIs). */
	UFUNCTION(BlueprintCallable, Category="Inventory|Net", meta=(ToolTip="Remove item by explicit bag + item instance identity.\nClient calls forward to ServerRemoveItemFromBag RPC."))
	bool RemoveItemFromBag(const FGuid& BagId, const FGuid& ItemInstanceId);
	UFUNCTION(Server, Reliable)
	void ServerRemoveItemFromBag(const FGuid& BagId, const FGuid& ItemInstanceId);

	/** Transfer an item between two explicit bags. Uses server-authoritative validation and stack/fit rules. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Net", meta=(ToolTip="Transfer item between explicit bags.\nArgs:\n- SourceBagId: source bag.\n- ItemInstanceId: runtime item identity in source bag.\n- DestBagId: destination bag.\n- Count: optional split count for stackable items (<=0 moves whole stack).\nReturns true when request is accepted (client optimistic) or succeeds on authority.\nOutDestIndex is only authoritative locally (client usually gets INDEX_NONE until replication updates)."))
	bool TransferItemBetweenBagsById(const FGuid& SourceBagId, const FGuid& ItemInstanceId, const FGuid& DestBagId, int32 Count, int32& OutDestIndex);
	UFUNCTION(Server, Reliable)
	void ServerTransferItemBetweenBagsById(const FGuid& SourceBagId, const FGuid& ItemInstanceId, const FGuid& DestBagId, int32 Count);

	/** Transfer an item into an explicit cell in another bag. Optional single-overlap swap keeps the operation atomic on the server. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Net", meta=(ToolTip="Transfer item between explicit bags to an exact destination cell.\nArgs:\n- SourceBagId: source bag.\n- ItemInstanceId: runtime item identity in source bag.\n- DestBagId: destination bag.\n- DestCell: exact target cell.\n- Count: optional split count for stackable items (<=0 moves whole stack).\n- bAllowSingleOverlapSwap: if true, server may perform an atomic single-item swap when destination cell is occupied by exactly one overlapping item.\nReturns true when request is accepted (client optimistic) or succeeds on authority."))
	bool TransferItemBetweenBagsAtCellById(const FGuid& SourceBagId, const FGuid& ItemInstanceId, const FGuid& DestBagId, FIntPoint DestCell, int32 Count, bool bAllowSingleOverlapSwap = false);
	UFUNCTION(Server, Reliable)
	void ServerTransferItemBetweenBagsAtCellById(const FGuid& SourceBagId, const FGuid& ItemInstanceId, const FGuid& DestBagId, FIntPoint DestCell, int32 Count, bool bAllowSingleOverlapSwap);

	/** Explicit swap-focused wrapper: same as TransferItemBetweenBagsAtCellById with single-overlap swap enabled. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Net", meta=(ToolTip="Swap-capable transfer into an exact destination cell using explicit bag + item identity. Server performs an atomic transfer-or-single-overlap-swap."))
	bool SwapItemIntoBagCellById(const FGuid& SourceBagId, const FGuid& ItemInstanceId, const FGuid& DestBagId, FIntPoint DestCell);
	UFUNCTION(Server, Reliable)
	void ServerSwapItemIntoBagCellById(const FGuid& SourceBagId, const FGuid& ItemInstanceId, const FGuid& DestBagId, FIntPoint DestCell);

	/** Combine a stack into another matching stack in the same explicit bag (server-authoritative). */
	UFUNCTION(BlueprintCallable, Category="Inventory|Net", meta=(ToolTip="Combine item stack into an existing matching stack in the same explicit bag.\nClient calls forward to ServerCombineItemInBag RPC."))
	bool CombineItemInBag(const FGuid& BagId, const FGuid& ItemInstanceId);
	UFUNCTION(Server, Reliable)
	void ServerCombineItemInBag(const FGuid& BagId, const FGuid& ItemInstanceId);

	/** Split a stack in an explicit bag. DesiredPos may be (-1,-1) to auto-place using first fit. */
	UFUNCTION(BlueprintCallable, Category="Inventory|Net", meta=(ToolTip="Split stack in explicit bag.\nArgs:\n- BagId: target bag.\n- ItemInstanceId: source stack identity.\n- Amount: quantity to split out.\n- DesiredPos: preferred cell, use (-1,-1) for auto-fit.\nClient calls forward to ServerSplitStackInBag RPC."))
	bool SplitStackInBag(const FGuid& BagId, const FGuid& ItemInstanceId, int32 Amount, FIntPoint DesiredPos);
	UFUNCTION(Server, Reliable)
	void ServerSplitStackInBag(const FGuid& BagId, const FGuid& ItemInstanceId, int32 Amount, FIntPoint DesiredPos);

	// --------- Standardized request/result API (suite-wide contract) ----------
	UFUNCTION(BlueprintCallable, Category="Inventory|Commands", meta=(ToolTip="Standard request API: move item using canonical item ref. Returns standardized request/result envelope."))
	FYIInventoryOpResult RequestMoveItem(const FYIInventoryMoveItemRequest& Request);

	UFUNCTION(BlueprintCallable, Category="Inventory|Commands", meta=(ToolTip="Standard request API: rotate item using canonical item ref. Returns standardized request/result envelope."))
	FYIInventoryOpResult RequestRotateItem(const FYIInventoryRotateItemRequest& Request);

	UFUNCTION(BlueprintCallable, Category="Inventory|Commands", meta=(ToolTip="Standard request API: remove item using canonical item ref. Returns standardized request/result envelope."))
	FYIInventoryOpResult RequestRemoveItem(const FYIInventoryRemoveItemRequest& Request);

	UFUNCTION(BlueprintCallable, Category="Inventory|Commands", meta=(ToolTip="Standard request API: transfer item between bags using canonical item ref. Supports exact-cell transfer/swap when requested."))
	FYIInventoryOpResult RequestTransferItem(const FYIInventoryTransferItemRequest& Request);

	UFUNCTION(BlueprintCallable, Category="Inventory|Commands", meta=(ToolTip="Standard request API: split stack using canonical item ref."))
	FYIInventoryOpResult RequestSplitStack(const FYIInventorySplitStackRequest& Request);

	UFUNCTION(BlueprintCallable, Category="Inventory|Commands", meta=(ToolTip="Standard request API: combine stack using canonical item ref."))
	FYIInventoryOpResult RequestCombineItem(const FYIInventoryCombineItemRequest& Request);

	UFUNCTION(Server, Reliable)
	void ServerSetActiveBagById(const FGuid& InBagId);

	UFUNCTION(Server, Reliable)
	void ServerSetActiveContextBagById(FGameplayTag ContextTag, const FGuid& InBagId);

	UFUNCTION(Server, Reliable)
	void ServerClearActiveContextBag(FGameplayTag ContextTag);

	UFUNCTION(Server, Reliable)
	void ServerOpenContainedBagByInstance(const FGuid& ParentBagId, const FGuid& ParentItemInstanceId);

	UFUNCTION(Server, Reliable)
	void ServerOpenParentBag(const FGuid& ChildBagId);

	/** Optional per-inventory SFX library for item-driven UI sounds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio", meta=(ToolTip="Optional per-inventory SFX library for item-driven UI sounds"))
	TSoftObjectPtr<class UYIItemSFXLibrary> ItemSFXLibrary;

	/** Master toggle for all inventory UI sounds (drag/hover/drop/etc). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio", meta=(ToolTip="Master toggle for inventory UI sounds"))
	bool bEnableInventorySounds = true;

	/** Debug: print on-screen messages for inventory actions (add/move/drop/transfer). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Debug", meta=(ToolTip="Enable runtime debug prints for inventory actions."))
	bool bDebugInventoryActions = false;

	UPROPERTY(BlueprintAssignable, Category="Inventory|Events", meta=(ToolTip="Fires when item is added to bag on this instance."))
	FYIOnInventoryItemAdded OnInventoryItemAdded;
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events", meta=(ToolTip="Fires when item is removed from bag on this instance."))
	FYIOnInventoryItemRemoved OnInventoryItemRemoved;
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events", meta=(ToolTip="Fires when item position changes in bag on this instance."))
	FYIOnInventoryItemMoved OnInventoryItemMoved;
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events", meta=(ToolTip="Fires when item is rotated in bag on this instance."))
	FYIOnInventoryItemRotated OnInventoryItemRotated;
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events", meta=(ToolTip="Fires when item transfers between bags."))
	FYIOnInventoryItemTransferred OnInventoryItemTransferred;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Owner-only replicated minimal bag state for UI; rebuilt into a transient bag on clients.
	UPROPERTY(ReplicatedUsing=OnRep_NetBag, Transient)
	TArray<FYINetBagItem> NetBagItems;
	UPROPERTY(Replicated, Transient)
	FIntPoint NetBagGridSize = FIntPoint(0, 0);
	UPROPERTY(Replicated, Transient)
	int32 NetBagRevision = 0;
	UPROPERTY(ReplicatedUsing=OnRep_NetBagDescriptors, Transient)
	TArray<FYINetBagDescriptor> NetBagDescriptors;
	UPROPERTY(ReplicatedUsing=OnRep_ActiveBagContexts, Transient)
	FGuid ActiveBagId;
	UPROPERTY(ReplicatedUsing=OnRep_ActiveBagContexts, Transient)
	TArray<FYIActiveBagContextEntry> ActiveBagContexts;
	UPROPERTY(ReplicatedUsing=OnRep_NetContextBagMirrors, Transient)
	TArray<FYINetBagMirrorView> NetContextBagMirrors;

	UFUNCTION()
	void OnRep_NetBag();
	UFUNCTION()
	void OnRep_NetBagDescriptors();
	UFUNCTION()
	void OnRep_ActiveBagContexts();
	UFUNCTION()
	void OnRep_NetContextBagMirrors();
	UFUNCTION()
	void OnRep_LockedBagItems();

	UPROPERTY(ReplicatedUsing=OnRep_LockedBagItems, Transient)
	TArray<FYIInventoryLockRef> LockedBagItems;

	/** Client-only preview bag built from NetBagItems (not authoritative). */
	UPROPERTY(Transient)
	TObjectPtr<UYIInventoryBag> ClientPreviewBag = nullptr;
	/** Client-only preview bags for active context mirrors (not authoritative). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UYIInventoryBag>> ClientContextPreviewBags;

private:
	friend struct FYIInventoryContainerRuntimeService;
	friend struct FYIInventoryBagContextService;
	friend struct FYIInventoryBootstrapService;
	friend struct FYIInventoryItemIngressService;
	friend struct FYIInventoryIdentityLockService;
	friend struct FYIInventoryEventBridgeService;
	friend struct FYIInventoryMirrorService;
	friend struct FYIInventoryMutationService;

	FDelegateHandle BagChangedHandle;
	UYIInventoryBag* BagEventSource = nullptr;

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

	bool GetBagItemIdentity(const UYIInventoryBag* Bag, int32 ItemIndex, FYIInventoryItemRef& OutIdentity) const;
	bool IsBagItemLockedByIdentity(const FYIInventoryItemRef& Identity) const;
	bool FindItemIndexByInstanceId(const UYIInventoryBag* Bag, const FGuid& InstanceId, int32& OutIndex) const;
	bool FindContainerParentForBag(const FGuid& ChildBagId, FGuid& OutParentBagId, FGuid& OutParentItemInstanceId) const;
	bool IsBagDescendantOf(const FGuid& CandidateBagId, const FGuid& PotentialAncestorBagId) const;
	UYIInventoryBag* EnsureContainedBagForItem(FYIBagItem& InOutItem, const UYIInventoryBag* ParentBag);
	bool TryOpenContainedBagInternal(UYIInventoryBag* ParentBag, int32 ItemIndex);
	bool SetBagItemLockedInternal(const FYIInventoryItemRef& ItemRef, bool bLocked);
	int32 FindActiveContextIndex(FGameplayTag ContextTag) const;
	UYIInventoryBag* FindClientContextPreviewBagById(const FGuid& BagId) const;
	UYIInventoryBag* FindOrCreateClientContextPreviewBagById(const FGuid& BagId);
	void RebuildClientPreviewBagFromNet(UYIInventoryBag* TargetBag, const TArray<FYINetBagItem>& InItems, const FIntPoint& InGridSize, const FGuid& InBagId);
};

