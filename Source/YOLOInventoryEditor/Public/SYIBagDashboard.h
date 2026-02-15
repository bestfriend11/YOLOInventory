#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SCompoundWidget.h"
#include "YIBagItemDetailsProxy.h"

class UYIInventoryBag;
class UYIItemDefinition;
class UYIEquipmentSchemaAsset;
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
	void ApplyRuntimeSpellbookPresetFromToolbar();
	void ValidateRuntimeSetupFromToolbar();

	void SetSelectedBag(UYIInventoryBag* InBag);
	UYIInventoryBag* GetSelectedBag() const;

private:
	void HandleGridSelectionChanged(int32 SelectedIndex);
	void RebuildBagView();
	TSharedRef<class SWidget> BuildBagAssetPicker();
	TSharedRef<class SWidget> BuildItemAssetPicker();
	TSharedRef<class SWidget> BuildEquipmentSchemaPicker();
	FReply SaveCurrentBag();
	FReply CreateNewBag();
	FReply SaveCurrentEquipmentSchema();
	FReply CreateNewEquipmentSchema();
	void SetSelectedEquipmentSchema(UYIEquipmentSchemaAsset* InSchema);
	UYIEquipmentSchemaAsset* GetSelectedEquipmentSchema() const;

	TWeakObjectPtr<UYIInventoryBag> SelectedBag;
	TWeakObjectPtr<UYIItemDefinition> SelectedPaletteItem;
	TWeakObjectPtr<UYIEquipmentSchemaAsset> SelectedEquipmentSchema;
	TStrongObjectPtr<UYIBagItemDetailsProxy> SelectedBagItemProxy;
	TSharedPtr<SBagEditor> GridWidget;
	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<IDetailsView> EquipmentLayoutDetailsView;
	TSharedPtr<class SBox> GridHost;
	TSharedPtr<class SWidget> EquipmentLayoutPanelWidget;
	TSharedPtr<class STextBlock> StatusTextWidget;
	FText StatusText;
	FGameplayTag RuntimeSpellbookSlotTag;
	int32 RuntimeSpellbookActionSlotIndex = 0;
};
