#pragma once

#include "CoreMinimal.h"
#include "YIItemPickup.h"
#include "YIAutoPickupDropActor.generated.h"

class USphereComponent;
class USceneComponent;
class UPrimitiveComponent;
class UYIInventoryComponent;
class UYIItemDefinition;
struct FHitResult;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FYIAutoPickupEvent, AActor*, PickerActor, UYIInventoryComponent*, InventoryComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FYIAutoPickupFailedEvent, AActor*, PickerActor);

/**
 * Drop actor inspired by action-RPG pickups:
 * - animated spawn scale + idle bob + spin
 * - overlap auto-pickup on server
 * - minimal setup: assign ItemDefinition/Count and place actor
 */
UCLASS(Blueprintable)
class YOLOINVENTORY_API AYIAutoPickupDropActor : public AYIItemPickup
{
	GENERATED_BODY()

public:
	AYIAutoPickupDropActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Runtime helper: assigns drop item and updates replicated payload (server-authoritative). */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Pickup")
	void SetDropItem(UYIItemDefinition* InDefinition, int32 InCount = 1);

	/** Optional direct item binding. If set and ItemCode is empty, this definition is used at BeginPlay (server). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="YOLOInventory|Pickup")
	TSoftObjectPtr<UYIItemDefinition> ItemDefinition;

	/** Count used with ItemDefinition when ItemCode is not explicitly set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="YOLOInventory|Pickup", meta=(ClampMin="1"))
	int32 ItemCount = 1;

	/** Auto transfer pickup into inventory when a valid actor overlaps (server-authoritative). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="YOLOInventory|Pickup")
	bool bAutoPickupOnOverlap = true;

	/** Only player-controlled pawns can pickup. Disable for AI/other actors. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="YOLOInventory|Pickup")
	bool bRequirePlayerControlledPawn = true;

	/** If true, actor can be picked up only once overlap lock expires (prevents duplicate overlap attempts). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="YOLOInventory|Pickup", meta=(ClampMin="0.01"))
	float RetryCooldownSeconds = 0.15f;

	/** Idle bob amplitude in world units. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="YOLOInventory|Visual")
	float BobAmplitude = 14.f;

	/** Idle bob cycles per second. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="YOLOInventory|Visual")
	float BobFrequency = 1.25f;

	/** Rotation speed around yaw in deg/sec. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="YOLOInventory|Visual")
	float SpinYawDegPerSecond = 120.f;

	/** Spawn scale-up duration (seconds). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="YOLOInventory|Visual", meta=(ClampMin="0.0"))
	float SpawnScaleDuration = 0.2f;

	/** Pickup trigger radius. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="YOLOInventory|Pickup", meta=(ClampMin="5.0"))
	float PickupRadius = 90.f;

	/** Fired on successful pickup transfer. */
	UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Pickup")
	FYIAutoPickupEvent OnPickedUp;

	/** Fired when overlap happens but add-to-inventory fails (no bag/space/rules). */
	UPROPERTY(BlueprintAssignable, Category="YOLOInventory|Pickup")
	FYIAutoPickupFailedEvent OnPickupFailed;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="YOLOInventory|Pickup")
	TObjectPtr<USceneComponent> VisualRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="YOLOInventory|Pickup")
	TObjectPtr<USphereComponent> PickupSphere;

	UFUNCTION()
	void HandlePickupOverlap(
		UPrimitiveComponent* OverlappedComp,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Pickup")
	bool TryPickupActor(AActor* PickerActor);

private:
	bool ResolveInventoryComponent(AActor* CandidateActor, UYIInventoryComponent*& OutInventory) const;
	void ApplyDefaultItemIfNeeded();

	float TimeAccumulator = 0.f;
	float SpawnTimeElapsed = 0.f;
	float OverlapRetryUnlockAt = 0.f;
	FVector BaseVisualRelativeLocation = FVector::ZeroVector;
};
