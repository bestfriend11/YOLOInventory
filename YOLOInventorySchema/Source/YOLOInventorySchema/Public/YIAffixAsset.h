#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Curves/CurveFloat.h"
#include "StructUtils/InstancedStruct.h"
#include "YIAffixAsset.generated.h"

class UYIAttributeModAsset;
class UYIDataTableAffixSource;
class UScriptStruct;

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

USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIAffixDefinitionFragmentBase
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIStaticAffixDefinitionFragment : public FYIAffixDefinitionFragmentBase
{
	GENERATED_BODY()

	/** When true, this fragment fully overrides legacy UYIAffixAsset fields. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	bool bOverrideLegacyFields = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment", meta=(MultiLine=true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	FText TooltipFormat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	EYIAffixKind Kind = EYIAffixKind::Prefix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	EYIAffixQuality Quality = EYIAffixQuality::Magic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment", meta=(ClampMin="1"))
	int32 Tier = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment", meta=(ClampMin="0"))
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	TArray<TSoftObjectPtr<UYIAttributeModAsset>> AttributeMods;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	float MinValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	float MaxValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment", meta=(ClampMin="0"))
	int32 PowerLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	FRuntimeFloatCurve ValueByLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	FGameplayTagContainer AllowedItemTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragment")
	FName ConflictGroup = NAME_None;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYSCHEMA_API FYIAffixResolvedDefinitionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
	FText TooltipFormat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
	EYIAffixKind Kind = EYIAffixKind::Prefix;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
	EYIAffixQuality Quality = EYIAffixQuality::Magic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
	int32 Tier = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
	float Weight = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
	TArray<TSoftObjectPtr<UYIAttributeModAsset>> AttributeMods;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
	float MinValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
	float MaxValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
	int32 PowerLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
	FRuntimeFloatCurve ValueByLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
	FGameplayTagContainer AllowedItemTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix")
	FName ConflictGroup = NAME_None;
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
class YOLOINVENTORYSCHEMA_API UYIAffixAsset : public UObject
{
    GENERATED_BODY()
public:
    // Identity
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

    /** Additional power factor for this affix (acts as a multiplier during roll). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Roll", meta=(ClampMin="0", ToolTip="Power level multiplier applied to the rolled value during generation (1 = no change)"))
    int32 PowerLevel = 1;

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

    // Data source linkage (optional)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Source", meta=(ToolTip="Data source that generated this affix asset (optional)."))
    TSoftObjectPtr<UYIDataTableAffixSource> SourceDataSource;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Source", meta=(ToolTip="Row name in the data source that generated this affix (optional)."))
    FName SourceRowName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Source", meta=(ToolTip="True if this affix was generated from a data source."))
    bool bGeneratedFromDataSource = false;

	/** Shared static fragments for this affix definition (loaded once and reused by runtime snapshots). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affix|Fragments", meta=(BaseStruct="/Script/YOLOInventory.YIAffixDefinitionFragmentBase", ExcludeBaseStruct))
	TArray<FInstancedStruct> DefinitionFragments;

	/** Returns static affix definition fragment if present. */
	const FYIStaticAffixDefinitionFragment* GetStaticDefinitionFragment() const;

	/** Find static affix definition fragment by script struct (exact type). */
	const FInstancedStruct* FindDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct) const;

	/** Find or add static affix definition fragment by script struct (exact type). */
	FInstancedStruct* FindOrAddDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct);

	/** Resolve effective authoring data from static fragment with legacy-field fallback. */
	void GetEffectiveDefinitionData(FYIAffixResolvedDefinitionData& OutData) const;

#if WITH_EDITOR
    /** Debug helper: sample a roll using the configured rules (callable in editor). */
    UFUNCTION(CallInEditor, Category="Affix|Debug")
    float SampleRoll(int32 Level, int32 Seed) const;
#endif
};
