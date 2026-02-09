#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SYIItemDashboard;
class SYIAffixDashboard;
class SYIGeneratorDashboard;

enum class EYIUnifiedDashboardTab : uint8
{
	Items,
	Affixes,
	Generators
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
	TSharedPtr<SYIItemDashboard> ItemDashboard;
	TSharedPtr<SYIAffixDashboard> AffixDashboard;
	TSharedPtr<SYIGeneratorDashboard> GeneratorDashboard;

	EYIUnifiedDashboardTab GetActiveTab() const { return ActiveTab; }
	void HandleTabChanged(EYIUnifiedDashboardTab NewTab);
	void SetActiveTab(EYIUnifiedDashboardTab NewTab);
};
