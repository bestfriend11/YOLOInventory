#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YIInventoryTypes.h"
#include "YIShopComponent.generated.h"

class UYIInventoryBag;
class UYIInventoryComponent;
class UYIPlayerInventoryStateComponent;
class UYIItemDefinition;

/** Price entry for a single resource (e.g., Gold, Silver). */
USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIShopPrice
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price")
    FName Resource = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price")
    int64 Amount = 0;
};

/** Per-item price list keyed by item code. */
USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIShopListing
{
    GENERATED_BODY()

    /** Unique item code (UYIItemDefinition::UniqueCode) this price applies to. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
    int64 ItemCode = 0;

    /** Price in one or more resources. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
    TArray<FYIShopPrice> Prices;

    /** Available stock; -1 = infinite. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
    int32 Stock = -1;
};

/**
 * UYIShopComponent
 * Attach to an NPC/Shop actor. It owns a runtime bag of stock and replicates a net-safe mirror to all clients.
 * Provides server-side purchase RPC that transfers items into the buyer's inventory if they have resources.
 */
UCLASS(ClassGroup=(Inventory), Blueprintable, meta=(BlueprintSpawnableComponent))
class YOLOINVENTORY_API UYIShopComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UYIShopComponent();

    /** Template bag asset used to seed stock on BeginPlay (server only). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop")
    TSoftObjectPtr<UYIInventoryBag> StockTemplate;

    /** Optional explicit listings (override price/stock). If empty, stock is free/infinite. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop")
    TArray<FYIShopListing> Listings;

    /** Auto-restock from template every interval (seconds). 0 disables. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop")
    float RestockInterval = 0.f;

    /** Runtime stock bag (server authoritative). */
    UPROPERTY(Transient, BlueprintReadOnly, Category="Shop")
    TObjectPtr<UYIInventoryBag> RuntimeStock = nullptr;

    /** Minimal replicated view of stock for UI. */
    UPROPERTY(ReplicatedUsing=OnRep_StockMirror)
    TArray<FYINetBagItem> StockMirror;

    UPROPERTY(ReplicatedUsing=OnRep_StockMirror)
    FIntPoint StockMirrorSize = FIntPoint(0,0);

    UFUNCTION()
    void OnRep_StockMirror();

    /** Client UI hook when stock mirror changes. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStockMirrorUpdated);
    UPROPERTY(BlueprintAssignable, Category="Shop|Events")
    FOnStockMirrorUpdated OnStockMirrorUpdated;

    /** Server: attempt to buy a slot from this shop into buyer's inventory component. */
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerBuyItem(int32 StockIndex, int32 Count, UYIInventoryComponent* BuyerInv);

    /** Blueprint helper to get listing info for UI (works on client). */
    UFUNCTION(BlueprintPure, Category="Shop")
    TArray<FYINetBagItem> GetStockMirror() const { return StockMirror; }

    UFUNCTION(BlueprintPure, Category="Shop")
    FIntPoint GetStockMirrorSize() const { return StockMirrorSize; }

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    void BuildRuntimeStock();
    void RefreshMirror();
    bool ConsumePrice(UYIPlayerInventoryStateComponent* BuyerState, int64 ItemCode, int32 Count);
    const FYIShopListing* FindListing(int64 ItemCode) const;

    FTimerHandle RestockTimer;
};
