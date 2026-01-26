#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "YOLOInventorySettings.generated.h"

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIRarityColorEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, config, Category = "Rarity", meta=(ToolTip="Rarity tag to match (e.g., Rarity.Common). The first matching entry will be used."))
	FGameplayTag RarityTag;

	UPROPERTY(EditAnywhere, config, Category = "Rarity", meta=(ToolTip="Color to apply to item names/borders for this rarity tag"))
	FLinearColor Color = FLinearColor::White;
};

/**
 * Plugin-level settings for YOLOInventory.
 * Currently only exposes debug toggles used by runtime widgets.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "YOLO Inventory"))
class YOLOINVENTORY_API UYOLOInventorySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UYOLOInventorySettings();
	/** Enables on-screen debug output for inventory drag/hover visualization. */
	UPROPERTY(EditAnywhere, config, Category = "Debug")
	bool bShowDebug = false;

	/** Palette used to color item names/borders by rarity tag. Edit in Project Settings > Plugins > YOLO Inventory. */
	UPROPERTY(EditAnywhere, config, Category = "Visuals", meta=(ToolTip="Ordered list of rarity tag colors; first matching tag is used"))
	TArray<FYIRarityColorEntry> RarityColors;

	/** Read-only access to settings. */
	static const UYOLOInventorySettings& Get();

	/** Mutable access for console commands. */
	static UYOLOInventorySettings& GetMutable();

#if WITH_EDITOR
	virtual FName GetCategoryName() const override;
#endif
};
