#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class IDetailsView;

class SYIAffixDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYIAffixDashboard) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<class SWidget> BuildAssetPicker();
	void OnAssetSelected(const FAssetData& AssetData);
	void OnAssetDoubleClicked(const FAssetData& AssetData);

	FReply CreateAffix();
	FReply CreateAffixPool();

	TSharedPtr<IDetailsView> DetailsView;
};
