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

const FYIItemStackingDefinitionFragment* UYIItemDefinition::GetStackingDefinitionFragment() const
{
	return YI_FindDefinitionFragment<FYIItemStackingDefinitionFragment>(DefinitionFragments);
}

void UYIItemDefinition::GetEffectiveDisplayData(FText& OutDisplayName, FText& OutDescription, TSoftObjectPtr<UTexture2D>& OutIcon) const
{
	OutDisplayName = FText::FromString(GetName());
	OutDescription = FText::GetEmpty();
	OutIcon = nullptr;

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

FText UYIItemDefinition::GetEffectiveDisplayName() const
{
	FText Name;
	FText Description;
	TSoftObjectPtr<UTexture2D> Icon;
	GetEffectiveDisplayData(Name, Description, Icon);
	return Name;
}

FText UYIItemDefinition::GetEffectiveDescription() const
{
	FText Name;
	FText Description;
	TSoftObjectPtr<UTexture2D> Icon;
	GetEffectiveDisplayData(Name, Description, Icon);
	return Description;
}

TSoftObjectPtr<UTexture2D> UYIItemDefinition::GetEffectiveIcon() const
{
	FText Name;
	FText Description;
	TSoftObjectPtr<UTexture2D> Icon;
	GetEffectiveDisplayData(Name, Description, Icon);
	return Icon;
}

FGameplayTag UYIItemDefinition::GetEffectivePrimaryEquipSlotTag() const
{
	if (const FYIItemEquipmentDefinitionFragment* Equip = GetEquipmentDefinitionFragment())
	{
		return Equip->PrimaryEquipSlot;
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

	if (!GetUIDefinitionFragment())
	{
		bChanged = true;
		FInstancedStruct& UI = DefinitionFragments.AddDefaulted_GetRef();
		UI.InitializeAs<FYIItemUIDefinitionFragment>();
	}

	if (!GetLayoutDefinitionFragment())
	{
		bChanged = true;
		FInstancedStruct& Layout = DefinitionFragments.AddDefaulted_GetRef();
		Layout.InitializeAs<FYIItemLayoutDefinitionFragment>();
	}

	if (!GetStackingDefinitionFragment())
	{
		bChanged = true;
		FInstancedStruct& Stacking = DefinitionFragments.AddDefaulted_GetRef();
		Stacking.InitializeAs<FYIItemStackingDefinitionFragment>();
	}

	if (!GetAffixDefinitionFragment())
	{
		bChanged = true;
		FInstancedStruct& Affix = DefinitionFragments.AddDefaulted_GetRef();
		Affix.InitializeAs<FYIItemAffixDefinitionFragment>();
	}

	if (!GetEquipmentDefinitionFragment())
	{
		bChanged = true;
		FInstancedStruct& Equip = DefinitionFragments.AddDefaulted_GetRef();
		Equip.InitializeAs<FYIItemEquipmentDefinitionFragment>();
	}

	return bChanged;
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

	if (const FYIItemAffixDefinitionFragment* AffixDef = GetAffixDefinitionFragment())
	{
		OutTemplateAffixes = AffixDef->TemplateAffixes;
		OutMinRandomModifiers = AffixDef->MinRandomModifiers;
		OutMaxRandomModifiers = AffixDef->MaxRandomModifiers;
		OutPrefixPool = AffixDef->PrefixPool;
		OutSuffixPool = AffixDef->SuffixPool;
	}
}

void UYIItemDefinition::GetEffectiveLayoutData(FIntPoint& OutDefaultSize, bool& bOutAllowRotation) const
{
	OutDefaultSize = FIntPoint(1, 1);
	bOutAllowRotation = true;

	if (const FYIItemLayoutDefinitionFragment* Layout = GetLayoutDefinitionFragment())
	{
		OutDefaultSize = Layout->DefaultSize;
		bOutAllowRotation = Layout->bAllowRotation;
		OutDefaultSize.X = FMath::Max(1, OutDefaultSize.X);
		OutDefaultSize.Y = FMath::Max(1, OutDefaultSize.Y);
	}
}

FIntPoint UYIItemDefinition::GetEffectiveDefaultSize() const
{
	FIntPoint Size = FIntPoint(1, 1);
	bool bRotation = true;
	GetEffectiveLayoutData(Size, bRotation);
	return Size;
}

bool UYIItemDefinition::IsEffectiveRotationAllowed() const
{
	FIntPoint Size = FIntPoint(1, 1);
	bool bRotation = true;
	GetEffectiveLayoutData(Size, bRotation);
	return bRotation;
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
		const FSoftObjectPath Path = TemplateAffix.ToSoftObjectPath();
		if (Path.IsValid())
		{
			OutSnapshot.Affix.TemplateAffixes.Add(Path);
		}
	}
	OutSnapshot.Affix.PrefixPool = PrefixPoolAsset.ToSoftObjectPath();
	OutSnapshot.Affix.SuffixPool = SuffixPoolAsset.ToSoftObjectPath();
}

void UYIItemDefinition::GetEffectiveStackingRules(bool& bOutAllowStacking, int32& OutMaxStackCount, bool& bOutUseRiskChecks) const
{
	bOutAllowStacking = true;
	OutMaxStackCount = 99;
	bOutUseRiskChecks = true;

	if (const FYIItemStackingDefinitionFragment* Stacking = GetStackingDefinitionFragment())
	{
		bOutAllowStacking = Stacking->bAllowStacking;
		OutMaxStackCount = Stacking->MaxStackCount;
		bOutUseRiskChecks = Stacking->bUseRiskChecks;
	}

	OutMaxStackCount = FMath::Max(1, OutMaxStackCount);
}

int32 UYIItemDefinition::GetEffectiveMaxStackCount() const
{
	bool bAllowStacking = true;
	int32 MaxStack = 99;
	bool bUseRiskChecks = true;
	GetEffectiveStackingRules(bAllowStacking, MaxStack, bUseRiskChecks);
	return MaxStack;
}

bool UYIItemDefinition::IsEffectiveStackingEnabled() const
{
	bool bAllowStacking = true;
	int32 MaxStack = 99;
	bool bUseRiskChecks = true;
	GetEffectiveStackingRules(bAllowStacking, MaxStack, bUseRiskChecks);
	return bAllowStacking && MaxStack > 1;
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

	TArray<FGameplayTag> TagArray;
	Tags.GetGameplayTagArray(TagArray);
	if (ItemType.IsValid())
	{
		TagArray.Add(ItemType);
	}
	for (const FGameplayTag& Tag : TagArray)
	{
		const FString Lower = Tag.ToString().ToLower();
		if (YI_NameSuggestsMutableStackState(Lower))
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
	bool bAllowStacking = true;
	int32 MaxStackCount = 99;
	bool bUseRiskChecks = true;
	GetEffectiveStackingRules(bAllowStacking, MaxStackCount, bUseRiskChecks);

	if (bIsContainerItem)
	{
		if (OutReason)
		{
			*OutReason = TEXT("container items are non-stackable");
		}
		return false;
	}

	if (!bAllowStacking || MaxStackCount <= 1)
	{
		if (OutReason)
		{
			*OutReason = TEXT("stacking disabled in definition");
		}
		return false;
	}

	FString RiskReason;
	if (bUseRiskChecks && HasStackingRisk(&RiskReason))
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

	FYIItemStackingDefinitionFragment* Stacking = YI_FindDefinitionFragmentMutable<FYIItemStackingDefinitionFragment>(DefinitionFragments);
	if (!Stacking || !Stacking->bAllowStacking || !Stacking->bUseRiskChecks)
	{
		return;
	}

	const FProperty* ChangedProperty = PropertyChangedEvent.Property;
	if (!ChangedProperty)
	{
		return;
	}

	const FName ChangedName = ChangedProperty->GetFName();
	const bool bRelevantChange =
		ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, DefinitionFragments)
		|| ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, bIsContainerItem)
		|| ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, ItemType)
		|| ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, Tags);
	if (!bRelevantChange)
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
			"Item '{0}' has stack-safety risk(s): {1}.\n\nPress Yes to keep stacking and disable risk checks.\nPress No to disable stacking."),
		FText::FromString(GetName()),
		FText::FromString(RiskReason));

	if (FMessageDialog::Open(EAppMsgType::YesNo, PromptText) == EAppReturnType::Yes)
	{
		Stacking->bUseRiskChecks = false;
	}
	else
	{
		Stacking->bAllowStacking = false;
		Stacking->MaxStackCount = 1;
		Stacking->bUseRiskChecks = true;
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
			if (UYIItemRegistrySubsystem* Registry = Engine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
			{
				Registry->BuildIndex(true);
				int64 NewCode = 0;
				do
				{
					NewCode = (int64)FMath::RandRange(100000, INT32_MAX) * 1000ll + (int64)FMath::RandRange(0, 999);
				}
				while (Registry->GetByCode(NewCode) != nullptr);
				UniqueCode = NewCode;
			}
		}
	}

	FYIItemStackingDefinitionFragment* Stacking = YI_FindDefinitionFragmentMutable<FYIItemStackingDefinitionFragment>(DefinitionFragments);
	if (!Stacking)
	{
		return;
	}

	Stacking->MaxStackCount = FMath::Max(1, Stacking->MaxStackCount);

	if (bIsContainerItem)
	{
		Stacking->bAllowStacking = false;
		Stacking->MaxStackCount = 1;
		Stacking->bUseRiskChecks = true;
		return;
	}

	FString RiskReason;
	if (Stacking->bAllowStacking && Stacking->MaxStackCount > 1 && Stacking->bUseRiskChecks && HasStackingRisk(&RiskReason))
	{
		Stacking->bAllowStacking = false;
		Stacking->MaxStackCount = 1;
		UE_LOG(LogTemp, Warning, TEXT("UYIItemDefinition '%s': stacking auto-disabled due to risk (%s). Disable risk checks to override."),
			*GetName(), *RiskReason);
	}
}
#endif
