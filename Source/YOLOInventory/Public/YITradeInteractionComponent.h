#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YIInventoryBag.h"
#include "YITradeSessionActor.h"
#include "YITradeInteractionComponent.generated.h"

class AYITradeSessionActor;
class UTradingScreenWidget;
class UYIInventoryComponent;
class UYIInventoryBag;
class UYIShopComponent;
struct FYIBagItem;
class UShopScreenWidget;

/**
 * Player-controller component that handles secure trade initiation and notifies the client UI.
 * Drop it on your PlayerController BP/Class; call RequestTrade (client) and bind to OnTradeSessionReady.
 */
UCLASS(ClassGroup=(Inventory), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class YOLOINVENTORY_API UYITradeInteractionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UYITradeInteractionComponent();

    /** Client call: asks the server to start a trade with Target. bTargetIsNPC=true if Target is an NPC pawn. */
    UFUNCTION(BlueprintCallable, Category="YOLOInventory|Trade")
    void RequestTrade(AActor* Target, bool bTargetIsNPC);

    /** Optional Blueprint/CPP hook to accept or reject a trade request before it is sent to the server. Return false to block. */
    UFUNCTION(BlueprintNativeEvent, Category="YOLOInventory|Trade")
    bool ValidateTradeRequest(AActor* Target, bool bTargetIsNPC) const;

    /** Fired on owning client after the server successfully creates a session. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTradeSessionReady, AYITradeSessionActor*, Session);
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Trade")
    FOnTradeSessionReady OnTradeSessionReady;

    /** Fired when trade UI should be considered opened/closed (designer-friendly). */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTradeOpened, AYITradeSessionActor*, Session);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTradeClosed);
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Trade")
    FOnTradeOpened OnTradeOpened;
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Trade")
    FOnTradeClosed OnTradeClosed;

    /** Fired on owning client if the request fails (authority rejects or spawn fails). */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTradeFailed, FText, Reason);
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Trade")
    FOnTradeFailed OnTradeFailed;

    /** Client call: request a server-authoritative item transfer during an active trade session. */
    UFUNCTION(BlueprintCallable, Category="YOLOInventory|Trade")
    void RequestTradeTransfer(ETradeSide FromSide, ETradeSide ToSide, int32 SourceIndex, FIntPoint DestPos, int32 Count = 0);

    /** Client call: request shop stock for a given shop component. */
    UFUNCTION(BlueprintCallable, Category="YOLOInventory|Shop")
    void RequestShop(UYIShopComponent* Shop);

    /** Optional Blueprint/CPP hook to accept or reject a shop request before it is sent to the server. */
    UFUNCTION(BlueprintNativeEvent, Category="YOLOInventory|Shop")
    bool ValidateShopRequest(UYIShopComponent* Shop) const;

    /** Fired on owning client when shop stock is ready for UI. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopStockUpdated, UYIShopComponent*, Shop);
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Shop")
    FOnShopStockUpdated OnShopStockUpdated;

    /** Fired when shop UI should be considered opened/closed (designer-friendly). */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopOpened, UYIShopComponent*, Shop);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopClosed);
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Shop")
    FOnShopOpened OnShopOpened;
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Shop")
    FOnShopClosed OnShopClosed;

    /** Fired on owning client when a shop buy/sell action completes. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnShopActionResult, UYIShopComponent*, Shop, bool, bSuccess, FText, Reason);
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Shop")
    FOnShopActionResult OnShopActionResult;

    /** Latest shop stock snapshot (client-owned). */
    UPROPERTY(BlueprintReadOnly, Category="YOLOInventory|Shop")
    UYIShopComponent* CurrentShop = nullptr;
    UPROPERTY(BlueprintReadOnly, Category="YOLOInventory|Shop")
    TArray<FYINetBagItem> CurrentShopStock;
    UPROPERTY(BlueprintReadOnly, Category="YOLOInventory|Shop")
    FIntPoint CurrentShopStockSize = FIntPoint(0,0);

    /** Client call: buy an item from the active shop (server authoritative). DestPos optional for exact placement. */
    UFUNCTION(BlueprintCallable, Category="YOLOInventory|Shop")
    void RequestShopBuy(UYIShopComponent* Shop, int32 StockIndex, int32 Count, UYIInventoryComponent* BuyerInv, FIntPoint DestPos);

    /** Client RPC: push shop stock data to owning client (used by server). */
    UFUNCTION(Client, Reliable)
    void Client_ShopStockReady(UYIShopComponent* Shop, const TArray<FYINetBagItem>& Stock, FIntPoint Size);

    /** Client RPC: notify result of a shop action. */
    UFUNCTION(Client, Reliable)
    void Client_ShopActionResult(UYIShopComponent* Shop, bool bSuccess, const FText& Reason);

    /** Client call: sell an item from the player's inventory into the shop (server authoritative). */
    UFUNCTION(BlueprintCallable, Category="YOLOInventory|Shop")
    void RequestShopSell(UYIShopComponent* Shop, int32 SourceIndex, int32 Count, UYIInventoryComponent* SellerInv);

    // Bag activity delegates (local pawn)
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBagItemAdded, int32, Index, FYIBagItem, Item);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBagItemRemoved, int32, Index, FYIBagItem, Item);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBagItemMoved, int32, Index, FIntPoint, NewPos);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBagItemRotated, int32, Index);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnBagItemTransferred, UYIInventoryBag*, Source, UYIInventoryBag*, Dest, int32, SourceIndex, int32, DestIndex);

    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Bag")
    FOnBagItemAdded OnBagItemAdded;
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Bag")
    FOnBagItemRemoved OnBagItemRemoved;
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Bag")
    FOnBagItemMoved OnBagItemMoved;
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Bag")
    FOnBagItemRotated OnBagItemRotated;
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Bag")
    FOnBagItemTransferred OnBagItemTransferred;

    /** Optional: auto create and show a trading widget on the owning client when a session starts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Trade|UI")
    bool bAutoShowWidget = false;

    /** Widget class to spawn when bAutoShowWidget is true. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Trade|UI")
    TSubclassOf<class UTradingScreenWidget> AutoTradeWidgetClass;

    /** Auto open the shop screen on the owning client when stock is ready. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Shop|UI")
    bool bAutoShowShopWidget = true;

    /** Widget class to spawn when bAutoShowShopWidget is true (fallback if no inventory component). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Shop|UI")
    TSubclassOf<UShopScreenWidget> AutoShopWidgetClass;

    /** Maximum distance allowed to start a trade (interaction range). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Trade|Distance")
    float TradeInteractionDistance = 350.f;

    /** Distance allowed to keep a trade open before auto-close/cancel. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Trade|Distance")
    float TradeKeepAliveDistance = 800.f;

    /** Maximum distance allowed to start a shop interaction. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Shop|Distance")
    float ShopInteractionDistance = 350.f;

    /** Distance allowed to keep a shop open before auto-close. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Shop|Distance")
    float ShopKeepAliveDistance = 800.f;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Server-side authority handler (keep validation lightweight to avoid disconnects)
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestTrade(AActor* Target, bool bTargetIsNPC);

	// Server-side shop handler
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestShop(UYIShopComponent* Shop);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestShopBuy(UYIShopComponent* Shop, int32 StockIndex, int32 Count, UYIInventoryComponent* BuyerInv, FIntPoint DestPos);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestShopSell(UYIShopComponent* Shop, int32 SourceIndex, int32 Count, UYIInventoryComponent* SellerInv);

	/** Server: execute a transfer request during an active trade session. */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_TransferItem(ETradeSide FromSide, ETradeSide ToSide, int32 SourceIndex, FIntPoint DestPos, int32 Count);

    // Client notifications
    UFUNCTION(Client, Reliable)
    void Client_TradeSessionStarted(AYITradeSessionActor* Session);

    UFUNCTION(Client, Reliable)
    void Client_TradeSessionFailed(const FText& Reason);

    /** Owning-client replicated session pointer for safe BP access (set on server). */
    UPROPERTY(ReplicatedUsing=OnRep_CurrentSession, BlueprintReadOnly, Category="YOLOInventory|Trade")
    AYITradeSessionActor* CurrentSession = nullptr;

    /** Debug: print on-screen messages for bag and trade events. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Trade|Debug")
    bool bDebugTradeInteraction = false;

    /** Called when CurrentSession replicates; will broadcast OnTradeSessionReady. */
    UFUNCTION()
	void OnRep_CurrentSession();

private:
	// Convenience to verify we are on a valid PC owner
	APlayerController* GetOwningPC() const;

	/** Internal guard to ensure the component is only used on PlayerControllers. */
	bool IsOwnerValidForTrade(bool bLogWarning = false) const;

    /** Distance helper. */
    bool IsWithinDistance(const AActor* Target, float MaxDistance) const;

    /** Resolve the other side actor for the current trade session. */
    AActor* GetOtherTradeActor() const;

    /** Close trade UI + broadcast event. */
    void CloseTradeLocal(bool bCancelServer);

    /** Close shop UI + broadcast event. */
    void CloseShopLocal();

	/** Server helper: push a session reference into this component and replicate to its owner. */
	void ServerAssignSession(AYITradeSessionActor* Session);

    // Replication
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION()
    void OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

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

    UFUNCTION()
    void HandleTradeEnded();

    TWeakObjectPtr<AYITradeSessionActor> BoundSession;
    bool bShopOpened = false;
};
