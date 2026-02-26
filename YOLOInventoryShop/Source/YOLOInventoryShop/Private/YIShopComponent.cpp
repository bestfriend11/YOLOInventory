#include "YIShopComponent.h"

#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "YIInventoryBag.h"
#include "YIInventoryComponent.h"
#include "YIItemDefinition.h"
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

bool UYIShopComponent::ConsumePrice(UObject* ResourceProvider, int64 ItemCode, int32 Count)
{
    if (!ResourceProvider) return false;
    const FYIShopListing* Listing = FindListing(ItemCode);
    if (!Listing) return true; // free

    // Check first
    for (const FYIShopPrice& Price : Listing->Prices)
    {
        if (YIShopPrivate::GetResourceAmount(ResourceProvider, Price.Resource) < Price.Amount * Count)
        {
            return false;
        }
    }
    // Consume
    for (const FYIShopPrice& Price : Listing->Prices)
    {
        YIShopPrivate::ConsumeResource(ResourceProvider, Price.Resource, Price.Amount * Count);
    }
    return true;
}

void UYIShopComponent::ServerBuyItem_Implementation(int32 StockIndex, int32 Count, UYIInventoryComponent* BuyerInv, FIntPoint DestPos)
{
    FYIShopBuyRequest Request;
    Request.Shop = this;
    Request.StockIndex = StockIndex;
    Request.Count = Count;
    Request.BuyerInv = BuyerInv;
    Request.DestPos = DestPos;

    FYIShopOpResult Unused;
    ExecuteBuyRequest(Request, Unused);
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
	if (!StockBag->Items.IsValidIndex(Request.StockIndex))
	{
		OnShopPurchase.Broadcast(BuyerPS, 0, Request.Count, false);
		OutResult.Error = EYIShopOpError::InvalidStockIndex;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_InvalidIndex", "Invalid stock index");
		return false;
	}

	FYIBagItem& StockItem = StockBag->Items[Request.StockIndex];
	if (StockItem.Item.Count < Request.Count)
	{
		OnShopPurchase.Broadcast(BuyerPS, 0, Request.Count, false);
		OutResult.Error = EYIShopOpError::NotEnoughStock;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Buy_NotEnough", "Not enough stock");
		return false;
	}

	const int64 Code = StockItem.Item.Definition.IsValid() ? StockItem.Item.Definition.Get()->UniqueCode : 0;
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

	if (!ConsumePrice(BuyerResourceProvider, Code, Request.Count))
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
			if (const FYIShopListing* Listing = FindListing(Code))
			{
				for (const FYIShopPrice& Price : Listing->Prices)
				{
					YIShopPrivate::AddResource(BuyerResourceProvider, Price.Resource, Price.Amount * Request.Count);
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
		StockBag->RemoveItem(Request.StockIndex);
	}
	Request.BuyerInv->SyncNetState();
	if (bAutoSortStock && StockBag)
	{
		StockBag->AutoPack();
	}

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

bool UYIShopComponent::ServerBuyItem_Validate(int32 StockIndex, int32 Count, UYIInventoryComponent* BuyerInv, FIntPoint DestPos)
{
    return true; // lightweight; implementation does full checks
}

void UYIShopComponent::ServerSellItem_Implementation(int32 SourceIndex, int32 Count, UYIInventoryComponent* SellerInv)
{
    FYIShopSellRequest Request;
    Request.Shop = this;
    Request.SourceIndex = SourceIndex;
    Request.Count = Count;
    Request.SellerInv = SellerInv;

    FYIShopOpResult Unused;
    ExecuteSellRequest(Request, Unused);
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
	if (!SellerBag || !SellerBag->Items.IsValidIndex(Request.SourceIndex))
	{
		OnShopSale.Broadcast(SellerPS, 0, Request.Count, false);
		OutResult.Error = EYIShopOpError::InvalidSourceIndex;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_InvalidIndex", "Invalid inventory index");
		return false;
	}

	FYIBagItem OriginalItem = SellerBag->Items[Request.SourceIndex];
	if (OriginalItem.Item.Count < Request.Count)
	{
		OnShopSale.Broadcast(SellerPS, 0, Request.Count, false);
		OutResult.Error = EYIShopOpError::InvalidCount;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_NotEnough", "Not enough items to sell");
		return false;
	}

	const int64 Code = OriginalItem.Item.Definition.IsValid() ? OriginalItem.Item.Definition.Get()->UniqueCode : 0;
	const FYIShopListing* Listing = FindListing(Code);
	if (!Listing && !bAllowSellingUnlisted)
	{
		OnShopSale.Broadcast(SellerPS, Code, Request.Count, false);
		OutResult.Error = EYIShopOpError::UnlistedNotAllowed;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_Unlisted", "Item cannot be sold here");
		return false;
	}

	if (OriginalItem.Item.Count == Request.Count)
	{
		if (!SellerBag->RemoveItem(Request.SourceIndex))
		{
			OutResult.Error = EYIShopOpError::ValidationFailed;
			OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_RemoveFail", "Failed to remove item from inventory");
			return false;
		}
	}
	else
	{
		SellerBag->Items[Request.SourceIndex].Item.Count -= Request.Count;
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
			SellerBag->Items[Request.SourceIndex].Item.Count += Request.Count;
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
			SellerBag->Items[Request.SourceIndex].Item.Count += Request.Count;
			SellerBag->OnChanged.Broadcast();
		}
		OnShopSale.Broadcast(SellerPS, Code, Request.Count, false);
		OutResult.Error = EYIShopOpError::NoSpace;
		OutResult.Message = NSLOCTEXT("YOLOInventory", "Shop_Sell_NoSpace", "Shop stock is full");
		return false;
	}

	if (SellerResourceProvider && Listing)
	{
		for (const FYIShopPrice& Price : Listing->Prices)
		{
			const int64 Pay = static_cast<int64>(Price.Amount * Request.Count * SellPriceMultiplier);
			if (Pay > 0)
			{
				YIShopPrivate::AddResource(SellerResourceProvider, Price.Resource, Pay);
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

bool UYIShopComponent::ServerSellItem_Validate(int32 SourceIndex, int32 Count, UYIInventoryComponent* SellerInv)
{
    return true;
}

void UYIShopComponent::RefreshMirror()
{
    if (!GetOwner() || !GetOwner()->HasAuthority()) return;
    StockMirror.Reset();
    StockMirrorSize = FIntPoint(8,6);
    if (RuntimeStock)
    {
        GetStockMirrorForBag(RuntimeStock, StockMirror, StockMirrorSize);
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

void UYIShopComponent::GetStockMirrorForBag(const UYIInventoryBag* Bag, TArray<FYINetBagItem>& OutItems, FIntPoint& OutSize) const
{
    OutItems.Reset();
    OutSize = FIntPoint(8,6);
    if (!Bag) return;

    OutSize = Bag->GridSize;
    for (const FYIBagItem& It : Bag->Items)
    {
        if (It.Item.Count <= 0) continue;
        FYINetBagItem Net;
        Net.Code = It.Item.Definition.IsValid() ? It.Item.Definition.Get()->UniqueCode : 0;
        if (Net.Code == 0 && It.Item.Definition.ToSoftObjectPath().IsValid())
        {
            if (UYIItemDefinition* Def = Cast<UYIItemDefinition>(It.Item.Definition.LoadSynchronous()))
            {
                Net.Code = Def->UniqueCode;
            }
        }
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
	GetStockMirrorForBag(StockBag, OutItems, OutSize);
}
