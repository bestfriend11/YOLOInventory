#pragma once

#include "CoreMinimal.h"
#include "YIInventoryGridFeatureAdapter.h"
#include "YIInventoryGridGameplayAdapter.generated.h"

class AYITradeSessionActor;
class UYIShopComponent;
enum class ETradeSide : uint8;

/**
 * Default gameplay integration adapter for grid widgets.
 * Keeps grid runtime plugin decoupled from trade/shop/equipment/world modules.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class YOLOINVENTORYUI_API UYIInventoryGridGameplayAdapter : public UYIInventoryGridFeatureAdapter
{
	GENERATED_BODY()
public:
	void SetTradeSession(AYITradeSessionActor* InSession);
	void SetTradeContext(AYITradeSessionActor* InSession, ETradeSide InSide);
	void SetShopContext(UYIShopComponent* InShop, bool bStockGrid);

	virtual EYIInventoryGridExternalOpResult TryHandleCrossGridDrop(
		UInventoryGridWidget* DestGrid,
		UInventoryGridWidget* SourceGrid,
		int32 SourceIndex,
		const FYIBagItem& DraggedItem,
		const FIntPoint& DestCell) override;

	virtual EYIInventoryGridExternalOpResult TryHandleTransferSelectedTo(
		UInventoryGridWidget* SourceGrid,
		UInventoryGridWidget* DestGrid,
		int32 SourceIndex,
		int32 Count,
		int32& OutDestIndex) override;

	virtual bool TryEquipItemFromInventory(
		UObject* EquipmentContextObject,
		class UYIInventoryComponent* SourceInventory,
		int32 SourceIndex,
		FGameplayTag RequestedSlotTag) override;

	virtual bool TrySpawnWorldDropFromInstance(
		UObject* WorldContextObject,
		const FYIItemInstance& Item,
		const FTransform& SpawnTransform) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<AYITradeSessionActor> ActiveTradeSession = nullptr;

	UPROPERTY(Transient)
	ETradeSide TradeSide = static_cast<ETradeSide>(0);

	UPROPERTY(Transient)
	bool bHasTradeSide = false;

	UPROPERTY(Transient)
	TObjectPtr<UYIShopComponent> ActiveShopComponent = nullptr;

	UPROPERTY(Transient)
	bool bIsShopStockGrid = false;
};

