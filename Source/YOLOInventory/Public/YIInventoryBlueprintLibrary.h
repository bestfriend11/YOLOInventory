#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "YIInventoryTypes.h"
#include "YIItemInstance.h"
#include "YIInventoryBlueprintLibrary.generated.h"

class UYIInventoryBag;
class UYIItemDefinition;
class UTexture2D;

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYITooltipData
{
	GENERATED_BODY()
public:
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

	// Build a simple tooltip data struct for a bag item
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Bag")
	static bool GetItemTooltipData(const UYIInventoryBag* Bag, int32 Index, FYITooltipData& OutData);

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
