#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
// #include "YIUnlock.h" // unlock removed
// #include "YICapabilities.h" // removed capabilities
#include "YICapability.h"
#include "YIRequirement.h"
#include "YIEvolutionPath.h"
#include "YIItemVariant.h"
#include "YIScriptGraph.h"
#include "YIItemSFXLibrary.h"
#include "YIItemDefinition.generated.h"

class UYIDataTableItemSource;

/**
 * Primary item definition. Each definition must have a globally-unique numeric code.
 *
 * Designer guidance:
 * - Use TemplateId to map items to external template systems or for runtime lookups.
 * - Use Tags and ItemType to drive classification and filter behaviors.
 */
UCLASS(BlueprintType)
class UYIItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	/**
	 * Globally unique numeric code for this item definition (e.g., Skyrim-like form ID).
	 * Keep unique across the project; the editor will auto-assign one if left at 0.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity", meta=(ToolTip="Project-unique numeric identifier (auto-assigned if zero in editor)"))
	int64 UniqueCode = 0;

	/** Optional string identifier used by external template systems and lookups (e.g., 'weapon_fire_01'). Useful for scripting/template queries. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Template", meta=(ToolTip="Optional string identifier used by template queries (e.g., 'weapon_fire_01')"))
	FString TemplateId;

	// Display
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Display", meta=(ToolTip="Gameplay-facing display name for the item"))
	FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Display", meta=(MultiLine=true, ToolTip="Long description shown in tooltips and item inspect views"))
	FText Description;

	// Classification (use gameplay tags to allow designers to extend types and rarity)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Classification", meta=(ToolTip="Primary gameplay tag classifying this item, e.g., Item.Weapon.Sword"))
	FGameplayTag ItemType;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Classification", meta=(ToolTip="Additional tags used for filtering and gameplay rules"))
	FGameplayTagContainer Tags;

	/** Designer-controlled rarity tag, e.g., Rarity.Common. Use tags so designers can customize rarity sets and UI color mapping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Classification", meta=(ToolTip="Designer-controlled rarity tag (e.g., Rarity.Common)"))
	FGameplayTag RarityTag;

	/** Optional audio tag used to resolve item SFX (falls back to ItemType if unset). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ToolTip="Optional audio tag used to resolve item SFX (falls back to ItemType if unset)"))
	FGameplayTag AudioTag;

	/** Optional per-item SFX override (highest priority). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ToolTip="Optional per-item SFX override (highest priority)"))
	TObjectPtr<UYIItemSFXProfile> SoundProfileOverride = nullptr;

	// Stacking/uniqueness
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stacking", meta=(ToolTip="Whether multiple counts of this item can be stacked in a single slot"))
	bool bAllowStacking = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stacking", meta=(EditCondition="bAllowStacking", ClampMin="1", ToolTip="Maximum items in a single stack"))
	int32 MaxStackCount = 99;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stacking", meta=(ToolTip="If true, only a single instance of this item type may exist (unique per type)"))
	bool bUniquePerType = false;

	// Grid sizing/placement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layout", meta=(ToolTip="Default grid size (width,height) this item occupies when placed in a bag"))
	FIntPoint DefaultSize = FIntPoint(1,1);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layout", meta=(ToolTip="Whether the item can be rotated in grid-based inventories"))
	bool bAllowRotation = true;

	// Attributes defaults (legacy, not used when GAS is authoritative)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attributes", meta=(EditCondition=false, EditConditionHides, ToolTip="Legacy per-name attribute defaults; use AttributeMods/GAS for runtime values"))
	TMap<FName,float> AttributeDefaults;
	// New Blueprintable capability entries
	// Capabilities removed

	// Requirements (Blueprintable objects)
	UPROPERTY(EditAnywhere, Instanced, Category="Requirements", meta=(ToolTip="Optional blueprint requirement objects (e.g., level, quest state) that gate use or equip") )
	TArray<TObjectPtr<UYIRequirement>> Requirements;

	// GAS-backed attribute mods (designer-authored, applied via GE with SetByCaller)
	UPROPERTY(EditAnywhere, Category="Attributes", meta=(ToolTip="List of attribute mod assets that apply to instances of this item"))
	TArray<TSoftObjectPtr<class UYIAttributeModAsset>> AttributeMods;

	// Template affixes applied to all instances of this item (implicit/prefix/suffix templates)
	UPROPERTY(EditAnywhere, Category="Affixes", meta=(ToolTip="Affix assets that are implicitly applied to every instance of this definition"))
	TArray<TSoftObjectPtr<class UYIAffixAsset>> TemplateAffixes;

	// Runtime generator constraints for randomized modifiers
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affixes|Roll", meta=(ClampMin="0", ToolTip="Minimum number of randomized modifiers to roll for this item"))
	int32 MinRandomModifiers = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affixes|Roll", meta=(ClampMin="0", ToolTip="Maximum number of randomized modifiers to roll for this item"))
	int32 MaxRandomModifiers = 0;

	// Optional pools for randomized prefixes/suffixes
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affixes", meta=(ToolTip="Optional affix pool used to sample prefixes for randomized item generation"))
	TSoftObjectPtr<class UYIAffixPoolAsset> PrefixPool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Affixes", meta=(ToolTip="Optional affix pool used to sample suffixes for randomized item generation"))
	TSoftObjectPtr<class UYIAffixPoolAsset> SuffixPool;

	// Optional: Icon shortcut in definition (UI capability may also provide this)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Display", meta=(ToolTip="Optional source icon used for thumbnails and UIs"))
	TSoftObjectPtr<UTexture2D> Icon;

	// Optional source linkage for dashboard-driven row regeneration.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source Link", meta=(ToolTip="If set, this item was generated from this data source and can be re-generated from dashboard"))
	TSoftObjectPtr<UYIDataTableItemSource> SourceDataSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source Link", meta=(ToolTip="Data table row name used when this item was generated from a source"))
	FName SourceRowName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source Link", meta=(ToolTip="Whether this item is linked to a data source row"))
	bool bGeneratedFromDataSource = false;
	// Editor safety: auto-assign unique code if zero on save (and ensure no collision)
#if WITH_EDITOR
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
#endif
};
