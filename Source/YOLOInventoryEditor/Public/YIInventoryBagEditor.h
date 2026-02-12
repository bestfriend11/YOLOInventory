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
class UTexture2D;
struct FSlateBrush;
template<typename ItemType> class SListView;
template<typename ItemType> class STileView;
struct FYIItemDashboardEntry;

enum class EYIBagPaletteMode : uint8
{
	Assets,
	DataRows,
	Runtime
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
	void HandleGridSelectionChanged(int32 Index);
	void RefreshDataRowEntries();
	void RefreshRuntimeEntries();
	TSharedRef<ITableRow> MakeDataRowWidget(TSharedPtr<FYIItemDashboardEntry> Entry, const TSharedRef<STableViewBase>& Owner);
	TSharedRef<ITableRow> MakeRuntimeItemTile(TSharedPtr<int32> ItemIndex, const TSharedRef<STableViewBase>& Owner);
	TSharedPtr<SWidget> BuildDataRowContextMenu(const TSharedPtr<FYIItemDashboardEntry>& Entry) const;
	FText BuildRuntimeItemTooltip(const UYIItemDefinition* Def, int32 Count) const;
	const FSlateBrush* ResolveRuntimeItemIcon(UYIItemDefinition* Def);
	bool CreateAssetFromEntry(const FYIItemDashboardEntry& Entry) const;
	bool AddEntryToBag(const FYIItemDashboardEntry& Entry);

	TSharedPtr<SBagEditor> GridWidget;
	TSharedPtr<class IDetailsView> DetailsView;
	// Cache for SComboBox options since OptionsSource_Lambda is not supported
	TArray<TSharedPtr<FString>> SortOptionsCache;
	TSharedPtr<class SComboBox<TSharedPtr<FString>>> SortCombo;
	TSharedPtr<SListView<TSharedPtr<FYIItemDashboardEntry>>> DataRowListView;
	TSharedPtr<STileView<TSharedPtr<int32>>> RuntimeItemTileView;
	TArray<TSharedPtr<FYIItemDashboardEntry>> DataRowEntries;
	TArray<TSharedPtr<int32>> RuntimeEntries;
	TMap<UTexture2D*, TSharedPtr<struct FSlateDynamicImageBrush>> RuntimeIconBrushCache;
	EYIBagPaletteMode PaletteMode = EYIBagPaletteMode::Assets;
	UYIInventoryBag* Bag = nullptr;
};
