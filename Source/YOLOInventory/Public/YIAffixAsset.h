#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Curves/CurveFloat.h"
#include "YIAffixAsset.generated.h"

class UYIAttributeModAsset;

UENUM(BlueprintType)
enum class EYIAffixKind : uint8
{
    Prefix UMETA(DisplayName="Prefix"),
    Suffix UMETA(DisplayName="Suffix"),
    Implicit UMETA(DisplayName="Implicit")
};

UENUM(BlueprintType)
enum class EYIAffixQuality : uint8
{
    Common UMETA(DisplayName="Common"),
    Magic UMETA(DisplayName="Magic"),
    Rare UMETA(DisplayName="Rare"),
    Unique UMETA(DisplayName="Unique")
};

/**
 * UYIAffixAsset
 *
 * Represents an affix (prefix/suffix/implicit) that can be applied to items. Designers author effect mods and roll rules here.
 *
 * Designer notes:
 * - Use TemplateId when you want to reference affixes from external template/loot tables.
 * - AttributeMods contain the actual gameplay modifications (GAS-compatible assets).
 */
UCLASS(BlueprintType)
class YOLOINVENTORY_API UYIAffixAsset : public UObject
{
    GENERATED_BODY()
public:
    // Identity
    /** Designer-friendly internal id used by tools and lookups. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Identity", meta=(ToolTip="Editor-friendly identifier for internal use"))
    FName InternalId;

    /** Numeric unique id (optional). Editor tools may auto-assign if left 0. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Identity", meta=(ToolTip="Project-unique numeric identifier for tooling"))
    int64 UniqueCode = 0;

    /** Optional string identifier for template lookups (e.g., 'affix_fire_damage'). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Template", meta=(ToolTip="Optional string identifier used by template queries (e.g., 'affix_fire_damage')"))
    FString TemplateId;

    // Presentation
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Presentation", meta=(ToolTip="Display name used in UIs and tooltips"))
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Presentation", meta=(MultiLine=true, ToolTip="Long description for the affix"))
    FText Description;

    /** Format string used to render the affix value in tooltips, e.g. "+{0}% Damage" */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Presentation", meta=(ToolTip="Format string used to render the roll value in a tooltip (e.g., '+{0}% Damage')"))
    FText TooltipFormat;

    // Classification
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Classification", meta=(ToolTip="Whether this is a prefix, suffix or implicit affix"))
    EYIAffixKind Kind = EYIAffixKind::Prefix;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Classification", meta=(ToolTip="Quality tier of the affix used for filtering and UI"))
    EYIAffixQuality Quality = EYIAffixQuality::Magic;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Classification", meta=(ClampMin="1", ToolTip="Tier (power level) used by designers to gate or weight affixes"))
    int32 Tier = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Classification", meta=(ClampMin="0", ToolTip="Relative weight used when sampling from a pool"))
    float Weight = 1.0f;

    // Effects
    /** Attribute mod assets applied by this affix (GAS-friendly assets). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Effects", meta=(ToolTip="Attribute mod assets that this affix applies"))
    TArray<TSoftObjectPtr<UYIAttributeModAsset>> AttributeMods;

    // Roll rules
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Roll", meta=(ToolTip="Minimum roll value for this affix"))
    float MinValue = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Roll", meta=(ToolTip="Maximum roll value for this affix"))
    float MaxValue = 0.f;

    /** Curve that maps item level to rolled value (sampled during generation). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Roll", meta=(ToolTip="Curve mapping level to a roll multiplier/value"))
    FRuntimeFloatCurve ValueByLevel;

    // Constraints
    /** Tags that the target item must have for this affix to be allowed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Constraints", meta=(ToolTip="Only apply this affix to items that match these tags"))
    FGameplayTagContainer AllowedItemTags;

    /** Affixes in the same ConflictGroup will not appear together on a single item. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Constraints", meta=(ToolTip="Affix conflict group used to avoid incompatible affix combinations"))
    FName ConflictGroup;

#if WITH_EDITOR
    /** Debug helper: sample a roll using the configured rules (callable in editor). */
    UFUNCTION(CallInEditor, Category="Affix|Debug")
    float SampleRoll(int32 Level, int32 Seed) const;
#endif
};
