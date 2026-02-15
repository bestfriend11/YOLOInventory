#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class IDetailsView;
class UYIAffixAsset;
class UYIDataTableAffixSource;
struct FYIFieldMapping;
struct FYITransformFunctionInfo;
struct FYIMappingPreviewRow;

enum class EYIAffixDashboardLayout : uint8
{
	Full,
	AssetListOnly
};

enum class EAffixDashTypeFilter : uint8
{
	All,
	DataRows,
	AssetsOnly
};

enum class EAffixDashStatusFilter : uint8
{
	All,
	NeedsAsset,
	HasAsset,
	AssetOnly
};

class SYIAffixDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYIAffixDashboard)
		: _LayoutMode(EYIAffixDashboardLayout::Full)
	{}
		SLATE_ARGUMENT(EYIAffixDashboardLayout, LayoutMode)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void OpenAsset(UObject* Asset);
	TSharedRef<class SWidget> GetAssetPanelWidget() const;
	TSharedRef<class SWidget> GetSourcePanelWidget() const;
	TSharedRef<class SWidget> GetDetailsPanelWidget() const;
	TSharedRef<class SWidget> GetMappingPanelWidget() const;
	TSharedRef<class SWidget> GetPreviewPanelWidget() const;

	void CreateAffixFromToolbar();
	void CreateAffixPoolFromToolbar();
	void CreateAffixSourceFromToolbar();
	void CreateOrUpdateSelectedRowsFromToolbar();
	void ImportFromSourceFromToolbar();
	void UpdateSelectedAffixFromToolbar();
	void AutoMatchMappingsFromToolbar();
	void AddAllMappingsFromToolbar();
	void SaveCurrentAssetFromToolbar();
	void GuidedSetupFromToolbar();

private:
	TSharedRef<class SWidget> BuildAssetPicker();
	TSharedRef<class SWidget> BuildSourcePicker();
	TSharedRef<class SWidget> BuildDetailsPanelWidget();
	TSharedRef<class SWidget> BuildMappingPanelWidget();
	TSharedRef<class SWidget> BuildPreviewPanelWidget();
	void OnAssetSelected(const FAssetData& AssetData);
	void OnAssetDoubleClicked(const FAssetData& AssetData);
	void RefreshList();
	void OnSearchTextChanged(const FText& NewText);
	TSharedPtr<SWidget> BuildListContextMenu();
	void SelectRowsNeedingAssets();
	void SelectRowsForCurrentSource();
	void ClearListSelection();
	void ShowDetailsForEntry(const TSharedPtr<struct FYIAffixDashboardEntry>& Entry);
	void OpenEntry(const TSharedPtr<struct FYIAffixDashboardEntry>& Entry);
	TSharedRef<ITableRow> MakeRowWidget(TSharedPtr<struct FYIAffixDashboardEntry> Entry, const TSharedRef<STableViewBase>& Owner);

	FReply CreateAffix();
	FReply CreateAffixPool();
	FReply CreateAffixSource();
	FReply CreateOrUpdateSelectedRows();
	FReply ImportFromSource();
	FReply UpdateSelectedAffix();
	bool SyncTargetPoolsForSource(const UYIDataTableAffixSource* Source);
	bool CreateOrUpdateEntryFromDataRow(const TSharedPtr<struct FYIAffixDashboardEntry>& Entry, TMap<int64, TSoftObjectPtr<UYIAffixAsset>>* ExistingByCode, UYIDataTableAffixSource** OutSourceUsed = nullptr);

	bool CreateOrUpdateAffixFromRow(const UYIDataTableAffixSource* Source, const UDataTable* Table, FName RowName, const uint8* RowPtr, int64 Code, TMap<int64, TSoftObjectPtr<UYIAffixAsset>>* ExistingByCode);
	void CacheExistingAffixesByCode(TMap<int64, TSoftObjectPtr<UYIAffixAsset>>& OutMap) const;
	int64 ExtractCodeFromRow(const UScriptStruct* Struct, const uint8* RowData, FName FieldName) const;
	FString GetRowString(const UScriptStruct* Struct, const uint8* RowData, FName Field) const;
	UYIDataTableAffixSource* ResolveCurrentSource() const;
	void RefreshInlineMappingEditor(UYIDataTableAffixSource* Source);
	void BuildTransformFunctionOptions();
	void RefreshMappingPreview();
	void AutoMatchInlineMappings(bool bAddAllFields);
	TSharedRef<ITableRow> MakeMappingRow(TSharedPtr<FYIFieldMapping> Mapping, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> MakePreviewRow(TSharedPtr<FYIMappingPreviewRow> Row, const TSharedRef<STableViewBase>& Owner);

	TSharedPtr<IDetailsView> DetailsView;
	TWeakObjectPtr<UObject> LastSelectedAsset;
	TWeakObjectPtr<UYIDataTableAffixSource> CurrentSource;
	EYIAffixDashboardLayout LayoutMode = EYIAffixDashboardLayout::Full;
	TSharedPtr<class SWidget> AssetPanelWidget;
	TSharedPtr<class SWidget> SourcePanelWidget;
	TSharedPtr<class SWidget> DetailsPanelWidget;
	TSharedPtr<class SWidget> MappingPanelWidget;
	TSharedPtr<class SWidget> PreviewPanelWidget;
	TArray<TSharedPtr<struct FYIAffixDashboardEntry>> Items;
	TArray<TSharedPtr<struct FYIAffixDashboardEntry>> FilteredItems;
	TSharedPtr<class SListView<TSharedPtr<struct FYIAffixDashboardEntry>>> ListView;
	FText SearchText;
	EAffixDashTypeFilter TypeFilter = EAffixDashTypeFilter::All;
	EAffixDashStatusFilter StatusFilter = EAffixDashStatusFilter::All;
	bool bOnlyCurrentSource = false;
	int32 TotalRowCount = 0;
	int32 TotalAssetCount = 0;
	int32 TotalNeedsAssetCount = 0;
	TArray<TSharedPtr<FString>> ListTypeOptions;
	TArray<TSharedPtr<FString>> ListStatusOptions;

	TArray<TSharedPtr<FYIFieldMapping>> MappingRows;
	TSharedPtr<class SListView<TSharedPtr<FYIFieldMapping>>> MappingListView;
	TArray<TSharedPtr<FString>> SourceFieldOptions;
	TArray<TSharedPtr<FString>> TargetPropertyOptions;
	TArray<TSharedPtr<FString>> ConverterOptions;
	TMap<FName, FProperty*> SourceFieldPropCache;
	TMap<FName, FProperty*> TargetFieldPropCache;
	TArray<TSharedPtr<FYITransformFunctionInfo>> TransformFunctionOptions;
	TArray<TSharedPtr<FYIMappingPreviewRow>> MappingPreviewRows;
	TSharedPtr<class SListView<TSharedPtr<FYIMappingPreviewRow>>> MappingPreviewListView;
	TArray<TSharedPtr<FString>> PreviewRowOptions;
	FName PreviewRowName = NAME_None;
};
