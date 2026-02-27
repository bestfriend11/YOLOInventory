#pragma once

#include "CoreMinimal.h"
#include "YIShopApiTypes.generated.h"

class UYIInventoryComponent;
class UYIShopComponent;

UENUM(BlueprintType)
enum class EYIShopOpError : uint8
{
	None UMETA(DisplayName="None"),
	InvalidRequest UMETA(DisplayName="Invalid Request"),
	InvalidShop UMETA(DisplayName="Invalid Shop"),
	InvalidInventory UMETA(DisplayName="Invalid Inventory"),
	InvalidOwner UMETA(DisplayName="Invalid Owner"),
	InvalidStockIndex UMETA(DisplayName="Invalid Stock Index"),
	InvalidSourceIndex UMETA(DisplayName="Invalid Source Index"),
	InvalidCount UMETA(DisplayName="Invalid Count"),
	TooFar UMETA(DisplayName="Too Far"),
	NoStock UMETA(DisplayName="No Stock"),
	NotEnoughStock UMETA(DisplayName="Not Enough Stock"),
	NoFunds UMETA(DisplayName="No Funds"),
	NoSpace UMETA(DisplayName="No Space"),
	UnlistedNotAllowed UMETA(DisplayName="Unlisted Not Allowed"),
	NotVisible UMETA(DisplayName="Not Visible"),
	NotBuyable UMETA(DisplayName="Not Buyable"),
	NotSellable UMETA(DisplayName="Not Sellable"),
	PriceUnavailable UMETA(DisplayName="Price Unavailable"),
	ValidationFailed UMETA(DisplayName="Validation Failed"),
	AuthorityRequired UMETA(DisplayName="Authority Required"),
	Unsupported UMETA(DisplayName="Unsupported")
};

UENUM(BlueprintType)
enum class EYIShopOpKind : uint8
{
	Unknown UMETA(DisplayName="Unknown"),
	Open UMETA(DisplayName="Open"),
	Buy UMETA(DisplayName="Buy"),
	Sell UMETA(DisplayName="Sell"),
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYSHOP_API FYIShopOpResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	bool bRequestAccepted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	EYIShopOpError Error = EYIShopOpError::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	EYIShopOpKind OpKind = EYIShopOpKind::Unknown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	FText Message;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	TObjectPtr<UYIShopComponent> Shop = nullptr;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYSHOP_API FYIShopOpenRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	TObjectPtr<UYIShopComponent> Shop = nullptr;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYSHOP_API FYIShopBuyRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	TObjectPtr<UYIShopComponent> Shop = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	int32 StockIndex = INDEX_NONE;

	/** Stable stock identity (preferred over StockIndex when set). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	FGuid StockItemInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	TObjectPtr<UYIInventoryComponent> BuyerInv = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	FIntPoint DestPos = FIntPoint(-1, -1);
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYSHOP_API FYIShopSellRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	TObjectPtr<UYIShopComponent> Shop = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	int32 SourceIndex = INDEX_NONE;

	/** Stable source identity (preferred over SourceIndex when set). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	FGuid SourceItemInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	int32 Count = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shop")
	TObjectPtr<UYIInventoryComponent> SellerInv = nullptr;
};
