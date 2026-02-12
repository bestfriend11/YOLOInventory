#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class IDetailsView;
class UYIDataTableAffixSource;
struct FYIFieldMapping;
struct FYITransformFunctionInfo;
struct FYIMappingPreviewRow;

class SYIAffixDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYIAffixDashboard) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void OpenAsset(UObject* Asset);

private:
	TSharedRef<class SWidget> BuildAssetPicker();
	TSharedRef<class SWidget> BuildSourcePicker();
	TSharedRef<class SWidget> BuildMappingPanelWidget();
	TSharedRef<class SWidget> BuildPreviewPanelWidget();
	void OnAssetSelected(const FAssetData& AssetData);
	void OnAssetDoubleClicked(const FAssetData& AssetData);
	void RefreshList();
	void OnSearchTextChanged(const FText& NewText);
	void ShowDetailsForEntry(const TSharedPtr<struct FYIAffixDashboardEntry>& Entry);
	void OpenEntry(const TSharedPtr<struct FYIAffixDashboardEntry>& Entry);
	TSharedRef<ITableRow> MakeRowWidget(TSharedPtr<struct FYIAffixDashboardEntry> Entry, const TSharedRef<STableViewBase>& Owner);

	FReply CreateAffix();
	FReply CreateAffixPool();
	FReply CreateAffixSource();
	FReply ImportFromSource();
	FReply UpdateSelectedAffix();
	bool SyncTargetPoolsForSource(const UYIDataTableAffixSource* Source);

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
	TArray<TSharedPtr<struct FYIAffixDashboardEntry>> Items;
	TArray<TSharedPtr<struct FYIAffixDashboardEntry>> FilteredItems;
	TSharedPtr<class SListView<TSharedPtr<struct FYIAffixDashboardEntry>>> ListView;
	FText SearchText;

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
