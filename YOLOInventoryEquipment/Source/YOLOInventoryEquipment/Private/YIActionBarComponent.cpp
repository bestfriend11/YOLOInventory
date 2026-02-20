#include "YIActionBarComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayAbilitySpec.h"
#include "YIEquipmentComponent.h"
#include "YIInventoryBag.h"
#include "YIInventoryComponent.h"
#include "YIItemDefinition.h"
#include "YIItemSchemaResolver.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "YIDebugLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogYIActionBar, Log, All);

namespace YIActionBarPrivate
{
	static FYIItemInstanceNet FullToNet(const FYIItemInstance& Full)
	{
		FYIItemInstanceNet Out;
		Out.Definition = Full.Definition;
		Out.Count = Full.Count;
		Out.InstanceId = Full.InstanceId;
		Out.StackId = Full.StackId;
		Out.CustomStackKey = Full.CustomStackKey;
		Out.ContainedBagId = Full.ContainedBagId;
		Out.bRotated = Full.bRotated;
		Out.Affixes = Full.Affixes;
		Out.Attributes.Reset();
		for (const TPair<FName, float>& KV : Full.Attributes)
		{
			FYIAttributeKV OutKV;
			OutKV.Name = KV.Key;
			OutKV.Value = KV.Value;
			Out.Attributes.Add(OutKV);
		}
		return Out;
	}
}

UYIActionBarComponent::UYIActionBarComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UYIActionBarComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		EnsureSlotArraySize();
		if (bAutoBindFromEquipment)
		{
			RebuildAutoBindingsFromEquipment();
		}
	}
}

void UYIActionBarComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindEquipmentEvents();
	Super::EndPlay(EndPlayReason);
}

void UYIActionBarComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UYIActionBarComponent, ActionBindings, COND_OwnerOnly);
}

void UYIActionBarComponent::OnRep_ActionBindings()
{
	for (int32 Index = 0; Index < ActionBindings.Num(); ++Index)
	{
		OnActionBindingChanged.Broadcast(Index, ActionBindings[Index].ActionTag, ActionBindings[Index].bEnabled);
	}
}

void UYIActionBarComponent::EnsureSlotArraySize()
{
	const int32 TargetSize = FMath::Max(1, NumActionSlots);
	if (ActionBindings.Num() < TargetSize)
	{
		ActionBindings.SetNum(TargetSize);
	}
	else if (ActionBindings.Num() > TargetSize)
	{
		ActionBindings.SetNum(TargetSize);
	}
}

void UYIActionBarComponent::InitializeActionSlots(int32 InNumSlots)
{
	NumActionSlots = FMath::Max(1, InNumSlots);
	EnsureSlotArraySize();
}

bool UYIActionBarComponent::GetBinding(int32 ActionSlotIndex, FYIActionBarBinding& OutBinding) const
{
	if (!ActionBindings.IsValidIndex(ActionSlotIndex))
	{
		return false;
	}
	OutBinding = ActionBindings[ActionSlotIndex];
	return true;
}

void UYIActionBarComponent::GetPersistedBindings(TArray<FYIActionBarBinding>& OutBindings) const
{
	OutBindings = ActionBindings;
}

void UYIActionBarComponent::LoadPersistedBindings(const TArray<FYIActionBarBinding>& InBindings)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	ActionBindings = InBindings;
	NumActionSlots = FMath::Max(1, ActionBindings.Num());
	EnsureSlotArraySize();
	for (int32 Index = 0; Index < ActionBindings.Num(); ++Index)
	{
		OnActionBindingChanged.Broadcast(Index, ActionBindings[Index].ActionTag, ActionBindings[Index].bEnabled);
	}
	EmitActionMessage(FString::Printf(TEXT("Loaded %d persisted action bindings."), ActionBindings.Num()), FColor::Cyan);
}

void UYIActionBarComponent::GetInvocationLog(TArray<FYIActionInvocationRecord>& OutLog) const
{
	OutLog = ServerInvocationLog;
}

void UYIActionBarComponent::LoadInvocationLog(const TArray<FYIActionInvocationRecord>& InLog)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	ServerInvocationLog = InLog;
	const int32 MaxLogEntries = FMath::Max(1, MaxInvocationLog);
	if (ServerInvocationLog.Num() > MaxLogEntries)
	{
		ServerInvocationLog.RemoveAt(0, ServerInvocationLog.Num() - MaxLogEntries);
	}
}

bool UYIActionBarComponent::ValidateActionBindings(TArray<FString>& OutBlockingIssues, TArray<FString>& OutWarnings) const
{
	OutBlockingIssues.Reset();
	OutWarnings.Reset();

	if (ActionBindings.Num() != NumActionSlots)
	{
		OutWarnings.Add(FString::Printf(TEXT("ActionBindings count (%d) differs from NumActionSlots (%d)."), ActionBindings.Num(), NumActionSlots));
	}

	for (int32 SlotIndex = 0; SlotIndex < ActionBindings.Num(); ++SlotIndex)
	{
		const FYIActionBarBinding& Binding = ActionBindings[SlotIndex];
		if (!Binding.bEnabled)
		{
			continue;
		}

		const bool bHasTag = Binding.ActionTag.IsValid();
		const bool bHasAbilityClass = !Binding.AbilityClass.ToSoftObjectPath().IsNull();
		if (!bHasTag && !bHasAbilityClass)
		{
			OutBlockingIssues.Add(FString::Printf(TEXT("Slot %d is enabled but has no ActionTag and no AbilityClass."), SlotIndex));
		}

		if (bHasTag && !Binding.ActionTag.ToString().StartsWith(ActionTagPrefix))
		{
			OutWarnings.Add(FString::Printf(TEXT("Slot %d action tag '%s' does not match prefix '%s'."), SlotIndex, *Binding.ActionTag.ToString(), *ActionTagPrefix));
		}

		if (Binding.SourceEquipSlotTag.IsValid() && Binding.SourceItem.Definition.ToSoftObjectPath().IsNull())
		{
			OutWarnings.Add(FString::Printf(TEXT("Slot %d references equip slot '%s' but has no cached source item."), SlotIndex, *Binding.SourceEquipSlotTag.ToString()));
		}
	}

	if (bAutoBindFromEquipment)
	{
		if (AutoBindRules.Num() == 0)
		{
			OutWarnings.Add(TEXT("Auto-bind from equipment is enabled but AutoBindRules is empty."));
		}
		TSet<int32> SeenActionSlots;
		for (const FYIEquipmentActionAutoBindRule& Rule : AutoBindRules)
		{
			if (!Rule.bEnabled)
			{
				continue;
			}
			if (!Rule.EquipSlotTag.IsValid())
			{
				OutBlockingIssues.Add(TEXT("Auto-bind rule has invalid EquipSlotTag."));
				continue;
			}
			if (Rule.ActionSlotIndex < 0 || Rule.ActionSlotIndex >= NumActionSlots)
			{
				OutBlockingIssues.Add(FString::Printf(
					TEXT("Auto-bind rule for slot '%s' targets invalid action slot index %d."),
					*Rule.EquipSlotTag.ToString(),
					Rule.ActionSlotIndex));
			}
			if (SeenActionSlots.Contains(Rule.ActionSlotIndex))
			{
				OutWarnings.Add(FString::Printf(TEXT("Multiple auto-bind rules target action slot %d."), Rule.ActionSlotIndex));
			}
			SeenActionSlots.Add(Rule.ActionSlotIndex);
			if (Rule.ActionTagOverride.IsValid() && !Rule.ActionTagOverride.ToString().StartsWith(ActionTagPrefix))
			{
				OutWarnings.Add(FString::Printf(
					TEXT("Auto-bind action tag '%s' does not match ActionTagPrefix '%s'."),
					*Rule.ActionTagOverride.ToString(),
					*ActionTagPrefix));
			}
		}
	}

	return OutBlockingIssues.Num() == 0;
}

void UYIActionBarComponent::BindEquipmentEvents(UYIEquipmentComponent* Equipment)
{
	if (ObservedEquipment.Get() == Equipment)
	{
		return;
	}

	UnbindEquipmentEvents();
	ObservedEquipment = Equipment;
	if (ObservedEquipment.IsValid())
	{
		ObservedEquipment->OnEquipmentChanged.AddDynamic(this, &UYIActionBarComponent::HandleEquipmentChanged);
	}
}

void UYIActionBarComponent::UnbindEquipmentEvents()
{
	if (ObservedEquipment.IsValid())
	{
		ObservedEquipment->OnEquipmentChanged.RemoveDynamic(this, &UYIActionBarComponent::HandleEquipmentChanged);
	}
	ObservedEquipment.Reset();
}

void UYIActionBarComponent::RebuildAutoBindingsFromEquipment(UYIEquipmentComponent* Equipment)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (!bAutoBindFromEquipment)
	{
		return;
	}

	if (!Equipment && GetOwner())
	{
		Equipment = GetOwner()->FindComponentByClass<UYIEquipmentComponent>();
	}
	if (!Equipment)
	{
		EmitActionMessage(TEXT("Auto-bind skipped: no equipment component found on owner."), FColor::Yellow);
		UnbindEquipmentEvents();
		return;
	}

	BindEquipmentEvents(Equipment);
	EnsureSlotArraySize();
	for (const FYIEquipmentActionAutoBindRule& Rule : AutoBindRules)
	{
		ApplyAutoBindRule(Rule, Equipment);
	}
}

void UYIActionBarComponent::ApplyAutoBindRule(const FYIEquipmentActionAutoBindRule& Rule, UYIEquipmentComponent* Equipment)
{
	if (!Rule.bEnabled || !Equipment)
	{
		return;
	}
	if (!Rule.EquipSlotTag.IsValid())
	{
		return;
	}

	EnsureSlotArraySize();
	if (!ActionBindings.IsValidIndex(Rule.ActionSlotIndex))
	{
		EmitActionMessage(FString::Printf(
			TEXT("Auto-bind ignored: action slot %d is out of range for equip slot '%s'."),
			Rule.ActionSlotIndex,
			*Rule.EquipSlotTag.ToString()), FColor::Yellow);
		return;
	}

	const FYIActionBarBinding& ExistingBinding = ActionBindings[Rule.ActionSlotIndex];
	const bool bHasManualBinding = ExistingBinding.bEnabled && !ExistingBinding.SourceEquipSlotTag.IsValid();
	const bool bHasBindingFromDifferentSlot =
		ExistingBinding.bEnabled &&
		ExistingBinding.SourceEquipSlotTag.IsValid() &&
		ExistingBinding.SourceEquipSlotTag != Rule.EquipSlotTag;
	if (!Rule.bAllowOverrideExistingBinding && (bHasManualBinding || bHasBindingFromDifferentSlot))
	{
		return;
	}

	FYIItemInstanceNet EquippedItem;
	const bool bHasEquippedItem = Equipment->GetEquippedItem(Rule.EquipSlotTag, EquippedItem);
	if (bHasEquippedItem)
	{
		TSubclassOf<UGameplayAbility> AbilityClass = nullptr;
		if (!Rule.AbilityClassOverride.IsNull())
		{
			AbilityClass = Rule.AbilityClassOverride.IsValid() ? Rule.AbilityClassOverride.Get() : Rule.AbilityClassOverride.LoadSynchronous();
		}
		BindActionFromEquippedSlot(
			Equipment,
			Rule.EquipSlotTag,
			Rule.ActionSlotIndex,
			Rule.ActionTagOverride,
			AbilityClass,
			FMath::Max(1, Rule.AbilityLevel));
	}
	else if (Rule.bClearWhenUnequipped)
	{
		if (ExistingBinding.bEnabled && ExistingBinding.SourceEquipSlotTag == Rule.EquipSlotTag)
		{
			ClearActionSlot(Rule.ActionSlotIndex);
		}
	}
}

void UYIActionBarComponent::HandleEquipmentChanged(FGameplayTag SlotTag, FYIItemInstanceNet Item)
{
	(void)Item;
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (!bAutoBindFromEquipment || !SlotTag.IsValid())
	{
		return;
	}

	UYIEquipmentComponent* Equipment = ObservedEquipment.Get();
	if (!Equipment && GetOwner())
	{
		Equipment = GetOwner()->FindComponentByClass<UYIEquipmentComponent>();
		BindEquipmentEvents(Equipment);
	}
	if (!Equipment)
	{
		return;
	}

	for (const FYIEquipmentActionAutoBindRule& Rule : AutoBindRules)
	{
		if (Rule.bEnabled && Rule.EquipSlotTag == SlotTag)
		{
			ApplyAutoBindRule(Rule, Equipment);
		}
	}
}

FGameplayTag UYIActionBarComponent::ResolveActionTagFromDefinition(const UYIItemDefinition* Definition) const
{
	if (!Definition)
	{
		return FGameplayTag();
	}

	TArray<FGameplayTag> ItemTags;
	FGameplayTagContainer EffectiveTags;
	YIItemSchema::GetTags(Definition, EffectiveTags);
	EffectiveTags.GetGameplayTagArray(ItemTags);
	for (const FGameplayTag& Tag : ItemTags)
	{
		if (Tag.ToString().StartsWith(ActionTagPrefix))
		{
			return Tag;
		}
	}
	return FGameplayTag();
}

void UYIActionBarComponent::EmitActionMessage(const FString& Message, const FColor& Color) const
{
	UYIDebugLibrary::EmitDebugMessage(
		const_cast<UYIActionBarComponent*>(this),
		EYIDebugChannel::ActionBar,
		Message,
		FLinearColor(Color),
		bDebugActionBar,
		bDebugActionBar,
		4.0f,
		bPinDebugMessages,
		false,
		TEXT("ActionBar"));
}

void UYIActionBarComponent::RecordInvocation(int32 SlotIndex, FGameplayTag ActionTag, bool bSuccess, const FString& Message)
{
	FYIActionInvocationRecord Record;
	Record.SlotIndex = SlotIndex;
	Record.ActionTag = ActionTag;
	Record.bSuccess = bSuccess;
	Record.Message = Message;
	Record.ServerTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	ServerInvocationLog.Add(Record);
	const int32 MaxLogEntries = FMath::Max(1, MaxInvocationLog);
	if (ServerInvocationLog.Num() > MaxLogEntries)
	{
		const int32 OverflowCount = ServerInvocationLog.Num() - MaxLogEntries;
		ServerInvocationLog.RemoveAt(0, OverflowCount);
	}

	OnActionInvoked.Broadcast(SlotIndex, ActionTag, bSuccess, Message);
	EmitActionMessage(Message, bSuccess ? FColor::Green : FColor::Red);
}

bool UYIActionBarComponent::BindActionFromInventoryItem(UYIInventoryComponent* SourceInventory, int32 SourceIndex, int32 ActionSlotIndex, FGameplayTag ActionTag, TSubclassOf<UGameplayAbility> AbilityClass, int32 AbilityLevel)
{
	if (!GetOwner())
	{
		return false;
	}
	if (!GetOwner()->HasAuthority())
	{
		ServerBindActionFromInventoryItem(SourceInventory, SourceIndex, ActionSlotIndex, ActionTag, AbilityClass, AbilityLevel);
		return true;
	}

	if (!SourceInventory || SourceInventory->GetOwner() != GetOwner())
	{
		EmitActionMessage(TEXT("Bind action failed: inventory missing or not owned by this actor."), FColor::Red);
		return false;
	}

	UYIInventoryBag* Bag = SourceInventory->EquippedBag;
	if (!Bag || !Bag->Items.IsValidIndex(SourceIndex))
	{
		EmitActionMessage(TEXT("Bind action failed: invalid inventory source index."), FColor::Red);
		return false;
	}

	EnsureSlotArraySize();
	if (!ActionBindings.IsValidIndex(ActionSlotIndex))
	{
		EmitActionMessage(FString::Printf(TEXT("Bind action failed: slot %d out of range."), ActionSlotIndex), FColor::Red);
		return false;
	}

	const FYIBagItem& BagItem = Bag->Items[SourceIndex];
	UYIItemDefinition* Definition = BagItem.Item.Definition.IsValid()
		? BagItem.Item.Definition.Get()
		: BagItem.Item.Definition.LoadSynchronous();

	if (!ActionTag.IsValid())
	{
		ActionTag = ResolveActionTagFromDefinition(Definition);
	}
	if (!ActionTag.IsValid())
	{
		EmitActionMessage(TEXT("Bind action failed: no action tag provided and item has no matching action tag."), FColor::Red);
		return false;
	}

	FYIActionBarBinding& Binding = ActionBindings[ActionSlotIndex];
	Binding.ActionTag = ActionTag;
	Binding.SourceItem = YIActionBarPrivate::FullToNet(BagItem.Item);
	Binding.AbilityClass = AbilityClass;
	Binding.AbilityLevel = FMath::Max(1, AbilityLevel);
	Binding.bEnabled = true;

	OnActionBindingChanged.Broadcast(ActionSlotIndex, Binding.ActionTag, true);
	EmitActionMessage(FString::Printf(TEXT("Bound slot %d to action '%s' from inventory item '%s'."),
		ActionSlotIndex,
		*Binding.ActionTag.ToString(),
		Definition ? *Definition->GetName() : TEXT("Unknown")), FColor::Green);
	return true;
}

void UYIActionBarComponent::ServerBindActionFromInventoryItem_Implementation(UYIInventoryComponent* SourceInventory, int32 SourceIndex, int32 ActionSlotIndex, FGameplayTag ActionTag, TSubclassOf<UGameplayAbility> AbilityClass, int32 AbilityLevel)
{
	if (!SourceInventory && GetOwner())
	{
		SourceInventory = GetOwner()->FindComponentByClass<UYIInventoryComponent>();
	}
	BindActionFromInventoryItem(SourceInventory, SourceIndex, ActionSlotIndex, ActionTag, AbilityClass, AbilityLevel);
}

bool UYIActionBarComponent::BindActionFromEquippedSlot(UYIEquipmentComponent* Equipment, FGameplayTag EquipSlotTag, int32 ActionSlotIndex, FGameplayTag ActionTag, TSubclassOf<UGameplayAbility> AbilityClass, int32 AbilityLevel)
{
	if (!GetOwner())
	{
		return false;
	}
	if (!GetOwner()->HasAuthority())
	{
		ServerBindActionFromEquippedSlot(Equipment, EquipSlotTag, ActionSlotIndex, ActionTag, AbilityClass, AbilityLevel);
		return true;
	}

	if (!Equipment || Equipment->GetOwner() != GetOwner())
	{
		EmitActionMessage(TEXT("Bind action failed: equipment component missing or not owned by this actor."), FColor::Red);
		return false;
	}

	EnsureSlotArraySize();
	if (!ActionBindings.IsValidIndex(ActionSlotIndex))
	{
		EmitActionMessage(FString::Printf(TEXT("Bind action failed: slot %d out of range."), ActionSlotIndex), FColor::Red);
		return false;
	}

	FYIItemInstanceNet EquippedItem;
	if (!Equipment->GetEquippedItem(EquipSlotTag, EquippedItem))
	{
		EmitActionMessage(FString::Printf(TEXT("Bind action failed: no equipped item in slot '%s'."), *EquipSlotTag.ToString()), FColor::Red);
		return false;
	}

	UYIItemDefinition* Definition = EquippedItem.Definition.IsValid()
		? EquippedItem.Definition.Get()
		: EquippedItem.Definition.LoadSynchronous();
	if (!ActionTag.IsValid())
	{
		ActionTag = ResolveActionTagFromDefinition(Definition);
	}
	if (!ActionTag.IsValid())
	{
		EmitActionMessage(TEXT("Bind action failed: action tag is required for equipped-slot binding."), FColor::Red);
		return false;
	}

	FYIActionBarBinding& Binding = ActionBindings[ActionSlotIndex];
	Binding.ActionTag = ActionTag;
	Binding.SourceEquipSlotTag = EquipSlotTag;
	Binding.SourceItem = EquippedItem;
	Binding.AbilityClass = AbilityClass;
	Binding.AbilityLevel = FMath::Max(1, AbilityLevel);
	Binding.bEnabled = true;

	OnActionBindingChanged.Broadcast(ActionSlotIndex, Binding.ActionTag, true);
	EmitActionMessage(FString::Printf(TEXT("Bound slot %d to action '%s' from equipped slot '%s'."),
		ActionSlotIndex,
		*Binding.ActionTag.ToString(),
		*EquipSlotTag.ToString()), FColor::Green);
	return true;
}

void UYIActionBarComponent::ServerBindActionFromEquippedSlot_Implementation(UYIEquipmentComponent* Equipment, FGameplayTag EquipSlotTag, int32 ActionSlotIndex, FGameplayTag ActionTag, TSubclassOf<UGameplayAbility> AbilityClass, int32 AbilityLevel)
{
	if (!Equipment && GetOwner())
	{
		Equipment = GetOwner()->FindComponentByClass<UYIEquipmentComponent>();
	}
	BindActionFromEquippedSlot(Equipment, EquipSlotTag, ActionSlotIndex, ActionTag, AbilityClass, AbilityLevel);
}

bool UYIActionBarComponent::ClearActionSlot(int32 ActionSlotIndex)
{
	if (!GetOwner())
	{
		return false;
	}
	if (!GetOwner()->HasAuthority())
	{
		ServerClearActionSlot(ActionSlotIndex);
		return true;
	}

	EnsureSlotArraySize();
	if (!ActionBindings.IsValidIndex(ActionSlotIndex))
	{
		return false;
	}

	FYIActionBarBinding& Binding = ActionBindings[ActionSlotIndex];
	const FGameplayTag PreviousTag = Binding.ActionTag;
	Binding = FYIActionBarBinding();
	OnActionBindingChanged.Broadcast(ActionSlotIndex, PreviousTag, false);
	EmitActionMessage(FString::Printf(TEXT("Cleared action slot %d."), ActionSlotIndex), FColor::Yellow);
	return true;
}

void UYIActionBarComponent::ServerClearActionSlot_Implementation(int32 ActionSlotIndex)
{
	ClearActionSlot(ActionSlotIndex);
}

bool UYIActionBarComponent::ActivateActionBySlot(int32 ActionSlotIndex)
{
	if (!GetOwner())
	{
		return false;
	}
	if (!GetOwner()->HasAuthority())
	{
		ServerActivateActionBySlot(ActionSlotIndex);
		return true;
	}

	FString Message;
	const bool bSuccess = ExecuteBindingInternal(ActionSlotIndex, Message);
	const FGameplayTag ActionTag = ActionBindings.IsValidIndex(ActionSlotIndex) ? ActionBindings[ActionSlotIndex].ActionTag : FGameplayTag();
	RecordInvocation(ActionSlotIndex, ActionTag, bSuccess, Message);
	return bSuccess;
}

void UYIActionBarComponent::ServerActivateActionBySlot_Implementation(int32 ActionSlotIndex)
{
	ActivateActionBySlot(ActionSlotIndex);
}

bool UYIActionBarComponent::ActivateActionByTag(FGameplayTag ActionTag)
{
	if (!GetOwner())
	{
		return false;
	}
	if (!GetOwner()->HasAuthority())
	{
		ServerActivateActionByTag(ActionTag);
		return true;
	}

	for (int32 Index = 0; Index < ActionBindings.Num(); ++Index)
	{
		const FYIActionBarBinding& Binding = ActionBindings[Index];
		if (!Binding.bEnabled || !Binding.ActionTag.IsValid())
		{
			continue;
		}
		if (Binding.ActionTag == ActionTag || Binding.ActionTag.MatchesTag(ActionTag))
		{
			return ActivateActionBySlot(Index);
		}
	}

	const FString Message = FString::Printf(TEXT("Action activation failed: no binding found for tag '%s'."), *ActionTag.ToString());
	RecordInvocation(INDEX_NONE, ActionTag, false, Message);
	return false;
}

void UYIActionBarComponent::ServerActivateActionByTag_Implementation(FGameplayTag ActionTag)
{
	ActivateActionByTag(ActionTag);
}

bool UYIActionBarComponent::ExecuteBindingInternal(int32 ActionSlotIndex, FString& OutMessage)
{
	if (!ActionBindings.IsValidIndex(ActionSlotIndex))
	{
		OutMessage = FString::Printf(TEXT("Activate failed: action slot %d is out of range."), ActionSlotIndex);
		return false;
	}

	FYIActionBarBinding& Binding = ActionBindings[ActionSlotIndex];
	if (!Binding.bEnabled)
	{
		OutMessage = FString::Printf(TEXT("Activate failed: slot %d is disabled."), ActionSlotIndex);
		return false;
	}
	if (!Binding.ActionTag.IsValid() && Binding.AbilityClass.IsNull())
	{
		OutMessage = FString::Printf(TEXT("Activate failed: slot %d has no action tag and no ability class."), ActionSlotIndex);
		return false;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!ASC)
	{
		OutMessage = TEXT("Activate failed: owner has no AbilitySystemComponent.");
		return false;
	}

	bool bActivated = false;

	if (!Binding.AbilityClass.IsNull())
	{
		UClass* LoadedClass = Binding.AbilityClass.IsValid() ? Binding.AbilityClass.Get() : Binding.AbilityClass.LoadSynchronous();
		TSubclassOf<UGameplayAbility> AbilityToActivate = LoadedClass;
		if (AbilityToActivate)
		{
			FGameplayAbilitySpec* ExistingSpec = ASC->FindAbilitySpecFromClass(AbilityToActivate);
			FGameplayAbilitySpecHandle SpecHandle;
			if (ExistingSpec)
			{
				SpecHandle = ExistingSpec->Handle;
			}
			else if (GetOwner() && GetOwner()->HasAuthority())
			{
				SpecHandle = ASC->GiveAbility(FGameplayAbilitySpec(AbilityToActivate, FMath::Max(1, Binding.AbilityLevel)));
			}

			if (SpecHandle.IsValid())
			{
				bActivated = ASC->TryActivateAbility(SpecHandle);
			}
		}
	}

	if (!bActivated && Binding.ActionTag.IsValid())
	{
		FGameplayTagContainer TagContainer;
		TagContainer.AddTag(Binding.ActionTag);
		bActivated = ASC->TryActivateAbilitiesByTag(TagContainer, true);
	}

	OutMessage = bActivated
		? FString::Printf(TEXT("Activated action '%s' from slot %d."), *Binding.ActionTag.ToString(), ActionSlotIndex)
		: FString::Printf(TEXT("Activation failed for action '%s' in slot %d."), *Binding.ActionTag.ToString(), ActionSlotIndex);
	return bActivated;
}
