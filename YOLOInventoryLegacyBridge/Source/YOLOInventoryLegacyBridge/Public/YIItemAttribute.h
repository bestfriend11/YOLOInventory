#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "YIItemAttribute.generated.h"

/**
 * An attribute that can be applied to an inventory item. Attributes can reference other assets/blueprints
 * like Gameplay Effects or Abilities to implement behavior. Runtime data-only representation.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class YOLOINVENTORYLEGACYBRIDGE_API UYIItemAttribute : public UObject
{
	GENERATED_BODY()
public:
	// Human-readable name
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attribute")
	FText Name;

	// Optional: link to a blueprint class implementing the effect (e.g., GAS Ability or GE)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attribute")
	TSoftClassPtr<UObject> EffectClass;

	// Optional: scalar magnitude (e.g., +10% ATK)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attribute")
	float Magnitude = 0.f;

	// Optional: tags or metadata
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Attribute")
	TMap<FName, float> Scalars;
};
