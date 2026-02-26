#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YIInventoryBag.h"
#include "YIShopApiTypes.h"
#include "YITradeApiTypes.h"
#include "YITradeSessionActor.h"
#include "YITradeInteractionComponent.generated.h"

class AYITradeSessionActor;
class UUserWidget;
class UYIInventoryComponent;
class UYIInventoryBag;
class UYIShopComponent;
struct FYIBagItem;

/**
 * Player-controller component that handles secure trade initiation and notifies the client UI.
 * Drop it on your PlayerController BP/Class; call RequestTrade (client) and bind to OnTradeSessionReady.
 */
UCLASS(ClassGroup=(Inventory), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class YOLOINVENTORYTRADE_API UYITradeInteractionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UYITradeInteractionComponent();

    /** Client call: asks the server to start a trade with Target. bTargetIsNPC=true if Target is an NPC pawn. */
    /** Optional Blueprint/CPP hook to accept or reject a trade request before it is sent to the server. Return false to block. */
    UFUNCTION(BlueprintNativeEvent, Category="YOLOInventory|Trade", meta=(ToolTip="Local preflight hook before request is sent to server.\nReturn false to block request UI-side."))
    bool ValidateTradeRequest(AActor* Target, bool bTargetIsNPC) const;

    /** Fired on owning client after the server successfully creates a session. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTradeSessionReady, AYITradeSessionActor*, Session);
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Trade", meta=(ToolTip="Owning-client event fired when trade session is created/replicated and ready for UI wiring."))
    FOnTradeSessionReady OnTradeSessionReady;

    /** Fired when trade UI should be considered opened/closed (designer-friendly). */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTradeOpened, AYITradeSessionActor*, Session);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTradeClosed);
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Trade", meta=(ToolTip="Owning-client event fired when trade UI/session should be considered opened."))
    FOnTradeOpened OnTradeOpened;
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Trade", meta=(ToolTip="Owning-client event fired when trade UI/session closes."))
    FOnTradeClosed OnTradeClosed;

    /** Fired on owning client if the request fails (authority rejects or spawn fails). */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTradeFailed, FText, Reason);
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Trade", meta=(ToolTip="Owning-client failure event for rejected/failed trade requests."))
    FOnTradeFailed OnTradeFailed;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTradeOpResultReceived, const FYITradeOpResult&, Result);
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Trade", meta=(ToolTip="Structured trade request/result feedback for RequestTradeEx / RequestTradeTransferEx / RequestTradeSetReadyEx.")) 
    FOnTradeOpResultReceived OnTradeOpResultReceived;

    /** Client call: request a server-authoritative item transfer during an active trade session. */
    /** Standardized request/result contract for opening a trade session. */
    UFUNCTION(BlueprintCallable, Category="YOLOInventory|Trade|API")
    FYITradeOpResult RequestTradeEx(const FYITradeOpenRequest& Request);

    /** Standardized request/result contract for trade transfer. */
    UFUNCTION(BlueprintCallable, Category="YOLOInventory|Trade|API")
    FYITradeOpResult RequestTradeTransferEx(const FYITradeTransferRequest& Request);

    /** Standardized request/result contract for trade readiness / commit initiation (commit is still implicit when both sides are ready). */
    UFUNCTION(BlueprintCallable, Category="YOLOInventory|Trade|API")
    FYITradeOpResult RequestTradeSetReadyEx(const FYITradeSetReadyRequest& Request);

    /** Client call: request shop stock for a given shop component. */
    /** Optional Blueprint/CPP hook to accept or reject a shop request before it is sent to the server. */
    UFUNCTION(BlueprintNativeEvent, Category="YOLOInventory|Shop", meta=(ToolTip="Local preflight hook before shop request is sent to server. Return false to block."))
    bool ValidateShopRequest(UYIShopComponent* Shop) const;

    /** Fired on owning client when shop stock is ready for UI. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopStockUpdated, UYIShopComponent*, Shop);
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Shop", meta=(ToolTip="Owning-client event fired when CurrentShop/CurrentShopStock are refreshed."))
    FOnShopStockUpdated OnShopStockUpdated;

    /** Fired when shop UI should be considered opened/closed (designer-friendly). */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopOpened, UYIShopComponent*, Shop);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopClosed);
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Shop", meta=(ToolTip="Owning-client event fired when shop UI/session is considered opened."))
    FOnShopOpened OnShopOpened;
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Shop", meta=(ToolTip="Owning-client event fired when shop UI/session is closed."))
    FOnShopClosed OnShopClosed;

    /** Fired on owning client when a shop buy/sell action completes. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnShopActionResult, UYIShopComponent*, Shop, bool, bSuccess, FText, Reason);
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Shop", meta=(ToolTip="Owning-client result event for buy/sell operations."))
    FOnShopActionResult OnShopActionResult;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopOpResultReceived, const FYIShopOpResult&, Result);
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Shop", meta=(ToolTip="Structured shop request/result feedback for RequestShop*Ex APIs."))
    FOnShopOpResultReceived OnShopOpResultReceived;

    /** Latest shop stock snapshot (client-owned). */
    UPROPERTY(BlueprintReadOnly, Category="YOLOInventory|Shop", meta=(ToolTip="Current shop context on owning client."))
    UYIShopComponent* CurrentShop = nullptr;
    UPROPERTY(BlueprintReadOnly, Category="YOLOInventory|Shop", meta=(ToolTip="Latest client stock snapshot for CurrentShop."))
    TArray<FYINetBagItem> CurrentShopStock;
    UPROPERTY(BlueprintReadOnly, Category="YOLOInventory|Shop", meta=(ToolTip="Grid size for CurrentShopStock snapshot."))
    FIntPoint CurrentShopStockSize = FIntPoint(0,0);

    /** Client call: buy an item from the active shop (server authoritative). DestPos optional for exact placement. */
    UFUNCTION(BlueprintCallable, Category="YOLOInventory|Shop|API")
    FYIShopOpResult RequestShopOpenEx(const FYIShopOpenRequest& Request);

    UFUNCTION(BlueprintCallable, Category="YOLOInventory|Shop|API")
    FYIShopOpResult RequestShopBuyEx(const FYIShopBuyRequest& Request);

    UFUNCTION(BlueprintCallable, Category="YOLOInventory|Shop|API")
    FYIShopOpResult RequestShopSellEx(const FYIShopSellRequest& Request);

    /** Client RPC: push shop stock data to owning client (used by server). */
    UFUNCTION(Client, Reliable)
    void Client_ShopStockReady(UYIShopComponent* Shop, const TArray<FYINetBagItem>& Stock, FIntPoint Size);

    /** Client RPC: notify result of a shop action. */
    UFUNCTION(Client, Reliable)
    void Client_ShopActionResult(UYIShopComponent* Shop, bool bSuccess, const FText& Reason);

    // Bag activity delegates (local pawn)
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBagItemAdded, int32, Index, FYIBagItem, Item);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBagItemRemoved, int32, Index, FYIBagItem, Item);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBagItemMoved, int32, Index, FIntPoint, NewPos);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBagItemRotated, int32, Index);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnBagItemTransferred, UYIInventoryBag*, Source, UYIInventoryBag*, Dest, int32, SourceIndex, int32, DestIndex);

    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Bag", meta=(ToolTip="Local bag event proxy: item added."))
    FOnBagItemAdded OnBagItemAdded;
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Bag", meta=(ToolTip="Local bag event proxy: item removed."))
    FOnBagItemRemoved OnBagItemRemoved;
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Bag", meta=(ToolTip="Local bag event proxy: item moved."))
    FOnBagItemMoved OnBagItemMoved;
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Bag", meta=(ToolTip="Local bag event proxy: item rotated."))
    FOnBagItemRotated OnBagItemRotated;
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Bag", meta=(ToolTip="Local bag event proxy: item transferred between bags."))
    FOnBagItemTransferred OnBagItemTransferred;

    /** Optional: auto create and show a trading widget on the owning client when a session starts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Trade|UI", meta=(ToolTip="If true, owning client auto-opens trade widget when session starts."))
    bool bAutoShowWidget = false;

    /** Widget class to spawn when bAutoShowWidget is true. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Trade|UI", meta=(ToolTip="Trade widget class used when bAutoShowWidget is enabled."))
    TSubclassOf<UUserWidget> AutoTradeWidgetClass;

    /** Auto open the shop screen on the owning client when stock is ready. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Shop|UI", meta=(ToolTip="If true, owning client auto-opens shop widget when stock arrives."))
    bool bAutoShowShopWidget = true;

    /** Widget class to spawn when bAutoShowShopWidget is true. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Shop|UI", meta=(ToolTip="Shop widget class used when bAutoShowShopWidget is enabled."))
    TSubclassOf<UUserWidget> AutoShopWidgetClass;

    /** Maximum distance allowed to start a trade (interaction range). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Trade|Distance", meta=(ToolTip="Max distance allowed to start trade interaction."))
    float TradeInteractionDistance = 350.f;

    /** Distance allowed to keep a trade open before auto-close/cancel. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Trade|Distance", meta=(ToolTip="Max distance allowed while trade remains open before auto-close/cancel."))
    float TradeKeepAliveDistance = 800.f;

    /** Maximum distance allowed to start a shop interaction. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Shop|Distance", meta=(ToolTip="Max distance allowed to start shop interaction."))
    float ShopInteractionDistance = 350.f;

    /** Distance allowed to keep a shop open before auto-close. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Shop|Distance", meta=(ToolTip="Max distance allowed while shop remains open before auto-close."))
    float ShopKeepAliveDistance = 800.f;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Server-side authority handler (keep validation lightweight to avoid disconnects)
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestTrade(AActor* Target, bool bTargetIsNPC);

	UFUNCTION(Server, Reliable)
	void Server_RequestTradeEx(FYITradeOpenRequest Request);

	// Server-side shop handler
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestShop(UYIShopComponent* Shop);

	UFUNCTION(Server, Reliable)
	void Server_RequestShopOpenEx(FYIShopOpenRequest Request);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestShopBuy(UYIShopComponent* Shop, int32 StockIndex, int32 Count, UYIInventoryComponent* BuyerInv, FIntPoint DestPos);

	UFUNCTION(Server, Reliable)
	void Server_RequestShopBuyEx(FYIShopBuyRequest Request);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestShopSell(UYIShopComponent* Shop, int32 SourceIndex, int32 Count, UYIInventoryComponent* SellerInv);

	UFUNCTION(Server, Reliable)
	void Server_RequestShopSellEx(FYIShopSellRequest Request);

	/** Server: execute a transfer request during an active trade session. */
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_TransferItem(ETradeSide FromSide, ETradeSide ToSide, int32 SourceIndex, FIntPoint DestPos, int32 Count);

	UFUNCTION(Server, Reliable)
	void Server_RequestTradeTransferEx(FYITradeTransferRequest Request);

	UFUNCTION(Server, Reliable)
	void Server_RequestTradeSetReadyEx(FYITradeSetReadyRequest Request);

    // Client notifications
    UFUNCTION(Client, Reliable)
    void Client_TradeSessionStarted(AYITradeSessionActor* Session);

    UFUNCTION(Client, Reliable)
    void Client_TradeSessionFailed(const FText& Reason);

    UFUNCTION(Client, Reliable)
    void Client_TradeOpResult(const FYITradeOpResult& Result);

    UFUNCTION(Client, Reliable)
    void Client_ShopOpResult(const FYIShopOpResult& Result);

    /** Owning-client replicated session pointer for safe BP access (set on server). */
    UPROPERTY(ReplicatedUsing=OnRep_CurrentSession, BlueprintReadOnly, Category="YOLOInventory|Trade", meta=(ToolTip="Replicated current trade session pointer for owning client. Use OnRep_CurrentSession/OnTradeSessionReady for UI wiring."))
    AYITradeSessionActor* CurrentSession = nullptr;

    /** Debug: print on-screen messages for bag and trade events. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Trade|Debug", meta=(ToolTip="Print runtime trade/shop interaction debug messages."))
    bool bDebugTradeInteraction = false;

    /** Called when CurrentSession replicates; will broadcast OnTradeSessionReady. */
    UFUNCTION()
	void OnRep_CurrentSession();

private:
	// Convenience to verify we are on a valid PC owner
	APlayerController* GetOwningPC() const;
	UYIInventoryComponent* GetOwningInventoryComponent() const;
	UYIInventoryBag* GetOwningInventoryBag() const;
	void OpenOrRefreshTradeWidget(AYITradeSessionActor* Session);
	void OpenOrRefreshShopWidget(UYIShopComponent* Shop, const TArray<FYINetBagItem>& Stock, FIntPoint Size);
	void CloseTradeWidget();
	void CloseShopWidget();

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
    void HandleTradeCancelled();
    UFUNCTION()
    void HandleTradeCommittedSession();
    UFUNCTION()
    void HandleTradeFailedSession();

    // Legacy C++ wrappers kept for suite internals while public/BP API migrates to *Ex request/result methods.
    void RequestTrade(AActor* Target, bool bTargetIsNPC);
    void RequestTradeTransfer(ETradeSide FromSide, ETradeSide ToSide, int32 SourceIndex, FIntPoint DestPos, int32 Count = 0);
    void RequestTradeSetReady(ETradeSide Side, bool bReady);
    void RequestShop(UYIShopComponent* Shop);
    void RequestShopBuy(UYIShopComponent* Shop, int32 StockIndex, int32 Count, UYIInventoryComponent* BuyerInv, FIntPoint DestPos);
    void RequestShopSell(UYIShopComponent* Shop, int32 SourceIndex, int32 Count, UYIInventoryComponent* SellerInv);

    TWeakObjectPtr<AYITradeSessionActor> BoundSession;
    TWeakObjectPtr<UUserWidget> ActiveTradeWidget;
    TWeakObjectPtr<UUserWidget> ActiveShopWidget;
    bool bShopOpened = false;
    FGuid PendingTradeOpenRequestId;
};
