#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

DECLARE_DELEGATE_OneParam(FYICatalogOpenAsset, UObject*);

enum class EYICatalogFilter : uint8
{
	All,
	Sources,
	Items,
	Affixes,
	Generators,
	Bags
};

struct FYICatalogEntry
{
	FString Group;
	FString Type;
	FString Name;
	FString Status;
	int32 LinkedCount = 0;
	FString Path;
	TSoftObjectPtr<UObject> Object;
};

class SYICatalogDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYICatalogDashboard) {}
		SLATE_EVENT(FYICatalogOpenAsset, OnOpenAsset)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void Refresh();
	UObject* GetSelectedAsset() const;

private:
	TSharedRef<ITableRow> MakeRow(TSharedPtr<FYICatalogEntry> Entry, const TSharedRef<STableViewBase>& Owner);
	void ApplyFilters();
	void OpenSelected();
	void OnSearchTextChanged(const FText& InText);
	void RebuildFilterOptions();

	FYICatalogOpenAsset OnOpenAsset;

	TArray<TSharedPtr<FYICatalogEntry>> AllEntries;
	TArray<TSharedPtr<FYICatalogEntry>> FilteredEntries;
	TArray<TSharedPtr<FString>> FilterOptions;
	TSharedPtr<SListView<TSharedPtr<FYICatalogEntry>>> ListView;

	FText SearchText;
	EYICatalogFilter ActiveFilter = EYICatalogFilter::All;
};

