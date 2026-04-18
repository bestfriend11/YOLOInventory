#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "YOLOInventorySettings.generated.h"

UENUM(BlueprintType)
enum class EYIDebugChannel : uint8
{
	General UMETA(DisplayName = "General"),
	Persistence UMETA(DisplayName = "Persistence"),
	Inventory UMETA(DisplayName = "Inventory"),
	Equipment UMETA(DisplayName = "Equipment"),
	ActionBar UMETA(DisplayName = "Action Bar"),
	Trade UMETA(DisplayName = "Trade"),
	Shop UMETA(DisplayName = "Shop"),
	Grid UMETA(DisplayName = "Grid"),
	Phase2 UMETA(DisplayName = "Phase2")
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYCORE_API FYIRarityColorEntry
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
class YOLOINVENTORYCORE_API UYOLOInventorySettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UYOLOInventorySettings();
	/** Enables on-screen debug output for inventory drag/hover visualization. */
	UPROPERTY(EditAnywhere, config, Category = "Debug")
	bool bShowDebug = false;

	/** Master switch for YOLO Inventory runtime debug routing. */
	UPROPERTY(EditAnywhere, config, Category = "Debug|Pipeline")
	bool bEnableDebugPipeline = true;

	/** Allow debug pipeline to write to output log. */
	UPROPERTY(EditAnywhere, config, Category = "Debug|Pipeline")
	bool bDebugOutputToLog = true;

	/** Allow debug pipeline to write on-screen messages. */
	UPROPERTY(EditAnywhere, config, Category = "Debug|Pipeline")
	bool bDebugOutputToScreen = false;

	/** If false, even "forced" runtime debug messages can be suppressed globally. */
	UPROPERTY(EditAnywhere, config, Category = "Debug|Pipeline")
	bool bAllowForcedDebugMessages = false;

	/** Default on-screen duration for non-pinned messages. */
	UPROPERTY(EditAnywhere, config, Category = "Debug|Pipeline", meta=(ClampMin="0.1", ClampMax="600.0"))
	float DebugScreenSeconds = 2.0f;

	/** On-screen duration for pinned debug messages. */
	UPROPERTY(EditAnywhere, config, Category = "Debug|Pipeline", meta=(ClampMin="1.0", ClampMax="7200.0"))
	float DebugPinnedScreenSeconds = 20.0f;

	/** Retained runtime message history size for debug UI windows. */
	UPROPERTY(EditAnywhere, config, Category = "Debug|Pipeline", meta=(ClampMin="16", ClampMax="8192"))
	int32 DebugHistoryMaxEntries = 1024;

	/** Keep writing messages into history (used by debug windows/subsystems). */
	UPROPERTY(EditAnywhere, config, Category = "Debug|Pipeline")
	bool bDebugRouteToHistory = true;

	/** If enabled, identical messages are rate-limited to reduce spam floods. */
	UPROPERTY(EditAnywhere, config, Category = "Debug|Pipeline")
	bool bDebugDeduplicateMessages = true;

	/** Minimum interval before the same message (channel+source+text) is emitted again. */
	UPROPERTY(EditAnywhere, config, Category = "Debug|Pipeline", meta=(ClampMin="0.0", ClampMax="30.0"))
	float DebugDuplicateIntervalSeconds = 0.35f;

	/** Reuse stable on-screen keys for same message so repeated emissions update in place. */
	UPROPERTY(EditAnywhere, config, Category = "Debug|Pipeline")
	bool bDebugUseStableScreenKeys = true;

	/** Channel filters. */
	UPROPERTY(EditAnywhere, config, Category = "Debug|Channels")
	bool bDebugChannelGeneral = true;

	UPROPERTY(EditAnywhere, config, Category = "Debug|Channels")
	bool bDebugChannelPersistence = true;

	UPROPERTY(EditAnywhere, config, Category = "Debug|Channels")
	bool bDebugChannelInventory = true;

	UPROPERTY(EditAnywhere, config, Category = "Debug|Channels")
	bool bDebugChannelEquipment = true;

	UPROPERTY(EditAnywhere, config, Category = "Debug|Channels")
	bool bDebugChannelActionBar = true;

	UPROPERTY(EditAnywhere, config, Category = "Debug|Channels")
	bool bDebugChannelTrade = true;

	UPROPERTY(EditAnywhere, config, Category = "Debug|Channels")
	bool bDebugChannelShop = true;

	UPROPERTY(EditAnywhere, config, Category = "Debug|Channels")
	bool bDebugChannelGrid = true;

	UPROPERTY(EditAnywhere, config, Category = "Debug|Channels")
	bool bDebugChannelPhase2 = true;

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

	/** Optional default runtime grid style used when widget and bag do not specify one. */
	UPROPERTY(EditAnywhere, config, Category = "Visuals", meta=(AllowedClasses="/Script/YOLOInventoryGrid.YIInventoryGridStyleAsset", ToolTip="Fallback grid style for runtime inventory grids (widget override and bag style take priority)."))
	TSoftObjectPtr<UObject> DefaultGridStyle;

	/** Snap drag ghost/highlight visuals to grid cells. Disable for free cursor movement drag visuals. */
	UPROPERTY(EditAnywhere, config, Category = "Visuals|Grid", meta=(ToolTip="If enabled, drag ghost and placement highlight snap to cell boundaries. Disable for free cursor movement visuals."))
	bool bSnapDragVisualsToGrid = true;

	/** Keep dragging the displaced item after a successful single-item swap, similar to Dungeon Siege style inventory flow. */
	UPROPERTY(EditAnywhere, config, Category = "Interaction|Grid", meta=(ToolTip="If enabled, swapping by drag-drop keeps the displaced item attached to the cursor instead of ending the drag."))
	bool bContinueDraggingSwappedItem = false;

	/** Read-only access to settings. */
	static const UYOLOInventorySettings& Get();

	/** Mutable access for console commands. */
	static UYOLOInventorySettings& GetMutable();

	/** Global channel filter query used by debug routing utilities. */
	bool IsDebugChannelEnabled(EYIDebugChannel Channel) const;

#if WITH_EDITOR
	virtual FName GetCategoryName() const override;
#endif
};
