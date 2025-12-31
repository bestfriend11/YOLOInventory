#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "Framework/Commands/UICommandInfo.h"
#include "InputCoreTypes.h"

class FYIInventoryAssetEditorCommands : public TCommands<FYIInventoryAssetEditorCommands>
{
public:
	FYIInventoryAssetEditorCommands()
		: TCommands<FYIInventoryAssetEditorCommands>(
			"YOLOInventoryEditor",
			NSLOCTEXT("YOLOInventory", "AssetEditorCommands", "YOLO Inventory Editor"),
			NAME_None,
			FAppStyle::GetAppStyleSetName())
	{}

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> AddVariant;
	TSharedPtr<FUICommandInfo> DuplicateVariant;
	TSharedPtr<FUICommandInfo> RemoveVariant;
	TSharedPtr<FUICommandInfo> ShowBaseItem;
};
