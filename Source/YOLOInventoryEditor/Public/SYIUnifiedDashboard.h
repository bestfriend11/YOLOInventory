#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "YIGeneratorDashboardBridge.h"

class SYIItemDashboard;
class SYIAffixDashboard;
class SYIUnifiedHelpPanel;

enum class EYIUnifiedDashboardTab : uint8
{
	Catalog,
	Items,
	Affixes,
	Generators,
	Crafting,
	Bags,
	Equipment
};

class SYIUnifiedDashboard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SYIUnifiedDashboard) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void OpenAsset(UObject* Asset);

private:
	EYIUnifiedDashboardTab ActiveTab = EYIUnifiedDashboardTab::Items;
	TSharedPtr<class SWidgetSwitcher> TabSwitcher;
	TSharedPtr<SYIUnifiedHelpPanel> HelpPanel;
	TSharedPtr<SYIItemDashboard> ItemDashboard;
	TSharedPtr<SYIAffixDashboard> AffixDashboard;
	TSharedPtr<IYIGeneratorDashboardBridge> GeneratorDashboard;

	EYIUnifiedDashboardTab GetActiveTab() const { return ActiveTab; }
	void HandleTabChanged(EYIUnifiedDashboardTab NewTab);
	void SetActiveTab(EYIUnifiedDashboardTab NewTab);
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
