#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YIInventoryTypes.h"
#include "YIInventoryBag.h"
#include "YIResourceWalletTypes.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "YITradeSessionActor.generated.h"

class UYIInventoryComponent;
class APlayerController;

UENUM(BlueprintType)
enum class ETradeSide : uint8
{
	SideA,
	SideB
};

/** Lightweight offer visible to UI (replicated owner-only). */
USTRUCT(BlueprintType)
struct YOLOINVENTORYTRADE_API FYITradeOffer
{
	GENERATED_BODY()

	/** Minimal, net-safe item data (code/count/pos/size/custom key). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FYINetBagItem> Items;

	/** Optional resource/currency offer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FYIResourceWallet Resources;
};

/** Internal server-only record of an offered slice pulled from a bag. */
struct FYITradeOfferSource
{
	TWeakObjectPtr<UYIInventoryComponent> SourceInv;
	int32 SlotIndex = INDEX_NONE;
	int32 Count = 0;
	FYIBagItem ItemCopy; // keeps CustomStackKey/affixes
};

/**
 * Lightweight, authority-owned trade session between two parties (player-player or player-NPC).
 * - Replicates owner-only to each participant so their UI can show both offers.
 * - Does not directly mutate inventories until Commit is called on the server.
 */
UCLASS()
class YOLOINVENTORYTRADE_API AYITradeSessionActor : public AActor
{
	GENERATED_BODY()
public:
	AYITradeSessionActor();

	// Participants (server authoritative)
	UPROPERTY(Replicated)
	TObjectPtr<APlayerState> PlayerA;

	UPROPERTY(Replicated)
	TObjectPtr<APlayerState> PlayerB; // nullptr when trading with NPC

	UPROPERTY(Replicated)
	bool bSideBIsNPC = false;

	/** Cached pawn refs for quick inventory access. */
	UPROPERTY(Replicated)
	TObjectPtr<APawn> PawnA;

	UPROPERTY(Replicated)
	TObjectPtr<APawn> PawnB;

	/** Pawn used for side B if NPC (must have inventory component). */
	UPROPERTY(Replicated)
	TObjectPtr<APawn> NPCPawn;

	// Offers (replicate to relevant clients; session should be relevant only to participants)
	UPROPERTY(ReplicatedUsing=OnRep_Offers, Transient)
	FYITradeOffer OfferA;

	UPROPERTY(ReplicatedUsing=OnRep_Offers, Transient)
	FYITradeOffer OfferB;

	UPROPERTY(Replicated)
	bool bAReady = false;

	UPROPERTY(Replicated)
	bool bBReady = false;

	/** Full inventory snapshots (net-safe) for each side, owner-only replication. */
	UPROPERTY(ReplicatedUsing=OnRep_Inventories, Transient)
	TArray<FYINetBagItem> InventoryA;

	UPROPERTY(ReplicatedUsing=OnRep_Inventories, Transient)
	TArray<FYINetBagItem> InventoryB;
	
	/** Grid sizes for the mirrored inventories (per side). */
	UPROPERTY(ReplicatedUsing=OnRep_Inventories, Transient)
	FIntPoint InventorySizeA = FIntPoint(10,6);

	UPROPERTY(ReplicatedUsing=OnRep_Inventories, Transient)
	FIntPoint InventorySizeB = FIntPoint(10,6);

	// UI events (owning client)
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTradeSimpleEvent);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTradeInventoryEvent);

	UPROPERTY(BlueprintAssignable, Category="Trade")
	FTradeSimpleEvent OnOffersUpdated;

	UPROPERTY(BlueprintAssignable, Category="Trade")
	FTradeSimpleEvent OnTradeCommitted;

	UPROPERTY(BlueprintAssignable, Category="Trade")
	FTradeSimpleEvent OnTradeCancelled;

	UPROPERTY(BlueprintAssignable, Category="Trade")
	FTradeSimpleEvent OnTradeFailed;

	/** Fired when full inventory snapshots update. */
	UPROPERTY(BlueprintAssignable, Category="Trade")
	FTradeInventoryEvent OnInventoriesUpdated;

	// --- Session internals ---
	UFUNCTION(Server, Reliable)
	void ServerCancel();

	/** Authority helper with explicit success/failure reason for structured request APIs. */
	bool TryTransferItemBetweenSides(ETradeSide FromSide, ETradeSide ToSide, int32 SourceIndex, FIntPoint DestPos, int32 Count, FText& OutError);

	/** Authority helper for adding offered inventory slices (non-mutating until commit). */
	bool TryAddOfferItem(ETradeSide Side, UYIInventoryComponent* SourceInv, int32 SourceIndex, int32 Count, APlayerController* RequestingPC, FText& OutError);

	/** Authority helper for removing an offered item entry by index. */
	bool TryRemoveOfferItem(ETradeSide Side, int32 OfferIndex, APlayerController* RequestingPC, FText& OutError);

	/** Authority helper for setting resource offer amount. */
	bool TrySetResourceOffer(ETradeSide Side, FName Resource, int64 Amount, APlayerController* RequestingPC, FText& OutError);

	/** Authority helper for readiness / commit initiation. Prefer UYITradeInteractionComponent::RequestTradeSetReadyEx. */
	bool TrySetReady(ETradeSide Side, bool bReady, APlayerController* RequestingPC, FText& OutError);

	UFUNCTION(BlueprintCallable, Category="Trade")
	void CancelTrade() { ServerCancel(); }

	/** Blueprint: resolve which side a given player state represents (defaults to SideA if unknown). */
	UFUNCTION(BlueprintPure, Category="Trade")
	ETradeSide GetSideForPlayer(APlayerState* Player) const;

	/** Blueprint: net-safe full inventory view for a side. */
	UFUNCTION(BlueprintPure, Category="Trade")
	TArray<FYINetBagItem> GetInventoryView(ETradeSide Side) const { return (Side == ETradeSide::SideA) ? InventoryA : InventoryB; }
	UFUNCTION(BlueprintPure, Category="Trade")
	FIntPoint GetInventorySize(ETradeSide Side) const { return (Side == ETradeSide::SideA) ? InventorySizeA : InventorySizeB; }
	const FText& GetLastFailureReason() const { return LastFailureReason; }

	// Commit is internal after both sides ready
	void TryCommit();

	// Client hook to refresh UI when offers change
	UFUNCTION()
	void OnRep_Offers();

	// Client hook to refresh UI when inventory snapshots change
	UFUNCTION()
	void OnRep_Inventories();

	// Server: rebuild inventory snapshots for both sides.
	void RefreshInventoryViews();

	// Keep mirrored inventories live while session is active.
	void BindSideBag(ETradeSide Side);
	void UnbindSideBags();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Helper to access offer by side
	FYITradeOffer& GetOffer(ETradeSide Side);
	const FYITradeOffer& GetOffer(ETradeSide Side) const;

	bool IsSideOwner(ETradeSide Side, APlayerController* PC) const;

	// Server-only storage of real items/sources
	TArray<FYITradeOfferSource> OfferSourcesA;
	TArray<FYITradeOfferSource> OfferSourcesB;

	FYITradeOfferSource& GetOfferSource(ETradeSide Side, int32 Index);
	TArray<FYITradeOfferSource>& GetOfferSources(ETradeSide Side);

	bool ApplyOffersToSide(ETradeSide From, ETradeSide To, FText& OutError);
	UYIInventoryComponent* GetInventoryForSide(ETradeSide Side) const;

	TWeakObjectPtr<class UYIInventoryBag> TrackedBagA;
	TWeakObjectPtr<class UYIInventoryBag> TrackedBagB;
	FDelegateHandle TrackedHandleA;
	FDelegateHandle TrackedHandleB;
	FText LastFailureReason;
};
