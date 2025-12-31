#pragma once
#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Containers/ArrayView.h"
#include "AssetRegistry/AssetData.h"

class UYIItemDefinition;

class FYIItemDefinitionEditor : public FAssetEditorToolkit
{
public:
	void Init(UYIItemDefinition* InAsset, const TSharedPtr<class IToolkitHost>& EditWithinLevelEditor);

	// FAssetEditorToolkit
	virtual FName GetToolkitFName() const override { return FName("YIItemDefinitionEditor"); }
	virtual FText GetBaseToolkitName() const override { return NSLOCTEXT("YOLOInventory", "ItemDefEditor", "Item Definition Editor"); }
	virtual FString GetWorldCentricTabPrefix() const override { return TEXT("ItemDef"); }
	virtual FLinearColor GetWorldCentricTabColorScale() const override { return FLinearColor(0.2f,0.4f,0.8f,1.f); }

	TSharedPtr<FTabManager> GetTabManagerPtr() { return GetTabManager(); }

private:
	TSharedRef<class SDockTab> SpawnMainTab(const class FSpawnTabArgs& Args);
	TSharedRef<class SWidget> BuildPreviewWidget();
	TSharedRef<class SWidget> BuildItemCard();

	// Mods UI builders
	TSharedRef<class SWidget> BuildModsList();
	TSharedRef<class SWidget> BuildLeftPanel();

	// Removed legacy Item Attribute Keywords UI
	void RefreshAttributeKeywordsList_REMOVED();
	FReply OnClearAttributesClicked_REMOVED();
	FReply OnAttrDrop_REMOVED(const FGeometry& Geo, const FDragDropEvent& Evt);

	// Mods drag-drop handlers and UI refresh
	bool OnModsDragOver(const FGeometry& Geo, const FDragDropEvent& Evt);
	FReply OnModsDrop(const FGeometry& Geo, const FDragDropEvent& Evt);
void RefreshAttributeModsList();
void OnModsAssetsDropped(const FDragDropEvent& Evt, TArrayView<FAssetData> InAssets);
bool AreModsAssetsAcceptable(TArrayView<FAssetData> InAssets);

	// AssetDropTarget handlers

	// UI refs (keywords removed)
	TSharedPtr<class SVerticalBox> AttrKeywordsList = nullptr;
	TSharedPtr<class SVerticalBox> AttrModsList;

	// Preview sampling state
	int32 SampleLevel = 1;
	int32 SampleSeed = 12345;
	int32 SampleNumPrefixes = 1;
	int32 SampleNumSuffixes = 1;
	TSharedPtr<class SVerticalBox> SampleAffixList;

	UYIItemDefinition* EditingAsset = nullptr;
	TSharedPtr<class IDetailsView> MainDetails;

	// Preview handlers
	void OnSampleAffixesClicked();
};
