#include "YIItemDefinition.h"

#include "YIItemRegistrySubsystem.h"
#include "YIItemSchemaResolver.h"
#include "YIItemTraitAsset.h"
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

	static const FInstancedStruct* YI_FindDefinitionFragmentByStruct(const TArray<FInstancedStruct>& Fragments, const UScriptStruct* FragmentStruct)
	{
		if (!FragmentStruct)
		{
			return nullptr;
		}

		for (const FInstancedStruct& Fragment : Fragments)
		{
			if (Fragment.GetScriptStruct() == FragmentStruct)
			{
				return &Fragment;
			}
		}
		return nullptr;
	}

	static const FInstancedStruct* YI_FindResolvedDefinitionFragmentByStruct(
		const UYIItemDefinition* ItemDef,
		const UScriptStruct* FragmentStruct,
		TSet<const UYIItemDefinition*>& VisitedDefinitions)
	{
		if (!ItemDef || !FragmentStruct || VisitedDefinitions.Contains(ItemDef))
		{
			return nullptr;
		}
		VisitedDefinitions.Add(ItemDef);

		// Local fragments have highest precedence.
		if (const FInstancedStruct* Local = YI_FindDefinitionFragmentByStruct(ItemDef->DefinitionFragments, FragmentStruct))
		{
			return Local;
		}

		// Traits are applied in order; later traits should override earlier traits.
		for (int32 TraitIndex = ItemDef->Traits.Num() - 1; TraitIndex >= 0; --TraitIndex)
		{
			const UYIItemTraitAsset* Trait = ItemDef->Traits[TraitIndex].LoadSynchronous();
			if (!Trait)
			{
				continue;
			}
			if (const FInstancedStruct* TraitFragment = YI_FindDefinitionFragmentByStruct(Trait->DefinitionFragments, FragmentStruct))
			{
				return TraitFragment;
			}
		}

		// Parent definition is the fallback baseline.
		const UYIItemDefinition* Parent = ItemDef->ParentDefinition.LoadSynchronous();
		return YI_FindResolvedDefinitionFragmentByStruct(Parent, FragmentStruct, VisitedDefinitions);
	}

	static void YI_CollectEffectiveTagsRecursive(const UYIItemDefinition* ItemDef, TSet<const UYIItemDefinition*>& VisitedDefinitions, FGameplayTagContainer& OutTags)
	{
		if (!ItemDef || VisitedDefinitions.Contains(ItemDef))
		{
			return;
		}
		VisitedDefinitions.Add(ItemDef);

		if (const UYIItemDefinition* Parent = ItemDef->ParentDefinition.LoadSynchronous())
		{
			YI_CollectEffectiveTagsRecursive(Parent, VisitedDefinitions, OutTags);
		}

		for (const TSoftObjectPtr<UYIItemTraitAsset>& TraitPtr : ItemDef->Traits)
		{
			const UYIItemTraitAsset* Trait = TraitPtr.LoadSynchronous();
			if (!Trait)
			{
				continue;
			}

			if (const FYIItemClassificationDefinitionFragment* Classification = YI_FindDefinitionFragment<FYIItemClassificationDefinitionFragment>(Trait->DefinitionFragments))
			{
				OutTags.AppendTags(Classification->Tags);
			}
		}

		if (const FYIItemClassificationDefinitionFragment* Classification = YI_FindDefinitionFragment<FYIItemClassificationDefinitionFragment>(ItemDef->DefinitionFragments))
		{
			OutTags.AppendTags(Classification->Tags);
		}
	}
}

const FYIItemUIDefinitionFragment* UYIItemDefinition::GetUIDefinitionFragment() const
{
	if (const FInstancedStruct* Fragment = FindResolvedDefinitionFragmentByStruct(FYIItemUIDefinitionFragment::StaticStruct()))
	{
		return Fragment->GetPtr<FYIItemUIDefinitionFragment>();
	}
	return nullptr;
}

const FYIItemClassificationDefinitionFragment* UYIItemDefinition::GetClassificationDefinitionFragment() const
{
	if (const FInstancedStruct* Fragment = FindResolvedDefinitionFragmentByStruct(FYIItemClassificationDefinitionFragment::StaticStruct()))
	{
		return Fragment->GetPtr<FYIItemClassificationDefinitionFragment>();
	}
	return nullptr;
}

const FYIItemAudioDefinitionFragment* UYIItemDefinition::GetAudioDefinitionFragment() const
{
	if (const FInstancedStruct* Fragment = FindResolvedDefinitionFragmentByStruct(FYIItemAudioDefinitionFragment::StaticStruct()))
	{
		return Fragment->GetPtr<FYIItemAudioDefinitionFragment>();
	}
	return nullptr;
}

const FYIItemPickupDefinitionFragment* UYIItemDefinition::GetPickupDefinitionFragment() const
{
	if (const FInstancedStruct* Fragment = FindResolvedDefinitionFragmentByStruct(FYIItemPickupDefinitionFragment::StaticStruct()))
	{
		return Fragment->GetPtr<FYIItemPickupDefinitionFragment>();
	}
	return nullptr;
}

const FYIItemEquipmentDefinitionFragment* UYIItemDefinition::GetEquipmentDefinitionFragment() const
{
	if (const FInstancedStruct* Fragment = FindResolvedDefinitionFragmentByStruct(FYIItemEquipmentDefinitionFragment::StaticStruct()))
	{
		return Fragment->GetPtr<FYIItemEquipmentDefinitionFragment>();
	}
	return nullptr;
}

const FYIItemAffixDefinitionFragment* UYIItemDefinition::GetAffixDefinitionFragment() const
{
	if (const FInstancedStruct* Fragment = FindResolvedDefinitionFragmentByStruct(FYIItemAffixDefinitionFragment::StaticStruct()))
	{
		return Fragment->GetPtr<FYIItemAffixDefinitionFragment>();
	}
	return nullptr;
}

const FYIItemLayoutDefinitionFragment* UYIItemDefinition::GetLayoutDefinitionFragment() const
{
	if (const FInstancedStruct* Fragment = FindResolvedDefinitionFragmentByStruct(FYIItemLayoutDefinitionFragment::StaticStruct()))
	{
		return Fragment->GetPtr<FYIItemLayoutDefinitionFragment>();
	}
	return nullptr;
}

const FYIItemStackingDefinitionFragment* UYIItemDefinition::GetStackingDefinitionFragment() const
{
	if (const FInstancedStruct* Fragment = FindResolvedDefinitionFragmentByStruct(FYIItemStackingDefinitionFragment::StaticStruct()))
	{
		return Fragment->GetPtr<FYIItemStackingDefinitionFragment>();
	}
	return nullptr;
}

const FYIItemRulesDefinitionFragment* UYIItemDefinition::GetRulesDefinitionFragment() const
{
	if (const FInstancedStruct* Fragment = FindResolvedDefinitionFragmentByStruct(FYIItemRulesDefinitionFragment::StaticStruct()))
	{
		return Fragment->GetPtr<FYIItemRulesDefinitionFragment>();
	}
	return nullptr;
}

const FYIItemContainerDefinitionFragment* UYIItemDefinition::GetContainerDefinitionFragment() const
{
	if (const FInstancedStruct* Fragment = FindResolvedDefinitionFragmentByStruct(FYIItemContainerDefinitionFragment::StaticStruct()))
	{
		return Fragment->GetPtr<FYIItemContainerDefinitionFragment>();
	}
	return nullptr;
}

const FYIItemAttributeModsDefinitionFragment* UYIItemDefinition::GetAttributeModsDefinitionFragment() const
{
	if (const FInstancedStruct* Fragment = FindResolvedDefinitionFragmentByStruct(FYIItemAttributeModsDefinitionFragment::StaticStruct()))
	{
		return Fragment->GetPtr<FYIItemAttributeModsDefinitionFragment>();
	}
	return nullptr;
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

FGameplayTag UYIItemDefinition::GetEffectiveItemType() const
{
	if (const FYIItemClassificationDefinitionFragment* Classification = GetClassificationDefinitionFragment())
	{
		return Classification->ItemType;
	}
	return FGameplayTag();
}

void UYIItemDefinition::GetEffectiveTags(FGameplayTagContainer& OutTags) const
{
	OutTags.Reset();
	TSet<const UYIItemDefinition*> VisitedDefinitions;
	YI_CollectEffectiveTagsRecursive(this, VisitedDefinitions, OutTags);
}

FGameplayTag UYIItemDefinition::GetEffectiveRarityTag() const
{
	if (const FYIItemClassificationDefinitionFragment* Classification = GetClassificationDefinitionFragment())
	{
		return Classification->RarityTag;
	}
	return FGameplayTag();
}

FGameplayTag UYIItemDefinition::GetEffectiveAudioTag() const
{
	if (const FYIItemAudioDefinitionFragment* Audio = GetAudioDefinitionFragment())
	{
		return Audio->AudioTag;
	}
	return FGameplayTag();
}

TSoftObjectPtr<UYIItemSFXProfile> UYIItemDefinition::GetEffectiveSoundProfileOverride() const
{
	if (const FYIItemAudioDefinitionFragment* Audio = GetAudioDefinitionFragment())
	{
		return Audio->SoundProfileOverride;
	}
	return nullptr;
}

bool UYIItemDefinition::IsEffectiveUniquePerType() const
{
	if (const FYIItemRulesDefinitionFragment* Rules = GetRulesDefinitionFragment())
	{
		return Rules->bUniquePerType;
	}
	return false;
}

int32 UYIItemDefinition::GetEffectiveEquipSlotCost() const
{
	if (const FYIItemRulesDefinitionFragment* Rules = GetRulesDefinitionFragment())
	{
		return FMath::Max(1, Rules->EquipSlotCost);
	}
	return 1;
}

bool UYIItemDefinition::IsEffectiveContainerItem() const
{
	if (const FYIItemContainerDefinitionFragment* Container = GetContainerDefinitionFragment())
	{
		return Container->bIsContainerItem;
	}
	return false;
}

TSoftObjectPtr<UObject> UYIItemDefinition::GetEffectiveContainerTemplateBag() const
{
	if (const FYIItemContainerDefinitionFragment* Container = GetContainerDefinitionFragment())
	{
		return Container->ContainerTemplateBag;
	}
	return nullptr;
}

FIntPoint UYIItemDefinition::GetEffectiveContainerDefaultGridSize() const
{
	if (const FYIItemContainerDefinitionFragment* Container = GetContainerDefinitionFragment())
	{
		return FIntPoint(FMath::Max(1, Container->ContainerDefaultGridSize.X), FMath::Max(1, Container->ContainerDefaultGridSize.Y));
	}
	return FIntPoint(6, 8);
}

void UYIItemDefinition::GetEffectiveAttributeMods(TArray<TSoftObjectPtr<UYIAttributeModAsset>>& OutAttributeMods) const
{
	OutAttributeMods.Reset();
	if (const FYIItemAttributeModsDefinitionFragment* AttrMods = GetAttributeModsDefinitionFragment())
	{
		OutAttributeMods = AttrMods->AttributeMods;
	}
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
	return YI_FindDefinitionFragmentByStruct(DefinitionFragments, FragmentStruct);
}

const FInstancedStruct* UYIItemDefinition::FindResolvedDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct) const
{
	TSet<const UYIItemDefinition*> VisitedDefinitions;
	return YI_FindResolvedDefinitionFragmentByStruct(this, FragmentStruct, VisitedDefinitions);
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

	if (!GetClassificationDefinitionFragment())
	{
		bChanged = true;
		FInstancedStruct& Classification = DefinitionFragments.AddDefaulted_GetRef();
		Classification.InitializeAs<FYIItemClassificationDefinitionFragment>();
	}

	if (!GetAudioDefinitionFragment())
	{
		bChanged = true;
		FInstancedStruct& Audio = DefinitionFragments.AddDefaulted_GetRef();
		Audio.InitializeAs<FYIItemAudioDefinitionFragment>();
	}

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

	if (!GetRulesDefinitionFragment())
	{
		bChanged = true;
		FInstancedStruct& Rules = DefinitionFragments.AddDefaulted_GetRef();
		Rules.InitializeAs<FYIItemRulesDefinitionFragment>();
	}

	if (!GetContainerDefinitionFragment())
	{
		bChanged = true;
		FInstancedStruct& Container = DefinitionFragments.AddDefaulted_GetRef();
		Container.InitializeAs<FYIItemContainerDefinitionFragment>();
	}

	if (!GetAttributeModsDefinitionFragment())
	{
		bChanged = true;
		FInstancedStruct& AttrMods = DefinitionFragments.AddDefaulted_GetRef();
		AttrMods.InitializeAs<FYIItemAttributeModsDefinitionFragment>();
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
	OutSnapshot = YIItemSchema::ResolveSnapshot(this);
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

	if (IsEffectiveContainerItem())
	{
		Reasons.Add(TEXT("container item (bag-in-bag instances must not stack)"));
	}

	TArray<FGameplayTag> TagArray;
	FGameplayTagContainer EffectiveTags;
	GetEffectiveTags(EffectiveTags);
	EffectiveTags.GetGameplayTagArray(TagArray);
	const FGameplayTag EffectiveItemType = GetEffectiveItemType();
	if (EffectiveItemType.IsValid())
	{
		TagArray.Add(EffectiveItemType);
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

	if (IsEffectiveContainerItem())
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
	YIItemSchema::InvalidateAllSnapshotCaches();
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
		|| ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, ParentDefinition)
		|| ChangedName == GET_MEMBER_NAME_CHECKED(UYIItemDefinition, Traits);
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
	YIItemSchema::InvalidateSnapshotCache(this);
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

	if (IsEffectiveContainerItem())
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
