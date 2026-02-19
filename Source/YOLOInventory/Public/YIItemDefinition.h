#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
// #include "YIUnlock.h" // unlock removed
// #include "YICapabilities.h" // removed capabilities
#include "YICapability.h"
#include "YIRequirement.h"
#include "YIEvolutionPath.h"
#include "YIItemVariant.h"
#include "YIScriptGraph.h"
#include "YIItemSFXLibrary.h"
#include "YIItemFragments.h"
#include "YIItemDefinition.generated.h"

class UYIDataTableItemSource;
class UYIInventoryBag;
class UTexture2D;
class UScriptStruct;

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
	/** Explicit override for advanced users who intentionally want mutable/randomized items to stack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stacking", AdvancedDisplay, meta=(EditCondition="bAllowStacking", ToolTip="If enabled, bypasses stack-safety validation and allows stacking even when this item has mutable/randomized state."))
	bool bAllowUnsafeStacking = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stacking", meta=(ToolTip="If true, only a single instance of this item type may exist (unique per type)"))
	bool bUniquePerType = false;

	// Grid sizing/placement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layout", meta=(ToolTip="Default grid size (width,height) this item occupies when placed in a bag"))
	FIntPoint DefaultSize = FIntPoint(1,1);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Layout", meta=(ToolTip="Whether the item can be rotated in grid-based inventories"))
	bool bAllowRotation = true;

	/** When enabled, each runtime instance of this item owns an inner bag (container-in-container). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container", meta=(ToolTip="If true, this item behaves as a container and gets a nested runtime bag instance."))
	bool bIsContainerItem = false;

	/** Optional template bag used to initialize nested container layout/content. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container", meta=(EditCondition="bIsContainerItem", ToolTip="Optional template bag cloned when this container item instance is first initialized."))
	TSoftObjectPtr<UYIInventoryBag> ContainerTemplateBag;

	/** Default nested bag size when no container template is provided. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container", meta=(EditCondition="bIsContainerItem", ClampMin="1", ToolTip="Default nested bag grid size for container items without a template."))
	FIntPoint ContainerDefaultGridSize = FIntPoint(6, 8);

	/** If set, item will occupy all these equipment slots when equipped (multi-slot items such as two-handed weapons). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment", meta=(ToolTip="Slots occupied by this item while equipped. If empty, only requested/auto-resolved slot is occupied."))
	FGameplayTagContainer OccupiedEquipSlots;

	/** Cost consumed from the target equipment slot capacity when this item is equipped. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment", meta=(ClampMin="1", ToolTip="How much slot capacity this item requires. Example: chest armor can use 6 while lighter armor uses 4."))
	int32 EquipSlotCost = 1;

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

	/** Shared static fragments for this definition (loaded once and shared by all instances). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragments", meta=(BaseStruct="/Script/YOLOInventory.YIItemDefinitionFragmentBase", ExcludeBaseStruct))
	TArray<FInstancedStruct> DefinitionFragments;

	/** Returns UI definition fragment if present. */
	const FYIItemUIDefinitionFragment* GetUIDefinitionFragment() const;

	/** Returns pickup definition fragment if present. */
	const FYIItemPickupDefinitionFragment* GetPickupDefinitionFragment() const;

	/** Returns equipment definition fragment if present. */
	const FYIItemEquipmentDefinitionFragment* GetEquipmentDefinitionFragment() const;

	/** Returns affix definition fragment if present. */
	const FYIItemAffixDefinitionFragment* GetAffixDefinitionFragment() const;

	/** Returns layout definition fragment if present. */
	const FYIItemLayoutDefinitionFragment* GetLayoutDefinitionFragment() const;

	/** Resolve display fields from static UI fragment with fallback to legacy fields. */
	void GetEffectiveDisplayData(FText& OutDisplayName, FText& OutDescription, TSoftObjectPtr<UTexture2D>& OutIcon) const;

	/** Resolve preferred equip slot from static equipment fragment with fallback to tag-based legacy resolution. */
	FGameplayTag GetEffectivePrimaryEquipSlotTag() const;

	/** Resolve occupied equip slots from static equipment fragment with fallback to legacy OccupiedEquipSlots. */
	void GetEffectiveOccupiedEquipSlots(FGameplayTagContainer& OutOccupiedSlots) const;

	/** Find static definition fragment by script struct (exact type). */
	const FInstancedStruct* FindDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct) const;

	/** Find or add static definition fragment by script struct (exact type). */
	FInstancedStruct* FindOrAddDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct);

	/** Returns stacking definition fragment if present. */
	const FYIItemStackingDefinitionFragment* GetStackingDefinitionFragment() const;

	/** Resolve effective affix generation data from legacy fields + optional static fragment override. */
	void GetEffectiveAffixDefinition(
		TArray<TSoftObjectPtr<class UYIAffixAsset>>& OutTemplateAffixes,
		int32& OutMinRandomModifiers,
		int32& OutMaxRandomModifiers,
		TSoftObjectPtr<class UYIAffixPoolAsset>& OutPrefixPool,
		TSoftObjectPtr<class UYIAffixPoolAsset>& OutSuffixPool) const;

	/** Resolve effective layout rules from legacy fields + optional static fragment override. */
	void GetEffectiveLayoutData(FIntPoint& OutDefaultSize, bool& bOutAllowRotation) const;

	/** Effective default size helper. */
	FIntPoint GetEffectiveDefaultSize() const;

	/** Effective rotation policy helper. */
	bool IsEffectiveRotationAllowed() const;

	/** Resolve effective stacking rules from legacy fields + optional static fragment override. */
	void GetEffectiveStackingRules(bool& bOutAllowStacking, int32& OutMaxStackCount, bool& bOutUseRiskChecks) const;

	/** Returns whether this item can be safely stacked under runtime safety rules. */
	bool IsRuntimeStackingAllowed(FString* OutReason = nullptr) const;

	/** Returns true if this item has mutable/randomized state that usually should not be stacked. */
	bool HasStackingRisk(FString* OutReason = nullptr) const;
	// Editor safety: auto-assign unique code if zero on save (and ensure no collision)
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
#endif
};
