#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YIInventoryTypes.h"
#include "YIShopComponent.generated.h"

class UYIInventoryBag;
class UYIInventoryComponent;
class UYIPlayerInventoryStateComponent;
class UYIItemDefinition;
class APlayerState;

UENUM(BlueprintType)
enum class EYIShopStockMode : uint8
{
    /** One shared stock for all players (multiplayer-friendly shared shop). */
    SharedStock UMETA(DisplayName="Shared Stock"),
    /** Each player has their own stock instance (multiplayer-friendly, per-player shop). */
    PerPlayerStock UMETA(DisplayName="Per-Player Stock"),
    /** Local-only stock (single player or designer testing). */
    LocalOnly UMETA(DisplayName="Local Only")
};

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

    /** Stock mode determines whether the shop is shared or per-player. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop")
    EYIShopStockMode StockMode = EYIShopStockMode::SharedStock;

    /** Template bag asset used to seed stock on BeginPlay (server only). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop")
    TSoftObjectPtr<UYIInventoryBag> StockTemplate;

    /** Optional explicit listings (override price/stock). If empty, stock is free/infinite. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop")
    TArray<FYIShopListing> Listings;

    /** Allow players to sell items into the shop. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop")
    bool bAllowSelling = true;

    /** Sell price multiplier applied to listing prices (e.g. 0.5 = 50% of buy price). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop", meta=(ClampMin="0.0", ClampMax="1.0"))
    float SellPriceMultiplier = 0.5f;

    /** If true, items without a listing can still be sold (price = 0). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop")
    bool bAllowSellingUnlisted = true;

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

    /** Server: attempt to sell a slot from player's inventory into this shop. */
    UFUNCTION(Server, Reliable, WithValidation)
    void ServerSellItem(int32 SourceIndex, int32 Count, UYIInventoryComponent* SellerInv);

    /** Blueprint helper to get listing info for UI (works on client). */
    UFUNCTION(BlueprintPure, Category="Shop")
    TArray<FYINetBagItem> GetStockMirror() const { return StockMirror; }

    UFUNCTION(BlueprintPure, Category="Shop")
    FIntPoint GetStockMirrorSize() const { return StockMirrorSize; }

    /** Build a stock mirror for a specific player (used for per-player stock mode). */
    void GetStockMirrorForPlayer(APlayerState* PlayerState, TArray<FYINetBagItem>& OutItems, FIntPoint& OutSize);

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    void BuildRuntimeStock();
    void RefreshMirror();
    bool ConsumePrice(UYIPlayerInventoryStateComponent* BuyerState, int64 ItemCode, int32 Count);
    const FYIShopListing* FindListing(int64 ItemCode) const;
    UYIInventoryBag* CreateStockInstance() const;
    UYIInventoryBag* GetStockForPlayer(APlayerState* PlayerState);
    void GetStockMirrorForBag(const UYIInventoryBag* Bag, TArray<FYINetBagItem>& OutItems, FIntPoint& OutSize) const;
    void NotifyStockForPlayer(APlayerState* PlayerState);

    FTimerHandle RestockTimer;

    /** Per-player stock bags (server only, used when StockMode==PerPlayerStock). */
    TMap<TWeakObjectPtr<APlayerState>, TObjectPtr<UYIInventoryBag>> PlayerStock;
};
