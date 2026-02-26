#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "YIInventoryBag.h"
#include "YIInventoryGridFeatureAdapter.generated.h"

class UInventoryGridWidget;
class UYIInventoryComponent;

UENUM(BlueprintType)
enum class EYIInventoryGridExternalOpResult : uint8
{
	NotHandled,
	HandledFailed,
	HandledSucceeded
};

/**
 * Optional adapter for feature-specific grid integrations (trade/shop/equip/world).
 * Grid runtime depends only on this abstraction and core inventory types.
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class YOLOINVENTORYGRID_API UYIInventoryGridFeatureAdapter : public UObject
{
	GENERATED_BODY()
public:
	virtual void OnAssignedToGrid(UInventoryGridWidget* InGrid) {}

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
};

