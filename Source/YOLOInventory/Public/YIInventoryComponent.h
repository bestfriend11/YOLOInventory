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
	bool bIsActive = false;
};

UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class YOLOINVENTORY_API UYIInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UYIInventoryComponent();

	// Currently equipped (open) bag
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory")
	TObjectPtr<UYIInventoryBag> EquippedBag;

	// All bags owned by this component
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory")
	TArray<TObjectPtr<UYIInventoryBag>> Bags;

	// Events
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBagOpened, UYIInventoryBag*, Bag);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBagClosed, UYIInventoryBag*, Bag);

	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnBagOpened OnBagOpened;

	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnBagClosed OnBagClosed;

	// Create a new bag (owned by this component)
	UFUNCTION(BlueprintCallable, Category="Inventory")
	UYIInventoryBag* CreateBag(FName BagName, FIntPoint GridSize);

	// Open/close bag (fire events)
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void OpenBag(UYIInventoryBag* Bag);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	void CloseBag(UYIInventoryBag* Bag);

	// Quick accessors
	UFUNCTION(BlueprintCallable, Category="Inventory")
	UYIInventoryBag* GetBag() const;

	UFUNCTION(BlueprintPure, Category="Inventory")
	UYIInventoryBag* GetBagById(const FGuid& BagId) const;

	UFUNCTION(BlueprintPure, Category="Inventory")
	UYIInventoryBag* GetBagByRoleTag(FGameplayTag BagRoleTag) const;

	UFUNCTION(BlueprintPure, Category="Inventory")
	UYIInventoryBag* GetBagByDisplayName(FName BagName) const;

	UFUNCTION(BlueprintPure, Category="Inventory")
	FGuid GetActiveBagId() const { return ActiveBagId; }

	UFUNCTION(BlueprintPure, Category="Inventory")
	FGuid GetActiveSpellbookBagId() const { return ActiveSpellbookBagId; }

	UFUNCTION(BlueprintPure, Category="Inventory")
	UYIInventoryBag* GetActiveSpellbookBag() const;

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool SetActiveBagById(const FGuid& InBagId);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool SetActiveBagByRoleTag(FGameplayTag InBagRoleTag);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool SetActiveSpellbookBagById(const FGuid& InBagId);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool SetActiveSpellbookBagByRoleTag(FGameplayTag InBagRoleTag);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	void GetReplicatedBagDescriptors(TArray<FYINetBagDescriptor>& OutDescriptors) const;

	// Add an item to a bag; returns success
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool AddItemToBag(UYIInventoryBag* Bag, TSoftObjectPtr<class UYIItemDefinition> ItemDef, int32 Count = 1);

	// Remove a bag owned by this component
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool RemoveBag(UYIInventoryBag* Bag);

	/** Server-only: push current bag state into net mirror to replicate to owning client. */
	void SyncNetState();

	// --------- Authority-safe inventory mutations (RPC-backed) ----------
	UFUNCTION(BlueprintCallable, Category="Inventory|Net")
	bool MoveItem(int32 Index, FIntPoint NewPos);
	UFUNCTION(Server, Reliable)
	void ServerMoveItem(int32 Index, FIntPoint NewPos);

	UFUNCTION(BlueprintCallable, Category="Inventory|Net")
	bool RotateItem(int32 Index);
	UFUNCTION(Server, Reliable)
	void ServerRotateItem(int32 Index);

	/** Add an already-built bag item (e.g., from drag/drop). */
	UFUNCTION(BlueprintCallable, Category="Inventory|Net")
	int32 AddBagItem(const FYIBagItem& Item);
	UFUNCTION(Server, Reliable)
	void ServerAddBagItem(const struct FYIItemInstanceNet& NetItem, FIntPoint Pos, FIntPoint Size);

	UFUNCTION(BlueprintCallable, Category="Inventory|Net")
	bool RemoveItem(int32 Index);
	UFUNCTION(Server, Reliable)
	void ServerRemoveItem(int32 Index);

	/** Drop an item instance to the world (server authoritative). */
	UFUNCTION(BlueprintCallable, Category="Inventory|Net")
	bool DropItemToWorld(const struct FYIItemInstanceNet& NetItem, const FTransform& SpawnTransform);
	UFUNCTION(Server, Reliable)
	void ServerDropItemToWorld(const struct FYIItemInstanceNet& NetItem, const FTransform& SpawnTransform);

	/** Soft class references so designers can assign widgets once and call the helpers below. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TSoftClassPtr<UInventoryScreenWidget> InventoryScreenClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TSoftClassPtr<UTradingScreenWidget> TradingScreenClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TSoftClassPtr<UShopScreenWidget> ShopScreenClass;

	/** Optional per-inventory SFX library for item-driven UI sounds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio", meta=(ToolTip="Optional per-inventory SFX library for item-driven UI sounds"))
	TSoftObjectPtr<class UYIItemSFXLibrary> ItemSFXLibrary;

	/** Master toggle for all inventory UI sounds (drag/hover/drop/etc). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Audio", meta=(ToolTip="Master toggle for inventory UI sounds"))
	bool bEnableInventorySounds = true;

	/** Debug: print on-screen messages for inventory actions (add/move/drop/transfer). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Debug")
	bool bDebugInventoryActions = false;

	// -------- Inventory action delegates (designer-friendly) ----------
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventoryItemAdded, UYIInventoryBag*, Bag, int32, Index, FYIBagItem, Item);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventoryItemRemoved, UYIInventoryBag*, Bag, int32, Index, FYIBagItem, Item);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnInventoryItemMoved, UYIInventoryBag*, Bag, int32, Index, FIntPoint, NewPos);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemRotated, UYIInventoryBag*, Bag, int32, Index);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnInventoryItemTransferred, UYIInventoryBag*, Source, UYIInventoryBag*, Dest, int32, SourceIndex, int32, DestIndex);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryItemDroppedToWorld, const FYIItemInstanceNet&, Item, const FTransform&, SpawnTransform);

	UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
	FOnInventoryItemAdded OnInventoryItemAdded;
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
	FOnInventoryItemRemoved OnInventoryItemRemoved;
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
	FOnInventoryItemMoved OnInventoryItemMoved;
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
	FOnInventoryItemRotated OnInventoryItemRotated;
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
	FOnInventoryItemTransferred OnInventoryItemTransferred;
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
	FOnInventoryItemDroppedToWorld OnInventoryItemDroppedToWorld;

	/** Open the inventory screen for the owning local player. Creates if needed, sets the current bag, adds to viewport. */
	UFUNCTION(BlueprintCallable, Category="UI")
	UInventoryScreenWidget* OpenInventoryScreen();

	/** Close and remove the inventory screen if it is open. */
	UFUNCTION(BlueprintCallable, Category="UI")
	void CloseInventoryScreen();

	/** Open a trading screen for an existing session (client-side). LocalBag can be left null to auto-use GetBag(). */
	UFUNCTION(BlueprintCallable, Category="UI")
	UTradingScreenWidget* OpenTradeScreen(AYITradeSessionActor* Session, UYIInventoryBag* LocalBag = nullptr);

	/** Close the trading screen if it is open. */
	UFUNCTION(BlueprintCallable, Category="UI")
	void CloseTradeScreen();

	/** Open a shop screen for a shop component (client-side). */
	UFUNCTION(BlueprintCallable, Category="UI")
	UShopScreenWidget* OpenShopScreen(UYIShopComponent* Shop, UYIInventoryBag* LocalBag, const TArray<FYINetBagItem>& Stock, FIntPoint StockSize);

	/** Update the shop screen if it is already open (no-op if closed). */
	UFUNCTION(BlueprintCallable, Category="UI")
	void UpdateShopScreen(UYIShopComponent* Shop, UYIInventoryBag* LocalBag, const TArray<FYINetBagItem>& Stock, FIntPoint StockSize);

	/** Close the shop screen if it is open. */
	UFUNCTION(BlueprintCallable, Category="UI")
	void CloseShopScreen();

	/** Close all inventory-related screens (inventory/trade/shop). */
	UFUNCTION(BlueprintCallable, Category="UI")
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
};
