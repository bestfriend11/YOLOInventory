#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "YIPlayerInventoryStateComponent.h" // for snapshots/resources
#include "YIInventorySaveGame.generated.h"

/** Minimal savegame that stores player-owned bag snapshots & resources. */
UCLASS()
class YOLOINVENTORY_API UYIInventorySaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	// Saved bag snapshots (already authority-only on PlayerState component).
	UPROPERTY()
	TArray<FYISavedBagSnapshot> SavedBags;

	// Saved resources/currencies.
	UPROPERTY()
	FYIResourceWallet SavedResources;
};
