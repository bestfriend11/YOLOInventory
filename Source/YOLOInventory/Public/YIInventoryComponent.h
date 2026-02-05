#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YIInventoryBag.h"
#include "UObject/SoftObjectPtr.h"
#include "YIInventoryComponent.generated.h"

class UYIInventoryBag;
class UInventoryScreenWidget;
class UTradingScreenWidget;
class AYITradeSessionActor;

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

	/** Soft class references so designers can assign widgets once and call the helpers below. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TSoftClassPtr<UInventoryScreenWidget> InventoryScreenClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="UI")
	TSoftClassPtr<UTradingScreenWidget> TradingScreenClass;

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

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Owner-only replicated minimal bag state for UI; rebuilt into a transient bag on clients.
	UPROPERTY(ReplicatedUsing=OnRep_NetBag, Transient)
	TArray<FYINetBagItem> NetBagItems;
	UPROPERTY(Replicated, Transient)
	FIntPoint NetBagGridSize = FIntPoint(0, 0);

	UFUNCTION()
	void OnRep_NetBag();

	/** Client-only preview bag built from NetBagItems (not authoritative). */
	UPROPERTY(Transient)
	TObjectPtr<UYIInventoryBag> ClientPreviewBag = nullptr;

private:
	FDelegateHandle BagChangedHandle;
	TWeakObjectPtr<UInventoryScreenWidget> ActiveInventoryScreen;
	TWeakObjectPtr<UTradingScreenWidget> ActiveTradeScreen;

	// Cleanup delegate when component is destroyed
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;

	virtual void BeginPlay() override;

	/** Create a runtime bag instance from a template asset (layout + items). */
	UYIInventoryBag* CloneBagTemplate(const UYIInventoryBag* TemplateBag);

	/** Returns true if the bag is an asset/template (outer is package/public). */
	bool IsTemplateBag(const UYIInventoryBag* Bag) const;
};
