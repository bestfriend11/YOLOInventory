#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "YIPlayerInventoryStateComponent.h" // for snapshots/resources
#include "YIInventorySaveGame.generated.h"

/** Persisted shared bag entry (keeps source reference and optional runtime snapshot). */
USTRUCT()
struct YOLOINVENTORY_API FYIPersistedSharedBagEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TSoftObjectPtr<UYIInventoryBag> BagAsset;

	UPROPERTY()
	bool bHasSnapshot = false;

	UPROPERTY()
	FYISavedBagSnapshot Snapshot;
};

/** Persisted party member entry with optional bag snapshot. */
USTRUCT()
struct YOLOINVENTORY_API FYIPersistedPartyMemberEntry
{
	GENERATED_BODY()

	UPROPERTY()
	FYIPartyMemberEntry Member;

	UPROPERTY()
	bool bHasInventorySnapshot = false;

	UPROPERTY()
	FYISavedBagSnapshot InventorySnapshot;
};

/** Minimal savegame that stores player-owned bag snapshots & resources. */
UCLASS()
class YOLOINVENTORY_API UYIInventorySaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	// Saved shared bags/stash state.
	UPROPERTY()
	TArray<FYIPersistedSharedBagEntry> SavedSharedBags;

	// Saved party member state.
	UPROPERTY()
	TArray<FYIPersistedPartyMemberEntry> SavedPartyMembers;

	// Saved bag snapshots (already authority-only on PlayerState component).
	UPROPERTY()
	TArray<FYISavedBagSnapshot> SavedBags;

	// Saved resources/currencies.
	UPROPERTY()
	FYIResourceWallet SavedResources;

	// Last observed/active party index.
	UPROPERTY()
	int32 SavedObservedPartyIndex = 0;
};
