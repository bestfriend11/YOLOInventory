#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "YIItemNetTypes.h"
#include "YIActionBarComponent.generated.h"

class UGameplayAbility;
class UYIInventoryComponent;
class UYIEquipmentComponent;
class UYIItemDefinition;

USTRUCT(BlueprintType)
struct YOLOINVENTORYEQUIPMENT_API FYIEquipmentActionAutoBindRule
{
	GENERATED_BODY()

	/** Disable rule without removing setup. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|AutoBind")
	bool bEnabled = true;

	/** Equipped slot watched by this rule (for example Equip.Slot.Spellbook.Primary). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|AutoBind")
	FGameplayTag EquipSlotTag;

	/** Action bar slot to write when this equipment slot changes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|AutoBind", meta=(ClampMin="0"))
	int32 ActionSlotIndex = 0;

	/** If false, a pre-existing manual binding in this action slot is preserved. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|AutoBind")
	bool bAllowOverrideExistingBinding = true;

	/** Clear action slot when item is unequipped (only if slot was bound from this equip slot). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|AutoBind")
	bool bClearWhenUnequipped = true;

	/** Optional explicit action tag. If empty, action tag is resolved from item definition tags. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|AutoBind")
	FGameplayTag ActionTagOverride;

	/** Optional explicit ability class. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|AutoBind")
	TSoftClassPtr<UGameplayAbility> AbilityClassOverride;

	/** Level for AbilityClassOverride. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|AutoBind", meta=(ClampMin="1"))
	int32 AbilityLevel = 1;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYEQUIPMENT_API FYIActionBarBinding
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
struct YOLOINVENTORYEQUIPMENT_API FYIActionInvocationRecord
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
class YOLOINVENTORYEQUIPMENT_API UYIActionBarComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UYIActionBarComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|Config", meta=(ClampMin="1", ClampMax="64", ToolTip="Action slot count. Replicated binding array is resized to this count on authority."))
	int32 NumActionSlots = 10;

	/** If ActionTag is omitted when binding from item, first item tag with this prefix is used. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|Config", meta=(ToolTip="Prefix used when resolving ActionTag from item definition tags (example: Actions.)."))
	FString ActionTagPrefix = TEXT("Actions.");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|Debug", meta=(ToolTip="If true, action-bar operations print runtime diagnostics to log/on-screen text."))
	bool bDebugActionBar = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|Debug", meta=(ToolTip="If true, debug messages stay on screen longer."))
	bool bPinDebugMessages = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|Debug", meta=(ClampMin="1", ClampMax="1024", ToolTip="Max retained server invocation history entries."))
	int32 MaxInvocationLog = 128;

	/** Auto-bind selected equipment slots into action slots to simplify spellbook/action workflows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|AutoBind", meta=(ToolTip="Enable authority-driven auto-binding from equipment slot changes using AutoBindRules."))
	bool bAutoBindFromEquipment = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ActionBar|AutoBind", meta=(EditCondition="bAutoBindFromEquipment", EditConditionHides, ToolTip="Rule table mapping equipment slots to action slots. Evaluated on server."))
	TArray<FYIEquipmentActionAutoBindRule> AutoBindRules;

	/** Replicated action slot bindings (owner-only; this is player UI state). */
	UPROPERTY(ReplicatedUsing=OnRep_ActionBindings, BlueprintReadOnly, Category="ActionBar", meta=(ToolTip="Replicated owner-side action bindings. Use OnActionBindingChanged to update UI."))
	TArray<FYIActionBarBinding> ActionBindings;

	/** Server-side invocation history for diagnostics. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ActionBar", meta=(ToolTip="Server-maintained invocation history for audit/debug tooling."))
	TArray<FYIActionInvocationRecord> ServerInvocationLog;

	UPROPERTY(BlueprintAssignable, Category="ActionBar|Events", meta=(ToolTip="Fires when a slot binding is written/cleared.\nArgs: SlotIndex, ActionTag, bBound."))
	FYIActionBarBindingEvent OnActionBindingChanged;

	UPROPERTY(BlueprintAssignable, Category="ActionBar|Events", meta=(ToolTip="Fires when activation is attempted/executed.\nArgs: SlotIndex, ActionTag, bSuccess, Message."))
	FYIActionInvokedEvent OnActionInvoked;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar", meta=(ToolTip="Initialize/resize action slots.\nServer should own final size."))
	void InitializeActionSlots(int32 InNumSlots);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar", meta=(ToolTip="Read binding by action slot index. Returns false when index invalid or slot unbound."))
	bool GetBinding(int32 ActionSlotIndex, FYIActionBarBinding& OutBinding) const;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar", meta=(ToolTip="Copy current bindings for persistence/save payload."))
	void GetPersistedBindings(TArray<FYIActionBarBinding>& OutBindings) const;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar", BlueprintAuthorityOnly, meta=(ToolTip="Authority-only load helper. Replaces current bindings from persisted payload."))
	void LoadPersistedBindings(const TArray<FYIActionBarBinding>& InBindings);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar", meta=(ToolTip="Copy current server invocation log."))
	void GetInvocationLog(TArray<FYIActionInvocationRecord>& OutLog) const;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar", BlueprintAuthorityOnly, meta=(ToolTip="Authority-only load helper for invocation history."))
	void LoadInvocationLog(const TArray<FYIActionInvocationRecord>& InLog);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar|Diagnostics", meta=(ToolTip="Validate binding table, rule consistency and slot indices.\nReturns true when no blocking issues."))
	bool ValidateActionBindings(TArray<FString>& OutBlockingIssues, TArray<FString>& OutWarnings) const;

	/** Re-evaluate auto-bind rules against current equipment state (authority only). */
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar|AutoBind", meta=(ToolTip="Rebuild auto-bindings from equipment state.\nIf Equipment is null, component resolves owner equipment component."))
	void RebuildAutoBindingsFromEquipment(UYIEquipmentComponent* Equipment = nullptr);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar|Net", meta=(ToolTip="Bind action slot from inventory item.\nArgs:\n- SourceInventory/SourceIndex: source item.\n- ActionSlotIndex: destination action slot.\n- ActionTag: optional explicit action tag (empty auto-resolve).\n- AbilityClass/AbilityLevel: optional explicit ability override.\nClient calls route to server RPC."))
	bool BindActionFromInventoryItem(UYIInventoryComponent* SourceInventory, int32 SourceIndex, int32 ActionSlotIndex, FGameplayTag ActionTag, TSubclassOf<UGameplayAbility> AbilityClass, int32 AbilityLevel);

	UFUNCTION(Server, Reliable)
	void ServerBindActionFromInventoryItem(UYIInventoryComponent* SourceInventory, int32 SourceIndex, int32 ActionSlotIndex, FGameplayTag ActionTag, TSubclassOf<UGameplayAbility> AbilityClass, int32 AbilityLevel);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar|Net", meta=(ToolTip="Bind action slot from equipped item in EquipSlotTag.\nArgs mirror BindActionFromInventoryItem.\nClient calls route to server RPC."))
	bool BindActionFromEquippedSlot(UYIEquipmentComponent* Equipment, FGameplayTag EquipSlotTag, int32 ActionSlotIndex, FGameplayTag ActionTag, TSubclassOf<UGameplayAbility> AbilityClass, int32 AbilityLevel);

	UFUNCTION(Server, Reliable)
	void ServerBindActionFromEquippedSlot(UYIEquipmentComponent* Equipment, FGameplayTag EquipSlotTag, int32 ActionSlotIndex, FGameplayTag ActionTag, TSubclassOf<UGameplayAbility> AbilityClass, int32 AbilityLevel);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar|Net", meta=(ToolTip="Clear binding at action slot index.\nClient calls route to server RPC."))
	bool ClearActionSlot(int32 ActionSlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerClearActionSlot(int32 ActionSlotIndex);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar|Net", meta=(ToolTip="Activate action by slot index.\nClient calls route to server RPC where validation/execution occurs."))
	bool ActivateActionBySlot(int32 ActionSlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerActivateActionBySlot(int32 ActionSlotIndex);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|ActionBar|Net", meta=(ToolTip="Activate first enabled binding matching ActionTag.\nClient calls route to server RPC."))
	bool ActivateActionByTag(FGameplayTag ActionTag);

	UFUNCTION(Server, Reliable)
	void ServerActivateActionByTag(FGameplayTag ActionTag);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_ActionBindings();

	UFUNCTION()
	void HandleEquipmentChanged(FGameplayTag SlotTag, FYIItemInstanceNet Item);

private:
	void EnsureSlotArraySize();
	FGameplayTag ResolveActionTagFromDefinition(const UYIItemDefinition* Definition) const;
	bool ExecuteBindingInternal(int32 ActionSlotIndex, FString& OutMessage);
	void RecordInvocation(int32 SlotIndex, FGameplayTag ActionTag, bool bSuccess, const FString& Message);
	void EmitActionMessage(const FString& Message, const FColor& Color) const;
	void BindEquipmentEvents(UYIEquipmentComponent* Equipment);
	void UnbindEquipmentEvents();
	void ApplyAutoBindRule(const FYIEquipmentActionAutoBindRule& Rule, UYIEquipmentComponent* Equipment);

	TWeakObjectPtr<UYIEquipmentComponent> ObservedEquipment;
};
