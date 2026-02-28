#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YIItemFragments.h"
#include "YIWorldPolicyFragments.generated.h"

/** Static pickup policy applied by world interaction and auto-pickup features. */
USTRUCT(BlueprintType, meta=(DisplayName="Pickup Policy"))
struct YOLOINVENTORYWORLD_API FYIItemPickupPolicyFragment : public FYIItemDefinitionFragmentBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World|Pickup")
	bool bAllowPickup = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World|Pickup")
	bool bAutoPickup = false;

	/** Require ownership relation check in pickup resolver when true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World|Pickup")
	bool bOwnerOnlyPickup = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World|Pickup")
	FGameplayTagContainer RequiredPickupTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World|Pickup")
	FGameplayTagContainer BlockedPickupTags;
};

