#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "YIItemFragments.h"
#include "YIItemSchemaTypes.h"
#include "YIItemDefinition.generated.h"

class UYIDataTableItemSource;
class UYIItemTraitAsset;
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

	/** Optional parent definition for base-mod authoring; local fragments override parent fragments by struct type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Authoring", meta=(ToolTip="Optional parent definition used as a shared baseline"))
	TSoftObjectPtr<UYIItemDefinition> ParentDefinition;

	/** Optional reusable trait bundles. Later traits override earlier traits by fragment struct type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Authoring", meta=(ToolTip="Optional trait assets that contribute reusable definition fragments"))
	TArray<TSoftObjectPtr<UYIItemTraitAsset>> Traits;

	/** Optional requirement objects that gate use/equip. */
	UPROPERTY(EditAnywhere, Instanced, Category="Requirements", meta=(ToolTip="Optional requirement objects (level/quest/etc)"))
	TArray<TObjectPtr<UYIRequirement>> Requirements;

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
	const FYIItemClassificationDefinitionFragment* GetClassificationDefinitionFragment() const;
	const FYIItemAudioDefinitionFragment* GetAudioDefinitionFragment() const;
	const FYIItemPickupDefinitionFragment* GetPickupDefinitionFragment() const;
	const FYIItemEquipmentDefinitionFragment* GetEquipmentDefinitionFragment() const;
	const FYIItemAffixDefinitionFragment* GetAffixDefinitionFragment() const;
	const FYIItemLayoutDefinitionFragment* GetLayoutDefinitionFragment() const;
	const FYIItemStackingDefinitionFragment* GetStackingDefinitionFragment() const;
	const FYIItemRulesDefinitionFragment* GetRulesDefinitionFragment() const;
	const FYIItemContainerDefinitionFragment* GetContainerDefinitionFragment() const;
	const FYIItemAttributeModsDefinitionFragment* GetAttributeModsDefinitionFragment() const;

	/** Resolve display payload from UI fragment with asset-name fallback. */
	void GetEffectiveDisplayData(FText& OutDisplayName, FText& OutDescription, TSoftObjectPtr<UTexture2D>& OutIcon) const;
	FText GetEffectiveDisplayName() const;
	FText GetEffectiveDescription() const;
	TSoftObjectPtr<UTexture2D> GetEffectiveIcon() const;
	FGameplayTag GetEffectiveItemType() const;
	void GetEffectiveTags(FGameplayTagContainer& OutTags) const;
	FGameplayTag GetEffectiveRarityTag() const;
	FGameplayTag GetEffectiveAudioTag() const;
	TSoftObjectPtr<UYIItemSFXProfile> GetEffectiveSoundProfileOverride() const;
	bool IsEffectiveUniquePerType() const;
	int32 GetEffectiveEquipSlotCost() const;
	bool IsEffectiveContainerItem() const;
	TSoftObjectPtr<UObject> GetEffectiveContainerTemplateBag() const;
	FIntPoint GetEffectiveContainerDefaultGridSize() const;
	void GetEffectiveAttributeMods(TArray<TSoftObjectPtr<UYIAttributeModAsset>>& OutAttributeMods) const;

	/** Resolve preferred equip slot from equipment fragment. */
	FGameplayTag GetEffectivePrimaryEquipSlotTag() const;

	/** Resolve occupied equip slots from equipment fragment. */
	void GetEffectiveOccupiedEquipSlots(FGameplayTagContainer& OutOccupiedSlots) const;

	/** Find static definition fragment by exact struct type. */
	const FInstancedStruct* FindDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct) const;
	const FInstancedStruct* FindResolvedDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct) const;

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
