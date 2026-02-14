#include "YIPlayerInventoryStateComponent.h"

#include "Net/UnrealNetwork.h"
#include "YIInventoryComponent.h"
#include "YIInventoryBag.h"
#include "YIInventoryPersistenceProvider.h"
#include "YIInventorySaveGame.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/Crc.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogYIInventoryPersistence, Log, All);

UYIPlayerInventoryStateComponent::UYIPlayerInventoryStateComponent()
{
	SetIsReplicatedByDefault(true);
}

UYIInventoryPersistenceProviderBase* UYIPlayerInventoryStateComponent::GetOrCreatePersistenceProvider()
{
	if (!PersistenceProvider)
	{
		PersistenceProvider = NewObject<UYISaveGameInventoryPersistenceProvider>(this, UYISaveGameInventoryPersistenceProvider::StaticClass(), NAME_None, RF_Transient);
	}
	return PersistenceProvider;
}

void UYIPlayerInventoryStateComponent::EmitSaveDiagnostic(const FString& Message, const FColor& Color, bool bForceOnScreen) const
{
	UE_LOG(LogYIInventoryPersistence, Log, TEXT("%s"), *Message);

	if (!bEnableSaveDiagnostics && !bForceOnScreen)
	{
		return;
	}

	if ((bShowSaveDiagnosticsOnScreen || bForceOnScreen) && GEngine)
	{
		const uint32 MessageHash = FCrc::StrCrc32(*Message);
		const uint64 PersistentMessageKey = 0x5949000000000000ULL | static_cast<uint64>(MessageHash); // "YI" namespace + message hash
		const float DisplaySeconds = bKeepDiagnosticsPinnedOnScreen ? 1800.0f : SaveDiagnosticOnScreenSeconds;
		const uint64 ScreenKey = bKeepDiagnosticsPinnedOnScreen ? PersistentMessageKey : static_cast<uint64>(-1);
		GEngine->AddOnScreenDebugMessage(ScreenKey, DisplaySeconds, Color, Message);
	}
}

bool UYIPlayerInventoryStateComponent::DiagnoseSaveSetup(FString& OutMessage) const
{
	if (!GetOwner())
	{
		OutMessage = TEXT("Persistence disabled: component has no owner.");
		return false;
	}
	if (!Cast<APlayerState>(GetOwner()))
	{
		OutMessage = FString::Printf(TEXT("Persistence warning: owner '%s' is not PlayerState."), *GetOwner()->GetName());
		return false;
	}
	if (GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		OutMessage = TEXT("Persistence inactive on this instance: not authority (expected on clients).");
		return false;
	}
	if (!bEnableAutoSave)
	{
		OutMessage = TEXT("Persistence disabled: bEnableAutoSave=false.");
		return false;
	}
	if (!const_cast<UYIPlayerInventoryStateComponent*>(this)->GetOrCreatePersistenceProvider())
	{
		OutMessage = TEXT("Persistence disabled: no persistence provider is configured.");
		return false;
	}

	OutMessage = FString::Printf(TEXT("Persistence ready. Slot='%s' User=%d"), *BuildEffectiveSaveSlotName(), SaveUserIndex);
	return true;
}

bool UYIPlayerInventoryStateComponent::RunRuntimePreflight(APawn* Pawn, TArray<FString>& OutBlockingIssues, TArray<FString>& OutWarnings) const
{
	OutBlockingIssues.Reset();
	OutWarnings.Reset();
	auto AddUniqueIssue = [](TArray<FString>& Target, const FString& Message)
	{
		if (!Target.Contains(Message))
		{
			Target.Add(Message);
		}
	};

	if (!Pawn)
	{
		OutBlockingIssues.Add(TEXT("Pawn is null."));
		return false;
	}

	UYIInventoryComponent* InventoryComp = Pawn->FindComponentByClass<UYIInventoryComponent>();
	if (!InventoryComp)
	{
		OutBlockingIssues.Add(FString::Printf(TEXT("Pawn '%s' is missing UYIInventoryComponent."), *Pawn->GetName()));
		return false;
	}

	UYIInventoryBag* ActiveBag = InventoryComp->GetBag();
	if (!ActiveBag)
	{
		ActiveBag = InventoryComp->EquippedBag;
	}
	if (!ActiveBag && InventoryComp->Bags.Num() == 0)
	{
		OutBlockingIssues.Add(FString::Printf(TEXT("Pawn '%s' has no active bag and Bags array is empty."), *Pawn->GetName()));
	}
	else if (!ActiveBag)
	{
		OutWarnings.Add(FString::Printf(TEXT("Pawn '%s' has Bags entries but no active equipped bag."), *Pawn->GetName()));
	}

	UYIEquipmentComponent* EquipmentComp = Pawn->FindComponentByClass<UYIEquipmentComponent>();
	if (!EquipmentComp)
	{
		OutWarnings.Add(FString::Printf(TEXT("Pawn '%s' is missing UYIEquipmentComponent (equip actions disabled)."), *Pawn->GetName()));
	}
	else
	{
		TSet<FGameplayTag> SeenSlots;
		for (const FYIEquippedItemEntry& Entry : EquipmentComp->EquippedItems)
		{
			if (!Entry.SlotTag.IsValid())
			{
				OutBlockingIssues.Add(TEXT("Equipment contains an entry with invalid SlotTag."));
				continue;
			}
			if (SeenSlots.Contains(Entry.SlotTag))
			{
				OutBlockingIssues.Add(FString::Printf(TEXT("Equipment slot '%s' is duplicated."), *Entry.SlotTag.ToString()));
			}
			SeenSlots.Add(Entry.SlotTag);

			if (EquipmentComp->AllowedEquipSlots.Num() > 0 && !EquipmentComp->AllowedEquipSlots.HasTagExact(Entry.SlotTag))
			{
				OutWarnings.Add(FString::Printf(TEXT("Equipped slot '%s' is not in AllowedEquipSlots."), *Entry.SlotTag.ToString()));
			}

			if (Entry.Item.Definition.ToSoftObjectPath().IsNull())
			{
				OutWarnings.Add(FString::Printf(TEXT("Equipped slot '%s' has no item definition reference."), *Entry.SlotTag.ToString()));
			}
		}

		TArray<FString> EquipmentBlockingIssues;
		TArray<FString> EquipmentWarnings;
		EquipmentComp->ValidateEquipmentSetup(EquipmentBlockingIssues, EquipmentWarnings);
		for (const FString& Issue : EquipmentBlockingIssues)
		{
			AddUniqueIssue(OutBlockingIssues, FString::Printf(TEXT("Equipment setup: %s"), *Issue));
		}
		for (const FString& Warning : EquipmentWarnings)
		{
			AddUniqueIssue(OutWarnings, FString::Printf(TEXT("Equipment setup: %s"), *Warning));
		}
	}

	UYIActionBarComponent* ActionBarComp = Pawn->FindComponentByClass<UYIActionBarComponent>();
	if (!ActionBarComp)
	{
		OutWarnings.Add(FString::Printf(TEXT("Pawn '%s' is missing UYIActionBarComponent (action bindings disabled)."), *Pawn->GetName()));
	}
	else
	{
		if (ActionBarComp->ActionBindings.Num() != ActionBarComp->NumActionSlots)
		{
			OutWarnings.Add(FString::Printf(
				TEXT("ActionBindings count (%d) does not match NumActionSlots (%d)."),
				ActionBarComp->ActionBindings.Num(),
				ActionBarComp->NumActionSlots));
		}

		for (int32 SlotIndex = 0; SlotIndex < ActionBarComp->ActionBindings.Num(); ++SlotIndex)
		{
			const FYIActionBarBinding& Binding = ActionBarComp->ActionBindings[SlotIndex];
			if (!Binding.bEnabled)
			{
				continue;
			}

			const bool bHasActionTag = Binding.ActionTag.IsValid();
			const bool bHasAbilityClass = !Binding.AbilityClass.ToSoftObjectPath().IsNull();
			if (!bHasActionTag && !bHasAbilityClass)
			{
				OutBlockingIssues.Add(FString::Printf(TEXT("Action slot %d is enabled but has no ActionTag and no AbilityClass."), SlotIndex));
			}

			if (bHasActionTag && !Binding.ActionTag.ToString().StartsWith(ActionBarComp->ActionTagPrefix))
			{
				OutWarnings.Add(FString::Printf(
					TEXT("Action slot %d tag '%s' does not start with expected prefix '%s'."),
					SlotIndex,
					*Binding.ActionTag.ToString(),
					*ActionBarComp->ActionTagPrefix));
			}

			if (Binding.SourceEquipSlotTag.IsValid())
			{
				if (!EquipmentComp)
				{
					OutBlockingIssues.Add(FString::Printf(
						TEXT("Action slot %d references equip slot '%s' but pawn has no equipment component."),
						SlotIndex,
						*Binding.SourceEquipSlotTag.ToString()));
				}
				else
				{
					FYIItemInstanceNet TempItem;
					if (!EquipmentComp->GetEquippedItem(Binding.SourceEquipSlotTag, TempItem))
					{
						OutWarnings.Add(FString::Printf(
							TEXT("Action slot %d references equip slot '%s' which has no equipped item."),
							SlotIndex,
							*Binding.SourceEquipSlotTag.ToString()));
					}
				}
			}
		}

		TArray<FString> ActionBlockingIssues;
		TArray<FString> ActionWarnings;
		ActionBarComp->ValidateActionBindings(ActionBlockingIssues, ActionWarnings);
		for (const FString& Issue : ActionBlockingIssues)
		{
			AddUniqueIssue(OutBlockingIssues, FString::Printf(TEXT("Action bar setup: %s"), *Issue));
		}
		for (const FString& Warning : ActionWarnings)
		{
			AddUniqueIssue(OutWarnings, FString::Printf(TEXT("Action bar setup: %s"), *Warning));
		}
	}

	return OutBlockingIssues.Num() == 0;
}

void UYIPlayerInventoryStateComponent::SaveNow()
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		EmitSaveDiagnostic(TEXT("SaveNow skipped: authority only."), FColor::Yellow, true);
		return;
	}

	if (ObservedPawn.IsValid())
	{
		SaveCurrentPawnInventory(ObservedPawn.Get());
	}
	SaveToDisk();
}

void UYIPlayerInventoryStateComponent::LoadNow()
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		EmitSaveDiagnostic(TEXT("LoadNow skipped: authority only."), FColor::Yellow, true);
		return;
	}

	LoadFromDisk();
	if (ObservedPawn.IsValid())
	{
		RestoreInventoryToPawn(ObservedPawn.Get());
		ApplySavedRuntimeStateToPawn(ObservedPawn.Get());
	}
}

void UYIPlayerInventoryStateComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!Cast<APlayerState>(GetOwner()))
	{
		EmitSaveDiagnostic(TEXT("UYIPlayerInventoryStateComponent should live on PlayerState for stable multiplayer persistence."), FColor::Orange, true);
	}

	// Only the server should drive persistence + bag rebinding.
	if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_Authority && bEnableAutoSave)
	{
		FString SetupMessage;
		DiagnoseSaveSetup(SetupMessage);
		EmitSaveDiagnostic(SetupMessage, FColor::Cyan, true);
		LoadFromDisk();
		// Poll until a pawn exists, then bind to its bag changes.
		TryAutoRegisterPawn();
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(AutoSavePollHandle, this, &UYIPlayerInventoryStateComponent::TryAutoRegisterPawn, 1.0f, true);
		}
	}
	else if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_Authority && !bEnableAutoSave)
	{
		EmitSaveDiagnostic(TEXT("Autosave is disabled (bEnableAutoSave=false)."), FColor::Yellow, true);
	}
}

void UYIPlayerInventoryStateComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoSavePollHandle);
		GetWorld()->GetTimerManager().ClearTimer(DebounceHandle);
	}
	UnbindAutoSave();
	Super::EndPlay(EndPlayReason);
}

int32 UYIPlayerInventoryStateComponent::AddSharedBag(UYIInventoryBag* Bag)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority || !Bag)
	{
		return INDEX_NONE;
	}
	const int32 Index = SharedBags.Add(TSoftObjectPtr<UYIInventoryBag>(Bag));
	SaveToDisk();
	return Index;
}

int32 UYIPlayerInventoryStateComponent::AddPartyMember(const FYIPartyMemberEntry& Entry)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return INDEX_NONE;
	}
	const int32 NewIndex = PartyMembers.Add(Entry);
	SaveToDisk();
	return NewIndex;
}

bool UYIPlayerInventoryStateComponent::AssignInventoryToPawn(APawn* Pawn, int32 PartyIndex)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority || !Pawn)
	{
		EmitSaveDiagnostic(TEXT("AssignInventoryToPawn failed: invalid owner/authority/pawn."), FColor::Red, true);
		return false;
	}

	UYIInventoryComponent* InvComp = Pawn->FindComponentByClass<UYIInventoryComponent>();
	if (!InvComp)
	{
		EmitSaveDiagnostic(FString::Printf(TEXT("AssignInventoryToPawn failed: pawn '%s' is missing UYIInventoryComponent."), *Pawn->GetName()), FColor::Red, true);
		return false;
	}

	// If the requested index is invalid, auto-create a party entry using the pawn's equipped bag (if any)
	if (!PartyMembers.IsValidIndex(PartyIndex))
	{
		if (!InvComp->EquippedBag)
		{
			return false; // cannot auto-create without a bag template
		}

		FYIPartyMemberEntry NewEntry;
		NewEntry.PawnClass = Pawn->GetClass();
		NewEntry.InventoryBag = InvComp->EquippedBag;
		NewEntry.DisplayName = FText::FromString(Pawn->GetName());
		PartyIndex = PartyMembers.Add(NewEntry);
	}

	const FYIPartyMemberEntry& Entry = PartyMembers[PartyIndex];
	if (!Entry.InventoryBag.IsValid() && Entry.InventoryBag.ToSoftObjectPath().IsNull())
	{
		return false;
	}

	UYIInventoryBag* Bag = Entry.InventoryBag.IsValid() ? Entry.InventoryBag.Get() : Entry.InventoryBag.LoadSynchronous();
	if (!Bag)
	{
		EmitSaveDiagnostic(FString::Printf(TEXT("AssignInventoryToPawn failed: party bag could not be loaded for pawn '%s'."), *Pawn->GetName()), FColor::Red, true);
		return false;
	}

	InvComp->EquippedBag = Bag;
	BindAutoSave(Pawn); // ensure autosave tracks the newly assigned bag
	ObservedPartyIndex = PartyIndex;
	// If we already have a saved snapshot for this index, restore it; otherwise capture initial state.
	if (!RestoreInventoryToPawn(Pawn))
	{
		SaveCurrentPawnInventory(Pawn);
	}
	ApplySavedRuntimeStateToPawn(Pawn);

	TArray<FString> BlockingIssues;
	TArray<FString> Warnings;
	const bool bPreflightOk = RunRuntimePreflight(Pawn, BlockingIssues, Warnings);
	for (const FString& Issue : BlockingIssues)
	{
		EmitSaveDiagnostic(FString::Printf(TEXT("Runtime Preflight BLOCK: %s"), *Issue), FColor::Red, true);
	}
	for (const FString& Warning : Warnings)
	{
		EmitSaveDiagnostic(FString::Printf(TEXT("Runtime Preflight WARN: %s"), *Warning), FColor::Yellow, true);
	}
	if (bPreflightOk)
	{
		EmitSaveDiagnostic(TEXT("Runtime preflight passed."), FColor::Green);
	}

	SaveToDisk();
	return true;
}

void UYIPlayerInventoryStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UYIPlayerInventoryStateComponent, SharedBags, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIPlayerInventoryStateComponent, PartyMembers, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIPlayerInventoryStateComponent, Resources, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIPlayerInventoryStateComponent, SavedBags, COND_OwnerOnly);
}

void UYIPlayerInventoryStateComponent::AddResource(FName ResourceName, int64 Delta)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority || ResourceName.IsNone())
	{
		return;
	}
	Resources.Add(ResourceName, Delta);
	SaveToDisk();
}

bool UYIPlayerInventoryStateComponent::ConsumeResource(FName ResourceName, int64 Amount)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority || ResourceName.IsNone())
	{
		return false;
	}
	const bool bConsumed = Resources.Consume(ResourceName, Amount);
	if (bConsumed)
	{
		SaveToDisk();
	}
	return bConsumed;
}

int64 UYIPlayerInventoryStateComponent::GetResourceAmount(FName ResourceName) const
{
	return Resources.Get(ResourceName);
}

static void CopyBagToSnapshot(const UYIInventoryBag* Bag, FYISavedBagSnapshot& Out)
{
	Out.DisplayName = Bag->DisplayName;
	Out.GridSize = Bag->GridSize;
	Out.CellPixelSize = Bag->CellPixelSize;
	Out.bAllowRotation = Bag->bAllowRotation;
	Out.MinifyScale = Bag->MinifyScale;
	Out.GridLineColor = Bag->GridLineColor;
	Out.OuterLineColor = Bag->OuterLineColor;
	Out.CellBgColor = Bag->CellBgColor;
	Out.GridThickness = Bag->GridThickness;
	Out.bShowCellTooltips = Bag->bShowCellTooltips;
	Out.bShowSortingHeaders = Bag->bShowSortingHeaders;
	Out.bEnableThumbnails = Bag->bEnableThumbnails;
	Out.bEnableHoverHighlight = Bag->bEnableHoverHighlight;
	Out.bUseTagFilter = Bag->bUseTagFilter;
	Out.TagFilters = Bag->TagFilters;
	Out.bUseFolderFilter = Bag->bUseFolderFilter;
	Out.FolderFilters = Bag->FolderFilters;
	Out.bAutoMergeOnAdd = Bag->bAutoMergeOnAdd;
	Out.Items = Bag->Items;
}

static UYIInventoryBag* SnapshotToBag(UObject* Outer, const FYISavedBagSnapshot& Snap)
{
	UYIInventoryBag* NewBag = NewObject<UYIInventoryBag>(Outer);
	if (!NewBag) return nullptr;
	NewBag->DisplayName = Snap.DisplayName;
	NewBag->GridSize = Snap.GridSize;
	NewBag->CellPixelSize = Snap.CellPixelSize;
	NewBag->bAllowRotation = Snap.bAllowRotation;
	NewBag->MinifyScale = Snap.MinifyScale;
	NewBag->GridLineColor = Snap.GridLineColor;
	NewBag->OuterLineColor = Snap.OuterLineColor;
	NewBag->CellBgColor = Snap.CellBgColor;
	NewBag->GridThickness = Snap.GridThickness;
	NewBag->bShowCellTooltips = Snap.bShowCellTooltips;
	NewBag->bShowSortingHeaders = Snap.bShowSortingHeaders;
	NewBag->bEnableThumbnails = Snap.bEnableThumbnails;
	NewBag->bEnableHoverHighlight = Snap.bEnableHoverHighlight;
	NewBag->bUseTagFilter = Snap.bUseTagFilter;
	NewBag->TagFilters = Snap.TagFilters;
	NewBag->bUseFolderFilter = Snap.bUseFolderFilter;
	NewBag->FolderFilters = Snap.FolderFilters;
	NewBag->bAutoMergeOnAdd = Snap.bAutoMergeOnAdd;
	NewBag->Items = Snap.Items;
	return NewBag;
}

bool UYIPlayerInventoryStateComponent::SaveCurrentPawnInventory(APawn* Pawn)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority || !Pawn)
	{
		EmitSaveDiagnostic(TEXT("SaveCurrentPawnInventory skipped: invalid owner/authority/pawn."), FColor::Yellow);
		return false;
	}
	UYIInventoryComponent* InvComp = Pawn->FindComponentByClass<UYIInventoryComponent>();
	if (!InvComp || !InvComp->EquippedBag)
	{
		EmitSaveDiagnostic(FString::Printf(TEXT("SaveCurrentPawnInventory skipped: pawn '%s' has no inventory component or equipped bag."), *Pawn->GetName()), FColor::Yellow);
		return false;
	}
	FYISavedBagSnapshot Snap;
	CopyBagToSnapshot(InvComp->EquippedBag, Snap);
	const int32 Index = ObservedPartyIndex;
	SavedBags.SetNum(FMath::Max(SavedBags.Num(), Index + 1));
	SavedBags[Index] = Snap;
	return true;
}

bool UYIPlayerInventoryStateComponent::RestoreInventoryToPawn(APawn* Pawn)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority || !Pawn)
	{
		EmitSaveDiagnostic(TEXT("RestoreInventoryToPawn skipped: invalid owner/authority/pawn."), FColor::Yellow);
		return false;
	}
	const int32 Index = ObservedPartyIndex;
	if (!SavedBags.IsValidIndex(Index))
	{
		EmitSaveDiagnostic(FString::Printf(TEXT("RestoreInventoryToPawn skipped: no saved snapshot for party index %d."), Index), FColor::Yellow);
		return false;
	}

	UYIInventoryComponent* InvComp = Pawn->FindComponentByClass<UYIInventoryComponent>();
	if (!InvComp)
	{
		EmitSaveDiagnostic(FString::Printf(TEXT("RestoreInventoryToPawn failed: pawn '%s' is missing UYIInventoryComponent."), *Pawn->GetName()), FColor::Red, true);
		return false;
	}

	// Build a runtime bag from snapshot; outer to pawn to ensure transient runtime use
	UYIInventoryBag* RuntimeBag = SnapshotToBag(Pawn, SavedBags[Index]);
	if (!RuntimeBag)
	{
		EmitSaveDiagnostic(FString::Printf(TEXT("RestoreInventoryToPawn failed: could not build runtime bag for pawn '%s'."), *Pawn->GetName()), FColor::Red, true);
		return false;
	}

	InvComp->EquippedBag = RuntimeBag;
	InvComp->OpenBag(RuntimeBag);
	if (Pawn->HasAuthority())
	{
		InvComp->SyncNetState();
	}
	return true;
}

void UYIPlayerInventoryStateComponent::BindAutoSave(APawn* Pawn)
{
	UnbindAutoSave();

	if (!Pawn || Pawn->GetLocalRole() != ROLE_Authority)
	{
		EmitSaveDiagnostic(TEXT("BindAutoSave skipped: invalid pawn or non-authority pawn."), FColor::Yellow);
		return;
	}

	if (UYIInventoryComponent* Inv = Pawn->FindComponentByClass<UYIInventoryComponent>())
	{
		UYIInventoryBag* Bag = Inv->GetBag();
		if (!Bag)
		{
			Bag = Inv->EquippedBag;
		}
		if (Bag && !Inv->EquippedBag)
		{
			Inv->OpenBag(Bag); // normalize runtime state so save tracking is always attached.
		}

		if (Bag)
		{
			Bag->OnChanged.AddUObject(this, &UYIPlayerInventoryStateComponent::HandleBagChanged);
			ObservedBag = Bag;
			ObservedPawn = Pawn;
			bReportedMissingInventoryComp = false;
			bReportedMissingBag = false;
			EmitSaveDiagnostic(FString::Printf(TEXT("Autosave bound to pawn '%s' bag."), *Pawn->GetName()), FColor::Green);
		}
		else if (!bReportedMissingBag)
		{
			bReportedMissingBag = true;
			EmitSaveDiagnostic(FString::Printf(TEXT("Autosave not bound: pawn '%s' inventory has no bag."), *Pawn->GetName()), FColor::Orange, true);
		}
	}
	else if (!bReportedMissingInventoryComp)
	{
		bReportedMissingInventoryComp = true;
		EmitSaveDiagnostic(FString::Printf(TEXT("Autosave not bound: pawn '%s' is missing UYIInventoryComponent."), *Pawn->GetName()), FColor::Orange, true);
	}
}

void UYIPlayerInventoryStateComponent::UnbindAutoSave()
{
	if (ObservedBag.IsValid())
	{
		ObservedBag->OnChanged.RemoveAll(this);
	}
	ObservedBag.Reset();
	ObservedPawn.Reset();
}

void UYIPlayerInventoryStateComponent::HandleBagChanged()
{
	if (!bEnableAutoSave || !GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}
	// Debounce to avoid saving mid-drag (AddBag removes then re-adds)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(DebounceHandle, this, &UYIPlayerInventoryStateComponent::DebouncedSave, AutoSaveDebounceSeconds, false);
	}
}

void UYIPlayerInventoryStateComponent::DebouncedSave()
{
	if (!ObservedPawn.IsValid())
	{
		return;
	}
	SaveCurrentPawnInventory(ObservedPawn.Get());
	SaveToDisk();
}

void UYIPlayerInventoryStateComponent::SaveToDisk()
{
	if (!bEnableAutoSave || !GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		EmitSaveDiagnostic(TEXT("SaveToDisk skipped: autosave disabled or non-authority instance."), FColor::Yellow);
		return;
	}
	UYIInventoryPersistenceProviderBase* Provider = GetOrCreatePersistenceProvider();
	if (!Provider)
	{
		const FString ProviderMissingMessage = TEXT("SaveToDisk failed: no persistence provider configured.");
		EmitSaveDiagnostic(ProviderMissingMessage, FColor::Red, true);
		OnSaveFinished.Broadcast(false, ProviderMissingMessage);
		return;
	}

	if (bSaveInProgress)
	{
		bSaveQueued = true;
		EmitSaveDiagnostic(TEXT("Save queued: previous async save is still running."), FColor::Yellow);
		return;
	}

	UYIInventorySaveGame* Save = NewObject<UYIInventorySaveGame>(this, UYIInventorySaveGame::StaticClass());
	if (!Save)
	{
		EmitSaveDiagnostic(TEXT("SaveToDisk failed: could not create savegame object."), FColor::Red, true);
		OnSaveFinished.Broadcast(false, TEXT("Could not create savegame object."));
		return;
	}

	Save->SavedBags = SavedBags;
	Save->SavedResources = Resources;
	Save->SavedObservedPartyIndex = ObservedPartyIndex;
	Save->SavedEquippedItems.Reset();
	Save->SavedActionBindings.Reset();
	Save->SavedActionInvocationLog.Reset();

	APawn* RuntimePawn = ObservedPawn.Get();
	if (!RuntimePawn)
	{
		if (APlayerState* PS = Cast<APlayerState>(GetOwner()))
		{
			RuntimePawn = PS->GetPawn();
		}
	}
	if (RuntimePawn)
	{
		CapturePawnRuntimeState(RuntimePawn, Save->SavedEquippedItems, Save->SavedActionBindings, Save->SavedActionInvocationLog);
	}

	Save->SavedSharedBags.Reset(SharedBags.Num());
	for (const TSoftObjectPtr<UYIInventoryBag>& SharedBag : SharedBags)
	{
		FYIPersistedSharedBagEntry PersistedBag;
		PersistedBag.BagAsset = SharedBag;

		UYIInventoryBag* Bag = SharedBag.IsValid() ? SharedBag.Get() : SharedBag.LoadSynchronous();
		if (Bag)
		{
			CopyBagToSnapshot(Bag, PersistedBag.Snapshot);
			PersistedBag.bHasSnapshot = true;
		}

		Save->SavedSharedBags.Add(MoveTemp(PersistedBag));
	}

	Save->SavedPartyMembers.Reset(PartyMembers.Num());
	for (const FYIPartyMemberEntry& Member : PartyMembers)
	{
		FYIPersistedPartyMemberEntry PersistedMember;
		PersistedMember.Member = Member;

		UYIInventoryBag* MemberBag = Member.InventoryBag.IsValid() ? Member.InventoryBag.Get() : Member.InventoryBag.LoadSynchronous();
		if (MemberBag)
		{
			CopyBagToSnapshot(MemberBag, PersistedMember.InventorySnapshot);
			PersistedMember.bHasInventorySnapshot = true;
		}

		Save->SavedPartyMembers.Add(MoveTemp(PersistedMember));
	}

	bSaveInProgress = true;
	bSaveQueued = false;
	const FString EffectiveSlotName = BuildEffectiveSaveSlotName();
	const FString StartMessage = FString::Printf(TEXT("Saving inventory state... Slot='%s' User=%d"), *EffectiveSlotName, SaveUserIndex);
	EmitSaveDiagnostic(StartMessage, FColor::Cyan, true);
	OnSaveStarted.Broadcast(StartMessage);

	const TWeakObjectPtr<UYIPlayerInventoryStateComponent> WeakThis(this);
	Provider->SaveAsync(this, Save, EffectiveSlotName, SaveUserIndex, [WeakThis, EffectiveSlotName](bool bSuccess)
	{
		UYIPlayerInventoryStateComponent* Self = WeakThis.Get();
		if (!Self)
		{
			return;
		}

		Self->bSaveInProgress = false;
		const FString ResultMessage = bSuccess
			? FString::Printf(TEXT("Save succeeded. Slot='%s' User=%d"), *EffectiveSlotName, Self->SaveUserIndex)
			: FString::Printf(TEXT("Save failed. Slot='%s' User=%d"), *EffectiveSlotName, Self->SaveUserIndex);
		Self->EmitSaveDiagnostic(ResultMessage, bSuccess ? FColor::Green : FColor::Red, true);
		Self->OnSaveFinished.Broadcast(bSuccess, ResultMessage);

		if (Self->bSaveQueued)
		{
			Self->bSaveQueued = false;
			Self->SaveToDisk();
		}
	});
}

void UYIPlayerInventoryStateComponent::LoadFromDisk()
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		EmitSaveDiagnostic(TEXT("LoadFromDisk skipped: non-authority instance."), FColor::Yellow);
		OnLoadFinished.Broadcast(false, TEXT("Skipped load on non-authority instance."));
		return;
	}
	UYIInventoryPersistenceProviderBase* Provider = GetOrCreatePersistenceProvider();
	if (!Provider)
	{
		const FString ProviderMissingMessage = TEXT("Load failed: no persistence provider configured.");
		EmitSaveDiagnostic(ProviderMissingMessage, FColor::Red, true);
		OnLoadFinished.Broadcast(false, ProviderMissingMessage);
		return;
	}

	const FString EffectiveSlotName = BuildEffectiveSaveSlotName();
	const FString LoadStartMessage = FString::Printf(TEXT("Loading inventory state... Slot='%s' User=%d"), *EffectiveSlotName, SaveUserIndex);
	EmitSaveDiagnostic(LoadStartMessage, FColor::Cyan, true);
	OnLoadStarted.Broadcast(LoadStartMessage);
	if (!Provider->DoesSaveExist(this, EffectiveSlotName, SaveUserIndex))
	{
		const FString MissingMessage = FString::Printf(TEXT("Load skipped: no save file found for Slot='%s' User=%d"), *EffectiveSlotName, SaveUserIndex);
		EmitSaveDiagnostic(MissingMessage, FColor::Yellow, true);
		OnLoadFinished.Broadcast(false, MissingMessage);
		return;
	}

	if (UYIInventorySaveGame* Save = Provider->Load(this, EffectiveSlotName, SaveUserIndex))
	{
		SavedBags = Save->SavedBags;
		Resources = Save->SavedResources;
		ObservedPartyIndex = FMath::Max(0, Save->SavedObservedPartyIndex);
		LoadedEquippedItems = Save->SavedEquippedItems;
		LoadedActionBindings = Save->SavedActionBindings;
		LoadedActionInvocationLog = Save->SavedActionInvocationLog;
		bHasLoadedEquipmentState = true;
		bHasLoadedActionBindings = true;
		bHasLoadedInvocationLog = true;

		SharedBags.Reset();
		SharedBags.Reserve(Save->SavedSharedBags.Num());
		for (const FYIPersistedSharedBagEntry& PersistedBag : Save->SavedSharedBags)
		{
			if (PersistedBag.BagAsset.ToSoftObjectPath().IsValid())
			{
				SharedBags.Add(PersistedBag.BagAsset);
			}
			else if (PersistedBag.bHasSnapshot)
			{
				// Fallback when a shared bag only existed as runtime data.
				if (UYIInventoryBag* RuntimeBag = SnapshotToBag(this, PersistedBag.Snapshot))
				{
					SharedBags.Add(TSoftObjectPtr<UYIInventoryBag>(RuntimeBag));
				}
			}
		}

		PartyMembers.Reset();
		PartyMembers.Reserve(Save->SavedPartyMembers.Num());
		for (int32 Index = 0; Index < Save->SavedPartyMembers.Num(); ++Index)
		{
			const FYIPersistedPartyMemberEntry& PersistedMember = Save->SavedPartyMembers[Index];
			FYIPartyMemberEntry Member = PersistedMember.Member;
			PartyMembers.Add(MoveTemp(Member));

			if (PersistedMember.bHasInventorySnapshot)
			{
				SavedBags.SetNum(FMath::Max(SavedBags.Num(), Index + 1));
				SavedBags[Index] = PersistedMember.InventorySnapshot;
			}
		}

		if (PartyMembers.Num() > 0)
		{
			ObservedPartyIndex = FMath::Clamp(ObservedPartyIndex, 0, PartyMembers.Num() - 1);
		}
		else
		{
			ObservedPartyIndex = 0;
		}

		const FString LoadedMessage = FString::Printf(
			TEXT("Load succeeded. PartyMembers=%d SharedBags=%d SavedSnapshots=%d"),
			PartyMembers.Num(),
			SharedBags.Num(),
			SavedBags.Num());
		EmitSaveDiagnostic(LoadedMessage, FColor::Green, true);
		OnLoadFinished.Broadcast(true, LoadedMessage);
		return;
	}

	const FString FailedMessage = FString::Printf(TEXT("Load failed: could not deserialize save for Slot='%s' User=%d"), *EffectiveSlotName, SaveUserIndex);
	EmitSaveDiagnostic(FailedMessage, FColor::Red, true);
	OnLoadFinished.Broadcast(false, FailedMessage);
}

FString UYIPlayerInventoryStateComponent::BuildEffectiveSaveSlotName() const
{
	const FString BaseSlotName = SaveSlotName.IsEmpty() ? TEXT("YOLOInventory_Autosave") : SaveSlotName;
	if (!bUsePerPlayerSaveSlot)
	{
		return BaseSlotName;
	}

	FString PlayerIdentity;
	if (const APlayerState* PlayerState = Cast<APlayerState>(GetOwner()))
	{
		const FUniqueNetIdRepl& UniqueId = PlayerState->GetUniqueId();
		if (UniqueId.IsValid())
		{
			PlayerIdentity = UniqueId->ToString();
		}

		if (PlayerIdentity.IsEmpty())
		{
			PlayerIdentity = PlayerState->GetPlayerName();
		}

		if (PlayerIdentity.IsEmpty())
		{
			PlayerIdentity = PlayerState->GetFName().ToString();
		}
	}

	if (PlayerIdentity.IsEmpty())
	{
		if (const UObject* OwnerObj = GetOwner())
		{
			PlayerIdentity = OwnerObj->GetFName().ToString();
		}
	}

	if (PlayerIdentity.IsEmpty())
	{
		PlayerIdentity = TEXT("Player");
	}

	for (TCHAR& Ch : PlayerIdentity)
	{
		if (!(FChar::IsAlnum(Ch) || Ch == TEXT('_') || Ch == TEXT('-')))
		{
			Ch = TEXT('_');
		}
	}

	return FString::Printf(TEXT("%s_%s"), *BaseSlotName, *PlayerIdentity);
}

void UYIPlayerInventoryStateComponent::TryAutoRegisterPawn()
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	APawn* Pawn = nullptr;
	if (APlayerState* PS = Cast<APlayerState>(GetOwner()))
	{
		Pawn = PS->GetPawn();
		if (!Pawn)
		{
			if (AController* PC = Cast<AController>(PS->GetOwner()))
			{
				Pawn = PC->GetPawn();
			}
		}
	}

	if (!Pawn)
	{
		if (!bReportedNoPawnYet)
		{
			bReportedNoPawnYet = true;
			EmitSaveDiagnostic(TEXT("Waiting for pawn possession before inventory autosave can bind."), FColor::Yellow, true);
		}
		return;
	}
	bReportedNoPawnYet = false;

	if (Pawn && Pawn != ObservedPawn.Get())
	{
		BindAutoSave(Pawn);
		// If we have saved data, restore into the pawn on first bind.
		if (SavedBags.Num() > 0)
		{
			RestoreInventoryToPawn(Pawn);
		}
		ApplySavedRuntimeStateToPawn(Pawn);
		TArray<FString> BlockingIssues;
		TArray<FString> Warnings;
		const bool bPreflightOk = RunRuntimePreflight(Pawn, BlockingIssues, Warnings);
		for (const FString& Issue : BlockingIssues)
		{
			EmitSaveDiagnostic(FString::Printf(TEXT("Runtime Preflight BLOCK: %s"), *Issue), FColor::Red, true);
		}
		for (const FString& Warning : Warnings)
		{
			EmitSaveDiagnostic(FString::Printf(TEXT("Runtime Preflight WARN: %s"), *Warning), FColor::Yellow, true);
		}
		if (bPreflightOk)
		{
			EmitSaveDiagnostic(TEXT("Runtime preflight passed."), FColor::Green);
		}
	}
	else if (Pawn)
	{
		ApplySavedRuntimeStateToPawn(Pawn);
	}
}

void UYIPlayerInventoryStateComponent::CapturePawnRuntimeState(
	APawn* Pawn,
	TArray<FYIEquippedItemEntry>& OutEquippedItems,
	TArray<FYIActionBarBinding>& OutActionBindings,
	TArray<FYIActionInvocationRecord>& OutInvocationLog) const
{
	OutEquippedItems.Reset();
	OutActionBindings.Reset();
	OutInvocationLog.Reset();

	if (!Pawn)
	{
		return;
	}

	if (UYIEquipmentComponent* EquipmentComp = Pawn->FindComponentByClass<UYIEquipmentComponent>())
	{
		EquipmentComp->GetPersistedEquipment(OutEquippedItems);
	}

	if (UYIActionBarComponent* ActionBarComp = Pawn->FindComponentByClass<UYIActionBarComponent>())
	{
		ActionBarComp->GetPersistedBindings(OutActionBindings);
		ActionBarComp->GetInvocationLog(OutInvocationLog);
	}
}

void UYIPlayerInventoryStateComponent::ApplySavedRuntimeStateToPawn(APawn* Pawn)
{
	if (!Pawn || Pawn->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (bHasLoadedEquipmentState)
	{
		if (UYIEquipmentComponent* EquipmentComp = Pawn->FindComponentByClass<UYIEquipmentComponent>())
		{
			EquipmentComp->LoadPersistedEquipment(LoadedEquippedItems);
			bHasLoadedEquipmentState = false;
		}
		else
		{
			EmitSaveDiagnostic(FString::Printf(TEXT("Loaded equipment state pending: pawn '%s' has no UYIEquipmentComponent."), *Pawn->GetName()), FColor::Yellow);
		}
	}

	if (bHasLoadedActionBindings || bHasLoadedInvocationLog)
	{
		if (UYIActionBarComponent* ActionBarComp = Pawn->FindComponentByClass<UYIActionBarComponent>())
		{
			if (bHasLoadedActionBindings)
			{
				ActionBarComp->LoadPersistedBindings(LoadedActionBindings);
				bHasLoadedActionBindings = false;
			}
			if (bHasLoadedInvocationLog)
			{
				ActionBarComp->LoadInvocationLog(LoadedActionInvocationLog);
				bHasLoadedInvocationLog = false;
			}
		}
		else
		{
			EmitSaveDiagnostic(FString::Printf(TEXT("Loaded action bar state pending: pawn '%s' has no UYIActionBarComponent."), *Pawn->GetName()), FColor::Yellow);
		}
	}
}
