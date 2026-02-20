#pragma once

#include "CoreMinimal.h"

class SWidget;
class UObject;
class UYIInventoryBag;

class YOLOINVENTORYEDITORCORE_API IYISchemaDashboardBridge
{
public:
	virtual ~IYISchemaDashboardBridge() = default;

	virtual TSharedRef<SWidget> GetItemsRootWidget() = 0;
	virtual TSharedRef<SWidget> GetFragmentsRootWidget() = 0;
	virtual TSharedRef<SWidget> GetCraftingRootWidget() = 0;

	virtual TSharedRef<SWidget> GetItemDetailsPanelWidget() const = 0;
	virtual TSharedRef<SWidget> GetItemMappingsPanelWidget() const = 0;
	virtual TSharedRef<SWidget> GetItemPreviewPanelWidget() const = 0;
	virtual TSharedRef<SWidget> GetItemPreflightPanelWidget() const = 0;
	virtual TSharedRef<SWidget> GetItemDiffPanelWidget() const = 0;
	virtual TSharedRef<SWidget> GetItemLogsPanelWidget() const = 0;

	virtual TSharedRef<SWidget> GetFragmentDetailsPanelWidget() const = 0;
	virtual TSharedRef<SWidget> GetFragmentMappingsPanelWidget() const = 0;
	virtual TSharedRef<SWidget> GetFragmentPreviewPanelWidget() const = 0;

	virtual void OpenAssetInItems(UObject* Asset) = 0;
	virtual void OpenAssetInFragments(UObject* Asset) = 0;
	virtual void OpenAssetInCrafting(UObject* Asset) = 0;

	virtual void SaveItemsFromToolbar() = 0;
	virtual void SaveFragmentsFromToolbar() = 0;
	virtual void SaveCraftingFromToolbar() = 0;
	virtual void CreateItemDataSourceFromToolbar() = 0;

	virtual void SetCraftingTargetBag(UYIInventoryBag* InBag) = 0;
	virtual UYIInventoryBag* GetCraftingTargetBag() const = 0;
};

DECLARE_DELEGATE_RetVal(TSharedRef<IYISchemaDashboardBridge>, FYICreateSchemaDashboardBridge);
