#pragma once

#include "CoreMinimal.h"
#include "YIInventoryGridFeatureAdapter.h"
#include "YIInventoryGridGameplayAdapter.generated.h"

class AYITradeSessionActor;
class UYIShopComponent;
enum class ETradeSide : uint8;
struct FYITooltipData;

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

	virtual EYIInventoryGridExternalOpResult TryHandleTransferRequest(
		const FYIInventoryGridTransferRequest& Request,
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

	virtual void AugmentTooltipData(
		UInventoryGridWidget* Grid,
		const UYIInventoryBag* Bag,
		int32 ItemIndex,
		FYITooltipData& InOutTooltipData) override;

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
