#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Widgets/SCompoundWidget.h"

class UYIInventoryBag;
class UYIItemDefinition;
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

	void SaveCurrentBagFromToolbar();
	void CreateBagFromToolbar();
	void ApplyRuntimeSpellbookPresetFromToolbar();
	void ValidateRuntimeSetupFromToolbar();

	void SetSelectedBag(UYIInventoryBag* InBag);
	UYIInventoryBag* GetSelectedBag() const;

private:
	void RebuildBagView();
	TSharedRef<class SWidget> BuildBagAssetPicker();
	TSharedRef<class SWidget> BuildItemAssetPicker();
	FReply SaveCurrentBag();
	FReply CreateNewBag();

	TWeakObjectPtr<UYIInventoryBag> SelectedBag;
	TWeakObjectPtr<UYIItemDefinition> SelectedPaletteItem;
	TSharedPtr<SBagEditor> GridWidget;
	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<class SBox> GridHost;
	TSharedPtr<class STextBlock> StatusTextWidget;
	FText StatusText;
	FGameplayTag RuntimeSpellbookSlotTag;
	int32 RuntimeSpellbookActionSlotIndex = 0;
};
