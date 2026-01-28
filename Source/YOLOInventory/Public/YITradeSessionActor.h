#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YIInventoryTypes.h"
#include "YIPlayerInventoryStateComponent.h" // for FYIResourceWallet
#include "YITradeSessionActor.generated.h"

class UYIInventoryComponent;
class APlayerController;

UENUM(BlueprintType)
enum class ETradeSide : uint8
{
	SideA,
	SideB
};

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

	// Offers (owner-only replicated so each side can see both)
	UPROPERTY(ReplicatedUsing=OnRep_Offers, Transient)
	FYITradeOffer OfferA;

	UPROPERTY(ReplicatedUsing=OnRep_Offers, Transient)
	FYITradeOffer OfferB;

	UPROPERTY(Replicated)
	bool bAReady = false;

	UPROPERTY(Replicated)
	bool bBReady = false;

	// --- RPCs ---
	UFUNCTION(Server, Reliable)
	void ServerAddItem(ETradeSide Side, const FYINetBagItem& Item);

	UFUNCTION(Server, Reliable)
	void ServerRemoveItem(ETradeSide Side, int32 Index);

	UFUNCTION(Server, Reliable)
	void ServerSetResource(ETradeSide Side, FName Resource, int64 Amount);

	UFUNCTION(Server, Reliable)
	void ServerSetReady(ETradeSide Side, bool bReady);

	UFUNCTION(Server, Reliable)
	void ServerCancel();

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
};
