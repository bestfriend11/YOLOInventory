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

	/** Runtime profile used by this drop component (server authoritative). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Drop", meta=(ToolTip="Server-side drop profile for this actor.\nIf set, TriggerDrop* uses this profile to generate loot results."))
	TSoftObjectPtr<UYILootDropProfile> DropProfile;

	/** Optional local switch to auto-bind owner destroyed signal (requires profile->bDropOnOwnerDestroyed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Drop", meta=(ToolTip="If true, component binds owner destroyed event and may auto-drop when profile allows it."))
	bool bAutoBindOwnerDestroyed = true;

	/** One-shot state replicated for consistency in multiplayer. */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_HasDropped, Category = "Loot Drop", meta=(ToolTip="Replicated one-shot guard.\nTrue means drop already executed and repeated triggers are blocked until ResetOneShotState."))
	bool bHasDropped = false;

	/** Fired when a drop attempt executes on this instance (server and clients via replication callback path). */
	UPROPERTY(BlueprintAssignable, Category = "Loot Drop", meta=(ToolTip="Drop execution event carrying full result payload for UI/logging/reactions."))
	FYILootDropExecutedSignature OnDropExecuted;

	/** Manual trigger (useful for chest/open interaction). */
	UFUNCTION(BlueprintCallable, Category = "YOLOInventory|Loot Drop", meta=(ToolTip="Trigger drop using default context.\nClient calls forward to server RPC when needed.\nReturns immediate local result payload."))
	FYILootDropResult TriggerDrop();

	UFUNCTION(BlueprintCallable, Category = "YOLOInventory|Loot Drop", meta=(ToolTip="Trigger drop with explicit context.\nArgs:\n- Context: roll seed/level/instigator parameters.\nServer authoritative execution."))
	FYILootDropResult TriggerDropWithContext(const FYILootDropContext& Context);

	/** Convenience helper for kill events. */
	UFUNCTION(BlueprintCallable, Category = "YOLOInventory|Loot Drop", meta=(ToolTip="Convenience wrapper for kill-driven drops.\nArgs:\n- InstigatorActor: killer/source actor.\n- ContextLevel: level used in profile filtering.\n- Seed: optional deterministic seed (0 = runtime seed)."))
	FYILootDropResult TriggerDropForInstigator(AActor* InstigatorActor, int32 ContextLevel = 1, int32 Seed = 0);

	UFUNCTION(BlueprintCallable, Category = "YOLOInventory|Loot Drop", meta=(ToolTip="Clears one-shot guard (bHasDropped=false).\nUse for resettable chests or test loops."))
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
