#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StructUtils/InstancedStruct.h"
#include "YIFragmentAsset.generated.h"

class UScriptStruct;

/**
 * Generic fragment authoring asset.
 * Use this asset to store premade fragment payloads that can be copied into item/affix instances at runtime.
 */
UCLASS(BlueprintType)
class YOLOINVENTORYSCHEMA_API UYIFragmentAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	/** Optional project-unique numeric id used by tooling/importers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
	int64 UniqueCode = 0;

	/** Optional external/template id for lookup pipelines. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
	FString TemplateId;

	/** Designer-facing display name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
	FText DisplayName;

	/** Definition-layer item fragments. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragments|Item", meta=(BaseStruct="/Script/YOLOInventorySchema.YIItemDefinitionFragmentBase", ExcludeBaseStruct))
	TArray<FInstancedStruct> ItemDefinitionFragments;

	/** Definition-layer affix fragments. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fragments|Affix", meta=(BaseStruct="/Script/YOLOInventorySchema.YIAffixDefinitionFragmentBase", ExcludeBaseStruct))
	TArray<FInstancedStruct> AffixDefinitionFragments;

	const FInstancedStruct* FindItemDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct) const;
	FInstancedStruct* FindOrAddItemDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct);

	const FInstancedStruct* FindAffixDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct) const;
	FInstancedStruct* FindOrAddAffixDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct);
};

