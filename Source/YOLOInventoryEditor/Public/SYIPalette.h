#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Views/SListView.h"
#include "SYIPaletteRow.h"

class UYINode_Item;

class SYIPalette : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYIPalette){}
		SLATE_ARGUMENT(TWeakPtr<class FYIItemDefinitionEditor>, OwnerEditor)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	TSharedRef<SWidget> MakePaletteEntry(const FText& Label, const FText& Tooltip, UClass* EntryClass, const FName& StackName); // legacy, not used by ListView
	void RefreshList();
	FText FilterText;
	TSharedPtr<class SSearchBox> SearchBox;
	TWeakPtr<class FYIItemDefinitionEditor> Owner;

	typedef TSharedPtr<FYIPaletteEntry> FEntryPtr;
	TArray<FEntryPtr> AllEntries;
	TArray<FEntryPtr> Filtered;
	TSharedPtr<SListView<FEntryPtr>> ListView;
};
