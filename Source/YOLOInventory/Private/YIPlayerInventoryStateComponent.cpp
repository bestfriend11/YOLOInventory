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
	const int32 Index = SharedBags.Add(Bag);
	return Index;
}

int32 UYIPlayerInventoryStateComponent::AddPartyMember(const FYIPartyMemberEntry& Entry)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return INDEX_NONE;
	}
	return PartyMembers.Add(Entry);
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
		SaveToDisk();
	}
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
}

bool UYIPlayerInventoryStateComponent::ConsumeResource(FName ResourceName, int64 Amount)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority || ResourceName.IsNone())
	{
		return false;
	}
	return Resources.Consume(ResourceName, Amount);
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

	UYIInventorySaveGame* Save = Cast<UYIInventorySaveGame>(UGameplayStatics::CreateSaveGameObject(UYIInventorySaveGame::StaticClass()));
	if (!Save) return;

	Save->SavedBags = SavedBags;
	Save->SavedResources = Resources;

	if (bSaveInProgress)
	{
		return; // drop this request; next OnChanged will requeue
	}
	bSaveInProgress = true;

	FAsyncSaveGameToSlotDelegate Finished;
	Finished.BindLambda([this](const FString&, const int32, bool)
	{
		bSaveInProgress = false;
	});
	UGameplayStatics::AsyncSaveGameToSlot(Save, SaveSlotName, SaveUserIndex, Finished);
}

void UYIPlayerInventoryStateComponent::LoadFromDisk()
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
	{
		return;
	}

	if (USaveGame* Raw = UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex))
	{
		if (UYIInventorySaveGame* Save = Cast<UYIInventorySaveGame>(Raw))
		{
			SavedBags = Save->SavedBags;
			Resources = Save->SavedResources;
		}
	}
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
