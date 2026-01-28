#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YIInventoryTypes.h"
#include "YIPlayerInventoryStateComponent.h" // for FYIResourceWallet
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
struct YOLOINVENTORY_API FYITradeOffer
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
class YOLOINVENTORY_API AYITradeSessionActor : public AActor
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

	// UI events (owning client)
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTradeSimpleEvent);

	UPROPERTY(BlueprintAssignable, Category="Trade")
	FTradeSimpleEvent OnOffersUpdated;

	UPROPERTY(BlueprintAssignable, Category="Trade")
	FTradeSimpleEvent OnTradeCommitted;

	UPROPERTY(BlueprintAssignable, Category="Trade")
	FTradeSimpleEvent OnTradeCancelled;

	UPROPERTY(BlueprintAssignable, Category="Trade")
	FTradeSimpleEvent OnTradeFailed;

	// --- RPCs ---
	/** Server: add from source bag slot (partial count allowed). */
	UFUNCTION(Server, Reliable)
	void ServerAddItem(ETradeSide Side, UYIInventoryComponent* SourceInv, int32 SlotIndex, int32 Count);

	UFUNCTION(Server, Reliable)
	void ServerRemoveItem(ETradeSide Side, int32 Index);

	UFUNCTION(Server, Reliable)
	void ServerSetResource(ETradeSide Side, FName Resource, int64 Amount);

	UFUNCTION(Server, Reliable)
	void ServerSetReady(ETradeSide Side, bool bReady);

	UFUNCTION(Server, Reliable)
	void ServerCancel();

	// --- Client-facing Blueprint helpers (call on owning client; they proxy to server RPCs) ---
	UFUNCTION(BlueprintCallable, Category="Trade")
	void AddOfferFromBag(ETradeSide Side, UYIInventoryComponent* SourceInv, int32 SlotIndex, int32 Count) { ServerAddItem(Side, SourceInv, SlotIndex, Count); }

	UFUNCTION(BlueprintCallable, Category="Trade")
	void RemoveOfferItem(ETradeSide Side, int32 Index) { ServerRemoveItem(Side, Index); }

	UFUNCTION(BlueprintCallable, Category="Trade")
	void SetResourceOffer(ETradeSide Side, FName Resource, int64 Amount) { ServerSetResource(Side, Resource, Amount); }

	UFUNCTION(BlueprintCallable, Category="Trade")
	void SetReady(ETradeSide Side, bool bReady) { ServerSetReady(Side, bReady); }

	UFUNCTION(BlueprintCallable, Category="Trade")
	void CancelTrade() { ServerCancel(); }

	// Commit is internal after both sides ready
	void TryCommit();

	// Client hook to refresh UI when offers change
	UFUNCTION()
	void OnRep_Offers();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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
};
