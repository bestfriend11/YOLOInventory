#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "YIInventoryTypes.h"
#include "YIItemInstance.h"
#include "YIInventoryBlueprintLibrary.generated.h"

class UYIInventoryBag;
class UYIItemDefinition;
class UTexture2D;
class AYIItemPickup;

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYITooltipRequirementLine
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip") FText Text;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip") bool bMet = true;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYITooltipAttributeLine
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip") FText Label;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip") float Value = 0.f;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYITooltipData
{
	GENERATED_BODY()
public:
	/** Optional name parts (prefix/base/suffix). FullName is the combined result (used for display if set). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip", meta=(ToolTip="Optional item name prefix (e.g., 'Ancient')")) FText NamePrefix;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip", meta=(ToolTip="Base item name")) FText NameBase;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip", meta=(ToolTip="Optional item name suffix (e.g., 'of Flames')")) FText NameSuffix;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip", meta=(ToolTip="Combined display name; falls back to Title if empty")) FText FullName;

	/** Short display title for the item (e.g., 'Exquisite Sword'). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip", meta=(ToolTip="Primary title displayed in tooltips")) FText Title;

	/** Longer descriptive text shown in the tooltip body. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip", meta=(ToolTip="Long description or flavor text")) FText Description;

	/** Color representing the item's rarity; used to tint title or borders. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip", meta=(ToolTip="Color used to indicate item rarity")) FLinearColor RarityColor = FLinearColor::White;

	/** Optional icon to display in the tooltip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip", meta=(ToolTip="Optional item icon for display in tooltips")) TSoftObjectPtr<UTexture2D> Icon;

	/** Lines describing affixes or stat summaries; designers can push localized text here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip", meta=(ToolTip="List of lines describing affixes or stats")) TArray<FText> AffixLines;

	/** Structured requirement lines (with pass/fail info). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip", meta=(ToolTip="Requirement lines with pass/fail state")) TArray<FYITooltipRequirementLine> RequirementLines;

	/** Attribute snippets displayed as buffs or stats. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip", meta=(ToolTip="Attribute/value pairs for buffs or stats")) TArray<FYITooltipAttributeLine> AttributeLines;

	/** Durability info if the item tracks durability. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip") bool bHasDurability = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip") float CurrentDurability = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip") float MaxDurability = 0.f;

	/** Economy info (sell price). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip") int32 SellPrice = 0;

	/** True if all requirements are met (for quick checks). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tooltip") bool bAllRequirementsMet = true;
};

UCLASS()
class YOLOINVENTORY_API UYIInventoryBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// Affix helpers
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Affixes")
	static bool AddRolledAffix(struct FYIBagItem& Item, class UYIAffixAsset* Affix, int32 Level, int32 Seed, float& OutRolledValue);
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Affixes")
	static int32 ApplyTemplateAffixesToInstance(const class UYIItemDefinition* Definition, struct FYIItemInstance& Instance);

	// Deterministic generator: sample randomized affixes from definition pools and apply to the bag item instance.
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Affixes")
	static bool GenerateAffixesForInstance(struct FYIBagItem& Item, int32 Level, int32 Seed, int32 NumPrefixes = 0, int32 NumSuffixes = 0);

	// Stack key helpers (used to determine merge compatibility)
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Stack")
	static int64 ComputeCustomStackKey(const struct FYIItemInstance& Instance);
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Stack")
	static void UpdateCustomStackKey(struct FYIItemInstance& Instance);

	// Capability helpers (Evolution)
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Capabilities")
	static bool HasEvolutionCapability(const UYIItemDefinition* Definition);
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Capabilities")
	static bool GetEvolutionXP(const FYIBagItem& Item, int32& OutXP);
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Capabilities")
	static void SetEvolutionXP(FYIBagItem& Item, int32 XP);

	// Merchant/stash pricing hooks (stubs that can be wrapped/overridden in BP)
	UFUNCTION(BlueprintPure, Category="YOLOInventory|Economy")
	static int32 GetBuyPrice(const UYIItemDefinition* Definition, float PriceMultiplier = 1.0f);
	UFUNCTION(BlueprintPure, Category="YOLOInventory|Economy")
	static int32 GetSellPrice(const UYIItemDefinition* Definition, float PriceMultiplier = 1.0f);

	// Transfer count items from Source[Index] to Dest. If Count<=0, moves entire stack. Returns true if any items were moved; OutDestIndex is index in Dest of the resulting stack.
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Bag")
	static bool TransferItemBetweenBags(UYIInventoryBag* Source, UYIInventoryBag* Dest, int32 Index, int32 Count, int32& OutDestIndex);

	// Compute first empty position for an asset in the bag based on asset DefaultSize
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Bag")
	static bool GetFirstEmptyPosForItem(const UYIInventoryBag* Bag, const UYIItemDefinition* Definition, FIntPoint& OutPos);

	/** Add an item by UniqueCode to a bag (server-authority recommended). Returns true on success. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Bag", meta=(DisplayName="Add Item To Bag By Code"))
	static bool AddItemToBagByCode(UYIInventoryBag* Bag, int64 Code, int32 Count = 1);

	// Build a simple tooltip data struct for a bag item
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Bag")
	static bool GetItemTooltipData(const UYIInventoryBag* Bag, int32 Index, FYITooltipData& OutData, const struct FYIRequirementContext& RequirementContext);
	// Convenience overload with default/empty requirement context
	static bool GetItemTooltipData(const UYIInventoryBag* Bag, int32 Index, FYITooltipData& OutData);

	/** Spawn a replicated pickup actor for an item definition code. Server-only; returns nullptr on clients. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Spawn", meta=(WorldContext="WorldContextObject", DisplayName="Spawn Item Pickup by Code", BlueprintAuthorityOnly="true"))
	static AYIItemPickup* SpawnItemPickupByCode(UObject* WorldContextObject, int64 Code, const FTransform& Transform, int32 Count = 1, TSubclassOf<AYIItemPickup> PickupClass = nullptr);

	/** Spawn a replicated pickup using a definition asset. Server-only; falls back to code lookup if asset null. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Spawn", meta=(WorldContext="WorldContextObject", DisplayName="Spawn Item Pickup from Definition", BlueprintAuthorityOnly="true"))
	static AYIItemPickup* SpawnItemPickup(UObject* WorldContextObject, UYIItemDefinition* Definition, const FTransform& Transform, int32 Count = 1, TSubclassOf<AYIItemPickup> PickupClass = nullptr);

	/** Spawn a replicated pickup from an item instance (preserves affixes/durability). Server-only. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Spawn", meta=(WorldContext="WorldContextObject", DisplayName="Spawn Item Pickup from Instance", BlueprintAuthorityOnly="true"))
	static AYIItemPickup* SpawnItemPickupFromInstance(UObject* WorldContextObject, const FYIItemInstance& Instance, const FTransform& Transform, TSubclassOf<AYIItemPickup> PickupClass = nullptr);

	/** Drop part or all of a bag stack into the world as a pickup. Server-only. Count<=0 drops full stack. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Bag", meta=(WorldContext="WorldContextObject", DisplayName="Drop Bag Item to World", BlueprintAuthorityOnly="true"))
	static bool DropBagItemToWorld(UObject* WorldContextObject, UYIInventoryBag* Bag, int32 Index, const FTransform& SpawnTransform, int32 Count = 0, TSubclassOf<AYIItemPickup> PickupClass = nullptr);

	/** Destroy part or all of a bag stack (no pickup). Server-only. Count<=0 destroys full stack. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Bag", meta=(WorldContext="WorldContextObject", DisplayName="Destroy Bag Item", BlueprintAuthorityOnly="true"))
	static bool DestroyBagItem(UObject* WorldContextObject, UYIInventoryBag* Bag, int32 Index, int32 Count = 0);

	/** Add a full item instance (affixes/durability preserved) to a bag. Server-only. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Bag", meta=(DisplayName="Add Item Instance To Bag", BlueprintAuthorityOnly="true"))
	static bool AddItemInstanceToBag(UYIInventoryBag* Bag, const FYIItemInstance& Instance);

	/** Consume a pickup actor into a bag (preserves instance) and destroy the pickup. Server-only. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Bag", meta=(WorldContext="WorldContextObject", DisplayName="Pickup Item Actor Into Bag", BlueprintAuthorityOnly="true"))
	static bool PickupItemActorIntoBag(UObject* WorldContextObject, UYIInventoryBag* Bag, AYIItemPickup* Pickup);

	// Returns a color for a designer-defined rarity tag. Looks up a palette at /Game/YOLOInventory/RarityPalette_Default if present, else falls back to common names.
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Tooltip")
	static FLinearColor GetColorForRarityTag(const FGameplayTag& RarityTag);
    // Template helpers: query assets/construct items by template string id
    UFUNCTION(BlueprintCallable, Category="YOLOInventory|Template")
    static class UYIItemDefinition* FindItemDefinitionByTemplateId(const FString& TemplateId);
    UFUNCTION(BlueprintCallable, Category="YOLOInventory|Template")
    static class UYIAffixAsset* FindAffixByTemplateId(const FString& TemplateId);
    UFUNCTION(BlueprintCallable, Category="YOLOInventory|Template")
    static FYIItemInstance MakeItemInstanceByTemplateId(const FString& TemplateId, int32 Count = 1);};
