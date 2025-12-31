#include "YIInventoryAssetEditorCommands.h"

#define LOCTEXT_NAMESPACE "YOLOInventory"

void FYIInventoryAssetEditorCommands::RegisterCommands()
{
	UI_COMMAND(AddVariant, "Add Variant", "Append a new variant to the item.", EUserInterfaceActionType::Button, FInputChord(EKeys::N, EModifierKey::Control));
	UI_COMMAND(DuplicateVariant, "Duplicate Variant", "Duplicate the selected variant.", EUserInterfaceActionType::Button, FInputChord(EKeys::D, EModifierKey::Control));
	UI_COMMAND(RemoveVariant, "Remove Variant", "Remove the selected variant.", EUserInterfaceActionType::Button, FInputChord(EKeys::Delete));
	UI_COMMAND(ShowBaseItem, "Show Base Item", "Show base item properties in the Details panel.", EUserInterfaceActionType::Button, FInputChord(EKeys::B, EModifierKey::Alt));
}

#undef LOCTEXT_NAMESPACE
