#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class IDetailLayoutBuilder;
class UYIDataTableItemSource;
class UScriptStruct;

class FYIDataTableItemSourceDetails final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	void RebuildCachedOptions();
	void RequestRefresh() const;
	void AddFragmentMappingsForSelectedFragment();
	void AddFragmentMappingsForStruct(const UScriptStruct* FragmentStruct);
	void AutoMatchExistingMappings();
	TSharedRef<SWidget> BuildMappingsTreeWidget();
	TSharedRef<SWidget> BuildMappingRowWidget(int32 MappingIndex);

private:
	IDetailLayoutBuilder* CachedDetailBuilder = nullptr;
	TWeakObjectPtr<UYIDataTableItemSource> EditedSource;

	TArray<TSharedPtr<FString>> FragmentStructOptions;
	TSharedPtr<FString> SelectedFragmentStructOption;
	TArray<TSharedPtr<FString>> SourceFieldOptions;
	TArray<TSharedPtr<FString>> ConversionOptions;

	struct FYITransformFunctionInfo
	{
		FString DisplayName;
		TSoftClassPtr<class UBlueprintFunctionLibrary> Library;
		FName FunctionName;
	};
	TArray<TSharedPtr<FYITransformFunctionInfo>> TransformFunctionOptions;
	TMap<FString, FString> PendingAddFieldByFragmentPath;
};
