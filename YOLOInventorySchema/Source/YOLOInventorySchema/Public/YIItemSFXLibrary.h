#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Sound/SoundBase.h"
#include "YIItemSFXLibrary.generated.h"

UENUM(BlueprintType)
enum class EYIItemSFXEvent : uint8
{
	HoverItem = 0,
	DragStart = 1,
	Drop = 2,
	Cancel = 3,
	InvalidMove = 4,
	Equip = 5
};

UCLASS(BlueprintType)
class YOLOINVENTORYSCHEMA_API UYIItemSFXProfile : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX")
	TObjectPtr<USoundBase> HoverItemSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX")
	TObjectPtr<USoundBase> DragStartSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX")
	TObjectPtr<USoundBase> DropSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX")
	TObjectPtr<USoundBase> EquipSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX")
	TObjectPtr<USoundBase> CancelSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX")
	TObjectPtr<USoundBase> InvalidMoveSound = nullptr;
};

UCLASS(BlueprintType)
class YOLOINVENTORYSCHEMA_API UYIItemSFXLibrary : public UDataAsset
{
	GENERATED_BODY()
public:
	/** Default profile used when no tag-specific profile is found. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX")
	TObjectPtr<UYIItemSFXProfile> DefaultProfile = nullptr;

	/** Tag to SFX profile map (use ItemType or AudioTag). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX")
	TMap<FGameplayTag, TObjectPtr<UYIItemSFXProfile>> TagToProfile;
};
