#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class IDetailsView;
class UYIDataTableAffixSource;

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
	void OnAssetSelected(const FAssetData& AssetData);
	void OnAssetDoubleClicked(const FAssetData& AssetData);

	FReply CreateAffix();
	FReply CreateAffixPool();
	FReply CreateAffixSource();
	FReply ImportFromSource();
	FReply UpdateSelectedAffix();

	bool CreateOrUpdateAffixFromRow(const UYIDataTableAffixSource* Source, const UDataTable* Table, FName RowName, const uint8* RowPtr, int64 Code, TMap<int64, TSoftObjectPtr<UYIAffixAsset>>* ExistingByCode);
	void CacheExistingAffixesByCode(TMap<int64, TSoftObjectPtr<UYIAffixAsset>>& OutMap) const;
	int64 ExtractCodeFromRow(const UScriptStruct* Struct, const uint8* RowData, FName FieldName) const;
	FString GetRowString(const UScriptStruct* Struct, const uint8* RowData, FName Field) const;
	UYIDataTableAffixSource* ResolveCurrentSource() const;

	TSharedPtr<IDetailsView> DetailsView;
	TWeakObjectPtr<UObject> LastSelectedAsset;
	TWeakObjectPtr<UYIDataTableAffixSource> CurrentSource;
};
