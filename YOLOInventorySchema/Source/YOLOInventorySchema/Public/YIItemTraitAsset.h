#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "YIItemTraitAsset.generated.h"

class UScriptStruct;
struct FPropertyChangedEvent;

/**
 * Reusable bundle of static item-definition fragments.
 * Item definitions can reference one or more trait assets to compose behavior without copying fragment blocks.
 */
UCLASS(BlueprintType)
class YOLOINVENTORYSCHEMA_API UYIItemTraitAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	/** Optional stable id used by import/build pipelines. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
	int64 UniqueCode = 0;

	/** Optional external/template id for lookup pipelines. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
	FString TemplateId;

	/** Designer-facing label used by dashboards/tooling. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
	FText DisplayName;

	/** Optional tags for filtering trait libraries and presets. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
	FGameplayTagContainer TraitTags;

	/** Item-definition fragments contributed by this trait. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragments", meta=(BaseStruct="/Script/YOLOInventorySchema.YIItemDefinitionFragmentBase", ExcludeBaseStruct))
	TArray<FInstancedStruct> DefinitionFragments;

	const FInstancedStruct* FindDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct) const;
	FInstancedStruct* FindOrAddDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
