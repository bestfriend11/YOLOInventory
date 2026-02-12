#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class IDetailsView;

enum class EYIGeneratorDashboardLayout : uint8
{
	Full,
	AssetListOnly
};

class SYIGeneratorDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYIGeneratorDashboard)
		: _LayoutMode(EYIGeneratorDashboardLayout::Full)
	{}
		SLATE_ARGUMENT(EYIGeneratorDashboardLayout, LayoutMode)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void OpenAsset(UObject* Asset);
	TSharedRef<class SWidget> GetAssetPanelWidget() const;
	TSharedRef<class SWidget> GetDetailsPanelWidget() const;
	TSharedRef<class SWidget> GetTestPanelWidget() const;

	void CreateLootTableFromToolbar();
	void CreateRarityProfileFromToolbar();
	void CreateItemGeneratorFromToolbar();
	void RunGeneratorTestFromToolbar();

private:
	TSharedRef<class SWidget> BuildAssetPicker();
	TSharedRef<class SWidget> BuildDetailsPanelWidget();
	TSharedRef<class SWidget> BuildTestPanelWidget();
	void OnAssetSelected(const FAssetData& AssetData);
	void OnAssetDoubleClicked(const FAssetData& AssetData);

	FReply CreateLootTable();
	FReply CreateRarityProfile();
	FReply CreateItemGenerator();
	FReply RunGeneratorTest();

	TSharedPtr<IDetailsView> DetailsView;
	TWeakObjectPtr<UObject> SelectedAsset;
	EYIGeneratorDashboardLayout LayoutMode = EYIGeneratorDashboardLayout::Full;
	TSharedPtr<class SWidget> AssetPanelWidget;
	TSharedPtr<class SWidget> DetailsPanelWidget;
	TSharedPtr<class SWidget> TestPanelWidget;
	int32 TestLevel = 1;
	int32 TestSeed = 1;
	int32 TestRuns = 1;
	FText TestResult;
};
