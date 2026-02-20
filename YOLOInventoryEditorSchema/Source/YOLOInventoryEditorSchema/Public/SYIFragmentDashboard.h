#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class IDetailsView;
class UYIAffixAsset;
class UYIFragmentAsset;

enum class EYIFragmentDashboardLayout : uint8
{
	Full,
	AssetListOnly
};

class YOLOINVENTORYEDITORSCHEMA_API SYIFragmentDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYIFragmentDashboard)
		: _LayoutMode(EYIFragmentDashboardLayout::Full)
	{}
		SLATE_ARGUMENT(EYIFragmentDashboardLayout, LayoutMode)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void OpenAsset(UObject* Asset);

	TSharedRef<SWidget> GetAssetPanelWidget() const;
	TSharedRef<SWidget> GetDetailsPanelWidget() const;
	TSharedRef<SWidget> GetMappingPanelWidget() const;
	TSharedRef<SWidget> GetPreviewPanelWidget() const;

	void SaveCurrentAssetFromToolbar();

private:
	TSharedRef<SWidget> BuildAssetPanelWidget();
	TSharedRef<SWidget> BuildDetailsPanelWidget();
	TSharedRef<SWidget> BuildMappingPanelWidget();
	TSharedRef<SWidget> BuildPreviewPanelWidget();
	void SetSelectedAsset(UObject* InAsset);
	FText BuildFragmentSummaryText() const;

private:
	EYIFragmentDashboardLayout LayoutMode = EYIFragmentDashboardLayout::Full;
	TWeakObjectPtr<UObject> SelectedAsset;
	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<SWidget> AssetPanelWidget;
	TSharedPtr<SWidget> DetailsPanelWidget;
	TSharedPtr<SWidget> MappingPanelWidget;
	TSharedPtr<SWidget> PreviewPanelWidget;
};
