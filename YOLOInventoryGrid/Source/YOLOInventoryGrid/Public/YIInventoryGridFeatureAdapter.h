#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "UObject/Object.h"
#include "YIInventoryBag.h"
#include "YIInventoryGridFeatureAdapter.generated.h"

class UInventoryGridWidget;
class UYIInventoryComponent;
struct FYITooltipData;

UENUM(BlueprintType)
enum class EYIInventoryGridExternalOpResult : uint8
{
	NotHandled,
	HandledFailed,
	HandledSucceeded
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYGRID_API FYIInventoryGridTransferRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Grid")
	TObjectPtr<UInventoryGridWidget> SourceGrid = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Grid")
	TObjectPtr<UInventoryGridWidget> DestGrid = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Grid")
	int32 SourceIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Grid")
	FYIBagItem Item;

	/** <=0 means whole stack/item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Grid")
	int32 Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Grid")
	bool bIsDragDrop = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Grid")
	bool bHasDestCell = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory|Grid")
	FIntPoint DestCell = FIntPoint::ZeroValue;
};

UINTERFACE(BlueprintType)
class YOLOINVENTORYGRID_API UYIInventoryGridAdapterInterface : public UInterface
{
	GENERATED_BODY()
};

class YOLOINVENTORYGRID_API IYIInventoryGridAdapterInterface
{
	GENERATED_BODY()
public:
	virtual void OnAssignedToGrid(UInventoryGridWidget* InGrid) {}

	/**
	 * Unified external transfer request used by both drag-drop and menu/keyboard transfer flows.
	 * Default behavior dispatches to legacy specialized hooks for backward compatibility.
	 */
	virtual EYIInventoryGridExternalOpResult TryHandleTransferRequest(
		const FYIInventoryGridTransferRequest& Request,
		int32& OutDestIndex)
	{
		if (Request.bIsDragDrop && Request.bHasDestCell)
		{
			return TryHandleCrossGridDrop(
				Request.DestGrid,
				Request.SourceGrid,
				Request.SourceIndex,
				Request.Item,
				Request.DestCell);
		}

		return TryHandleTransferSelectedTo(
			Request.SourceGrid,
			Request.DestGrid,
			Request.SourceIndex,
			Request.Count,
			OutDestIndex);
	}

	/** Handle special cross-grid drops (shop/trade/etc). Return NotHandled to fall back to core bag transfer logic. */
	virtual EYIInventoryGridExternalOpResult TryHandleCrossGridDrop(
		UInventoryGridWidget* DestGrid,
		UInventoryGridWidget* SourceGrid,
		int32 SourceIndex,
		const FYIBagItem& DraggedItem,
		const FIntPoint& DestCell)
	{
		return EYIInventoryGridExternalOpResult::NotHandled;
	}

	/** Handle non-grid transfer requests (menu-driven, keyboard, etc) that are not plain bag-to-bag transfers. */
	virtual EYIInventoryGridExternalOpResult TryHandleTransferSelectedTo(
		UInventoryGridWidget* SourceGrid,
		UInventoryGridWidget* DestGrid,
		int32 SourceIndex,
		int32 Count,
		int32& OutDestIndex)
	{
		return EYIInventoryGridExternalOpResult::NotHandled;
	}

	/** Execute equip action using external equipment system. */
	virtual bool TryEquipItemFromInventory(
		UObject* EquipmentContextObject,
		UYIInventoryComponent* SourceInventory,
		int32 SourceIndex,
		FGameplayTag RequestedSlotTag)
	{
		return false;
	}

	/** Optional world-drop fallback used when detached drag restore fails and grid wants to avoid item loss. */
	virtual bool TrySpawnWorldDropFromInstance(
		UObject* WorldContextObject,
		const FYIItemInstance& Item,
		const FTransform& SpawnTransform)
	{
		return false;
	}

	/** Optional tooltip augmentation hook for feature-specific price/status lines. */
	virtual void AugmentTooltipData(
		UInventoryGridWidget* Grid,
		const UYIInventoryBag* Bag,
		int32 ItemIndex,
		FYITooltipData& InOutTooltipData)
	{
	}
};

/**
 * Optional adapter for feature-specific grid integrations (trade/shop/equip/world).
 * Grid runtime depends only on this abstraction and core inventory types.
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class YOLOINVENTORYGRID_API UYIInventoryGridFeatureAdapter : public UObject, public IYIInventoryGridAdapterInterface
{
	GENERATED_BODY()
public:
	virtual void OnAssignedToGrid(UInventoryGridWidget* InGrid) override {}

	virtual EYIInventoryGridExternalOpResult TryHandleTransferRequest(
		const FYIInventoryGridTransferRequest& Request,
		int32& OutDestIndex) override
	{
		if (Request.bIsDragDrop && Request.bHasDestCell)
		{
			return TryHandleCrossGridDrop(
				Request.DestGrid,
				Request.SourceGrid,
				Request.SourceIndex,
				Request.Item,
				Request.DestCell);
		}

		return TryHandleTransferSelectedTo(
			Request.SourceGrid,
			Request.DestGrid,
			Request.SourceIndex,
			Request.Count,
			OutDestIndex);
	}

	/** Handle special cross-grid drops (shop/trade/etc). Return NotHandled to fall back to core bag transfer logic. */
	virtual EYIInventoryGridExternalOpResult TryHandleCrossGridDrop(
		UInventoryGridWidget* DestGrid,
		UInventoryGridWidget* SourceGrid,
		int32 SourceIndex,
		const FYIBagItem& DraggedItem,
		const FIntPoint& DestCell) override
	{
		return EYIInventoryGridExternalOpResult::NotHandled;
	}

	/** Handle non-grid transfer requests (menu-driven, keyboard, etc) that are not plain bag-to-bag transfers. */
	virtual EYIInventoryGridExternalOpResult TryHandleTransferSelectedTo(
		UInventoryGridWidget* SourceGrid,
		UInventoryGridWidget* DestGrid,
		int32 SourceIndex,
		int32 Count,
		int32& OutDestIndex) override
	{
		return EYIInventoryGridExternalOpResult::NotHandled;
	}

	/** Execute equip action using external equipment system. */
	virtual bool TryEquipItemFromInventory(
		UObject* EquipmentContextObject,
		UYIInventoryComponent* SourceInventory,
		int32 SourceIndex,
		FGameplayTag RequestedSlotTag) override
	{
		return false;
	}

	/** Optional world-drop fallback used when detached drag restore fails and grid wants to avoid item loss. */
	virtual bool TrySpawnWorldDropFromInstance(
		UObject* WorldContextObject,
		const FYIItemInstance& Item,
		const FTransform& SpawnTransform) override
	{
		return false;
	}

	virtual void AugmentTooltipData(
		UInventoryGridWidget* Grid,
		const UYIInventoryBag* Bag,
		int32 ItemIndex,
		FYITooltipData& InOutTooltipData) override
	{
	}
};
