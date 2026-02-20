#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "YIAttributeModAsset.h"
#include "YIItemVariant.generated.h"

class UYIItemDefinition;

UCLASS(BlueprintType)
class YOLOINVENTORYLEGACYBRIDGE_API UYIItemVariantAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	// The base item definition this variant belongs to (for editor grouping and validation)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Variant")
	TSoftObjectPtr<UYIItemDefinition> BaseDefinition;

	// Optional display overrides
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Display")
	FText DisplayNameOverride;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Display")
	TSoftObjectPtr<UTexture2D> IconOverride;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Display")
	FLinearColor TintOverride = FLinearColor::Transparent;

	// Attribute mods added by this variant (merged with base definition mods)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attributes")
	TArray<TSoftObjectPtr<UYIAttributeModAsset>> AttributeMods;

	// Optional tags to classify the variant (e.g., Item.Variant.Fire)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Tags")
	FGameplayTagContainer Tags;
};
