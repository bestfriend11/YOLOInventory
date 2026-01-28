#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/EngineTypes.h" // for FDirectoryPath
#include "YIInventoryBag.h"
#include "YIPlayerInventoryStateComponent.generated.h"

class UYIInventoryBag;
class APawn;

/** Snapshot of a bag for persistence (owner-only replicated). */
USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYISavedBagSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FIntPoint GridSize = FIntPoint(0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	float CellPixelSize = 32.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bAllowRotation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	float MinifyScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FLinearColor GridLineColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FLinearColor OuterLineColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	FLinearColor CellBgColor = FLinearColor::Black;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	float GridThickness = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bShowCellTooltips = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bShowSortingHeaders = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bEnableThumbnails = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bEnableHoverHighlight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bUseTagFilter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	TArray<FName> TagFilters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bUseFolderFilter = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	TArray<FDirectoryPath> FolderFilters;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	bool bAutoMergeOnAdd = true;

	/** Bag item data including positions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	TArray<struct FYIBagItem> Items;
};

/**
 * Party member entry used by the player inventory state component.
 * Stores the pawn class and an assigned inventory bag (soft) so hirelings/pets/mules
 * can be respawned with their own inventory in multiplayer games.
 */
USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIPartyMemberEntry
{
	GENERATED_BODY()

	/** Pawn class to spawn for this party member (hero, pet, mule, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Party")
	TSubclassOf<APawn> PawnClass;

	/** Inventory bag assigned to this member (soft so it can live on PlayerState and survive respawn/travel). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Party")
	TSoftObjectPtr<UYIInventoryBag> InventoryBag;

	/** Designer/display name shown in UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Party")
	FText DisplayName;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIResourceEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources")
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources")
	int64 Amount = 0;
};

/** Generic resource/currency wallet keyed by designer-defined names (Gold, Silver, Iron, Oil, Food, etc.). */
USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIResourceWallet
{
	GENERATED_BODY()

	/** Resource entries; replicated as array for net safety. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Resources")
	TArray<FYIResourceEntry> Entries;

	/** Convenience getter. Returns 0 if missing. */
	int64 Get(const FName& Name) const
	{
		if (const FYIResourceEntry* Ptr = Entries.FindByPredicate([&](const FYIResourceEntry& E){ return E.Name == Name; }))
		{
			return Ptr->Amount;
		}
		return 0;
	}

	/** Add (or subtract) a delta. Negative is spend. Returns new value. */
	int64 Add(const FName& Name, int64 Delta)
	{
		if (FYIResourceEntry* Ptr = Entries.FindByPredicate([&](const FYIResourceEntry& E){ return E.Name == Name; }))
		{
			Ptr->Amount = FMath::Max<int64>(0, Ptr->Amount + Delta);
			return Ptr->Amount;
		}
		FYIResourceEntry NewEntry;
		NewEntry.Name = Name;
		NewEntry.Amount = FMath::Max<int64>(0, Delta);
		Entries.Add(NewEntry);
		return NewEntry.Amount;
	}

	/** Try to consume Amount; returns false if insufficient. */
	bool Consume(const FName& Name, int64 Amount)
	{
		if (Amount <= 0) return true;
		if (FYIResourceEntry* Ptr = Entries.FindByPredicate([&](const FYIResourceEntry& E){ return E.Name == Name; }))
		{
			if (Ptr->Amount < Amount) return false;
			Ptr->Amount -= Amount;
			return true;
		}
		return false;
	}
};

/**
 * Player-level inventory state meant to be added to PlayerState.
 * Holds shared stash and party member inventories; pawns pull bags from here on spawn.
 *
 * Usage (plug & play):
 * 1) Add this component to your PlayerState blueprint/class.
 * 2) At pawn spawn/possession, call AssignInventoryToPawn(Server) with the party index
 *    you want bound; the component will set the pawn's UYIInventoryComponent EquippedBag.
 * 3) Use AddPartyMember / AddSharedBag on the server to build your party; data replicates
 *    owner-only to the owning client for UI.
 */
UCLASS(ClassGroup=(Inventory), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class YOLOINVENTORY_API UYIPlayerInventoryStateComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UYIPlayerInventoryStateComponent();

	// Shared stash bags (owner-only replicated). Soft references survive travel.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_SharedBags, Category="Inventory")
	TArray<TSoftObjectPtr<UYIInventoryBag>> SharedBags;

	// Party members (hirelings/pets/mules). Owner-only replicated.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Party, Category="Inventory")
	TArray<FYIPartyMemberEntry> PartyMembers;

	// Generic resources/currencies (owner-only replicated). Designer adds any key (Gold/Silver/Iron/Oil/etc.).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_Resources, Category="Inventory")
	FYIResourceWallet Resources;

	/** Add a shared stash bag (server only). Returns index or -1. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|PlayerState", BlueprintAuthorityOnly)
	int32 AddSharedBag(UYIInventoryBag* Bag);

	/** Add a party member entry with a bag (server only). Returns index or -1. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|PlayerState", BlueprintAuthorityOnly)
	int32 AddPartyMember(const FYIPartyMemberEntry& Entry);

	/** Assign a party member's bag to a pawn's inventory component (server only). */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|PlayerState", BlueprintAuthorityOnly)
	bool AssignInventoryToPawn(APawn* Pawn, int32 PartyIndex);

	/** Add or subtract a resource amount (server only). */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|PlayerState", BlueprintAuthorityOnly)
	void AddResource(FName ResourceName, int64 Delta);

	/** Try to consume a resource amount (server only). Returns success. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|PlayerState", BlueprintAuthorityOnly)
	bool ConsumeResource(FName ResourceName, int64 Amount);

	/** Get a resource amount (pure, reads replicated wallet). */
	UFUNCTION(BlueprintPure, Category="YOLOInventory|PlayerState")
	int64 GetResourceAmount(FName ResourceName) const;

	/** Server: save the currently equipped runtime bag from a pawn into owner-only replicated snapshot. */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|PlayerState", BlueprintAuthorityOnly)
	bool SaveCurrentPawnInventory(APawn* Pawn);

	/** Server: restore the saved snapshot into the pawn's inventory component (runtime bag clone). */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|PlayerState", BlueprintAuthorityOnly)
	bool RestoreInventoryToPawn(APawn* Pawn);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_SharedBags() {}

	UFUNCTION()
	void OnRep_Party() {}

	UFUNCTION()
	void OnRep_Resources() {}

	UFUNCTION()
	void OnRep_SavedBags() {}

	/** Owner-only replicated snapshot(s) of saved pawn inventory bags. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing=OnRep_SavedBags, Category="Inventory")
	TArray<FYISavedBagSnapshot> SavedBags;
};
