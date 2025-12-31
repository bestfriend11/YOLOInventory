#pragma once
#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Sound/SoundBase.h"
#include "GameplayTagContainer.h"
class UFXSystemAsset;
#include "YICapabilities.generated.h"

// Legacy InstancedStruct marker removed; using Blueprintable capability objects now.
USTRUCT(BlueprintType)
struct FYICapabilitySpec
{
	GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FYICap_UI : public FYICapabilitySpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI")
	TSoftObjectPtr<USkeletalMesh> MeshSkeletal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI")
	TSoftObjectPtr<UStaticMesh> MeshStatic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI")
	TSoftObjectPtr<UFXSystemAsset> Effect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI")
	TSoftObjectPtr<USoundBase> DropSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="UI")
	FLinearColor RarityColorOverride = FLinearColor::Transparent;
};

USTRUCT(BlueprintType)
struct FYICap_Equip : public FYICapabilitySpec
{
	GENERATED_BODY()

	// Slot tag (e.g., Equip.Weapon.MainHand)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equip")
	FGameplayTag SlotTag;

	// Additional constraints (optional)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equip")
	FGameplayTagContainer Constraints;
};

USTRUCT(BlueprintType)
struct FYICap_Durability : public FYICapabilitySpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Durability")
	float MaxDurability = 100.f;
};

USTRUCT(BlueprintType)
struct FYICap_Economy : public FYICapabilitySpec
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Economy")
	int32 BaseValue = 0;
};
