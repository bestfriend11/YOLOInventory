#include "YIInventoryGraphSchema.h"
#include "ToolMenus.h"
#include "YINode_Item.h"
#include "YIStackEntry_Examples.h"

#if WITH_EDITOR
void UYIInventoryGraphSchema::GetContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	if (!Context) return;
	if (Context->Node)
	{
		if (const UYINode_Item* Item = Cast<UYINode_Item>(Context->Node))
		{
			FToolMenuSection& UISection = Menu->AddSection("YOLO_UI", NSLOCTEXT("YOLOInventory", "UISection", "UI")); UISection.AddMenuEntry("InfoUI", NSLOCTEXT("YOLOInventory","InfoUI","UI stack holds name, description, visuals, etc."), FText(), FSlateIcon(), FUIAction());
			UISection.AddMenuEntry("AddUIDescription",
				NSLOCTEXT("YOLOInventory", "AddUIDescription", "Add UI: Name/Description"),
				FText(),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([Item]() { if (auto* Mutable = const_cast<UYINode_Item*>(Item)) { Mutable->AddStackEntry(UYIUI_NameDesc::StaticClass(), TEXT("UI")); } })));

			FToolMenuSection& AbilitySection = Menu->AddSection("YOLO_Ability", NSLOCTEXT("YOLOInventory", "AbilitySection", "Abilities"));
			AbilitySection.AddMenuEntry("AddGrantAbility",
				NSLOCTEXT("YOLOInventory", "AddGrantAbility", "Add Grant Ability"),
				FText(),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([Item]() { if (auto* Mutable = const_cast<UYINode_Item*>(Item)) { Mutable->AddStackEntry(UYIAbility_GrantAbility::StaticClass(), TEXT("Ability")); } })));

			FToolMenuSection& UpgradeSection = Menu->AddSection("YOLO_Upgrade", NSLOCTEXT("YOLOInventory", "UpgradeSection", "Upgrade"));
			UpgradeSection.AddMenuEntry("AddUpgradePath",
				NSLOCTEXT("YOLOInventory", "AddUpgradePath", "Add Upgrade Path"),
				FText(),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([Item]() { if (auto* Mutable = const_cast<UYINode_Item*>(Item)) { Mutable->AddStackEntry(UYIUpgrade_Path::StaticClass(), TEXT("Upgrade")); } })));

			FToolMenuSection& EcoSection = Menu->AddSection("YOLO_Economy", NSLOCTEXT("YOLOInventory", "EconomySection", "Economy"));
			EcoSection.AddMenuEntry("AddEconomy",
				NSLOCTEXT("YOLOInventory", "AddEconomy", "Add Market/Selling"),
				FText(),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([Item]() { if (auto* Mutable = const_cast<UYINode_Item*>(Item)) { Mutable->AddStackEntry(UYIEconomy_Market::StaticClass(), TEXT("Economy")); } })));
		}
	}
}
#endif
