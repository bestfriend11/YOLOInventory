#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffectTypes.h"
#include "YIAttributeDef.generated.h"

UCLASS(BlueprintType)
class UYIAttributeDef : public UDataAsset
{
	GENERATED_BODY()
public:
	// Keyword used by our system (e.g., Strength, Intelligence)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attribute")
	FName Keyword;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attribute")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attribute", meta=(MultiLine=true))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attribute")
	TSoftObjectPtr<UTexture2D> Icon;

	// Optional link to a GAS attribute for runtime lookup
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attribute")
	FGameplayAttribute GameplayAttribute;

	// Color used to render this attribute in editor/game UIs (e.g., Strength = red)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Attribute|UI")
	FLinearColor DisplayColor = FLinearColor::White;
};
