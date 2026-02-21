#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "YIItemDefinitionPresetLibrary.h"

class IDetailLayoutBuilder;
class UYIItemDefinition;
template<typename ItemType> class SComboBox;

class FYIItemDefinitionDetails final : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	FReply OnApplyPresetClicked();
	FText GetSelectedPresetText() const;
	void BuildPresetOptions();

private:
	TArray<TWeakObjectPtr<UYIItemDefinition>> EditedItemDefinitions;
	TArray<TSharedPtr<EYIItemDefinitionPreset>> PresetOptions;
	TSharedPtr<EYIItemDefinitionPreset> SelectedPreset;
	TWeakPtr<SComboBox<TSharedPtr<EYIItemDefinitionPreset>>> PresetComboBox;
};

