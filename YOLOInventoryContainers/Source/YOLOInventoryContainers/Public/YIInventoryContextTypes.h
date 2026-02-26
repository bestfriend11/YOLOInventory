#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YIInventoryContextTypes.generated.h"

USTRUCT(BlueprintType)
struct YOLOINVENTORYCONTAINERS_API FYIActiveBagContextEntry
{
	GENERATED_BODY()

	/** Semantic UI/gameplay context tag (for example UI.Context.Secondary, UI.Context.CraftingSource). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGameplayTag ContextTag;

	/** Runtime bag currently assigned to this context. Owner-only replicated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FGuid BagId;
};

