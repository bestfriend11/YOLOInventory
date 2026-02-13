#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YILootDropProfile.h"
#include "YILootDropComponent.generated.h"

class UYILootDropProfile;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FYILootDropExecutedSignature, const FYILootDropResult&, Result);

UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class YOLOINVENTORY_API UYILootDropComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UYILootDropComponent();

	/** Runtime profile used by this drop component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Drop")
	TSoftObjectPtr<UYILootDropProfile> DropProfile;

	/** Optional local switch to auto-bind owner destroyed signal (requires profile->bDropOnOwnerDestroyed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Drop")
	bool bAutoBindOwnerDestroyed = true;

	/** One-shot state replicated for consistency in multiplayer. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HasDropped, Category = "Loot Drop")
	bool bHasDropped = false;

	UPROPERTY(BlueprintAssignable, Category = "Loot Drop")
	FYILootDropExecutedSignature OnDropExecuted;

	/** Manual trigger (useful for chest/open interaction). */
	UFUNCTION(BlueprintCallable, Category = "YOLOInventory|Loot Drop")
	FYILootDropResult TriggerDrop();

	UFUNCTION(BlueprintCallable, Category = "YOLOInventory|Loot Drop")
	FYILootDropResult TriggerDropWithContext(const FYILootDropContext& Context);

	/** Convenience helper for kill events. */
	UFUNCTION(BlueprintCallable, Category = "YOLOInventory|Loot Drop")
	FYILootDropResult TriggerDropForInstigator(AActor* InstigatorActor, int32 ContextLevel = 1, int32 Seed = 0);

	UFUNCTION(BlueprintCallable, Category = "YOLOInventory|Loot Drop")
	void ResetOneShotState();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION(Server, Reliable)
	void ServerTriggerDropWithContext(const FYILootDropContext& Context);

	UFUNCTION()
	void HandleOwnerDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void OnRep_HasDropped();

	FYILootDropResult TriggerDropInternal(const FYILootDropContext& Context);
};

