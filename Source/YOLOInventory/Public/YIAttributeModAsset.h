#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "YIAttributeDef.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "YIAttributeModAsset.generated.h"

UCLASS(BlueprintType)
class YOLOINVENTORY_API UYIAttributeModAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	// Which attribute this modifies (designer-authored)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attribute")
	TSoftObjectPtr<UYIAttributeDef> Attribute;

	// Flat magnitude to add (e.g., +10 Strength)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attribute")
	float Magnitude = 0.f;

	// Optional GameplayEffect template to use when applying via GAS
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS")
	TSubclassOf<UGameplayEffect> GameplayEffectClass;

	// SetByCaller parameter tag expected by the GameplayEffect (e.g., Data.Attribute.Strength)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS")
	FGameplayTag ParamTag;
};
