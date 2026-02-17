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
}

bool UYIItemDefinition::HasStackingRisk(FString* OutReason) const
{
	TArray<FString> Reasons;

	const bool bHasRandomRollSetup = MinRandomModifiers > 0
		|| MaxRandomModifiers > 0
		|| PrefixPool.ToSoftObjectPath().IsValid()
		|| SuffixPool.ToSoftObjectPath().IsValid();
	if (bHasRandomRollSetup)
	{
		Reasons.Add(TEXT("randomized affix rolls/pools"));
	}

	if (TemplateAffixes.Num() > 0)
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
	const bool bHasRisk = HasStackingRisk(&RiskReason);
	if (bHasRisk && !bAllowUnsafeStacking)
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
	if (bAllowStacking && !bAllowUnsafeStacking && HasStackingRisk(&RiskReason))
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
