#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
class SYIUnifiedHelpPanel;

enum class EYIUnifiedDashboardTab : uint8
{
	Items,
	Affixes,
	Generators,
	Crafting,
	Bags,
	Equipment
};

class SYIUnifiedHelpPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYIUnifiedHelpPanel) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void SetActiveTab(EYIUnifiedDashboardTab NewTab);

private:
	TSharedPtr<class SWidgetSwitcher> HelpSwitcher;
	TSharedRef<SWidget> BuildHelpForItems();
	TSharedRef<SWidget> BuildHelpForAffixes();
	TSharedRef<SWidget> BuildHelpForGenerators();
	TSharedRef<SWidget> MakeHelpCard(const FText& Title, const FText& Body, const FLinearColor& Accent, bool bExpanded = true);
	TSharedRef<SWidget> MakeWizardStep(int32 StepNumber, const FText& Title, const FText& Body, const FText& ButtonText, TFunction<void()> OnClick);
	void CreateItemSource();
	void CreateAffix();
	void CreateAffixPool();
	void CreateLootTable();
	void CreateRarityProfile();
	void CreateItemGenerator();
};
