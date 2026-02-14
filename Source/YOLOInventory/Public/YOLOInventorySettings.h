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

	/** If enabled, equipment slot tags are validated/filtered against settings below when component-local lists are empty. */
	UPROPERTY(EditAnywhere, config, Category = "Gameplay Tags", meta=(ToolTip="Enable global equipment slot tag locking (used as fallback when component-local slot rules are empty)."))
	bool bEnableEquipmentSlotTagLocking = false;

	/** Prefix used to identify equip slot tags globally (for example Equip.Slot.). */
	UPROPERTY(EditAnywhere, config, Category = "Gameplay Tags", meta=(ToolTip="Global equip slot tag prefix used by validation and helpers."))
	FString EquipmentSlotTagPrefix = TEXT("Equip.Slot.");

	/** Optional explicit allow-list for equipment slot tags (when locking is enabled). */
	UPROPERTY(EditAnywhere, config, Category = "Gameplay Tags", meta=(ToolTip="Optional explicit allow-list for equip slots. If empty, prefix-based validation is used."))
	FGameplayTagContainer AllowedEquipmentSlotTags;

	/** Prefixes used by optional tag chooser helper APIs to surface common inventory tags. */
	UPROPERTY(EditAnywhere, config, Category = "Gameplay Tags", meta=(ToolTip="Tag prefixes used by inventory tag suggestion helpers."))
	TArray<FString> SuggestedInventoryTagPrefixes;

	/** Read-only access to settings. */
	static const UYOLOInventorySettings& Get();

	/** Mutable access for console commands. */
	static UYOLOInventorySettings& GetMutable();

#if WITH_EDITOR
	virtual FName GetCategoryName() const override;
#endif
};
