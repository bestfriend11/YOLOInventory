#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YITradeInteractionComponent.generated.h"

class AYITradeSessionActor;
class UTradingScreenWidget;

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

    /** Fired on owning client if the request fails (authority rejects or spawn fails). */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTradeFailed, FText, Reason);
    UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Trade")
    FOnTradeFailed OnTradeFailed;

    /** Optional: auto create and show a trading widget on the owning client when a session starts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Trade|UI")
    bool bAutoShowWidget = false;

    /** Widget class to spawn when bAutoShowWidget is true. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Trade|UI")
    TSubclassOf<class UTradingScreenWidget> AutoTradeWidgetClass;

protected:
    virtual void BeginPlay() override;

    // Server-side authority handler (keep validation lightweight to avoid disconnects)
    UFUNCTION(Server, Reliable, WithValidation)
    void Server_RequestTrade(AActor* Target, bool bTargetIsNPC);

    // Client notifications
    UFUNCTION(Client, Reliable)
    void Client_TradeSessionStarted(AYITradeSessionActor* Session);

    UFUNCTION(Client, Reliable)
    void Client_TradeSessionFailed(const FText& Reason);

    /** Owning-client replicated session pointer for safe BP access (set on server). */
    UPROPERTY(ReplicatedUsing=OnRep_CurrentSession, BlueprintReadOnly, Category="YOLOInventory|Trade")
    AYITradeSessionActor* CurrentSession = nullptr;

    /** Called when CurrentSession replicates; will broadcast OnTradeSessionReady. */
    UFUNCTION()
    void OnRep_CurrentSession();

private:
    // Convenience to verify we are on a valid PC owner
    APlayerController* GetOwningPC() const;

    /** Internal guard to ensure the component is only used on PlayerControllers. */
    bool IsOwnerValidForTrade(bool bLogWarning = false) const;

    /** Cached widget if auto-show is enabled. */
    UPROPERTY(Transient)
    TWeakObjectPtr<UTradingScreenWidget> ActiveWidget;

    // Replication
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
