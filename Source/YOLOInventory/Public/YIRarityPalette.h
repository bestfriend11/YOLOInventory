#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "YIRarityPalette.generated.h"

USTRUCT(BlueprintType)
struct FRarityPaletteEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rarity")
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rarity")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rarity")
	FLinearColor Color = FLinearColor::White;
};

UCLASS(BlueprintType)
class UYIRarityPalette : public UDataAsset
{
	GENERATED_BODY()
public:
	// Ordered list of entries designers can author in the editor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rarity")
	TArray<FRarityPaletteEntry> Entries;
};
