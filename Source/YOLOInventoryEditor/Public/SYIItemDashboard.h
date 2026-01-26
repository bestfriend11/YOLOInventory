#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Input/Reply.h"
#include "Input/Events.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/WeakObjectPtrTemplates.h"

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
	TSoftObjectPtr<class UDataTable> DataTable;
	TSoftObjectPtr<class UYIDataTableItemSource> DataSource;
};

class SYIItemDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYIItemDashboard) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<ITableRow> MakeRowWidget(TSharedPtr<FYIItemDashboardEntry> Entry, const TSharedRef<STableViewBase>& Owner) const;
	void Refresh();
	void OnSearchTextChanged(const FText& NewText);
	void OpenEntry(const TSharedPtr<FYIItemDashboardEntry>& Entry);
	void OpenDataSource(const TSharedPtr<FYIItemDashboardEntry>& Entry) const;
	void CreateDataTableSourceAsset() const;
	void ValidateUniqueCodes() const;
	bool CreateAssetFromEntry(const FYIItemDashboardEntry& Entry) const;
	FString GetRowString(const UScriptStruct* Struct, const uint8* RowData, FName Field) const;
	FText BuildPreviewText(const TSharedPtr<FYIItemDashboardEntry>& Entry) const;
	TSharedPtr<SWidget> BuildContextMenuForEntry(const TSharedPtr<FYIItemDashboardEntry>& Entry) const;
	TSharedPtr<SWidget> BuildListContextMenu();
	void ShowDetailsForEntry(const TSharedPtr<FYIItemDashboardEntry>& Entry);
	UObject* ResolveDetailObject(const FYIItemDashboardEntry& Entry) const;

private:
	TArray<TSharedPtr<FYIItemDashboardEntry>> Items;
	TArray<TSharedPtr<FYIItemDashboardEntry>> FilteredItems;
	TSharedPtr<class SListView<TSharedPtr<FYIItemDashboardEntry>>> ListView;
	TSharedPtr<class IDetailsView> DetailsView;
	TWeakObjectPtr<UObject> LastDetailObject;
	FText SearchText;
};
