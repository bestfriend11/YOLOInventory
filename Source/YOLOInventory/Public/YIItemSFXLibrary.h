#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Sound/SoundBase.h"
#include "YIItemSFXLibrary.generated.h"

UENUM(BlueprintType)
enum class EYIItemSFXEvent : uint8
{
	HoverItem,
	DragStart,
	Drop,
	Cancel,
	InvalidMove
};

UCLASS(BlueprintType)
class YOLOINVENTORY_API UYIItemSFXProfile : public UDataAsset
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
	TObjectPtr<USoundBase> CancelSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="SFX")
	TObjectPtr<USoundBase> InvalidMoveSound = nullptr;
};

UCLASS(BlueprintType)
class YOLOINVENTORY_API UYIItemSFXLibrary : public UDataAsset
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
