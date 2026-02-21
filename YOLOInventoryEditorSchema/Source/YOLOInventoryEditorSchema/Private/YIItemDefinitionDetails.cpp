#include "YIItemDefinitionDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "YIItemDefinition.h"

TSharedRef<IDetailCustomization> FYIItemDefinitionDetails::MakeInstance()
{
	return MakeShared<FYIItemDefinitionDetails>();
}

void FYIItemDefinitionDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	EditedItemDefinitions.Reset();

	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);
	for (const TWeakObjectPtr<UObject>& Object : Objects)
	{
		if (UYIItemDefinition* ItemDef = Cast<UYIItemDefinition>(Object.Get()))
		{
			EditedItemDefinitions.Add(ItemDef);
		}
	}

	if (EditedItemDefinitions.Num() == 0)
	{
		return;
	}

	BuildPresetOptions();

	IDetailCategoryBuilder& PresetsCategory = DetailBuilder.EditCategory(
		TEXT("YOLO Presets"),
		FText::FromString(TEXT("YOLO Presets")),
		ECategoryPriority::Important);

	PresetsCategory.AddCustomRow(FText::FromString(TEXT("Item Preset")))
	.NameContent()
	[
		SNew(STextBlock)
		.Text(FText::FromString(TEXT("Preset")))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.MinDesiredWidth(420.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.Padding(0.f, 0.f, 6.f, 0.f)
		[
			SAssignNew(PresetComboBox, SComboBox<TSharedPtr<EYIItemDefinitionPreset>>)
			.OptionsSource(&PresetOptions)
			.InitiallySelectedItem(SelectedPreset)
			.OnGenerateWidget_Lambda([](const TSharedPtr<EYIItemDefinitionPreset> Option)
			{
				const FText Label = Option.IsValid()
					? UEnum::GetDisplayValueAsText(*Option.Get())
					: FText::FromString(TEXT("Invalid"));
				return SNew(STextBlock).Text(Label);
			})
			.OnSelectionChanged_Lambda([this](const TSharedPtr<EYIItemDefinitionPreset> InPreset, ESelectInfo::Type)
			{
				SelectedPreset = InPreset;
			})
			[
				SNew(STextBlock)
				.Text(this, &FYIItemDefinitionDetails::GetSelectedPresetText)
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Apply Preset")))
			.ToolTipText(FText::FromString(TEXT("Adds/updates fragments for the selected preset on all selected item definitions.")))
			.OnClicked(this, &FYIItemDefinitionDetails::OnApplyPresetClicked)
		]
	];
}

FReply FYIItemDefinitionDetails::OnApplyPresetClicked()
{
	if (!SelectedPreset.IsValid())
	{
		return FReply::Handled();
	}

	for (const TWeakObjectPtr<UYIItemDefinition>& ItemDefPtr : EditedItemDefinitions)
	{
		if (UYIItemDefinition* ItemDef = ItemDefPtr.Get())
		{
			UYIItemDefinitionPresetLibrary::ApplyPresetToItemDefinition(ItemDef, *SelectedPreset.Get(), true);
		}
	}

	return FReply::Handled();
}

FText FYIItemDefinitionDetails::GetSelectedPresetText() const
{
	if (!SelectedPreset.IsValid())
	{
		return FText::FromString(TEXT("Select preset"));
	}
	return UEnum::GetDisplayValueAsText(*SelectedPreset.Get());
}

void FYIItemDefinitionDetails::BuildPresetOptions()
{
	PresetOptions.Reset();

	const UEnum* PresetEnum = StaticEnum<EYIItemDefinitionPreset>();
	if (!PresetEnum)
	{
		SelectedPreset.Reset();
		return;
	}

	for (int32 Index = 0; Index < PresetEnum->NumEnums() - 1; ++Index)
	{
		const EYIItemDefinitionPreset Preset = static_cast<EYIItemDefinitionPreset>(PresetEnum->GetValueByIndex(Index));
		PresetOptions.Add(MakeShared<EYIItemDefinitionPreset>(Preset));
	}

	SelectedPreset = PresetOptions.Num() > 0 ? PresetOptions[0] : nullptr;
}
