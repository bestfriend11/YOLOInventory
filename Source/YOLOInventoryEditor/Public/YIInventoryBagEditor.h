#pragma once
#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Widgets/Input/SComboBox.h"

class UYIInventoryBag;
class SWidget;
class SDockTab;
class SBagEditor;

class YOLOINVENTORYEDITOR_API FYIInventoryBagEditor : public FAssetEditorToolkit
{
public:
	static TWeakPtr<FYIInventoryBagEditor> ActiveEditor;
	static TWeakPtr<FYIInventoryBagEditor> GetActiveEditor() { return ActiveEditor; }

	void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UYIInventoryBag* InBag);

	// FAssetEditorToolkit
	virtual FName GetToolkitFName() const override { return FName("YOLOInventoryBagEditor"); }
	virtual FText GetBaseToolkitName() const override { return NSLOCTEXT("YOLOInventory","BagEditor","Inventory Bag Editor"); }
	virtual FString GetWorldCentricTabPrefix() const override { return TEXT("Bag"); }
	virtual FLinearColor GetWorldCentricTabColorScale() const override { return FLinearColor(0.1f,0.6f,0.9f); }

	TSharedRef<SDockTab> SpawnGridTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnDetailsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnPaletteTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnInfoTab(const FSpawnTabArgs& Args);

	// Called when a palette asset is double-clicked — open it in the editor
	void OnPaletteAssetDoubleClicked(const FAssetData& AssetData);

	TSharedPtr<class FTabManager> GetTabManager() const { return TabManager; }

	UYIInventoryBag* GetBag() const { return Bag; }

private:
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& InTabManager) override;

	TSharedPtr<SBagEditor> GridWidget;
	// Cache for SComboBox options since OptionsSource_Lambda is not supported
	TArray<TSharedPtr<FString>> SortOptionsCache;
	TSharedPtr<class SComboBox<TSharedPtr<FString>>> SortCombo;
	UYIInventoryBag* Bag = nullptr;
};
