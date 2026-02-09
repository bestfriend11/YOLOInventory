#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class IDetailsView;

class SYIGeneratorDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYIGeneratorDashboard) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void OpenAsset(UObject* Asset);

private:
	TSharedRef<class SWidget> BuildAssetPicker();
	void OnAssetSelected(const FAssetData& AssetData);
	void OnAssetDoubleClicked(const FAssetData& AssetData);

	FReply CreateLootTable();
	FReply CreateRarityProfile();
	FReply CreateItemGenerator();
	FReply RunGeneratorTest();

	TSharedPtr<IDetailsView> DetailsView;
	TWeakObjectPtr<UObject> SelectedAsset;
	int32 TestLevel = 1;
	int32 TestSeed = 1;
	FText TestResult;
};
