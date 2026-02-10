#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Input/Reply.h"
#include "Input/Events.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Data/YIDataTableItemSource.h"
#include "Containers/Array.h"

class UYIDataTableItemSource;
class UYIItemDefinition;

struct FYIItemDashboardEntry
{
	int64 Code = 0;
	FString Name;
	FString TemplateId;
	FString Source;
	bool bIsDataTable = false;
	FName RowName = NAME_None;
	TSoftObjectPtr<UObject> Object;
	bool bHasAsset = false;
	TSoftObjectPtr<class UYIItemDefinition> ItemAsset;
	TSoftObjectPtr<class UDataTable> DataTable;
	TSoftObjectPtr<class UYIDataTableItemSource> DataSource;
};

enum class EDashTypeFilter : uint8
{
	All,
	DataTableRows,
	AssetsOnly
};

enum class EDashStatusFilter : uint8
{
	All,
	NeedsAsset,
	HasAsset,
	AssetOnly
};

class SYIItemDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYIItemDashboard) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void OpenAsset(UObject* Asset);
	virtual ~SYIItemDashboard();

private:
	TSharedRef<ITableRow> MakeRowWidget(TSharedPtr<FYIItemDashboardEntry> Entry, const TSharedRef<STableViewBase>& Owner);
	void Refresh();
	void OnSearchTextChanged(const FText& NewText);
	void OpenEntry(const TSharedPtr<FYIItemDashboardEntry>& Entry);
	void OpenDataSource(const TSharedPtr<FYIItemDashboardEntry>& Entry);
	void CreateDataTableSourceAsset() const;
	void ValidateUniqueCodes() const;
	bool CreateAssetFromEntry(const FYIItemDashboardEntry& Entry) const;
	FString GetRowString(const UScriptStruct* Struct, const uint8* RowData, FName Field) const;
	FText BuildPreviewText(const TSharedPtr<FYIItemDashboardEntry>& Entry) const;
	TSharedPtr<SWidget> BuildContextMenuForEntry(const TSharedPtr<FYIItemDashboardEntry>& Entry) const;
	TSharedPtr<SWidget> BuildListContextMenu();
	void ShowDetailsForEntry(const TSharedPtr<FYIItemDashboardEntry>& Entry);
	UObject* ResolveDetailObject(const FYIItemDashboardEntry& Entry) const;
	void RefreshInlineMappingEditor(UYIDataTableItemSource* Source);
	TSharedRef<ITableRow> MakeMappingRow(TSharedPtr<FYIFieldMapping> Mapping, const TSharedRef<STableViewBase>& OwnerTable);
	FReply HandleListKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
	void RefreshLogEntries();
	TSharedRef<ITableRow> MakeLogRow(TSharedPtr<struct FYIEditorLogEntry> Entry, const TSharedRef<STableViewBase>& Owner);
	void AutoMatchInlineMappings(bool bAddAllFields);

private:
	TArray<TSharedPtr<FYIItemDashboardEntry>> Items;
	TArray<TSharedPtr<FYIItemDashboardEntry>> FilteredItems;
	TSharedPtr<class SListView<TSharedPtr<FYIItemDashboardEntry>>> ListView;
	TSharedPtr<class IDetailsView> DetailsView;
	TWeakObjectPtr<UObject> LastDetailObject;
	TStrongObjectPtr<UObject> DetailKeepAlive;
	TWeakObjectPtr<UYIDataTableItemSource> CurrentMappingSource;
	TArray<TSharedPtr<FYIFieldMapping>> MappingRows;
	TSharedPtr<class SListView<TSharedPtr<FYIFieldMapping>>> MappingListView;
	TArray<TSharedPtr<FString>> SourceFieldOptions;
	TArray<TSharedPtr<FString>> TargetPropertyOptions;
	TArray<TSharedPtr<FString>> ConverterOptions;
	TMap<FName, FProperty*> SourceFieldPropCache;
	TMap<FName, FProperty*> TargetFieldPropCache;
	TArray<TSharedPtr<struct FYIEditorLogEntry>> LogEntries;
	TSharedPtr<class SListView<TSharedPtr<struct FYIEditorLogEntry>>> LogListView;
	FDelegateHandle LogChangedHandle;
	FText SearchText;
	EDashTypeFilter TypeFilter = EDashTypeFilter::All;
	EDashStatusFilter StatusFilter = EDashStatusFilter::All;
	bool bGroupBySource = false;
};
