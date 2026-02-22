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

	/** Optional runtime-fragment templates copied into generated item instances by runtime systems. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragments", meta=(BaseStruct="/Script/YOLOInventorySchema.YIItemFragmentBase", ExcludeBaseStruct))
	TArray<FInstancedStruct> DefaultInstanceFragments;

	/** Find static definition fragment by exact struct type. */
	const FInstancedStruct* FindDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct) const;

	/** Find or add static definition fragment by exact struct type. */
	FInstancedStruct* FindOrAddDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct);

	/** Find or add runtime-fragment template by exact struct type. */
	FInstancedStruct* FindOrAddDefaultInstanceFragmentByStruct(const UScriptStruct* FragmentStruct);

	/** Export decoupled suite-facing snapshot for non-schema consumers. */
	void BuildSchemaSnapshot(FYIItemSchemaSnapshot& OutSnapshot) const;

	/** Returns whether runtime stacking is currently allowed by fragment policy/risk rules. */
	bool IsRuntimeStackingAllowed(FString* OutReason = nullptr) const;

	/** Returns true when stacking should be considered unsafe due to mutable/randomized item state. */
	bool HasStackingRisk(FString* OutReason = nullptr) const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
#endif
};
