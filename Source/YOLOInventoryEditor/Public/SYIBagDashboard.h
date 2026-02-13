#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Widgets/SCompoundWidget.h"

class UYIInventoryBag;
class UYIItemDefinition;
class UYIEquipmentLayoutAsset;
class SBagEditor;
class IDetailsView;

class SYIBagDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYIBagDashboard) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void OpenAsset(UObject* Asset);
	TSharedRef<class SWidget> GetDetailsPanelWidget() const;
	TSharedRef<class SWidget> GetEquipmentLayoutPanelWidget() const;

	void SaveCurrentBagFromToolbar();
	void CreateBagFromToolbar();
	void SaveCurrentEquipmentLayoutFromToolbar();
	void CreateEquipmentLayoutFromToolbar();
	void RefreshEquipmentLayoutPreviewFromToolbar();
	void ApplyRuntimeSpellbookPresetFromToolbar();
	void ValidateRuntimeSetupFromToolbar();

	void SetSelectedBag(UYIInventoryBag* InBag);
	UYIInventoryBag* GetSelectedBag() const;

private:
	void RebuildBagView();
	void RebuildEquipmentLayoutPreview();
	TSharedRef<class SWidget> BuildBagAssetPicker();
	TSharedRef<class SWidget> BuildItemAssetPicker();
	TSharedRef<class SWidget> BuildEquipmentLayoutPicker();
	FReply SaveCurrentBag();
	FReply CreateNewBag();
	FReply SaveCurrentEquipmentLayout();
	FReply CreateNewEquipmentLayout();
	void SetSelectedEquipmentLayout(UYIEquipmentLayoutAsset* InLayout);
	UYIEquipmentLayoutAsset* GetSelectedEquipmentLayout() const;

	TWeakObjectPtr<UYIInventoryBag> SelectedBag;
	TWeakObjectPtr<UYIItemDefinition> SelectedPaletteItem;
	TWeakObjectPtr<UYIEquipmentLayoutAsset> SelectedEquipmentLayout;
	TSharedPtr<SBagEditor> GridWidget;
	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<IDetailsView> EquipmentLayoutDetailsView;
	TSharedPtr<class SBox> GridHost;
	TSharedPtr<class SBox> EquipmentLayoutPreviewHost;
	TSharedPtr<class SBox> EquipmentLayoutDockPreviewHost;
	TSharedPtr<class SWidget> EquipmentLayoutPanelWidget;
	TSharedPtr<class STextBlock> StatusTextWidget;
	FText StatusText;
	FGameplayTag RuntimeSpellbookSlotTag;
	int32 RuntimeSpellbookActionSlotIndex = 0;
};
