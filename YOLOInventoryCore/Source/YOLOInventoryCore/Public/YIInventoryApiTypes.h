#pragma once

#include "CoreMinimal.h"
#include "YIInventoryCoreTypes.h"
#include "YIInventoryApiTypes.generated.h"

/**
 * Standardized inventory operation error codes for suite-wide command APIs.
 * Use these for UI feedback, telemetry, and server validation reporting.
 */
UENUM(BlueprintType)
enum class EYIInventoryOpError : uint8
{
	None UMETA(DisplayName="None"),
	InvalidRequest UMETA(DisplayName="Invalid Request"),
	InvalidRef UMETA(DisplayName="Invalid Ref"),
	BagNotFound UMETA(DisplayName="Bag Not Found"),
	ItemNotFound UMETA(DisplayName="Item Not Found"),
	Locked UMETA(DisplayName="Locked"),
	NoSpace UMETA(DisplayName="No Space"),
	Blocked UMETA(DisplayName="Blocked"),
	SwapNotAllowed UMETA(DisplayName="Swap Not Allowed"),
	CycleDetected UMETA(DisplayName="Cycle Detected"),
	RevisionMismatch UMETA(DisplayName="Revision Mismatch"),
	ValidationFailed UMETA(DisplayName="Validation Failed"),
	Unsupported UMETA(DisplayName="Unsupported"),
};

UENUM(BlueprintType)
enum class EYIInventoryOpKind : uint8
{
	Unknown UMETA(DisplayName="Unknown"),
	Move UMETA(DisplayName="Move"),
	Rotate UMETA(DisplayName="Rotate"),
	Remove UMETA(DisplayName="Remove"),
	Transfer UMETA(DisplayName="Transfer"),
	Split UMETA(DisplayName="Split"),
	Combine UMETA(DisplayName="Combine"),
};

/**
 * Standard result envelope for inventory commands.
 * bRequestAccepted indicates whether the request was accepted locally (or queued to server).
 * bSucceeded is authoritative success when executed locally on authority; clients usually get optimistic acceptance and later reconcile via replication.
 */
USTRUCT(BlueprintType)
struct YOLOINVENTORYCORE_API FYIInventoryOpResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bRequestAccepted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	EYIInventoryOpError Error = EYIInventoryOpError::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	EYIInventoryOpKind OpKind = EYIInventoryOpKind::Unknown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid TransactionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid AffectedBagId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid AffectedItemInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 SourceBagRevision = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 DestBagRevision = INDEX_NONE;
};

/**
 * Request: move an item within its current bag.
 * If bUseExactCell is false, move follows container topology move semantics.
 * If bUseExactCell is true, the implementation may perform exact-cell move/swap behavior.
 */
USTRUCT(BlueprintType)
struct YOLOINVENTORYCORE_API FYIInventoryMoveItemRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FYIInventoryItemRef ItemRef;

	/** Optional client-generated correlation id; auto-filled by Request* wrappers when empty. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FIntPoint TargetCell = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bUseExactCell = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bAllowSingleOverlapSwap = true;

	/** Optional optimistic concurrency check. INDEX_NONE disables checking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 ExpectedSourceBagRevision = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYCORE_API FYIInventoryRotateItemRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FYIInventoryItemRef ItemRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 ExpectedSourceBagRevision = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYCORE_API FYIInventoryRemoveItemRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FYIInventoryItemRef ItemRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 ExpectedSourceBagRevision = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYCORE_API FYIInventoryTransferItemRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FYIInventoryItemRef ItemRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid DestBagId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bUseExactCell = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FIntPoint DestCell = FIntPoint::ZeroValue;

	/** <=0 means whole stack/item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bAllowSingleOverlapSwap = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 ExpectedSourceBagRevision = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 ExpectedDestBagRevision = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYCORE_API FYIInventorySplitStackRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FYIInventoryItemRef ItemRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 Amount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FIntPoint DesiredPos = FIntPoint(-1, -1);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 ExpectedSourceBagRevision = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYCORE_API FYIInventoryCombineItemRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FYIInventoryItemRef ItemRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	int32 ExpectedSourceBagRevision = INDEX_NONE;
};
