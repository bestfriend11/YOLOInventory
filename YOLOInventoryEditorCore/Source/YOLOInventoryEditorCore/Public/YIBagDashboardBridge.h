#pragma once

#include "CoreMinimal.h"

class SWidget;
class UObject;
class UYIInventoryBag;

class YOLOINVENTORYEDITORCORE_API IYIBagDashboardBridge
{
public:
	virtual ~IYIBagDashboardBridge() = default;

	virtual TSharedRef<SWidget> GetRootWidget() = 0;
	virtual TSharedRef<SWidget> GetDetailsPanelWidget() const = 0;
	virtual TSharedRef<SWidget> GetEquipmentLayoutPanelWidget() const = 0;

	virtual void OpenAsset(UObject* Asset) = 0;
	virtual void SaveCurrentBagFromToolbar() = 0;
	virtual void SaveCurrentEquipmentLayoutFromToolbar() = 0;
	virtual void SetSelectedBag(UYIInventoryBag* InBag) = 0;
	virtual UYIInventoryBag* GetSelectedBag() const = 0;
};

DECLARE_DELEGATE_RetVal(TSharedRef<IYIBagDashboardBridge>, FYICreateBagDashboardBridge);

