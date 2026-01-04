#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "YOLOInventorySettings.generated.h"

/**
 * Plugin-level settings for YOLOInventory.
 * Currently only exposes debug toggles used by runtime widgets.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "YOLO Inventory"))
class YOLOINVENTORY_API UYOLOInventorySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Enables on-screen debug output for inventory drag/hover visualization. */
	UPROPERTY(EditAnywhere, config, Category = "Debug")
	bool bShowDebug = false;

	/** Read-only access to settings. */
	static const UYOLOInventorySettings& Get();

	/** Mutable access for console commands. */
	static UYOLOInventorySettings& GetMutable();

#if WITH_EDITOR
	virtual FName GetCategoryName() const override;
#endif
};
