#include "YIItemDefinition.h"
#include "YIItemRegistrySubsystem.h"
#include "Engine/Engine.h"
#include "UObject/ObjectSaveContext.h"
#if WITH_EDITOR
#include "Misc/MessageDialog.h"
#include "UObject/UnrealType.h"
#endif

namespace
{
	static bool YI_NameSuggestsMutableStackState(const FString& LowerName)
	{
		return LowerName.Contains(TEXT("durability"))
			|| LowerName.Contains(TEXT("condition"))
			|| LowerName.Contains(TEXT("charge"))
			|| LowerName.Contains(TEXT("charges"))
			|| LowerName.Contains(TEXT("uses"))
			|| LowerName.Contains(TEXT("current"));
	}

	template<typename TFragment>
	static const TFragment* YI_FindDefinitionFragment(const TArray<FInstancedStruct>& Fragments)
	{
		for (const FInstancedStruct& Fragment : Fragments)
		{
			if (const TFragment* Value = Fragment.GetPtr<TFragment>())
			{
				return Value;
			}
		}
		return nullptr;
	}
}

const FYIItemUIDefinitionFragment* UYIItemDefinition::GetUIDefinitionFragment() const
{
	return YI_FindDefinitionFragment<FYIItemUIDefinitionFragment>(DefinitionFragments);
}

const FYIItemPickupDefinitionFragment* UYIItemDefinition::GetPickupDefinitionFragment() const
{
	return YI_FindDefinitionFragment<FYIItemPickupDefinitionFragment>(DefinitionFragments);
}

const FYIItemEquipmentDefinitionFragment* UYIItemDefinition::GetEquipmentDefinitionFragment() const
{
	return YI_FindDefinitionFragment<FYIItemEquipmentDefinitionFragment>(DefinitionFragments);
}

const FYIItemAffixDefinitionFragment* UYIItemDefinition::GetAffixDefinitionFragment() const
{
	return YI_FindDefinitionFragment<FYIItemAffixDefinitionFragment>(DefinitionFragments);
}

const FYIItemLayoutDefinitionFragment* UYIItemDefinition::GetLayoutDefinitionFragment() const
{
	return YI_FindDefinitionFragment<FYIItemLayoutDefinitionFragment>(DefinitionFragments);
}

void UYIItemDefinition::GetEffectiveDisplayData(FText& OutDisplayName, FText& OutDescription, TSoftObjectPtr<UTexture2D>& OutIcon) const
{
	OutDisplayName = DisplayName;
	OutDescription = Description;
	OutIcon = Icon;

	if (const FYIItemUIDefinitionFragment* UI = GetUIDefinitionFragment())
	{
		if (!UI->DisplayName.IsEmpty())
		{
			OutDisplayName = UI->DisplayName;
		}
		if (!UI->Description.IsEmpty())
		{
			OutDescription = UI->Description;
		}
		if (UI->Icon.ToSoftObjectPath().IsValid())
		{
			OutIcon = UI->Icon;
		}
	}
}

FGameplayTag UYIItemDefinition::GetEffectivePrimaryEquipSlotTag() const
{
	if (const FYIItemEquipmentDefinitionFragment* Equip = GetEquipmentDefinitionFragment())
	{
		if (Equip->PrimaryEquipSlot.IsValid())
		{
			return Equip->PrimaryEquipSlot;
		}
	}
	return FGameplayTag();
}

void UYIItemDefinition::GetEffectiveOccupiedEquipSlots(FGameplayTagContainer& OutOccupiedSlots) const
{
	OutOccupiedSlots.Reset();
	OutOccupiedSlots.AppendTags(OccupiedEquipSlots);

	if (const FYIItemEquipmentDefinitionFragment* Equip = GetEquipmentDefinitionFragment())
	{
		OutOccupiedSlots.AppendTags(Equip->OccupiedSlots);
	}
}

const FInstancedStruct* UYIItemDefinition::FindDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct) const
{
	if (!FragmentStruct)
	{
		return nullptr;
	}

	for (const FInstancedStruct& Fragment : DefinitionFragments)
	{
		if (Fragment.GetScriptStruct() == FragmentStruct)
		{
			return &Fragment;
		}
	}
	return nullptr;
}

FInstancedStruct* UYIItemDefinition::FindOrAddDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct)
{
	if (!FragmentStruct)
	{
		return nullptr;
	}

	for (FInstancedStruct& Fragment : DefinitionFragments)
	{
		if (Fragment.GetScriptStruct() == FragmentStruct)
		{
			return &Fragment;
		}
	}

	FInstancedStruct& NewFragment = DefinitionFragments.AddDefaulted_GetRef();
	NewFragment.InitializeAs(FragmentStruct);
	return &NewFragment;
}

const FYIItemStackingDefinitionFragment* UYIItemDefinition::GetStackingDefinitionFragment() const
{
	return YI_FindDefinitionFragment<FYIItemStackingDefinitionFragment>(DefinitionFragments);
}

void UYIItemDefinition::GetEffectiveAffixDefinition(
	TArray<TSoftObjectPtr<UYIAffixAsset>>& OutTemplateAffixes,
	int32& OutMinRandomModifiers,
	int32& OutMaxRandomModifiers,
	TSoftObjectPtr<UYIAffixPoolAsset>& OutPrefixPool,
	TSoftObjectPtr<UYIAffixPoolAsset>& OutSuffixPool) const
{
	OutTemplateAffixes = TemplateAffixes;
	OutMinRandomModifiers = MinRandomModifiers;
	OutMaxRandomModifiers = MaxRandomModifiers;
	OutPrefixPool = PrefixPool;
	OutSuffixPool = SuffixPool;

	if (const FYIItemAffixDefinitionFragment* AffixDef = GetAffixDefinitionFragment())
	{
		if (AffixDef->bOverrideLegacyAffixFields)
		{
			OutTemplateAffixes = AffixDef->TemplateAffixes;
			OutMinRandomModifiers = AffixDef->MinRandomModifiers;
			OutMaxRandomModifiers = AffixDef->MaxRandomModifiers;
			OutPrefixPool = AffixDef->PrefixPool;
			OutSuffixPool = AffixDef->SuffixPool;
			return;
		}

		for (const TSoftObjectPtr<UYIAffixAsset>& Affix : AffixDef->TemplateAffixes)
		{
			OutTemplateAffixes.AddUnique(Affix);
		}
		OutMinRandomModifiers = AffixDef->MinRandomModifiers;
		OutMaxRandomModifiers = AffixDef->MaxRandomModifiers;
		if (AffixDef->PrefixPool.ToSoftObjectPath().IsValid())
		{
			OutPrefixPool = AffixDef->PrefixPool;
		}
		if (AffixDef->SuffixPool.ToSoftObjectPath().IsValid())
		{
			OutSuffixPool = AffixDef->SuffixPool;
		}
	}
}

void UYIItemDefinition::GetEffectiveLayoutData(FIntPoint& OutDefaultSize, bool& bOutAllowRotation) const
{
	OutDefaultSize = DefaultSize;
	bOutAllowRotation = bAllowRotation;

	if (const FYIItemLayoutDefinitionFragment* Layout = GetLayoutDefinitionFragment())
	{
		if (Layout->bOverrideLegacyLayoutFields)
		{
			OutDefaultSize = Layout->DefaultSize;
			bOutAllowRotation = Layout->bAllowRotation;
		}
	}

	OutDefaultSize.X = FMath::Max(1, OutDefaultSize.X);
	OutDefaultSize.Y = FMath::Max(1, OutDefaultSize.Y);
}

FIntPoint UYIItemDefinition::GetEffectiveDefaultSize() const
{
	FIntPoint Size = FIntPoint(1, 1);
	bool bRotationAllowed = true;
	GetEffectiveLayoutData(Size, bRotationAllowed);
	return Size;
}

bool UYIItemDefinition::IsEffectiveRotationAllowed() const
{
	FIntPoint Size = FIntPoint(1, 1);
	bool bRotationAllowed = true;
	GetEffectiveLayoutData(Size, bRotationAllowed);
	return bRotationAllowed;
}

void UYIItemDefinition::GetEffectiveStackingRules(bool& bOutAllowStacking, int32& OutMaxStackCount, bool& bOutUseRiskChecks) const
{
	bOutAllowStacking = bAllowStacking;
	OutMaxStackCount = MaxStackCount;
	bOutUseRiskChecks = true;

	if (const FYIItemStackingDefinitionFragment* Stacking = GetStackingDefinitionFragment())
	{
		if (Stacking->bOverrideLegacyStackingFields)
		{
			bOutAllowStacking = Stacking->bAllowStacking;
			OutMaxStackCount = Stacking->MaxStackCount;
			bOutUseRiskChecks = Stacking->bUseRiskChecks;
		}
	}
}

bool UYIItemDefinition::HasStackingRisk(FString* OutReason) const
{
	TArray<FString> Reasons;

	TArray<TSoftObjectPtr<UYIAffixAsset>> EffectiveTemplateAffixes;
	int32 EffectiveMinRandomModifiers = 0;
	int32 EffectiveMaxRandomModifiers = 0;
	TSoftObjectPtr<UYIAffixPoolAsset> EffectivePrefixPool;
	TSoftObjectPtr<UYIAffixPoolAsset> EffectiveSuffixPool;
	GetEffectiveAffixDefinition(
		EffectiveTemplateAffixes,
		EffectiveMinRandomModifiers,
		EffectiveMaxRandomModifiers,
		EffectivePrefixPool,
		EffectiveSuffixPool);

	const bool bHasRandomRollSetup = EffectiveMinRandomModifiers > 0
		|| EffectiveMaxRandomModifiers > 0
		|| EffectivePrefixPool.ToSoftObjectPath().IsValid()
		|| EffectiveSuffixPool.ToSoftObjectPath().IsValid();
	if (bHasRandomRollSetup)
	{
		Reasons.Add(TEXT("randomized affix rolls/pools"));
	}

	if (EffectiveTemplateAffixes.Num() > 0)
	{
		Reasons.Add(TEXT("template affixes (instance data can diverge after runtime changes)"));
	}

	if (bIsContainerItem)
	{
		Reasons.Add(TEXT("container item (bag-in-bag instances must not stack)"));
	}

	for (const TPair<FName, float>& Entry : AttributeDefaults)
	{
		const FString KeyLower = Entry.Key.ToString().ToLower();
		if (YI_NameSuggestsMutableStackState(KeyLower))
		{
			Reasons.Add(FString::Printf(TEXT("attribute '%s' looks mutable (durability/charges/etc)"), *Entry.Key.ToString()));
			break;
		}
	}

	TArray<FGameplayTag> TagArray;
	Tags.GetGameplayTagArray(TagArray);
	if (ItemType.IsValid())
	{
		TagArray.Add(ItemType);
	}
	for (const FGameplayTag& Tag : TagArray)
	{
		const FString TagLower = Tag.ToString().ToLower();
		if (YI_NameSuggestsMutableStackState(TagLower))
		{
			Reasons.Add(FString::Printf(TEXT("tag '%s' suggests mutable per-instance state"), *Tag.ToString()));
			break;
		}
	}

	if (OutReason)
	{
		*OutReason = FString::Join(Reasons, TEXT(", "));
	}
	return Reasons.Num() > 0;
}

bool UYIItemDefinition::IsRuntimeStackingAllowed(FString* OutReason) const
{
	bool bEffectiveAllowStacking = false;
	int32 EffectiveMaxStackCount = 1;
	bool bUseRiskChecks = true;
	GetEffectiveStackingRules(bEffectiveAllowStacking, EffectiveMaxStackCount, bUseRiskChecks);

	if (bIsContainerItem)
	{
		if (OutReason)
		{
			*OutReason = TEXT("container items are non-stackable");
		}
		return false;
	}

	if (!bEffectiveAllowStacking || EffectiveMaxStackCount <= 1)
	{
		if (OutReason)
		{
			*OutReason = TEXT("stacking disabled in definition");
		}
		return false;
	}

	FString RiskReason;
	const bool bHasRisk = HasStackingRisk(&RiskReason);
	if (bHasRisk && !bAllowUnsafeStacking && bUseRiskChecks)
	{
		if (OutReason)
		{
			*OutReason = FString::Printf(TEXT("stacking risk detected: %s"), *RiskReason);
		}
		return false;
	}

	if (OutReason)
	{
		OutReason->Reset();
	}
	return true;
}

#if WITH_EDITOR
void UYIItemDefinition::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FProperty* ChangedProperty = PropertyChangedEvent.Property;
	if (!ChangedProperty || !bAllowStacking)
	{
		return;
	}

	const FName ChangedName = ChangedProperty->GetFName();
	const bool bStackRelevantChange =
		ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, bAllowStacking)
		|| ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, MaxStackCount)
		|| ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, MinRandomModifiers)
		|| ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, MaxRandomModifiers)
		|| ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, PrefixPool)
		|| ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, SuffixPool)
		|| ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, Tags)
		|| ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, ItemType)
		|| ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, AttributeDefaults)
		|| ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, TemplateAffixes)
		|| ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, DefinitionFragments)
		|| ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, bIsContainerItem)
		|| ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, ContainerTemplateBag)
		|| ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, bAllowUnsafeStacking);
	if (!bStackRelevantChange || bAllowUnsafeStacking)
	{
		return;
	}

	FString RiskReason;
	if (!HasStackingRisk(&RiskReason))
	{
		return;
	}

	const FText PromptText = FText::Format(
		NSLOCTEXT("YOLOInventory", "ItemDefStackRiskPrompt",
			"Item '{0}' has stack-safety risk(s): {1}.\n\nThese items usually should not stack.\n\nPress Yes to keep stacking and set 'Allow Unsafe Stacking'.\nPress No to disable stacking."),
		FText::FromString(GetName()),
		FText::FromString(RiskReason));

	if (FMessageDialog::Open(EAppMsgType::YesNo, PromptText) == EAppReturnType::Yes)
	{
		bAllowUnsafeStacking = true;
	}
	else
	{
		bAllowStacking = false;
		MaxStackCount = 1;
		bAllowUnsafeStacking = false;
	}
}

void UYIItemDefinition::PreSave(FObjectPreSaveContext SaveContext)
{
	Super::PreSave(SaveContext);
	if (UniqueCode == 0)
	{
		if (UEngine* Engine = GEngine)
		{
			if (UYIItemRegistrySubsystem* Sys = Engine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
			{
				Sys->BuildIndex(true);
				TSet<int64> Seen;
				// collect existing codes
				// Note: registry API provides map but not enumerator; rebuild index ensures keys present
				// We'll loop and generate until not in registry by checking GetByCode
				int64 NewCode = 0;
				do {
					NewCode = (int64)FMath::RandRange(100000, INT32_MAX) * 1000ll + (int64)FMath::RandRange(0,999);
				} while (Sys->GetByCode(NewCode) != nullptr);
				UniqueCode = NewCode;
			}
		}
	}

	// Safety net: if risky state is detected and override is not set, prevent accidental stack merges.
	FString RiskReason;
	bool bEffectiveAllowStacking = false;
	int32 EffectiveMaxStackCount = 1;
	bool bUseRiskChecks = true;
	GetEffectiveStackingRules(bEffectiveAllowStacking, EffectiveMaxStackCount, bUseRiskChecks);
	if (bEffectiveAllowStacking && EffectiveMaxStackCount > 1 && !bAllowUnsafeStacking && bUseRiskChecks && HasStackingRisk(&RiskReason))
	{
		bAllowStacking = false;
		MaxStackCount = 1;
		UE_LOG(LogTemp, Warning, TEXT("UYIItemDefinition '%s': stacking auto-disabled due to risk (%s). Enable bAllowUnsafeStacking to override."),
			*GetName(), *RiskReason);
	}

	// Container items are always non-stackable runtime instances.
	if (bIsContainerItem)
	{
		bAllowStacking = false;
		MaxStackCount = 1;
	}
}
#endif
