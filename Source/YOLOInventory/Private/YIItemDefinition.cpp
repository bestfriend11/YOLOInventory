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

	template<typename TFragment>
	static TFragment* YI_FindDefinitionFragmentMutable(TArray<FInstancedStruct>& Fragments)
	{
		for (FInstancedStruct& Fragment : Fragments)
		{
			if (TFragment* Value = Fragment.GetMutablePtr<TFragment>())
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
	OutDisplayName = FText::FromString(GetName());
	OutDescription = FText::GetEmpty();
	OutIcon = nullptr;

	const FYIItemUIDefinitionFragment* UI = GetUIDefinitionFragment();
	if (!UI)
	{
		return;
	}

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

bool UYIItemDefinition::EnsureBaselineDefinitionFragments()
{
	bool bChanged = false;

	// UI fragment: mirror legacy display data when missing/empty.
	if (FYIItemUIDefinitionFragment* UI = YI_FindDefinitionFragmentMutable<FYIItemUIDefinitionFragment>(DefinitionFragments))
	{
		if (UI->DisplayName.IsEmpty() && !DisplayName.IsEmpty())
		{
			bChanged = true;
			UI->DisplayName = DisplayName;
		}
		if (UI->Description.IsEmpty() && !Description.IsEmpty())
		{
			bChanged = true;
			UI->Description = Description;
		}
		if (!UI->Icon.ToSoftObjectPath().IsValid() && Icon.ToSoftObjectPath().IsValid())
		{
			bChanged = true;
			UI->Icon = Icon;
		}
	}
	else if (!DisplayName.IsEmpty() || !Description.IsEmpty() || Icon.ToSoftObjectPath().IsValid())
	{
		bChanged = true;
		FInstancedStruct& UIFragment = DefinitionFragments.AddDefaulted_GetRef();
		UIFragment.InitializeAs<FYIItemUIDefinitionFragment>();
		if (FYIItemUIDefinitionFragment* UIFrag = UIFragment.GetMutablePtr<FYIItemUIDefinitionFragment>())
		{
			UIFrag->DisplayName = DisplayName;
			UIFrag->Description = Description;
			UIFrag->Icon = Icon;
		}
	}

	// Layout fragment: keep fragment values synced to legacy unless explicitly overriding.
	if (FYIItemLayoutDefinitionFragment* Layout = YI_FindDefinitionFragmentMutable<FYIItemLayoutDefinitionFragment>(DefinitionFragments))
	{
		if (!Layout->bOverrideLegacyLayoutFields)
		{
			if (Layout->DefaultSize != DefaultSize)
			{
				bChanged = true;
				Layout->DefaultSize = DefaultSize;
			}
			if (Layout->bAllowRotation != bAllowRotation)
			{
				bChanged = true;
				Layout->bAllowRotation = bAllowRotation;
			}
		}
	}
	else
	{
		bChanged = true;
		FInstancedStruct& LayoutFragment = DefinitionFragments.AddDefaulted_GetRef();
		LayoutFragment.InitializeAs<FYIItemLayoutDefinitionFragment>();
		if (FYIItemLayoutDefinitionFragment* LayoutFrag = LayoutFragment.GetMutablePtr<FYIItemLayoutDefinitionFragment>())
		{
			LayoutFrag->bOverrideLegacyLayoutFields = false;
			LayoutFrag->DefaultSize = DefaultSize;
			LayoutFrag->bAllowRotation = bAllowRotation;
		}
	}

	// Stacking fragment: preserve runtime behavior while exposing fragment mirror data.
	if (FYIItemStackingDefinitionFragment* Stacking = YI_FindDefinitionFragmentMutable<FYIItemStackingDefinitionFragment>(DefinitionFragments))
	{
		if (!Stacking->bOverrideLegacyStackingFields)
		{
			const bool bDesiredRiskChecks = !bAllowUnsafeStacking;
			if (Stacking->bAllowStacking != bAllowStacking)
			{
				bChanged = true;
				Stacking->bAllowStacking = bAllowStacking;
			}
			if (Stacking->MaxStackCount != MaxStackCount)
			{
				bChanged = true;
				Stacking->MaxStackCount = MaxStackCount;
			}
			if (Stacking->bUseRiskChecks != bDesiredRiskChecks)
			{
				bChanged = true;
				Stacking->bUseRiskChecks = bDesiredRiskChecks;
			}
		}
	}
	else
	{
		bChanged = true;
		FInstancedStruct& StackingFragment = DefinitionFragments.AddDefaulted_GetRef();
		StackingFragment.InitializeAs<FYIItemStackingDefinitionFragment>();
		if (FYIItemStackingDefinitionFragment* StackingFrag = StackingFragment.GetMutablePtr<FYIItemStackingDefinitionFragment>())
		{
			StackingFrag->bOverrideLegacyStackingFields = false;
			StackingFrag->bAllowStacking = bAllowStacking;
			StackingFrag->MaxStackCount = MaxStackCount;
			StackingFrag->bUseRiskChecks = !bAllowUnsafeStacking;
		}
	}

	// Affix fragment: keep baseline generation config mirrored for schema-driven tools.
	if (FYIItemAffixDefinitionFragment* AffixDef = YI_FindDefinitionFragmentMutable<FYIItemAffixDefinitionFragment>(DefinitionFragments))
	{
		if (!AffixDef->bOverrideLegacyAffixFields)
		{
			if (AffixDef->TemplateAffixes != TemplateAffixes)
			{
				bChanged = true;
				AffixDef->TemplateAffixes = TemplateAffixes;
			}
			if (AffixDef->MinRandomModifiers != MinRandomModifiers)
			{
				bChanged = true;
				AffixDef->MinRandomModifiers = MinRandomModifiers;
			}
			if (AffixDef->MaxRandomModifiers != MaxRandomModifiers)
			{
				bChanged = true;
				AffixDef->MaxRandomModifiers = MaxRandomModifiers;
			}
			if (AffixDef->PrefixPool.ToSoftObjectPath() != PrefixPool.ToSoftObjectPath())
			{
				bChanged = true;
				AffixDef->PrefixPool = PrefixPool;
			}
			if (AffixDef->SuffixPool.ToSoftObjectPath() != SuffixPool.ToSoftObjectPath())
			{
				bChanged = true;
				AffixDef->SuffixPool = SuffixPool;
			}
		}
	}
	else if (TemplateAffixes.Num() > 0 || MinRandomModifiers > 0 || MaxRandomModifiers > 0
		|| PrefixPool.ToSoftObjectPath().IsValid() || SuffixPool.ToSoftObjectPath().IsValid())
	{
		bChanged = true;
		FInstancedStruct& AffixFragment = DefinitionFragments.AddDefaulted_GetRef();
		AffixFragment.InitializeAs<FYIItemAffixDefinitionFragment>();
		if (FYIItemAffixDefinitionFragment* AffixFrag = AffixFragment.GetMutablePtr<FYIItemAffixDefinitionFragment>())
		{
			AffixFrag->bOverrideLegacyAffixFields = false;
			AffixFrag->TemplateAffixes = TemplateAffixes;
			AffixFrag->MinRandomModifiers = MinRandomModifiers;
			AffixFrag->MaxRandomModifiers = MaxRandomModifiers;
			AffixFrag->PrefixPool = PrefixPool;
			AffixFrag->SuffixPool = SuffixPool;
		}
	}

	// Equipment fragment baseline: occupied slot list can be represented as schema fragment data.
	if (FYIItemEquipmentDefinitionFragment* Equipment = YI_FindDefinitionFragmentMutable<FYIItemEquipmentDefinitionFragment>(DefinitionFragments))
	{
		if (Equipment->OccupiedSlots.Num() == 0 && OccupiedEquipSlots.Num() > 0)
		{
			bChanged = true;
			Equipment->OccupiedSlots = OccupiedEquipSlots;
		}
	}
	else if (OccupiedEquipSlots.Num() > 0)
	{
		bChanged = true;
		FInstancedStruct& EquipmentFragment = DefinitionFragments.AddDefaulted_GetRef();
		EquipmentFragment.InitializeAs<FYIItemEquipmentDefinitionFragment>();
		if (FYIItemEquipmentDefinitionFragment* EquipmentFrag = EquipmentFragment.GetMutablePtr<FYIItemEquipmentDefinitionFragment>())
		{
			EquipmentFrag->OccupiedSlots = OccupiedEquipSlots;
		}
	}

	return bChanged;
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
	OutTemplateAffixes.Reset();
	OutMinRandomModifiers = 0;
	OutMaxRandomModifiers = 0;
	OutPrefixPool = nullptr;
	OutSuffixPool = nullptr;

	const FYIItemAffixDefinitionFragment* AffixDef = GetAffixDefinitionFragment();
	if (!AffixDef)
	{
		return;
	}

	OutTemplateAffixes = AffixDef->TemplateAffixes;
	OutMinRandomModifiers = AffixDef->MinRandomModifiers;
	OutMaxRandomModifiers = AffixDef->MaxRandomModifiers;
	OutPrefixPool = AffixDef->PrefixPool;
	OutSuffixPool = AffixDef->SuffixPool;
}

void UYIItemDefinition::GetEffectiveLayoutData(FIntPoint& OutDefaultSize, bool& bOutAllowRotation) const
{
	OutDefaultSize = FIntPoint(1, 1);
	bOutAllowRotation = true;

	const FYIItemLayoutDefinitionFragment* Layout = GetLayoutDefinitionFragment();
	if (!Layout)
	{
		return;
	}

	OutDefaultSize = Layout->DefaultSize;
	bOutAllowRotation = Layout->bAllowRotation;

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

void UYIItemDefinition::BuildSchemaSnapshot(FYIItemSchemaSnapshot& OutSnapshot) const
{
	OutSnapshot = FYIItemSchemaSnapshot();
	OutSnapshot.UniqueCode = UniqueCode;
	OutSnapshot.TemplateId = TemplateId;
	OutSnapshot.ItemType = ItemType;
	OutSnapshot.Tags = Tags;

	GetEffectiveDisplayData(OutSnapshot.Display.DisplayName, OutSnapshot.Display.Description, OutSnapshot.Display.Icon);
	GetEffectiveLayoutData(OutSnapshot.Layout.DefaultSize, OutSnapshot.Layout.bAllowRotation);
	GetEffectiveStackingRules(
		OutSnapshot.Stacking.bAllowStacking,
		OutSnapshot.Stacking.MaxStackCount,
		OutSnapshot.Stacking.bUseRiskChecks);

	TArray<TSoftObjectPtr<UYIAffixAsset>> TemplateAffixAssets;
	TSoftObjectPtr<UYIAffixPoolAsset> PrefixPoolAsset;
	TSoftObjectPtr<UYIAffixPoolAsset> SuffixPoolAsset;
	GetEffectiveAffixDefinition(
		TemplateAffixAssets,
		OutSnapshot.Affix.MinRandomModifiers,
		OutSnapshot.Affix.MaxRandomModifiers,
		PrefixPoolAsset,
		SuffixPoolAsset);

	OutSnapshot.Affix.TemplateAffixes.Reserve(TemplateAffixAssets.Num());
	for (const TSoftObjectPtr<UYIAffixAsset>& TemplateAffix : TemplateAffixAssets)
	{
		const FSoftObjectPath AssetPath = TemplateAffix.ToSoftObjectPath();
		if (AssetPath.IsValid())
		{
			OutSnapshot.Affix.TemplateAffixes.Add(AssetPath);
		}
	}
	OutSnapshot.Affix.PrefixPool = PrefixPoolAsset.ToSoftObjectPath();
	OutSnapshot.Affix.SuffixPool = SuffixPoolAsset.ToSoftObjectPath();
}

void UYIItemDefinition::GetEffectiveStackingRules(bool& bOutAllowStacking, int32& OutMaxStackCount, bool& bOutUseRiskChecks) const
{
	bOutAllowStacking = false;
	OutMaxStackCount = 1;
	bOutUseRiskChecks = true;

	const FYIItemStackingDefinitionFragment* Stacking = GetStackingDefinitionFragment();
	if (!Stacking)
	{
		return;
	}

	bOutAllowStacking = Stacking->bAllowStacking;
	OutMaxStackCount = Stacking->MaxStackCount;
	bOutUseRiskChecks = Stacking->bUseRiskChecks;
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
	EnsureBaselineDefinitionFragments();

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
	EnsureBaselineDefinitionFragments();

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
