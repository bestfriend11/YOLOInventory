#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "YIItemFragments.h"
#include "YIItemSchemaTypes.h"
#include "YIItemDefinition.generated.h"

class UYIDataTableItemSource;
class UYIRequirement;
class UYIItemSFXProfile;
class UYIAttributeModAsset;
class UYIAffixAsset;
class UYIAffixPoolAsset;
class UTexture2D;
class UScriptStruct;

/**
 * Primary item definition.
 * Authoring is fragment-first: definition behavior and UI data come from DefinitionFragments.
 */
UCLASS(BlueprintType)
class YOLOINVENTORYSCHEMA_API UYIItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	/** Project-unique numeric identifier (auto-assigned if zero on save in editor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity", meta=(ToolTip="Project-unique numeric identifier (auto-assigned if zero in editor)"))
	int64 UniqueCode = 0;

	/** Optional external/template identifier used by integrations and scripting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity", meta=(ToolTip="Optional external/template identifier, e.g. 'weapon_fire_01'"))
	FString TemplateId;

	/** Primary gameplay classification tag (e.g. Item.Weapon.Sword). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Classification", meta=(ToolTip="Primary gameplay classification tag"))
	FGameplayTag ItemType;

	/** Additional gameplay tags used by filtering/rules. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Classification", meta=(ToolTip="Additional gameplay tags for filtering and rules"))
	FGameplayTagContainer Tags;

	/** Designer rarity tag for UI/gameplay tuning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Classification", meta=(ToolTip="Designer rarity tag (e.g., Rarity.Common)"))
	FGameplayTag RarityTag;

	/** Optional audio routing tag, falls back to ItemType when unset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ToolTip="Optional audio tag override"))
	FGameplayTag AudioTag;

	/** Optional item-specific SFX profile (highest priority). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Audio", meta=(ToolTip="Optional per-item SFX profile override"))
	TObjectPtr<UYIItemSFXProfile> SoundProfileOverride = nullptr;

	/** If true, only one instance of this item type can exist in a target bag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stacking", meta=(ToolTip="If true, only one instance of this item type may exist in a target bag"))
	bool bUniquePerType = false;

	/** Container items own a nested runtime bag instance (bag-in-bag). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container", meta=(ToolTip="If true, this item behaves as a container item"))
	bool bIsContainerItem = false;

	/** Optional template bag asset used to initialize container runtime bags. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container", meta=(EditCondition="bIsContainerItem", AllowedClasses="/Script/YOLOInventoryContainers.YIInventoryBag", ToolTip="Optional template bag cloned for container item instances"))
	TSoftObjectPtr<UObject> ContainerTemplateBag;

	/** Default nested bag size when no container template is supplied. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Container", meta=(EditCondition="bIsContainerItem", ClampMin="1", ToolTip="Default nested bag size for container items without a template"))
	FIntPoint ContainerDefaultGridSize = FIntPoint(6, 8);

	/** Slot-capacity cost consumed when equipped. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment", meta=(ClampMin="1", ToolTip="Slot capacity cost consumed by this item when equipped"))
	int32 EquipSlotCost = 1;

	/** Optional requirement objects that gate use/equip. */
	UPROPERTY(EditAnywhere, Instanced, Category="Requirements", meta=(ToolTip="Optional requirement objects (level/quest/etc)"))
	TArray<TObjectPtr<UYIRequirement>> Requirements;

	/** GAS-backed attribute-mod assets applied by inventory/equipment systems. */
	UPROPERTY(EditAnywhere, Category="Attributes", meta=(ToolTip="Attribute mod assets applied by this item"))
	TArray<TSoftObjectPtr<UYIAttributeModAsset>> AttributeMods;

	/** Optional source linkage for dashboard regeneration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source Link", meta=(ToolTip="Source asset this item was generated from"))
	TSoftObjectPtr<UYIDataTableItemSource> SourceDataSource;

	/** Source row name used when generated from a source. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source Link", meta=(ToolTip="Source row used to generate this item"))
	FName SourceRowName = NAME_None;

	/** Whether this item is source-linked for regeneration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Source Link", meta=(ToolTip="Whether this item is linked to a source row"))
	bool bGeneratedFromDataSource = false;

	/** Shared static definition fragments (loaded once, used by all item instances). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragments", meta=(BaseStruct="/Script/YOLOInventorySchema.YIItemDefinitionFragmentBase", ExcludeBaseStruct))
	TArray<FInstancedStruct> DefinitionFragments;

	const FYIItemUIDefinitionFragment* GetUIDefinitionFragment() const;
	const FYIItemPickupDefinitionFragment* GetPickupDefinitionFragment() const;
	const FYIItemEquipmentDefinitionFragment* GetEquipmentDefinitionFragment() const;
	const FYIItemAffixDefinitionFragment* GetAffixDefinitionFragment() const;
	const FYIItemLayoutDefinitionFragment* GetLayoutDefinitionFragment() const;
	const FYIItemStackingDefinitionFragment* GetStackingDefinitionFragment() const;

	/** Resolve display payload from UI fragment with asset-name fallback. */
	void GetEffectiveDisplayData(FText& OutDisplayName, FText& OutDescription, TSoftObjectPtr<UTexture2D>& OutIcon) const;
	FText GetEffectiveDisplayName() const;
	FText GetEffectiveDescription() const;
	TSoftObjectPtr<UTexture2D> GetEffectiveIcon() const;

	/** Resolve preferred equip slot from equipment fragment. */
	FGameplayTag GetEffectivePrimaryEquipSlotTag() const;

	/** Resolve occupied equip slots from equipment fragment. */
	void GetEffectiveOccupiedEquipSlots(FGameplayTagContainer& OutOccupiedSlots) const;

	/** Find static definition fragment by exact struct type. */
	const FInstancedStruct* FindDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct) const;

	/** Find or add static definition fragment by exact struct type. */
	FInstancedStruct* FindOrAddDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct);

	/** Ensure baseline required fragments exist. Returns true when fragment array changed. */
	bool EnsureBaselineDefinitionFragments();

	/** Resolve affix-generation payload from affix fragment. */
	void GetEffectiveAffixDefinition(
		TArray<TSoftObjectPtr<UYIAffixAsset>>& OutTemplateAffixes,
		int32& OutMinRandomModifiers,
		int32& OutMaxRandomModifiers,
		TSoftObjectPtr<UYIAffixPoolAsset>& OutPrefixPool,
		TSoftObjectPtr<UYIAffixPoolAsset>& OutSuffixPool) const;

	/** Resolve layout payload from layout fragment. */
	void GetEffectiveLayoutData(FIntPoint& OutDefaultSize, bool& bOutAllowRotation) const;

	FIntPoint GetEffectiveDefaultSize() const;
	bool IsEffectiveRotationAllowed() const;

	/** Export decoupled suite-facing snapshot for non-schema consumers. */
	void BuildSchemaSnapshot(FYIItemSchemaSnapshot& OutSnapshot) const;

	/** Resolve stacking payload from stacking fragment. */
	void GetEffectiveStackingRules(bool& bOutAllowStacking, int32& OutMaxStackCount, bool& bOutUseRiskChecks) const;
	int32 GetEffectiveMaxStackCount() const;
	bool IsEffectiveStackingEnabled() const;

	/** Returns whether runtime stacking is currently allowed by fragment policy/risk rules. */
	bool IsRuntimeStackingAllowed(FString* OutReason = nullptr) const;

	/** Returns true when stacking should be considered unsafe due to mutable/randomized item state. */
	bool HasStackingRisk(FString* OutReason = nullptr) const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
#endif
};
