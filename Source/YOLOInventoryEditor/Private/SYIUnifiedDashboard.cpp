#include "SYIUnifiedDashboard.h"
#include "SYIItemDashboard.h"
#include "SYIAffixDashboard.h"
#include "SYIGeneratorDashboard.h"
#include "YIItemDefinition.h"
#include "Data/YIDataTableItemSource.h"
#include "YIAffixAsset.h"
#include "YIAffixPoolAsset.h"
#include "YILootTable.h"
#include "YIRarityProfile.h"
#include "YIItemGenerator.h"
#include "Engine/DataTable.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/AppStyle.h"

void SYIUnifiedDashboard::Construct(const FArguments& InArgs)
{
	ItemDashboard = SNew(SYIItemDashboard);
	AffixDashboard = SNew(SYIAffixDashboard);
	GeneratorDashboard = SNew(SYIGeneratorDashboard);

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(8, 6)
			[
				SNew(SSegmentedControl<EYIUnifiedDashboardTab>)
				.Value(this, &SYIUnifiedDashboard::GetActiveTab)
				.OnValueChanged(this, &SYIUnifiedDashboard::HandleTabChanged)
				+ SSegmentedControl<EYIUnifiedDashboardTab>::Slot(EYIUnifiedDashboardTab::Items)
				.Text(NSLOCTEXT("YOLOInventory", "UnifiedDash_Items", "Items"))
				+ SSegmentedControl<EYIUnifiedDashboardTab>::Slot(EYIUnifiedDashboardTab::Affixes)
				.Text(NSLOCTEXT("YOLOInventory", "UnifiedDash_Affixes", "Affixes"))
				+ SSegmentedControl<EYIUnifiedDashboardTab>::Slot(EYIUnifiedDashboardTab::Generators)
				.Text(NSLOCTEXT("YOLOInventory", "UnifiedDash_Generators", "Generators"))
			]
			+ SVerticalBox::Slot().FillHeight(1.f).Padding(6, 0, 6, 6)
			[
				SAssignNew(TabSwitcher, SWidgetSwitcher)
				+ SWidgetSwitcher::Slot()
				[
					ItemDashboard.ToSharedRef()
				]
				+ SWidgetSwitcher::Slot()
				[
					AffixDashboard.ToSharedRef()
				]
				+ SWidgetSwitcher::Slot()
				[
					GeneratorDashboard.ToSharedRef()
				]
			]
		]
	];

	SetActiveTab(EYIUnifiedDashboardTab::Items);
}

void SYIUnifiedDashboard::HandleTabChanged(EYIUnifiedDashboardTab NewTab)
{
	SetActiveTab(NewTab);
}

void SYIUnifiedDashboard::SetActiveTab(EYIUnifiedDashboardTab NewTab)
{
	ActiveTab = NewTab;
	if (!TabSwitcher.IsValid())
	{
		return;
	}

	int32 Index = 0;
	switch (ActiveTab)
	{
	case EYIUnifiedDashboardTab::Items:
		Index = 0;
		break;
	case EYIUnifiedDashboardTab::Affixes:
		Index = 1;
		break;
	case EYIUnifiedDashboardTab::Generators:
		Index = 2;
		break;
	default:
		break;
	}
	TabSwitcher->SetActiveWidgetIndex(Index);
}

void SYIUnifiedDashboard::OpenAsset(UObject* Asset)
{
	if (!Asset)
	{
		return;
	}

	if (Asset->IsA<UYIItemDefinition>() || Asset->IsA<UYIDataTableItemSource>() || Asset->IsA<UDataTable>())
	{
		SetActiveTab(EYIUnifiedDashboardTab::Items);
		if (ItemDashboard.IsValid())
		{
			ItemDashboard->OpenAsset(Asset);
		}
		return;
	}

	if (Asset->IsA<UYIAffixAsset>() || Asset->IsA<UYIAffixPoolAsset>())
	{
		SetActiveTab(EYIUnifiedDashboardTab::Affixes);
		if (AffixDashboard.IsValid())
		{
			AffixDashboard->OpenAsset(Asset);
		}
		return;
	}

	if (Asset->IsA<UYILootTable>() || Asset->IsA<UYIRarityProfile>() || Asset->IsA<UYIItemGenerator>())
	{
		SetActiveTab(EYIUnifiedDashboardTab::Generators);
		if (GeneratorDashboard.IsValid())
		{
			GeneratorDashboard->OpenAsset(Asset);
		}
		return;
	}
}
