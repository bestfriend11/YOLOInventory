#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UYIInventoryBag;
class UYIItemDefinition;
class UYIAffixAsset;

class SYICraftingDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYICraftingDashboard) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void OpenAsset(UObject* Asset);

	void AddCraftedItemFromToolbar();
	void SaveTargetBagFromToolbar();

	void SetTargetBag(UYIInventoryBag* InBag);
	UYIInventoryBag* GetTargetBag() const;

private:
	struct FCraftAffixEntry
	{
		TSoftObjectPtr<UYIAffixAsset> Affix;
		FString Name;
		FString Kind;
		int32 Tier = 0;
		int32 Power = 0;
		bool bSelected = false;
	};

	void RefreshAffixEntries();
	FReply AddCraftedItemToBag();
	FReply SaveCurrentBag();
	TSharedRef<class ITableRow> MakeAffixRow(TSharedPtr<FCraftAffixEntry> Entry, const TSharedRef<class STableViewBase>& Owner);
	void ApplySearchFilter();

	TWeakObjectPtr<UYIInventoryBag> TargetBag;
	TWeakObjectPtr<UYIItemDefinition> TargetItemDefinition;
	TArray<TSharedPtr<FCraftAffixEntry>> AllAffixEntries;
	TArray<TSharedPtr<FCraftAffixEntry>> FilteredAffixEntries;
	TSharedPtr<class SListView<TSharedPtr<FCraftAffixEntry>>> AffixListView;
	FText SearchText;
	int32 ItemCount = 1;
	int32 RollLevel = 1;
	int32 RollSeed = 1337;
	bool bIncludeTemplateAffixes = true;
	FText StatusText;
};

