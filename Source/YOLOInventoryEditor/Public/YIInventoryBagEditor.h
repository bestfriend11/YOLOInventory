#pragma once
#include "CoreMinimal.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Widgets/Input/SComboBox.h"

class UYIInventoryBag;
class SWidget;
class SDockTab;
class SBagEditor;
class ITableRow;
class STableViewBase;
template<typename ItemType> class SListView;
struct FYIItemDashboardEntry;

enum class EYIBagPaletteMode : uint8
{
	Assets,
	DataRows
};

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
	void RefreshDataRowEntries();
	TSharedRef<ITableRow> MakeDataRowWidget(TSharedPtr<FYIItemDashboardEntry> Entry, const TSharedRef<STableViewBase>& Owner);
	TSharedPtr<SWidget> BuildDataRowContextMenu(const TSharedPtr<FYIItemDashboardEntry>& Entry) const;
	bool CreateAssetFromEntry(const FYIItemDashboardEntry& Entry) const;
	void AddEntryToBag(const FYIItemDashboardEntry& Entry);

	TSharedPtr<SBagEditor> GridWidget;
	// Cache for SComboBox options since OptionsSource_Lambda is not supported
	TArray<TSharedPtr<FString>> SortOptionsCache;
	TSharedPtr<class SComboBox<TSharedPtr<FString>>> SortCombo;
	TSharedPtr<SListView<TSharedPtr<FYIItemDashboardEntry>>> DataRowListView;
	TArray<TSharedPtr<FYIItemDashboardEntry>> DataRowEntries;
	EYIBagPaletteMode PaletteMode = EYIBagPaletteMode::Assets;
	UYIInventoryBag* Bag = nullptr;
};
