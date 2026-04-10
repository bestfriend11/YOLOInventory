#include "YIItemDefinition.h"

#include "YIItemRegistrySubsystem.h"
#include "YIItemSchemaResolver.h"
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

}

const FInstancedStruct* UYIItemDefinition::FindDefinitionFragmentByStruct(const UScriptStruct* FragmentStruct) const
{
	return YI_FindDefinitionFragmentByStruct(DefinitionFragments, FragmentStruct);
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

FInstancedStruct* UYIItemDefinition::FindOrAddDefaultInstanceFragmentByStruct(const UScriptStruct* FragmentStruct)
{
	if (!FragmentStruct)
	{
		return nullptr;
	}

	for (FInstancedStruct& Fragment : DefaultInstanceFragments)
	{
		if (Fragment.GetScriptStruct() == FragmentStruct)
		{
			return &Fragment;
		}
	}

	FInstancedStruct& NewFragment = DefaultInstanceFragments.AddDefaulted_GetRef();
	NewFragment.InitializeAs(FragmentStruct);
	return &NewFragment;
}

void UYIItemDefinition::BuildSchemaSnapshot(FYIItemSchemaSnapshot& OutSnapshot) const
{
	OutSnapshot = YIItemSchema::ResolveSnapshot(this);
}

bool UYIItemDefinition::HasStackingRisk(FString* OutReason) const
{
	const FYIItemSchemaSnapshot& Snapshot = YIItemSchema::ResolveSnapshot(this);
	TArray<FString> Reasons;

	const bool bHasRandomRollSetup = Snapshot.Affix.MinRandomModifiers > 0
		|| Snapshot.Affix.MaxRandomModifiers > 0
		|| Snapshot.Affix.PrefixPool.IsValid()
		|| Snapshot.Affix.SuffixPool.IsValid();
	if (bHasRandomRollSetup)
	{
		Reasons.Add(TEXT("randomized affix rolls/pools"));
	}

	if (Snapshot.Affix.TemplateAffixes.Num() > 0)
	{
		Reasons.Add(TEXT("template affixes (instance data can diverge after runtime changes)"));
	}

	if (Snapshot.Container.bIsContainerItem)
	{
		Reasons.Add(TEXT("container item (bag-in-bag instances must not stack)"));
	}

	TArray<FGameplayTag> TagArray;
	Snapshot.Classification.Tags.GetGameplayTagArray(TagArray);
	if (Snapshot.Classification.ItemType.IsValid())
	{
		TagArray.Add(Snapshot.Classification.ItemType);
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
	const FYIItemSchemaSnapshot& Snapshot = YIItemSchema::ResolveSnapshot(this);
	if (Snapshot.Container.bIsContainerItem)
	{
		if (OutReason)
		{
			*OutReason = TEXT("container items are non-stackable");
		}
		return false;
	}

	if (Snapshot.Stacking.MaxStackCount <= 1)
	{
		if (OutReason)
		{
			*OutReason = TEXT("stacking is not defined for this item");
		}
		return false;
	}

	FString RiskReason;
	if (Snapshot.Stacking.bUseRiskChecks && HasStackingRisk(&RiskReason))
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

	FYIItemStackingDefinitionFragment* Stacking = YI_FindDefinitionFragmentMutable<FYIItemStackingDefinitionFragment>(DefinitionFragments);
	if (!Stacking || Stacking->MaxStackCount <= 1 || !Stacking->bUseRiskChecks)
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
		Stacking->MaxStackCount = 1;
		Stacking->bUseRiskChecks = true;
	}
}

void UYIItemDefinition::PreSave(FObjectPreSaveContext SaveContext)
{
	Super::PreSave(SaveContext);
	YIItemSchema::InvalidateSnapshotCache(this);

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

	if (YIItemSchema::ResolveSnapshot(this).Container.bIsContainerItem)
	{
		Stacking->MaxStackCount = 1;
		Stacking->bUseRiskChecks = true;
		return;
	}

	FString RiskReason;
	if (Stacking->MaxStackCount > 1 && Stacking->bUseRiskChecks && HasStackingRisk(&RiskReason))
	{
		Stacking->MaxStackCount = 1;
		UE_LOG(LogTemp, Warning, TEXT("UYIItemDefinition '%s': stacking auto-disabled due to risk (%s). Disable risk checks to override."),
			*GetName(), *RiskReason);
	}
}
#endif
