#include "YIPlayerInventoryStateComponent.h"

#include "Net/UnrealNetwork.h"
#include "YIInventoryComponent.h"
#include "YIInventoryBag.h"
#include "GameFramework/Pawn.h"
#include "UObject/Package.h"

UYIPlayerInventoryStateComponent::UYIPlayerInventoryStateComponent()
{
	SetIsReplicatedByDefault(true);
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
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority || !Pawn || !PartyMembers.IsValidIndex(PartyIndex))
	{
		return false;
	}

	UYIInventoryComponent* InvComp = Pawn->FindComponentByClass<UYIInventoryComponent>();
	if (!InvComp)
	{
		return false;
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
	SavedBags.SetNum(1);
	SavedBags[0] = Snap;
	return true;
}

bool UYIPlayerInventoryStateComponent::RestoreInventoryToPawn(APawn* Pawn)
{
	if (!GetOwner() || GetOwner()->GetLocalRole() != ROLE_Authority || !Pawn)
	{
		return false;
	}
	if (SavedBags.Num() <= 0)
	{
		return false;
	}

	UYIInventoryComponent* InvComp = Pawn->FindComponentByClass<UYIInventoryComponent>();
	if (!InvComp)
	{
		return false;
	}

	// Build a runtime bag from snapshot; outer to pawn to ensure transient runtime use
	UYIInventoryBag* RuntimeBag = SnapshotToBag(Pawn, SavedBags[0]);
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
