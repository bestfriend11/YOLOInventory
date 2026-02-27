#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "YIInventoryTypes.h"
#include "YIItemInstance.h"
#include "YIShopApiTypes.h"
#include "YIShopComponent.generated.h"

class UYIInventoryBag;
class UYIInventoryComponent;
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
struct YOLOINVENTORYSHOP_API FYIShopPrice
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price")
    FName Resource = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price")
    int64 Amount = 0;
};

/** Per-item price list keyed by item code. */
USTRUCT(BlueprintType)
struct YOLOINVENTORYSHOP_API FYIShopListing
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
class YOLOINVENTORYSHOP_API UYIShopComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UYIShopComponent();

    /** Stock mode determines whether the shop is shared or per-player. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop", meta=(ToolTip="How stock is owned at runtime:\nSharedStock = one stock for all players.\nPerPlayerStock = separate stock per player.\nLocalOnly = local/testing mode."))
    EYIShopStockMode StockMode = EYIShopStockMode::SharedStock;

    /** Template bag asset used to seed stock on BeginPlay (server only). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop", meta=(ToolTip="Template bag used by server to seed runtime shop stock on BeginPlay/restock."))
    TSoftObjectPtr<UYIInventoryBag> StockTemplate;

    /** Optional explicit listings (override price/stock). If empty, stock is free/infinite. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop", meta=(ToolTip="Optional explicit per-item listings.\nIf missing for an item, pricing/stock behavior falls back to component rules."))
    TArray<FYIShopListing> Listings;

    /** If true, explicit Listings prices override fragment-driven price fragments when both exist. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop|Pricing", meta=(ToolTip="When enabled, Listings price entries override fragment-driven shop prices for matching item code."))
    bool bListingsOverrideFragmentPrices = true;

    /** Optional semantic tags describing this shop context (Blacksmith, Magic, Faction.*). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop|Policy", meta=(ToolTip="Semantic context tags used by shop policy fragments (required/blocked tags)."))
    FGameplayTagContainer ShopContextTags;

    /** Allow players to sell items into the shop. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop", meta=(ToolTip="Allow players to sell items into shop stock."))
    bool bAllowSelling = true;

    /** Auto-pack stock after buy/sell to keep it tidy. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop", meta=(ToolTip="Auto tidy/pack stock after buy/sell operations."))
    bool bAutoSortStock = true;

    /** Sell price multiplier applied to listing prices (e.g. 0.5 = 50% of buy price). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop", meta=(ClampMin="0.0", ClampMax="1.0", ToolTip="Multiplier applied to buy price when player sells to shop (0.5 = 50%)."))
    float SellPriceMultiplier = 0.5f;

    /** If true, items without a listing can still be sold (price = 0). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop", meta=(ToolTip="If true, unlisted items can still be sold (typically for zero/derived price)."))
    bool bAllowSellingUnlisted = true;

    /** Debug: print on-screen messages for shop actions. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop|Debug", meta=(ToolTip="Print runtime debug messages for buy/sell/stock actions."))
    bool bDebugShopActions = false;

    // Shop action events (designer-friendly)
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnShopPurchase, APlayerState*, Buyer, int64, ItemCode, int32, Count, bool, bSuccess);
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnShopSale, APlayerState*, Seller, int64, ItemCode, int32, Count, bool, bSuccess);

    UPROPERTY(BlueprintAssignable, Category="Shop|Events", meta=(ToolTip="Server-side purchase event.\nArgs: Buyer, ItemCode, Count, bSuccess."))
    FOnShopPurchase OnShopPurchase;
    UPROPERTY(BlueprintAssignable, Category="Shop|Events", meta=(ToolTip="Server-side sale event.\nArgs: Seller, ItemCode, Count, bSuccess."))
    FOnShopSale OnShopSale;

    /** Auto-restock from template every interval (seconds). 0 disables. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop", meta=(ToolTip="Server restock interval in seconds.\n0 disables timed restock."))
    float RestockInterval = 0.f;

    /** Runtime stock bag (server authoritative). */
    UPROPERTY(Transient, BlueprintReadOnly, Category="Shop", meta=(ToolTip="Server-authoritative runtime stock bag (not directly replicated)."))
    TObjectPtr<UYIInventoryBag> RuntimeStock = nullptr;

    /** Minimal replicated view of stock for UI. */
    UPROPERTY(ReplicatedUsing=OnRep_StockMirror, meta=(ToolTip="Replicated minimal stock snapshot for client UI."))
    TArray<FYINetBagItem> StockMirror;

    UPROPERTY(ReplicatedUsing=OnRep_StockMirror, meta=(ToolTip="Replicated stock snapshot grid size for client UI."))
    FIntPoint StockMirrorSize = FIntPoint(0,0);

    UFUNCTION()
    void OnRep_StockMirror();

    /** Client UI hook when stock mirror changes. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStockMirrorUpdated);
    UPROPERTY(BlueprintAssignable, Category="Shop|Events", meta=(ToolTip="Client UI event fired when replicated stock mirror updates."))
    FOnStockMirrorUpdated OnStockMirrorUpdated;

    /** Authority-only execution path used by higher-level interaction components to get structured results. */
    bool ExecuteBuyRequest(const FYIShopBuyRequest& Request, FYIShopOpResult& OutResult);

    /** Authority-only execution path used by higher-level interaction components to get structured results. */
    bool ExecuteSellRequest(const FYIShopSellRequest& Request, FYIShopOpResult& OutResult);

    /** Blueprint helper to get listing info for UI (works on client). */
    UFUNCTION(BlueprintPure, Category="Shop", meta=(ToolTip="Client-safe getter for current replicated stock mirror items."))
    TArray<FYINetBagItem> GetStockMirror() const { return StockMirror; }

    UFUNCTION(BlueprintPure, Category="Shop", meta=(ToolTip="Client-safe getter for current replicated stock mirror size."))
    FIntPoint GetStockMirrorSize() const { return StockMirrorSize; }

    /** Resolve display price rows for this item using listing + fragment pipeline. */
    UFUNCTION(BlueprintCallable, Category="Shop|Pricing")
    bool ResolveDisplayPriceForItem(const FYIItemInstance& Item, bool bForBuy, APlayerState* ViewerPlayerState, int32 Count, TArray<FYIShopPrice>& OutPrices) const;

    /** Resolve static policy flags from item shop policy fragment for UI/debug surfaces. */
    UFUNCTION(BlueprintCallable, Category="Shop|Policy")
    void ResolveDisplayPolicyForItem(const FYIItemInstance& Item, bool& bOutVisible, bool& bOutBuyable, bool& bOutSellable) const;

    /** Build a stock mirror for a specific player (used for per-player stock mode). */
    void GetStockMirrorForPlayer(APlayerState* PlayerState, TArray<FYINetBagItem>& OutItems, FIntPoint& OutSize);

    /** Exposes current shop context tags for resolver services. */
    const FGameplayTagContainer& GetShopContextTags() const { return ShopContextTags; }

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    void BuildRuntimeStock();
    void RefreshMirror();
    bool ConsumePrice(UObject* ResourceProvider, const TArray<FYIShopPrice>& Prices);
    const FYIShopListing* FindListing(int64 ItemCode) const;
    int32 ResolveItemIndex(const UYIInventoryBag* Bag, int32 RequestedIndex, const FGuid& RequestedInstanceId) const;
    bool ResolvePolicyForItem(
        const FYIItemInstance& Item,
        bool& bOutVisible,
        bool& bOutBuyable,
        bool& bOutSellable,
        bool& bOutRequirePriceForVisibility,
        bool& bOutRequirePriceForBuy,
        bool& bOutRequirePriceForSell) const;
    bool ResolvePriceForItem(
        const FYIItemInstance& Item,
        APlayerState* BuyerPlayerState,
        APlayerState* SellerPlayerState,
        int32 Count,
        bool bForBuy,
        TArray<FYIShopPrice>& OutPrices,
        bool& bOutResolvedFromFragment) const;
    UYIInventoryBag* CreateStockInstance() const;
    UYIInventoryBag* GetStockForPlayer(APlayerState* PlayerState);
    void GetStockMirrorForBag(const UYIInventoryBag* Bag, APlayerState* ViewerPlayerState, TArray<FYINetBagItem>& OutItems, FIntPoint& OutSize) const;
    FTimerHandle RestockTimer;

    /** Per-player stock bags (server only, used when StockMode==PerPlayerStock). */
    TMap<TWeakObjectPtr<APlayerState>, TObjectPtr<UYIInventoryBag>> PlayerStock;
};
