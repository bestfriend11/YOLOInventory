#pragma once

#include "CoreMinimal.h"

class SWidget;
class UObject;

class YOLOINVENTORYEDITORCORE_API IYIGeneratorDashboardBridge
{
public:
	virtual ~IYIGeneratorDashboardBridge() = default;

	virtual TSharedRef<SWidget> GetRootWidget() = 0;
	virtual TSharedRef<SWidget> GetDetailsPanelWidget() const = 0;
	virtual TSharedRef<SWidget> GetTestPanelWidget() const = 0;

	virtual void OpenAsset(UObject* Asset) = 0;
	virtual void SaveCurrentAssetFromToolbar() = 0;
};

DECLARE_DELEGATE_RetVal(TSharedRef<IYIGeneratorDashboardBridge>, FYICreateGeneratorDashboardBridge);
