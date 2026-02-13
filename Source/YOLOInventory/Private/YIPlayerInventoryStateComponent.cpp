#include "YIPlayerInventoryStateComponent.h"

#include "Net/UnrealNetwork.h"
#include "YIInventoryComponent.h"
#include "YIInventoryBag.h"
#include "YIInventorySaveGame.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"

UYIPlayerInventoryStateComponent::UYIPlayerInventoryStateComponent()
{
	SetIsReplicatedByDefault(true);
}

void UYIPlayerInventoryStateComponent::BeginPlay()
{
	Super::BeginPlay();

	// Only the server should drive persistence + bag rebinding.
	if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_Authority && bEnableAutoSave)
	{
		LoadFromDisk();
		// Poll until a pawn exists, then bind to its bag changes.
		TryAutoRegisterPawn();
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(AutoSavePollHandle, this, &UYIPlayerInventoryStateComponent::TryAutoRegisterPawn, 1.0f, true);
		}
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
		return false;
	}

	UYIInventoryComponent* InvComp = Pawn->FindComponentByClass<UYIInventoryComponent>();
	if (!InvComp)
	{
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
		return false;
	}
	UYIInventoryComponent* InvComp = Pawn->FindComponentByClass<UYIInventoryComponent>();
	if (!InvComp || !InvComp->EquippedBag)
	{
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
		return false;
	}
	const int32 Index = ObservedPartyIndex;
	if (!SavedBags.IsValidIndex(Index))
	{
		return false;
	}

	UYIInventoryComponent* InvComp = Pawn->FindComponentByClass<UYIInventoryComponent>();
	if (!InvComp)
	{
		return false;
	}

	// Build a runtime bag from snapshot; outer to pawn to ensure transient runtime use
	UYIInventoryBag* RuntimeBag = SnapshotToBag(Pawn, SavedBags[Index]);
	if (!RuntimeBag)
	{
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
		return;
	}

	if (UYIInventoryComponent* Inv = Pawn->FindComponentByClass<UYIInventoryComponent>())
	{
		if (UYIInventoryBag* Bag = Inv->GetBag())
		{
			Bag->OnChanged.AddUObject(this, &UYIPlayerInventoryStateComponent::HandleBagChanged);
			ObservedBag = Bag;
			ObservedPawn = Pawn;
		}
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
		return;
	}

	if (bSaveInProgress)
	{
		bSaveQueued = true;
		return;
	}

	UYIInventorySaveGame* Save = Cast<UYIInventorySaveGame>(UGameplayStatics::CreateSaveGameObject(UYIInventorySaveGame::StaticClass()));
	if (!Save) return;

	Save->SavedBags = SavedBags;
	Save->SavedResources = Resources;
	Save->SavedObservedPartyIndex = ObservedPartyIndex;

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

	const TWeakObjectPtr<UYIPlayerInventoryStateComponent> WeakThis(this);
	FAsyncSaveGameToSlotDelegate Finished;
	Finished.BindLambda([WeakThis](const FString&, const int32, bool)
	{
		UYIPlayerInventoryStateComponent* Self = WeakThis.Get();
		if (!Self)
		{
			return;
		}

		Self->bSaveInProgress = false;
		if (Self->bSaveQueued)
		{
			Self->bSaveQueued = false;
			Self->SaveToDisk();
		}
	});
	UGameplayStatics::AsyncSaveGameToSlot(Save, BuildEffectiveSaveSlotName(), SaveUserIndex, Finished);
}

void UYIPlayerInventoryStateComponent::LoadFromDisk()
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	const FString EffectiveSlotName = BuildEffectiveSaveSlotName();
	if (!UGameplayStatics::DoesSaveGameExist(EffectiveSlotName, SaveUserIndex))
	{
		return;
	}

	if (USaveGame* Raw = UGameplayStatics::LoadGameFromSlot(EffectiveSlotName, SaveUserIndex))
	{
		if (UYIInventorySaveGame* Save = Cast<UYIInventorySaveGame>(Raw))
		{
			SavedBags = Save->SavedBags;
			Resources = Save->SavedResources;
			ObservedPartyIndex = FMath::Max(0, Save->SavedObservedPartyIndex);

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
		}
	}
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

	if (Pawn && Pawn != ObservedPawn.Get())
	{
		BindAutoSave(Pawn);
		// If we have saved data, restore into the pawn on first bind.
		if (SavedBags.Num() > 0)
		{
			RestoreInventoryToPawn(Pawn);
		}
	}
}
