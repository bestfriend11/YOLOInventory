#include "YIShopComponent.h"

#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "YIInventoryBag.h"
#include "YIInventoryComponent.h"
#include "YIItemDefinition.h"
#include "YIShopPriceResolver.h"
#include "YIShopVisibilityResolver.h"
#include "YIDebugLibrary.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Components/ActorComponent.h"

namespace YIShopPrivate
{
	static UObject* ResolveResourceProvider(APlayerState* PlayerState)
	{
		if (!PlayerState)
		{
			return nullptr;
		}

		TInlineComponentArray<UActorComponent*> Components(PlayerState);
		for (UActorComponent* Component : Components)
		{
			if (!Component)
			{
				continue;
			}
			if (Component->FindFunction(TEXT("GetResourceAmount")) &&
				Component->FindFunction(TEXT("ConsumeResource")) &&
				Component->FindFunction(TEXT("AddResource")))
			{
				return Component;
			}
		}

		return nullptr;
	}

	static int64 GetResourceAmount(UObject* Provider, FName ResourceName)
	{
		if (!Provider)
		{
			return 0;
		}

		struct FGetResourceAmountParams
		{
			FName InResourceName = NAME_None;
			int64 ReturnValue = 0;
		};

		if (UFunction* Fn = Provider->FindFunction(TEXT("GetResourceAmount")))
		{
			FGetResourceAmountParams Params;
			Params.InResourceName = ResourceName;
			Provider->ProcessEvent(Fn, &Params);
			return Params.ReturnValue;
		}

		return 0;
	}

	static bool ConsumeResource(UObject* Provider, FName ResourceName, int64 Amount)
	{
		if (!Provider)
		{
			return false;
		}

		struct FConsumeResourceParams
		{
			FName InResourceName = NAME_None;
			int64 InAmount = 0;
			bool ReturnValue = false;
		};

		if (UFunction* Fn = Provider->FindFunction(TEXT("ConsumeResource")))
		{
			FConsumeResourceParams Params;
			Params.InResourceName = ResourceName;
			Params.InAmount = Amount;
			Provider->ProcessEvent(Fn, &Params);
			return Params.ReturnValue;
		}

		return false;
	}

	static void AddResource(UObject* Provider, FName ResourceName, int64 Delta)
	{
		if (!Provider)
		{
			return;
		}

		struct FAddResourceParams
		{
			FName InResourceName = NAME_None;
			int64 InDelta = 0;
		};

		if (UFunction* Fn = Provider->FindFunction(TEXT("AddResource")))
		{
			FAddResourceParams Params;
			Params.InResourceName = ResourceName;
			Params.InDelta = Delta;
			Provider->ProcessEvent(Fn, &Params);
		}
	}
}

namespace
{
	static FYIShopOpResult MakeShopResult(UYIShopComponent* Shop, EYIShopOpKind OpKind, const FGuid& RequestId, bool bAccepted, bool bSucceeded, EYIShopOpError Error, const FText& Message)
	{
		FYIShopOpResult Result;
		Result.Shop = Shop;
		Result.OpKind = OpKind;
		Result.RequestId = RequestId;
		Result.bRequestAccepted = bAccepted;
		Result.bSucceeded = bSucceeded;
		Result.Error = Error;
		Result.Message = Message;
		return Result;
	}
}

UYIShopComponent::UYIShopComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UYIShopComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetOwner() && GetOwner()->HasAuthority())
    {
        BuildRuntimeStock();
        if (StockMode == EYIShopStockMode::SharedStock)
        {
            RefreshMirror();
        }
        if (RestockInterval > 0.f)
        {
            GetWorld()->GetTimerManager().SetTimer(RestockTimer, this, &UYIShopComponent::BuildRuntimeStock, RestockInterval, true);
        }
    }
}

void UYIShopComponent::BuildRuntimeStock()
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;

    RuntimeStock = CreateStockInstance();

    if (StockMode == EYIShopStockMode::PerPlayerStock)
    {
        for (auto& Pair : PlayerStock)
        {
            Pair.Value = CreateStockInstance();
        }
    }

    if (StockMode == EYIShopStockMode::SharedStock)
    {
        RefreshMirror();
    }
}

const FYIShopListing* UYIShopComponent::FindListing(int64 ItemCode) const
{
    return Listings.FindByPredicate([&](const FYIShopListing& L){ return L.ItemCode == ItemCode; });
}

int32 UYIShopComponent::ResolveItemIndex(const UYIInventoryBag* Bag, int32 RequestedIndex, const FGuid& RequestedInstanceId) const
{
	if (!Bag)
	{
		return INDEX_NONE;
	}

	if (RequestedInstanceId.IsValid())
	{
		int32 ResolvedIndex = INDEX_NONE;
		if (Bag->FindItemIndexByInstanceIdFast(RequestedInstanceId, ResolvedIndex))
		{
			return ResolvedIndex;
		}
		return INDEX_NONE;
	}

	return RequestedIndex;
}

bool UYIShopComponent::ConsumePrice(UObject* ResourceProvider, const TArray<FYIShopPrice>& Prices)
{
    if (!ResourceProvider)
    {
        return Prices.Num() == 0;
    }

    if (Prices.Num() == 0)
    {
        return true;
    }

    // Check first
    for (const FYIShopPrice& Price : Prices)
    {
        if (Price.Amount <= 0 || Price.Resource.IsNone())
        {
            continue;
        }
        if (YIShopPrivate::GetResourceAmount(ResourceProvider, Price.Resource) < Price.Amount)
        {
            return false;
        }
    }
    // Consume
    for (const FYIShopPrice& Price : Prices)
    {
        if (Price.Amount <= 0 || Price.Resource.IsNone())
        {
            continue;
        }
        YIShopPrivate::ConsumeResource(ResourceProvider, Price.Resource, Price.Amount);
    }
    return true;
}

bool UYIShopComponent::ResolvePolicyForItem(
	const FYIItemInstance& Item,
	bool& bOutVisible,
	bool& bOutBuyable,
	bool& bOutSellable,
	bool& bOutRequirePriceForVisibility,
	bool& bOutRequirePriceForBuy,
	bool& bOutRequirePriceForSell) const
{
	bOutVisible = true;
	bOutBuyable = true;
	bOutSellable = true;
	bOutRequirePriceForVisibility = false;
	bOutRequirePriceForBuy = false;
	bOutRequirePriceForSell = false;

	UYIItemDefinition* Definition = Item.Definition.Get();
	if (!Definition)
	{
		return false;
	}

	const FYIShopResolvedVisibility Resolved = FYIShopVisibilityResolver::ResolvePolicy({this, Definition});
	bOutVisible = Resolved.bVisibleInShop;
	bOutBuyable = Resolved.bBuyable;
	bOutSellable = Resolved.bSellable;
	bOutRequirePriceForVisibility = Resolved.bRequirePriceForVisibility;
	bOutRequirePriceForBuy = Resolved.bRequirePriceForBuy;
	bOutRequirePriceForSell = Resolved.bRequirePriceForSell;
	return true;
}

bool UYIShopComponent::ResolvePriceForItem(
	const FYIItemInstance& Item,
	APlayerState* BuyerPlayerState,
	APlayerState* SellerPlayerState,
	int32 Count,
	bool bForBuy,
	TArray<FYIShopPrice>& OutPrices,
	bool& bOutResolvedFromFragment) const
{
	OutPrices.Reset();
	bOutResolvedFromFragment = false;

	if (Count <= 0)
	{
		return false;
	}

	UYIItemDefinition* Definition = Item.Definition.Get();

	const int64 ItemCode = Definition ? Definition->UniqueCode : 0;
	const FYIShopListing* Listing = ItemCode != 0 ? FindListing(ItemCode) : nullptr;

	FYIShopResolvedPriceResult FragmentPrices;
	bool bHasFragmentPrices = false;
	if (Definition)
	{
		FYIShopPriceResolverContext Context;
		Context.Shop = this;
		Context.Definition = Definition;
		Context.ItemInstance = &Item;
		Context.BuyerPlayerState = BuyerPlayerState;
		Context.SellerPlayerState = SellerPlayerState;
		Context.Count = Count;
		Context.PriceKind = bForBuy ? EYIShopResolvedPriceKind::Buy : EYIShopResolvedPriceKind::Sell;
		bHasFragmentPrices = FYIShopPriceResolver::ResolveFragmentPrice(Context, FragmentPrices);
	}

	if (Listing && Listing->Prices.Num() > 0 && (bListingsOverrideFragmentPrices || !bHasFragmentPrices))
	{
		OutPrices.Reserve(Listing->Prices.Num());
		for (const FYIShopPrice& ListingPrice : Listing->Prices)
		{
			if (ListingPrice.Resource.IsNone() || ListingPrice.Amount <= 0)
			{
				continue;
			}

			FYIShopPrice PriceRow;
			PriceRow.Resource = ListingPrice.Resource;
			if (bForBuy)
			{
				PriceRow.Amount = ListingPrice.Amount * static_cast<int64>(Count);
			}
			else
			{
				const int64 BaseSellAmount = static_cast<int64>(FMath::RoundToInt64(static_cast<double>(ListingPrice.Amount) * SellPriceMultiplier));
				PriceRow.Amount = BaseSellAmount * static_cast<int64>(Count);
			}

			if (PriceRow.Amount > 0)
			{
				OutPrices.Add(PriceRow);
			}
		}
		return OutPrices.Num() > 0;
	}

	if (!bHasFragmentPrices)
	{
		return false;
	}

	OutPrices.Reserve(FragmentPrices.Rows.Num());
	for (const FYIShopResolvedPriceRow& Row : FragmentPrices.Rows)
	{
		if (Row.Resource.IsNone() || Row.Amount <= 0)
		{
			continue;
		}

		FYIShopPrice& PriceRow = OutPrices.AddDefaulted_GetRef();
		PriceRow.Resource = Row.Resource;
		PriceRow.Amount = Row.Amount;
	}

	bOutResolvedFromFragment = OutPrices.Num() > 0;
	return OutPrices.Num() > 0;
}

bool UYIShopComponent::ResolveDisplayPriceForItem(const FYIItemInstance& Item, bool bForBuy, APlayerState* ViewerPlayerState, int32 Count, TArray<FYIShopPrice>& OutPrices) const
{
	bool bResolvedFromFragment = false;
	return ResolvePriceForItem(
		Item,
		bForBuy ? ViewerPlayerState : nullptr,
		bForBuy ? nullptr : ViewerPlayerState,
		FMath::Max(1, Count),
		bForBuy,
		OutPrices,
		bResolvedFromFragment);
}

void UYIShopComponent::ResolveDisplayPolicyForItem(const FYIItemInstance& Item, bool& bOutVisible, bool& bOutBuyable, bool& bOutSellable) const
{
	bool bRequirePriceForVisibility = false;
	bool bRequirePriceForBuy = false;
	bool bRequirePriceForSell = false;
	ResolvePolicyForItem(
		Item,
		bOutVisible,
		bOutBuyable,
		bOutSellable,
		bRequirePriceForVisibility,
		bRequirePriceForBuy,
		bRequirePriceForSell);
}

bool UYIShopComponent::ExecuteBuyRequest(const FYIShopBuyRequest& Request, FYIShopOpResult& OutResult)
{
	OutResult = MakeShopResult(this, EYIShopOpKind::Buy, Request.RequestId, false, false, EYIShopOpError::InvalidRequest, FText::GetEmpty());

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		OutResult.Error = EYIShopOpError::AuthorityRequired;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_NoAuthority", "Shop buy requires server authority");
		return false;
	}
	if (Request.Shop && Request.Shop != this)
	{
		OutResult.Error = EYIShopOpError::InvalidShop;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_WrongShop", "Request targets a different shop");
		return false;
	}
	if (!Request.BuyerInv)
	{
		OutResult.Error = EYIShopOpError::InvalidInventory;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_NoInv", "Buyer inventory is missing");
		return false;
	}
	if (Request.Count <= 0)
	{
		OutResult.Error = EYIShopOpError::InvalidCount;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_BadCount", "Invalid buy count");
		return false;
	}
	OutResult.bRequestAccepted = true;

	APawn* BuyerPawn = Cast<APawn>(Request.BuyerInv->GetOwner());
	APlayerState* BuyerPS = BuyerPawn ? BuyerPawn->GetPlayerState() : nullptr;
	if (!BuyerPS && BuyerPawn && BuyerPawn->GetController())
	{
		BuyerPS = BuyerPawn->GetController()->PlayerState;
	}
	UObject* BuyerResourceProvider = YIShopPrivate::ResolveResourceProvider(BuyerPS);
	UYIInventoryBag* StockBag = GetStockForPlayer(BuyerPS);
	if (!StockBag)
	{
		OnShopPurchase.Broadcast(BuyerPS, 0, Request.Count, false);
		OutResult.Error = EYIShopOpError::NoStock;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_NoStock", "Shop has no stock");
		return false;
	}
	const int32 StockIndex = ResolveItemIndex(StockBag, Request.StockIndex, Request.StockItemInstanceId);
	if (!StockBag->Items.IsValidIndex(StockIndex))
	{
		OnShopPurchase.Broadcast(BuyerPS, 0, Request.Count, false);
		OutResult.Error = EYIShopOpError::InvalidStockIndex;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_InvalidIndex", "Invalid stock index");
		return false;
	}

	FYIBagItem& StockItem = StockBag->Items[StockIndex];
	if (StockItem.Item.Count < Request.Count)
	{
		OnShopPurchase.Broadcast(BuyerPS, 0, Request.Count, false);
		OutResult.Error = EYIShopOpError::NotEnoughStock;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_NotEnough", "Not enough stock");
		return false;
	}

	const int64 Code = StockItem.Item.Definition.IsValid() ? StockItem.Item.Definition.Get()->UniqueCode : 0;
	bool bVisible = true;
	bool bBuyable = true;
	bool bSellable = true;
	bool bRequirePriceForVisibility = false;
	bool bRequirePriceForBuy = false;
	bool bRequirePriceForSell = false;
	ResolvePolicyForItem(StockItem.Item, bVisible, bBuyable, bSellable, bRequirePriceForVisibility, bRequirePriceForBuy, bRequirePriceForSell);
	if (!bVisible)
	{
		OnShopPurchase.Broadcast(BuyerPS, Code, Request.Count, false);
		OutResult.Error = EYIShopOpError::NotVisible;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_NotVisible", "Item is not visible in this shop");
		return false;
	}
	if (!bBuyable)
	{
		OnShopPurchase.Broadcast(BuyerPS, Code, Request.Count, false);
		OutResult.Error = EYIShopOpError::NotBuyable;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_NotBuyable", "Item cannot be bought from this shop");
		return false;
	}

	TArray<FYIShopPrice> ResolvedPrices;
	bool bUnusedResolvedFromFragment = false;
	const bool bHasResolvedPrice = ResolvePriceForItem(StockItem.Item, BuyerPS, nullptr, Request.Count, true, ResolvedPrices, bUnusedResolvedFromFragment);
	if (!bHasResolvedPrice && bRequirePriceForBuy)
	{
		OnShopPurchase.Broadcast(BuyerPS, Code, Request.Count, false);
		OutResult.Error = EYIShopOpError::PriceUnavailable;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_PriceUnavailable", "Item has no valid buy price in this shop");
		return false;
	}

	if (Request.DestPos.X >= 0 && Request.DestPos.Y >= 0)
	{
		UYIInventoryBag* BuyerBag = Request.BuyerInv->GetBag();
		if (!BuyerBag)
		{
			OnShopPurchase.Broadcast(BuyerPS, Code, Request.Count, false);
			OutResult.Error = EYIShopOpError::InvalidInventory;
			OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_NoBag", "No inventory bag");
			return false;
		}
		if (!BuyerBag->CanPlaceAt(Request.DestPos, StockItem.Size))
		{
			OnShopPurchase.Broadcast(BuyerPS, Code, Request.Count, false);
			OutResult.Error = EYIShopOpError::NoSpace;
			OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_InvalidCell", "Invalid placement");
			return false;
		}
	}

	if (!ConsumePrice(BuyerResourceProvider, ResolvedPrices))
	{
		OnShopPurchase.Broadcast(BuyerPS, Code, Request.Count, false);
		OutResult.Error = EYIShopOpError::NoFunds;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_NoFunds", "Not enough resources");
		return false;
	}

	FYIBagItem Slice = StockItem;
	Slice.Item.Count = Request.Count;
	if (Request.DestPos.X >= 0 && Request.DestPos.Y >= 0)
	{
		Slice.Pos = Request.DestPos;
	}

	int32 Added = INDEX_NONE;
	if (UYIInventoryBag* BuyerBag = Request.BuyerInv->GetBag())
	{
		if (Request.DestPos.X >= 0 && Request.DestPos.Y >= 0)
		{
			const bool bSavedAutoMerge = BuyerBag->bAutoMergeOnAdd;
			BuyerBag->bAutoMergeOnAdd = false;
			Added = BuyerBag->AddBagItem(Slice);
			BuyerBag->bAutoMergeOnAdd = bSavedAutoMerge;
		}
		else
		{
			Added = BuyerBag->AddBagItem(Slice);
		}
	}

	if (Added == INDEX_NONE)
	{
		if (BuyerResourceProvider)
		{
			for (const FYIShopPrice& Price : ResolvedPrices)
			{
				if (Price.Amount > 0 && !Price.Resource.IsNone())
				{
					YIShopPrivate::AddResource(BuyerResourceProvider, Price.Resource, Price.Amount);
				}
			}
		}
		OnShopPurchase.Broadcast(BuyerPS, Code, Request.Count, false);
		OutResult.Error = EYIShopOpError::NoSpace;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_NoSpace", "No space in inventory");
		return false;
	}

	StockItem.Item.Count -= Request.Count;
	if (StockItem.Item.Count <= 0)
	{
		StockBag->RemoveItem(StockIndex);
	}
	Request.BuyerInv->SyncNetState();

	if (StockMode == EYIShopStockMode::SharedStock)
	{
		RefreshMirror();
	}

	OnShopPurchase.Broadcast(BuyerPS, Code, Request.Count, true);
	OutResult.bSucceeded = true;
	OutResult.Error = EYIShopOpError::None;
	OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_Success", "Purchased");

	UYIDebugLibrary::EmitDebugMessage(
		this,
		EYIDebugChannel::Shop,
		FString::Printf(TEXT("Buy code=%lld x%d"), Code, Request.Count),
		FLinearColor(FColor::Green),
		bDebugShopActions,
		bDebugShopActions,
		2.0f,
		false,
		false,
		TEXT("Shop"));
	return true;
}

bool UYIShopComponent::ExecuteSellRequest(const FYIShopSellRequest& Request, FYIShopOpResult& OutResult)
{
	OutResult = MakeShopResult(this, EYIShopOpKind::Sell, Request.RequestId, false, false, EYIShopOpError::InvalidRequest, FText::GetEmpty());

	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		OutResult.Error = EYIShopOpError::AuthorityRequired;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_NoAuthority", "Shop sell requires server authority");
		return false;
	}
	if (Request.Shop && Request.Shop != this)
	{
		OutResult.Error = EYIShopOpError::InvalidShop;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_WrongShop", "Request targets a different shop");
		return false;
	}
	if (!Request.SellerInv)
	{
		OutResult.Error = EYIShopOpError::InvalidInventory;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_NoInv", "Seller inventory is missing");
		return false;
	}
	if (Request.Count <= 0)
	{
		OutResult.Error = EYIShopOpError::InvalidCount;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_BadCount", "Invalid sell count");
		return false;
	}
	if (!bAllowSelling)
	{
		OutResult.Error = EYIShopOpError::Unsupported;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_Disabled", "Shop does not allow selling");
		return false;
	}
	OutResult.bRequestAccepted = true;

	APawn* SellerPawn = Cast<APawn>(Request.SellerInv->GetOwner());
	APlayerState* SellerPS = SellerPawn ? SellerPawn->GetPlayerState() : nullptr;
	if (!SellerPS && SellerPawn && SellerPawn->GetController())
	{
		SellerPS = SellerPawn->GetController()->PlayerState;
	}
	UObject* SellerResourceProvider = YIShopPrivate::ResolveResourceProvider(SellerPS);

	UYIInventoryBag* SellerBag = Request.SellerInv->GetBag();
	const int32 SourceIndex = ResolveItemIndex(SellerBag, Request.SourceIndex, Request.SourceItemInstanceId);
	if (!SellerBag || !SellerBag->Items.IsValidIndex(SourceIndex))
	{
		OnShopSale.Broadcast(SellerPS, 0, Request.Count, false);
		OutResult.Error = EYIShopOpError::InvalidSourceIndex;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_InvalidIndex", "Invalid inventory index");
		return false;
	}

	FYIBagItem OriginalItem = SellerBag->Items[SourceIndex];
	if (OriginalItem.Item.Count < Request.Count)
	{
		OnShopSale.Broadcast(SellerPS, 0, Request.Count, false);
		OutResult.Error = EYIShopOpError::InvalidCount;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_NotEnough", "Not enough items to sell");
		return false;
	}

	const int64 Code = OriginalItem.Item.Definition.IsValid() ? OriginalItem.Item.Definition.Get()->UniqueCode : 0;
	bool bVisible = true;
	bool bBuyable = true;
	bool bSellable = true;
	bool bRequirePriceForVisibility = false;
	bool bRequirePriceForBuy = false;
	bool bRequirePriceForSell = false;
	ResolvePolicyForItem(OriginalItem.Item, bVisible, bBuyable, bSellable, bRequirePriceForVisibility, bRequirePriceForBuy, bRequirePriceForSell);
	if (!bSellable)
	{
		OnShopSale.Broadcast(SellerPS, Code, Request.Count, false);
		OutResult.Error = EYIShopOpError::NotSellable;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_NotSellable", "Item cannot be sold to this shop");
		return false;
	}

	TArray<FYIShopPrice> ResolvedPrices;
	bool bUnusedResolvedFromFragment = false;
	const bool bHasResolvedPrice = ResolvePriceForItem(OriginalItem.Item, nullptr, SellerPS, Request.Count, false, ResolvedPrices, bUnusedResolvedFromFragment);
	if (!bHasResolvedPrice && !bAllowSellingUnlisted)
	{
		OnShopSale.Broadcast(SellerPS, Code, Request.Count, false);
		OutResult.Error = EYIShopOpError::UnlistedNotAllowed;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_Unlisted", "Item cannot be sold here");
		return false;
	}
	if (!bHasResolvedPrice && bRequirePriceForSell)
	{
		OnShopSale.Broadcast(SellerPS, Code, Request.Count, false);
		OutResult.Error = EYIShopOpError::PriceUnavailable;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_PriceUnavailable", "Item has no valid sell price in this shop");
		return false;
	}

	if (OriginalItem.Item.Count == Request.Count)
	{
		if (!SellerBag->RemoveItem(SourceIndex))
		{
			OutResult.Error = EYIShopOpError::ValidationFailed;
			OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_RemoveFail", "Failed to remove item from inventory");
			return false;
		}
	}
	else
	{
		SellerBag->Items[SourceIndex].Item.Count -= Request.Count;
		SellerBag->OnChanged.Broadcast();
	}

	UYIInventoryBag* StockBag = GetStockForPlayer(SellerPS);
	if (!StockBag)
	{
		if (OriginalItem.Item.Count == Request.Count)
		{
			SellerBag->AddBagItem(OriginalItem);
		}
		else
		{
			SellerBag->Items[SourceIndex].Item.Count += Request.Count;
			SellerBag->OnChanged.Broadcast();
		}
		OnShopSale.Broadcast(SellerPS, Code, Request.Count, false);
		OutResult.Error = EYIShopOpError::NoStock;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_NoStock", "Shop cannot accept items");
		return false;
	}

	FYIBagItem Slice = OriginalItem;
	Slice.Item.Count = Request.Count;
	const int32 Added = StockBag->AddBagItem(Slice);
	if (Added == INDEX_NONE)
	{
		if (OriginalItem.Item.Count == Request.Count)
		{
			SellerBag->AddBagItem(OriginalItem);
		}
		else
		{
			SellerBag->Items[SourceIndex].Item.Count += Request.Count;
			SellerBag->OnChanged.Broadcast();
		}
		OnShopSale.Broadcast(SellerPS, Code, Request.Count, false);
		OutResult.Error = EYIShopOpError::NoSpace;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_NoSpace", "Shop stock is full");
		return false;
	}

	if (SellerResourceProvider)
	{
		for (const FYIShopPrice& Price : ResolvedPrices)
		{
			if (Price.Amount > 0 && !Price.Resource.IsNone())
			{
				YIShopPrivate::AddResource(SellerResourceProvider, Price.Resource, Price.Amount);
			}
		}
	}

	Request.SellerInv->SyncNetState();
	if (bAutoSortStock && StockBag)
	{
		StockBag->AutoPack();
	}

	if (StockMode == EYIShopStockMode::SharedStock)
	{
		RefreshMirror();
	}

	OnShopSale.Broadcast(SellerPS, Code, Request.Count, true);
	OutResult.bSucceeded = true;
	OutResult.Error = EYIShopOpError::None;
	OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_Success", "Sold");

	UYIDebugLibrary::EmitDebugMessage(
		this,
		EYIDebugChannel::Shop,
		FString::Printf(TEXT("Sell code=%lld x%d"), Code, Request.Count),
		FLinearColor(FColor::Yellow),
		bDebugShopActions,
		bDebugShopActions,
		2.0f,
		false,
		false,
		TEXT("Shop"));
	return true;
}

void UYIShopComponent::RefreshMirror()
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    StockMirror.Reset();
    StockMirrorSize = FIntPoint(8,6);
    if (RuntimeStock)
    {
        GetStockMirrorForBag(RuntimeStock, nullptr, StockMirror, StockMirrorSize);
    }
    // Push
    if (AActor* OwnerActor = GetOwner())
    {
        OwnerActor->ForceNetUpdate();
    }
    // Listen-server UI needs a local notify (OnRep won't fire on server)
    if (GetWorld() && GetWorld()->GetNetMode() != NM_DedicatedServer)
    {
        OnStockMirrorUpdated.Broadcast();
    }
}

void UYIShopComponent::OnRep_StockMirror()
{
    OnStockMirrorUpdated.Broadcast();
}

void UYIShopComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UYIShopComponent, StockMirror);
    DOREPLIFETIME(UYIShopComponent, StockMirrorSize);
}

UYIInventoryBag* UYIShopComponent::CreateStockInstance() const
{
    if (StockTemplate.IsValid())
    {
        return DuplicateObject<UYIInventoryBag>(StockTemplate.Get(), const_cast<UYIShopComponent*>(this));
    }
    if (StockTemplate.ToSoftObjectPath().IsValid())
    {
        if (UYIInventoryBag* Loaded = StockTemplate.LoadSynchronous())
        {
            return DuplicateObject<UYIInventoryBag>(Loaded, const_cast<UYIShopComponent*>(this));
        }
    }
    UYIInventoryBag* NewBag = NewObject<UYIInventoryBag>(const_cast<UYIShopComponent*>(this));
    NewBag->GridSize = FIntPoint(8,6);
    return NewBag;
}

UYIInventoryBag* UYIShopComponent::GetStockForPlayer(APlayerState* PlayerState)
{
    if (StockMode == EYIShopStockMode::SharedStock)
    {
        return RuntimeStock;
    }
    if (StockMode == EYIShopStockMode::LocalOnly)
    {
        return RuntimeStock;
    }
    if (!PlayerState)
    {
        return RuntimeStock;
    }
    if (TObjectPtr<UYIInventoryBag>* Found = PlayerStock.Find(PlayerState))
    {
        return Found->Get();
    }
    UYIInventoryBag* NewBag = CreateStockInstance();
    PlayerStock.Add(PlayerState, NewBag);
    return NewBag;
}

void UYIShopComponent::GetStockMirrorForBag(const UYIInventoryBag* Bag, APlayerState* ViewerPlayerState, TArray<FYINetBagItem>& OutItems, FIntPoint& OutSize) const
{
    OutItems.Reset();
    OutSize = FIntPoint(8,6);
    if (!Bag) return;

    OutSize = Bag->GridSize;
    for (const FYIBagItem& It : Bag->Items)
    {
        if (It.Item.Count <= 0) continue;

		bool bVisible = true;
		bool bBuyable = true;
		bool bSellable = true;
		bool bRequirePriceForVisibility = false;
		bool bRequirePriceForBuy = false;
		bool bRequirePriceForSell = false;
		ResolvePolicyForItem(It.Item, bVisible, bBuyable, bSellable, bRequirePriceForVisibility, bRequirePriceForBuy, bRequirePriceForSell);
		if (!bVisible)
		{
			continue;
		}

		if (bRequirePriceForVisibility)
		{
			TArray<FYIShopPrice> VisibilityPrices;
			bool bUnusedResolvedFromFragment = false;
			const bool bHasPrice = ResolvePriceForItem(
				It.Item,
				ViewerPlayerState,
				nullptr,
				1,
				true,
				VisibilityPrices,
				bUnusedResolvedFromFragment);
			if (!bHasPrice)
			{
				continue;
			}
		}

        FYINetBagItem Net;
        Net.Code = It.Item.Definition.IsValid() ? It.Item.Definition.Get()->UniqueCode : 0;
        Net.Count = It.Item.Count;
        Net.InstanceId = It.Item.InstanceId;
        Net.StackId = It.Item.StackId;
        Net.Pos = It.Pos;
        Net.Size = It.Size;
        Net.CustomStackKey = It.Item.CustomStackKey;
        Net.ContainedBagId = It.Item.ContainedBagId;
        OutItems.Add(Net);
    }
}

void UYIShopComponent::GetStockMirrorForPlayer(APlayerState* PlayerState, TArray<FYINetBagItem>& OutItems, FIntPoint& OutSize)
{
	const UYIInventoryBag* StockBag = GetStockForPlayer(PlayerState);
	APlayerState* VisibilityViewer = (StockMode == EYIShopStockMode::PerPlayerStock) ? PlayerState : nullptr;
	GetStockMirrorForBag(StockBag, VisibilityViewer, OutItems, OutSize);
}
