#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Input/Reply.h"
#include "Input/Events.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "UObject/SoftObjectPtr.h"
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

struct FYITransformFunctionInfo
{
	FString DisplayName;
	TSoftClassPtr<class UBlueprintFunctionLibrary> Library;
	FName FunctionName;
};

struct FYIMappingPreviewRow
{
	FName SourceField;
	FName TargetProperty;
	FString SourceValue;
	FString ConvertedValue;
	FString TransformedValue;
	FText Status;
	FLinearColor StatusColor = FLinearColor::White;
};

enum class EYIDashboardIssueSeverity : uint8
{
	Info,
	Warning,
	Error
};

struct FYIPreflightIssue
{
	EYIDashboardIssueSeverity Severity = EYIDashboardIssueSeverity::Info;
	bool bBlocking = false;
	FText Message;
	FText Context;
};

struct FYIFieldDiffRow
{
	FName FieldName = NAME_None;
	FString BeforeValue;
	FString AfterValue;
	FText Status;
	FLinearColor StatusColor = FLinearColor::White;
};

enum class EYIBatchJobStatus : uint8
{
	Pending,
	Succeeded,
	Failed
};

enum class EYIDashboardBottomPanel : uint8
{
	Preflight,
	Diff,
	Batch,
	Logs
};

struct FYIBatchJobEntry
{
	TSharedPtr<FYIItemDashboardEntry> Entry;
	EYIBatchJobStatus Status = EYIBatchJobStatus::Pending;
	FText Result;
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

enum class EYIItemDashboardLayout : uint8
{
	Full,
	ItemListOnly
};

class SYIItemDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYIItemDashboard)
		: _LayoutMode(EYIItemDashboardLayout::Full)
	{}
		SLATE_ARGUMENT(EYIItemDashboardLayout, LayoutMode)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void OpenAsset(UObject* Asset);
	virtual ~SYIItemDashboard();
	TSharedRef<SWidget> GetItemsPanelWidget() const;
	TSharedRef<SWidget> GetDetailsPanelWidget() const;
	TSharedRef<SWidget> GetMappingPanelWidget() const;
	TSharedRef<SWidget> GetPreviewPanelWidget() const;
	TSharedRef<SWidget> GetPreflightPanelWidget() const;
	TSharedRef<SWidget> GetDiffPanelWidget() const;
	TSharedRef<SWidget> GetBatchPanelWidget() const;
	TSharedRef<SWidget> GetLogsPanelWidget() const;
	void RefreshFromToolbar();
	void CreateDataSourceFromToolbar();
	void ValidateUniqueCodesFromToolbar();
	void CreateOrUpdateSelectedFromToolbar();
	void UpdateLinkedSelectedFromToolbar();
	void PreflightSelectedFromToolbar();
	void ApplySuggestedMappingsFromToolbar();
	void QueueSelectedFromToolbar();
	void RunQueueFromToolbar();
	void SaveCurrentAssetFromToolbar();
	void GuidedSetupFromToolbar();

private:
	TSharedRef<SWidget> BuildItemsPanelWidget();
	TSharedRef<SWidget> BuildDetailsPanelWidget();
	TSharedRef<SWidget> BuildMappingPanelWidget();
	TSharedRef<SWidget> BuildPreviewPanelWidget();
	TSharedRef<SWidget> BuildPreflightPanelWidget();
	TSharedRef<SWidget> BuildDiffPanelWidget();
	TSharedRef<SWidget> BuildBatchPanelWidget();
	TSharedRef<SWidget> BuildLogsPanelWidget();
	TSharedRef<ITableRow> MakeRowWidget(TSharedPtr<FYIItemDashboardEntry> Entry, const TSharedRef<STableViewBase>& Owner);
	void Refresh();
	void OnSearchTextChanged(const FText& NewText);
	void OpenEntry(const TSharedPtr<FYIItemDashboardEntry>& Entry);
	void OpenDataSource(const TSharedPtr<FYIItemDashboardEntry>& Entry);
	void CreateDataTableSourceAsset();
	void ValidateUniqueCodes() const;
	bool CreateAssetFromEntry(const FYIItemDashboardEntry& Entry) const;
	bool UpdateAssetFromLinkedSource(class UYIItemDefinition* ItemDef) const;
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
	void BuildTransformFunctionOptions();
	void RefreshMappingPreview();
	TSharedRef<ITableRow> MakePreviewRow(TSharedPtr<FYIMappingPreviewRow> Row, const TSharedRef<STableViewBase>& Owner);
	void RebuildPreflightForSelection();
	bool RunPreflightForEntry(const FYIItemDashboardEntry& Entry, TArray<FYIPreflightIssue>& OutIssues, bool bLogIssues) const;
	TSharedRef<ITableRow> MakePreflightRow(TSharedPtr<FYIPreflightIssue> Entry, const TSharedRef<STableViewBase>& Owner);
	void RebuildDiffForSelection();
	TSharedRef<ITableRow> MakeDiffRow(TSharedPtr<FYIFieldDiffRow> Row, const TSharedRef<STableViewBase>& Owner);
	void EnqueueSelectedRows();
	void ProcessBatchQueue();
	TSharedRef<ITableRow> MakeBatchRow(TSharedPtr<FYIBatchJobEntry> Row, const TSharedRef<STableViewBase>& Owner);
	void ApplySuggestedMappings();

private:
	EYIItemDashboardLayout LayoutMode = EYIItemDashboardLayout::Full;
	TSharedPtr<SWidget> ItemsPanelWidget;
	TSharedPtr<SWidget> DetailsPanelWidget;
	TSharedPtr<SWidget> MappingPanelWidget;
	TSharedPtr<SWidget> PreviewPanelWidget;
	TSharedPtr<SWidget> PreflightPanelWidget;
	TSharedPtr<SWidget> DiffPanelWidget;
	TSharedPtr<SWidget> BatchPanelWidget;
	TSharedPtr<SWidget> LogsPanelWidget;
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
	TArray<TSharedPtr<FString>> ListTypeOptions;
	TArray<TSharedPtr<FString>> ListStatusOptions;
	TMap<FName, FProperty*> SourceFieldPropCache;
	TMap<FName, FProperty*> TargetFieldPropCache;
	TArray<TSharedPtr<FYITransformFunctionInfo>> TransformFunctionOptions;
	TArray<TSharedPtr<FYIMappingPreviewRow>> MappingPreviewRows;
	TSharedPtr<class SListView<TSharedPtr<FYIMappingPreviewRow>>> MappingPreviewListView;
	TArray<TSharedPtr<FString>> PreviewRowOptions;
	FName PreviewRowName = NAME_None;
	TArray<TSharedPtr<struct FYIEditorLogEntry>> LogEntries;
	TArray<TSharedPtr<struct FYIEditorLogEntry>> FilteredLogEntries;
	TSharedPtr<class SListView<TSharedPtr<struct FYIEditorLogEntry>>> LogListView;
	bool bShowInfoLogs = true;
	bool bShowWarningLogs = true;
	bool bShowErrorLogs = true;
	TArray<TSharedPtr<FYIPreflightIssue>> PreflightIssues;
	TSharedPtr<class SListView<TSharedPtr<FYIPreflightIssue>>> PreflightListView;
	TArray<TSharedPtr<FYIFieldDiffRow>> DiffRows;
	TSharedPtr<class SListView<TSharedPtr<FYIFieldDiffRow>>> DiffListView;
	TArray<TSharedPtr<FYIBatchJobEntry>> BatchQueueEntries;
	TSharedPtr<class SListView<TSharedPtr<FYIBatchJobEntry>>> BatchQueueListView;
	FDelegateHandle LogChangedHandle;
	FText SearchText;
	EDashTypeFilter TypeFilter = EDashTypeFilter::All;
	EDashStatusFilter StatusFilter = EDashStatusFilter::All;
	bool bGroupBySource = false;
	bool bShowDetailsPanel = true;
	bool bShowMappingPanel = true;
	bool bShowPreviewPanel = true;
	bool bShowLogPanel = true;
	bool bShowPreflightPanel = true;
	bool bShowDiffPanel = true;
	bool bShowBatchPanel = true;
	EYIDashboardBottomPanel ActiveBottomPanel = EYIDashboardBottomPanel::Logs;
};
