#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "YIItemPickup.h"
#include "YIActionBarComponent.generated.h"

class UGameplayAbility;
class UYIInventoryComponent;
class UYIEquipmentComponent;
class UYIItemDefinition;

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIActionBarBinding
{
	GENERATED_BODY()

	/** Designer/runtime action identity (example: Actions.Spell.Binding.StellarBinding). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar")
	FGameplayTag ActionTag;

	/** Optional source equip slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar")
	FGameplayTag SourceEquipSlotTag;

	/** Source item data used for tooltips and audit/debug. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar")
	FYIItemInstanceNet SourceItem;

	/** Optional direct ability class to execute for this binding. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar")
	TSoftClassPtr<UGameplayAbility> AbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar", meta=(ClampMin="1"))
	int32 AbilityLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar")
	bool bEnabled = true;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORY_API FYIActionInvocationRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar")
	int32 SlotIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar")
	FGameplayTag ActionTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar")
	bool bSuccess = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar")
	FString Message;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar")
	float ServerTimeSeconds = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FYIActionBarBindingEvent, int32, SlotIndex, FGameplayTag, ActionTag, bool, bBound);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FYIActionInvokedEvent, int32, SlotIndex, FGameplayTag, ActionTag, bool, bSuccess, FString, Message);

/**
 * Server-side action pipeline for item/spell bindings.
 * - Clients request activation; server validates and executes via GAS, then logs result.
 */
UCLASS(ClassGroup=(Inventory), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class YOLOINVENTORY_API UYIActionBarComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UYIActionBarComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|Config", meta=(ClampMin="1", ClampMax="64"))
	int32 NumActionSlots = 10;

	/** If ActionTag is omitted when binding from item, first item tag with this prefix is used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|Config")
	FString ActionTagPrefix = TEXT("Actions.");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|Debug")
	bool bDebugActionBar = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|Debug")
	bool bPinDebugMessages = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|Debug", meta=(ClampMin="1", ClampMax="1024"))
	int32 MaxInvocationLog = 128;

	/** Replicated action slot bindings (owner-only; this is player UI state). */
	UPROPERTY(ReplicatedUsing=OnRep_ActionBindings, BlueprintReadOnly, Category="ActionBar")
	TArray<FYIActionBarBinding> ActionBindings;

	/** Server-side invocation history for diagnostics. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ActionBar")
	TArray<FYIActionInvocationRecord> ServerInvocationLog;

	UPROPERTY(BlueprintAssignable, Category="ActionBar|Events")
	FYIActionBarBindingEvent OnActionBindingChanged;

	UPROPERTY(BlueprintAssignable, Category="ActionBar|Events")
	FYIActionInvokedEvent OnActionInvoked;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar")
	void InitializeActionSlots(int32 InNumSlots);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar")
	bool GetBinding(int32 ActionSlotIndex, FYIActionBarBinding& OutBinding) const;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar")
	void GetPersistedBindings(TArray<FYIActionBarBinding>& OutBindings) const;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar", BlueprintAuthorityOnly)
	void LoadPersistedBindings(const TArray<FYIActionBarBinding>& InBindings);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar")
	void GetInvocationLog(TArray<FYIActionInvocationRecord>& OutLog) const;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar", BlueprintAuthorityOnly)
	void LoadInvocationLog(const TArray<FYIActionInvocationRecord>& InLog);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar|Net")
	bool BindActionFromInventoryItem(UYIInventoryComponent* SourceInventory, int32 SourceIndex, int32 ActionSlotIndex, FGameplayTag ActionTag, TSubclassOf<UGameplayAbility> AbilityClass, int32 AbilityLevel);

	UFUNCTION(Server, Reliable)
	void ServerBindActionFromInventoryItem(UYIInventoryComponent* SourceInventory, int32 SourceIndex, int32 ActionSlotIndex, FGameplayTag ActionTag, TSubclassOf<UGameplayAbility> AbilityClass, int32 AbilityLevel);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar|Net")
	bool BindActionFromEquippedSlot(UYIEquipmentComponent* Equipment, FGameplayTag EquipSlotTag, int32 ActionSlotIndex, FGameplayTag ActionTag, TSubclassOf<UGameplayAbility> AbilityClass, int32 AbilityLevel);

	UFUNCTION(Server, Reliable)
	void ServerBindActionFromEquippedSlot(UYIEquipmentComponent* Equipment, FGameplayTag EquipSlotTag, int32 ActionSlotIndex, FGameplayTag ActionTag, TSubclassOf<UGameplayAbility> AbilityClass, int32 AbilityLevel);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar|Net")
	bool ClearActionSlot(int32 ActionSlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerClearActionSlot(int32 ActionSlotIndex);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar|Net")
	bool ActivateActionBySlot(int32 ActionSlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerActivateActionBySlot(int32 ActionSlotIndex);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar|Net")
	bool ActivateActionByTag(FGameplayTag ActionTag);

	UFUNCTION(Server, Reliable)
	void ServerActivateActionByTag(FGameplayTag ActionTag);

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_ActionBindings();

private:
	void EnsureSlotArraySize();
	FGameplayTag ResolveActionTagFromDefinition(const UYIItemDefinition* Definition) const;
	bool ExecuteBindingInternal(int32 ActionSlotIndex, FString& OutMessage);
	void RecordInvocation(int32 SlotIndex, FGameplayTag ActionTag, bool bSuccess, const FString& Message);
	void EmitActionMessage(const FString& Message, const FColor& Color) const;
};
