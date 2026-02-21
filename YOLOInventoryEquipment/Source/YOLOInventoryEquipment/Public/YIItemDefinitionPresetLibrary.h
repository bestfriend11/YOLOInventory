#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YIItemDefinitionPresetLibrary.generated.h"

class UYIItemDefinition;

UENUM(BlueprintType)
enum class EYIItemDefinitionPreset : uint8
{
	Weapon UMETA(DisplayName="Weapon"),
	Armor UMETA(DisplayName="Armor"),
	Consumable UMETA(DisplayName="Consumable"),
	Container UMETA(DisplayName="Container"),
	Spellbook UMETA(DisplayName="Spellbook")
};

/**
 * Optional preset helpers for rapid authoring.
 * Keeps core schema empty by applying opinionated fragments only when explicitly requested.
 */
UCLASS()
class YOLOINVENTORYEQUIPMENT_API UYIItemDefinitionPresetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Apply a designer-selected preset by adding/updating relevant definition fragments. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="YOLOInventory|Schema|Presets")
	static bool ApplyPresetToItemDefinition(UYIItemDefinition* ItemDefinition, EYIItemDefinitionPreset Preset, bool bOverwritePresetFields = true);
};

