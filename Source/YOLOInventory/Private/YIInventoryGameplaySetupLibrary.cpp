#include "YIInventoryGameplaySetupLibrary.h"

#include "GameFramework/Pawn.h"
#include "YIActionBarComponent.h"
#include "YIEquipmentComponent.h"
#include "YIEquipmentLayoutAsset.h"
#include "YIInventoryComponent.h"
#include "YIItemDefinition.h"

namespace YIInventorySetupPrivate
{
	template <typename TComponent>
	static TComponent* EnsureComponent(APawn* Pawn, bool bCreateIfMissing, TArray<FString>& OutBlockingIssues)
	{
		if (!Pawn)
		{
			return nullptr;
		}

		TComponent* Component = Pawn->FindComponentByClass<TComponent>();
		if (Component || !bCreateIfMissing)
		{
			return Component;
		}

		Component = NewObject<TComponent>(Pawn, TComponent::StaticClass(), NAME_None, RF_Transactional);
		if (!Component)
		{
			OutBlockingIssues.Add(FString::Printf(TEXT("Failed to create component '%s' on pawn '%s'."), *TComponent::StaticClass()->GetName(), *Pawn->GetName()));
			return nullptr;
		}

		Pawn->AddInstanceComponent(Component);
		Component->RegisterComponent();
		Component->SetIsReplicated(true);
		return Component;
	}

	static void BuildSummary(FYIInventoryGameplaySetupResult& InOutResult)
	{
		InOutResult.bSuccess = InOutResult.BlockingIssues.Num() == 0;
		InOutResult.Summary = FString::Printf(
			TEXT("Setup %s. Blocking=%d Warnings=%d"),
			InOutResult.bSuccess ? TEXT("OK") : TEXT("FAILED"),
			InOutResult.BlockingIssues.Num(),
			InOutResult.Warnings.Num());
	}
}

bool UYIInventoryGameplaySetupLibrary::EnsurePawnInventoryGameplaySetup(
	APawn* Pawn,
	const FYIInventoryGameplaySetupOptions& Options,
	FYIInventoryGameplaySetupResult& OutResult)
{
	OutResult = FYIInventoryGameplaySetupResult();
	if (!Pawn)
	{
		OutResult.BlockingIssues.Add(TEXT("Pawn is null."));
		YIInventorySetupPrivate::BuildSummary(OutResult);
		return false;
	}

	if (!Pawn->HasAuthority())
	{
		OutResult.BlockingIssues.Add(FString::Printf(TEXT("EnsurePawnInventoryGameplaySetup must run on authority for pawn '%s'."), *Pawn->GetName()));
		YIInventorySetupPrivate::BuildSummary(OutResult);
		return false;
	}

	UYIInventoryComponent* InventoryComp = YIInventorySetupPrivate::EnsureComponent<UYIInventoryComponent>(Pawn, Options.bCreateMissingInventoryComponent, OutResult.BlockingIssues);
	UYIEquipmentComponent* EquipmentComp = YIInventorySetupPrivate::EnsureComponent<UYIEquipmentComponent>(Pawn, Options.bCreateMissingEquipmentComponent, OutResult.BlockingIssues);
	UYIActionBarComponent* ActionBarComp = YIInventorySetupPrivate::EnsureComponent<UYIActionBarComponent>(Pawn, Options.bCreateMissingActionBarComponent, OutResult.BlockingIssues);

	if (!InventoryComp)
	{
		OutResult.BlockingIssues.Add(FString::Printf(TEXT("Pawn '%s' has no UYIInventoryComponent."), *Pawn->GetName()));
	}
	else if (Options.bEnsureAtLeastOneBag)
	{
		if (InventoryComp->Bags.Num() == 0)
		{
			UYIInventoryBag* NewBag = InventoryComp->CreateBag(FName(*Options.DefaultBagName), Options.DefaultBagGridSize);
			if (!NewBag)
			{
				OutResult.BlockingIssues.Add(TEXT("Failed to create default inventory bag."));
			}
		}

		if (!InventoryComp->EquippedBag)
		{
			if (UYIInventoryBag* BagToOpen = InventoryComp->GetBag())
			{
				InventoryComp->OpenBag(BagToOpen);
			}
		}
	}

	if (!ActionBarComp)
	{
		OutResult.Warnings.Add(FString::Printf(TEXT("Pawn '%s' has no UYIActionBarComponent."), *Pawn->GetName()));
	}
	else
	{
		ActionBarComp->InitializeActionSlots(FMath::Max(1, Options.NumActionSlots));
		ActionBarComp->bAutoBindFromEquipment = Options.bEnableActionBarAutoBindFromEquipment;
		ActionBarComp->AutoBindRules = Options.AutoBindRules;
		if (ActionBarComp->bAutoBindFromEquipment)
		{
			ActionBarComp->RebuildAutoBindingsFromEquipment(EquipmentComp);
		}
	}

	ValidatePawnInventoryGameplaySetup(Pawn, OutResult);
	return OutResult.bSuccess;
}

bool UYIInventoryGameplaySetupLibrary::QuickStartPawnInventory(APawn* Pawn, bool bOpenInventoryScreen, FYIInventoryGameplaySetupResult& OutResult)
{
	OutResult = FYIInventoryGameplaySetupResult();
	if (!Pawn)
	{
		OutResult.BlockingIssues.Add(TEXT("Pawn is null."));
		YIInventorySetupPrivate::BuildSummary(OutResult);
		return false;
	}

	if (Pawn->HasAuthority())
	{
		FYIInventoryGameplaySetupOptions DefaultOptions;
		EnsurePawnInventoryGameplaySetup(Pawn, DefaultOptions, OutResult);
	}
	else
	{
		ValidatePawnInventoryGameplaySetup(Pawn, OutResult);
	}

	if (bOpenInventoryScreen)
	{
		if (UYIInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UYIInventoryComponent>())
		{
			if (!InventoryComp->OpenInventoryScreen())
			{
				OutResult.Warnings.Add(FString::Printf(TEXT("Inventory screen did not open for pawn '%s'. Set InventoryScreenClass or call on local owning client."), *Pawn->GetName()));
			}
		}
		else
		{
			OutResult.Warnings.Add(FString::Printf(TEXT("Pawn '%s' has no UYIInventoryComponent yet; screen could not open."), *Pawn->GetName()));
		}
	}

	YIInventorySetupPrivate::BuildSummary(OutResult);
	return OutResult.bSuccess;
}

bool UYIInventoryGameplaySetupLibrary::ValidatePawnInventoryGameplaySetup(APawn* Pawn, FYIInventoryGameplaySetupResult& OutResult)
{
	OutResult = FYIInventoryGameplaySetupResult();
	if (!Pawn)
	{
		OutResult.BlockingIssues.Add(TEXT("Pawn is null."));
		YIInventorySetupPrivate::BuildSummary(OutResult);
		return false;
	}

	UYIInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UYIInventoryComponent>();
	UYIEquipmentComponent* EquipmentComp = Pawn->FindComponentByClass<UYIEquipmentComponent>();
	UYIActionBarComponent* ActionBarComp = Pawn->FindComponentByClass<UYIActionBarComponent>();

	if (!InventoryComp)
	{
		OutResult.BlockingIssues.Add(FString::Printf(TEXT("Pawn '%s' is missing UYIInventoryComponent."), *Pawn->GetName()));
	}
	else
	{
		const bool bHasActiveBag = InventoryComp->GetBag() != nullptr || InventoryComp->EquippedBag != nullptr;
		if (!bHasActiveBag && InventoryComp->Bags.Num() == 0)
		{
			OutResult.BlockingIssues.Add(FString::Printf(TEXT("Pawn '%s' has no active bag and Bags array is empty."), *Pawn->GetName()));
		}
		else if (!bHasActiveBag)
		{
			OutResult.Warnings.Add(FString::Printf(TEXT("Pawn '%s' has Bags entries but no active opened bag."), *Pawn->GetName()));
		}
	}

	if (!EquipmentComp)
	{
		OutResult.Warnings.Add(FString::Printf(TEXT("Pawn '%s' is missing UYIEquipmentComponent."), *Pawn->GetName()));
	}
	else
	{
		TArray<FString> BlockingIssues;
		TArray<FString> Warnings;
		EquipmentComp->ValidateEquipmentSetup(BlockingIssues, Warnings);
		for (const FString& Issue : BlockingIssues)
		{
			OutResult.BlockingIssues.Add(FString::Printf(TEXT("Equipment setup: %s"), *Issue));
		}
		for (const FString& Warning : Warnings)
		{
			OutResult.Warnings.Add(FString::Printf(TEXT("Equipment setup: %s"), *Warning));
		}
	}

	if (!ActionBarComp)
	{
		OutResult.Warnings.Add(FString::Printf(TEXT("Pawn '%s' is missing UYIActionBarComponent."), *Pawn->GetName()));
	}
	else
	{
		TArray<FString> BlockingIssues;
		TArray<FString> Warnings;
		ActionBarComp->ValidateActionBindings(BlockingIssues, Warnings);
		for (const FString& Issue : BlockingIssues)
		{
			OutResult.BlockingIssues.Add(FString::Printf(TEXT("Action bar setup: %s"), *Issue));
		}
		for (const FString& Warning : Warnings)
		{
			OutResult.Warnings.Add(FString::Printf(TEXT("Action bar setup: %s"), *Warning));
		}
	}

	YIInventorySetupPrivate::BuildSummary(OutResult);
	return OutResult.bSuccess;
}

bool UYIInventoryGameplaySetupLibrary::ApplySpellbookActionPreset(
	APawn* Pawn,
	FGameplayTag SpellbookEquipSlotTag,
	int32 ActionSlotIndex,
	FYIInventoryGameplaySetupResult& OutResult)
{
	FYIInventoryGameplaySetupOptions Options;
	Options.bCreateMissingInventoryComponent = true;
	Options.bCreateMissingEquipmentComponent = true;
	Options.bCreateMissingActionBarComponent = true;
	Options.bEnsureAtLeastOneBag = true;
	Options.bEnableActionBarAutoBindFromEquipment = true;
	Options.NumActionSlots = FMath::Max(ActionSlotIndex + 1, 1);

	FYIEquipmentActionAutoBindRule Rule;
	Rule.bEnabled = true;
	Rule.EquipSlotTag = SpellbookEquipSlotTag;
	Rule.ActionSlotIndex = FMath::Max(0, ActionSlotIndex);
	Rule.bAllowOverrideExistingBinding = true;
	Rule.bClearWhenUnequipped = true;
	Options.AutoBindRules.Add(Rule);

	return EnsurePawnInventoryGameplaySetup(Pawn, Options, OutResult);
}

bool UYIInventoryGameplaySetupLibrary::SyncEquipmentLayoutFromComponentSlots(UYIEquipmentComponent* EquipmentComp, UYIEquipmentLayoutAsset* LayoutAsset, bool bClearExisting)
{
	if (!EquipmentComp || !LayoutAsset)
	{
		return false;
	}

	if (bClearExisting)
	{
		LayoutAsset->Slots.Reset();
	}

	TSet<FGameplayTag> ExistingTags;
	for (const FYIEquipmentSlotLayoutEntry& Existing : LayoutAsset->Slots)
	{
		if (Existing.SlotTag.IsValid())
		{
			ExistingTags.Add(Existing.SlotTag);
		}
	}

	int32 SortBase = LayoutAsset->Slots.Num();
	for (const FYIEquipmentSlotDefinition& SlotDef : EquipmentComp->SlotDefinitions)
	{
		if (!SlotDef.SlotTag.IsValid() || ExistingTags.Contains(SlotDef.SlotTag))
		{
			continue;
		}

		FYIEquipmentSlotLayoutEntry Entry;
		Entry.SlotTag = SlotDef.SlotTag;

		const FString TagString = SlotDef.SlotTag.ToString();
		int32 LastDotIndex = INDEX_NONE;
		if (TagString.FindLastChar('.', LastDotIndex) && LastDotIndex >= 0 && LastDotIndex + 1 < TagString.Len())
		{
			Entry.DisplayName = FText::FromString(TagString.RightChop(LastDotIndex + 1));
		}
		else
		{
			Entry.DisplayName = FText::FromString(TagString);
		}

		Entry.SortOrder = SortBase++;
		LayoutAsset->Slots.Add(Entry);
		ExistingTags.Add(SlotDef.SlotTag);
	}

	LayoutAsset->SortSlotsByOrder();
	LayoutAsset->MarkPackageDirty();
	return true;
}

bool UYIInventoryGameplaySetupLibrary::EnsureItemSupportsEquipSlot(UYIItemDefinition* ItemDef, FGameplayTag SlotTag, bool bSetItemTypeToSlotTag, bool bAddToOccupiedSlots)
{
	if (!ItemDef || !SlotTag.IsValid())
	{
		return false;
	}

	ItemDef->Modify();
	if (!ItemDef->Tags.HasTagExact(SlotTag))
	{
		ItemDef->Tags.AddTag(SlotTag);
	}

	if (bSetItemTypeToSlotTag)
	{
		ItemDef->ItemType = SlotTag;
	}

	if (bAddToOccupiedSlots && !ItemDef->OccupiedEquipSlots.HasTagExact(SlotTag))
	{
		ItemDef->OccupiedEquipSlots.AddTag(SlotTag);
	}

	ItemDef->MarkPackageDirty();
	return true;
}
