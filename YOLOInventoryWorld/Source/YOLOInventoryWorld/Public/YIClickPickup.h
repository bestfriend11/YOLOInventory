#pragma once

#include "CoreMinimal.h"
#include "YIItemPickup.h"
#include "YIClickPickup.generated.h"

class UYIInventoryComponent;

/**
 * Clickable pickup: left-click to add to the local player's equipped bag.
 * Works in multiplayer: click runs a server RPC to transfer the item and destroy the pickup.
 */
UCLASS(Blueprintable)
class YOLOINVENTORYWORLD_API AYIClickPickup : public AYIItemPickup
{
	GENERATED_BODY()
public:
	AYIClickPickup();

	// If true, enables input for the first local player on begin play (client-side) so OnClicked fires.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="YOLOInventory|Pickup")
	bool bAutoEnableInput = true;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleClicked(AActor* TouchedActor, FKey Button);

	UFUNCTION(Server, Reliable)
	void ServerPickup(class APlayerController* PC);
};
