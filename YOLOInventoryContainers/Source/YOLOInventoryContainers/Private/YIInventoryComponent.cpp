#include "YIInventoryComponent.h"
#include "YIInventoryBagContextService.h"
#include "YIInventoryBootstrapService.h"
#include "YIInventoryContainerRuntimeService.h"
#include "YIInventoryEventBridgeService.h"
#include "YIInventoryIdentityLockService.h"
#include "YIInventoryItemIngressService.h"
#include "YIInventoryMirrorService.h"
#include "YIInventoryMutationService.h"
#include "YIInventoryBag.h"
#include "Net/UnrealNetwork.h"
#include "UObject/Package.h"
#include "YIItemNetTypes.h"

namespace
{
	static FYIInventoryOpResult YIInventory_MakeOpResultRejected(EYIInventoryOpError Error, const FYIInventoryItemRef* ItemRef = nullptr)
	{
		FYIInventoryOpResult Result;
		Result.bRequestAccepted = false;
		Result.bSucceeded = false;
		Result.Error = Error;
		if (ItemRef)
		{
			Result.AffectedBagId = ItemRef->Bag.BagId;
			Result.AffectedItemInstanceId = ItemRef->Item.ItemInstanceId;
		}
		return Result;
	}

	static void YIInventory_PopulateResultIds(FYIInventoryOpResult& Result, const FYIInventoryItemRef& ItemRef)
	{
		Result.AffectedBagId = ItemRef.Bag.BagId;
		Result.AffectedItemInstanceId = ItemRef.Item.ItemInstanceId;
	}
}

UYIInventoryComponent::UYIInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

UYIInventoryBag* UYIInventoryComponent::CreateBag(FName BagName, FIntPoint GridSize)
{
	return FYIInventoryBagContextService::CreateBag(*this, BagName, GridSize);
}

void UYIInventoryComponent::OpenBag(UYIInventoryBag* Bag)
{
	FYIInventoryBagContextService::OpenBag(*this, Bag);
}

void UYIInventoryComponent::CloseBag(UYIInventoryBag* Bag)
{
	FYIInventoryBagContextService::CloseBag(*this, Bag);
}

UYIInventoryBag* UYIInventoryComponent::GetBag() const
{
	return FYIInventoryBagContextService::GetBag(*this);
}

FGuid UYIInventoryComponent::GetActiveContextBagId(FGameplayTag ContextTag) const
{
	return FYIInventoryBagContextService::GetActiveContextBagId(*this, ContextTag);
}

UYIInventoryBag* UYIInventoryComponent::GetActiveContextBag(FGameplayTag ContextTag) const
{
	return FYIInventoryBagContextService::GetActiveContextBag(*this, ContextTag);
}

UYIInventoryBag* UYIInventoryComponent::GetBagById(const FGuid& BagId) const
{
	return FYIInventoryBagContextService::GetBagById(*this, BagId);
}

UYIInventoryBag* UYIInventoryComponent::FindClientContextPreviewBagById(const FGuid& BagId) const
{
	return FYIInventoryMirrorService::FindClientContextPreviewBagById(*this, BagId);
}

UYIInventoryBag* UYIInventoryComponent::FindOrCreateClientContextPreviewBagById(const FGuid& BagId)
{
	return FYIInventoryMirrorService::FindOrCreateClientContextPreviewBagById(*this, BagId);
}

void UYIInventoryComponent::RebuildClientPreviewBagFromNet(UYIInventoryBag* TargetBag, const TArray<FYINetBagItem>& InItems, const FIntPoint& InGridSize, const FGuid& InBagId)
{
	FYIInventoryMirrorService::RebuildClientPreviewBagFromNet(*this, TargetBag, InItems, InGridSize, InBagId);
}

int32 UYIInventoryComponent::FindActiveContextIndex(FGameplayTag ContextTag) const
{
	return FYIInventoryBagContextService::FindActiveContextIndex(*this, ContextTag);
}

bool UYIInventoryComponent::SetActiveBagById(const FGuid& InBagId)
{
	return FYIInventoryBagContextService::SetActiveBagById(*this, InBagId);
}

bool UYIInventoryComponent::SetActiveBagByRoleTag(FGameplayTag InBagRoleTag)
{
	return FYIInventoryBagContextService::SetActiveBagByRoleTag(*this, InBagRoleTag);
}

bool UYIInventoryComponent::OpenContainedBagAtIndex(int32 ItemIndex)
{
	return FYIInventoryBagContextService::OpenContainedBagAtIndex(*this, ItemIndex);
}

UYIInventoryBag* UYIInventoryComponent::EnsureContainedBagAtIndex(int32 ItemIndex)
{
	return FYIInventoryBagContextService::EnsureContainedBagAtIndex(*this, ItemIndex);
}

bool UYIInventoryComponent::OpenParentBag()
{
	return FYIInventoryBagContextService::OpenParentBag(*this);
}

bool UYIInventoryComponent::SetActiveContextBagById(FGameplayTag ContextTag, const FGuid& InBagId)
{
	return FYIInventoryBagContextService::SetActiveContextBagById(*this, ContextTag, InBagId);
}

bool UYIInventoryComponent::SetActiveContextBagByRoleTag(FGameplayTag ContextTag, FGameplayTag InBagRoleTag)
{
	return FYIInventoryBagContextService::SetActiveContextBagByRoleTag(*this, ContextTag, InBagRoleTag);
}

bool UYIInventoryComponent::ClearActiveContextBag(FGameplayTag ContextTag)
{
	return FYIInventoryBagContextService::ClearActiveContextBag(*this, ContextTag);
}

int32 UYIInventoryComponent::ClearActiveContextsForBagId(const FGuid& InBagId)
{
	return FYIInventoryBagContextService::ClearActiveContextsForBagId(*this, InBagId);
}

void UYIInventoryComponent::ServerSetActiveBagById_Implementation(const FGuid& InBagId)
{
	SetActiveBagById(InBagId);
}

void UYIInventoryComponent::ServerSetActiveContextBagById_Implementation(FGameplayTag ContextTag, const FGuid& InBagId)
{
	SetActiveContextBagById(ContextTag, InBagId);
}

void UYIInventoryComponent::ServerClearActiveContextBag_Implementation(FGameplayTag ContextTag)
{
	ClearActiveContextBag(ContextTag);
}

void UYIInventoryComponent::ServerOpenContainedBagByInstance_Implementation(const FGuid& ParentBagId, const FGuid& ParentItemInstanceId)
{
	if (!ParentBagId.IsValid() || !ParentItemInstanceId.IsValid())
	{
		return;
	}

	UYIInventoryBag* ParentBag = GetBagById(ParentBagId);
	if (!ParentBag)
	{
		return;
	}

	int32 ItemIndex = INDEX_NONE;
	if (!FindItemIndexByInstanceId(ParentBag, ParentItemInstanceId, ItemIndex))
	{
		return;
	}

	TryOpenContainedBagInternal(ParentBag, ItemIndex);
}

void UYIInventoryComponent::ServerOpenParentBag_Implementation(const FGuid& ChildBagId)
{
	FGuid ParentBagId;
	FGuid ParentItemId;
	if (!FindContainerParentForBag(ChildBagId, ParentBagId, ParentItemId))
	{
		return;
	}
	SetActiveBagById(ParentBagId);
}

UYIInventoryBag* UYIInventoryComponent::GetBagByRoleTag(FGameplayTag BagRoleTag) const
{
	return FYIInventoryBagContextService::GetBagByRoleTag(*this, BagRoleTag);
}

UYIInventoryBag* UYIInventoryComponent::GetBagByDisplayName(FName BagName) const
{
	return FYIInventoryBagContextService::GetBagByDisplayName(*this, BagName);
}

int32 UYIInventoryComponent::GetBagRuntimeRevisionById(const FGuid& BagId) const
{
	if (UYIInventoryBag* Bag = GetBagById(BagId))
	{
		return Bag->RuntimeRevision;
	}
	return INDEX_NONE;
}

bool UYIInventoryComponent::GetBagItemIdentity(const UYIInventoryBag* Bag, int32 ItemIndex, FYIInventoryItemRef& OutIdentity) const
{
	return FYIInventoryIdentityLockService::GetBagItemIdentity(*this, Bag, ItemIndex, OutIdentity);
}

bool UYIInventoryComponent::IsBagItemLockedByIdentity(const FYIInventoryItemRef& Identity) const
{
	return FYIInventoryIdentityLockService::IsBagItemLockedByIdentity(*this, Identity);
}

bool UYIInventoryComponent::SetBagItemLocked(UYIInventoryBag* Bag, int32 ItemIndex, bool bLocked)
{
	return FYIInventoryIdentityLockService::SetBagItemLocked(*this, Bag, ItemIndex, bLocked);
}

bool UYIInventoryComponent::SetBagItemLockedByCoreRef(const FYIInventoryItemRef& ItemRef, bool bLocked)
{
	return FYIInventoryIdentityLockService::SetBagItemLockedByCoreRef(*this, ItemRef, bLocked);
}

bool UYIInventoryComponent::GetBagItemCoreRef(UYIInventoryBag* Bag, int32 ItemIndex, FYIInventoryItemRef& OutItemRef) const
{
	return FYIInventoryIdentityLockService::GetBagItemCoreRef(*this, Bag, ItemIndex, OutItemRef);
}

bool UYIInventoryComponent::SetBagItemLockedInternal(const FYIInventoryItemRef& ItemRef, bool bLocked)
{
	return FYIInventoryIdentityLockService::SetBagItemLockedInternal(*this, ItemRef, bLocked);
}

bool UYIInventoryComponent::IsBagItemLocked(UYIInventoryBag* Bag, int32 ItemIndex) const
{
	return FYIInventoryIdentityLockService::IsBagItemLocked(*this, Bag, ItemIndex);
}

bool UYIInventoryComponent::IsBagItemLockedByCoreRef(const FYIInventoryItemRef& ItemRef) const
{
	return FYIInventoryIdentityLockService::IsBagItemLockedByCoreRef(*this, ItemRef);
}

bool UYIInventoryComponent::FindItemIndexByInstanceId(const UYIInventoryBag* Bag, const FGuid& InstanceId, int32& OutIndex) const
{
	return FYIInventoryIdentityLockService::FindItemIndexByInstanceId(*this, Bag, InstanceId, OutIndex);
}

bool UYIInventoryComponent::FindContainerParentForBag(const FGuid& ChildBagId, FGuid& OutParentBagId, FGuid& OutParentItemInstanceId) const
{
	return FYIInventoryIdentityLockService::FindContainerParentForBag(*this, ChildBagId, OutParentBagId, OutParentItemInstanceId);
}

bool UYIInventoryComponent::IsBagDescendantOf(const FGuid& CandidateBagId, const FGuid& PotentialAncestorBagId) const
{
	return FYIInventoryIdentityLockService::IsBagDescendantOf(*this, CandidateBagId, PotentialAncestorBagId);
}

UYIInventoryBag* UYIInventoryComponent::EnsureContainedBagForItem(FYIBagItem& InOutItem, const UYIInventoryBag* ParentBag)
{
	return FYIInventoryContainerRuntimeService::EnsureContainedBagForItem(*this, InOutItem, ParentBag);
}

bool UYIInventoryComponent::TryOpenContainedBagInternal(UYIInventoryBag* ParentBag, int32 ItemIndex)
{
	return FYIInventoryContainerRuntimeService::TryOpenContainedBagInternal(*this, ParentBag, ItemIndex);
}

void UYIInventoryComponent::GetReplicatedBagDescriptors(TArray<FYINetBagDescriptor>& OutDescriptors) const
{
	FYIInventoryBagContextService::GetReplicatedBagDescriptors(*this, OutDescriptors);
}

bool UYIInventoryComponent::RemoveBag(UYIInventoryBag* Bag)
{
	return FYIInventoryBagContextService::RemoveBag(*this, Bag);
}

bool UYIInventoryComponent::AddItemToBag(UYIInventoryBag* Bag, TSoftObjectPtr<UYIItemDefinition> ItemDef, int32 Count)
{
	return FYIInventoryItemIngressService::AddItemToBag(*this, Bag, ItemDef, Count);
}

void UYIInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	FYIInventoryBootstrapService::BeginPlay(*this);
}

UYIInventoryBag* UYIInventoryComponent::CloneBagTemplate(const UYIInventoryBag* TemplateBag)
{
	return FYIInventoryContainerRuntimeService::CloneBagTemplate(*this, TemplateBag);
}

bool UYIInventoryComponent::IsTemplateBag(const UYIInventoryBag* Bag) const
{
	if (!Bag) return false;
	const UPackage* Package = Bag->GetOutermost();
	const bool bIsAssetPkg = Package && !Package->HasAnyPackageFlags(PKG_PlayInEditor | PKG_ContainsMapData);
	const bool bPublic = Bag->HasAnyFlags(RF_Public);
	return bIsAssetPkg || bPublic;
}

void UYIInventoryComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (EquippedBag && BagChangedHandle.IsValid())
	{
		EquippedBag->OnChanged.Remove(BagChangedHandle);
		BagChangedHandle.Reset();
	}

	for (UYIInventoryBag* Bag : Bags)
	{
		if (!Bag)
		{
			continue;
		}

		Bag->OnItemAdded.RemoveDynamic(this, &UYIInventoryComponent::HandleBagItemAdded);
		Bag->OnItemRemoved.RemoveDynamic(this, &UYIInventoryComponent::HandleBagItemRemoved);
		Bag->OnItemMoved.RemoveDynamic(this, &UYIInventoryComponent::HandleBagItemMoved);
		Bag->OnItemRotated.RemoveDynamic(this, &UYIInventoryComponent::HandleBagItemRotated);
		Bag->OnItemTransferred.RemoveDynamic(this, &UYIInventoryComponent::HandleBagItemTransferred);
	}

	if (BagEventSource)
	{
		BagEventSource->OnItemAdded.RemoveDynamic(this, &UYIInventoryComponent::HandleBagItemAdded);
		BagEventSource->OnItemRemoved.RemoveDynamic(this, &UYIInventoryComponent::HandleBagItemRemoved);
		BagEventSource->OnItemMoved.RemoveDynamic(this, &UYIInventoryComponent::HandleBagItemMoved);
		BagEventSource->OnItemRotated.RemoveDynamic(this, &UYIInventoryComponent::HandleBagItemRotated);
		BagEventSource->OnItemTransferred.RemoveDynamic(this, &UYIInventoryComponent::HandleBagItemTransferred);
		BagEventSource = nullptr;
	}
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UYIInventoryComponent::SyncNetState()
{
	FYIInventoryMirrorService::SyncNetState(*this);
}

void UYIInventoryComponent::OnRep_NetBag()
{
	FYIInventoryMirrorService::OnRep_NetBag(*this);
}

void UYIInventoryComponent::OnRep_NetBagDescriptors()
{
	FYIInventoryMirrorService::OnRep_NetBagDescriptors(*this);
}

void UYIInventoryComponent::OnRep_ActiveBagContexts()
{
	FYIInventoryMirrorService::OnRep_ActiveBagContexts(*this);
}

void UYIInventoryComponent::OnRep_NetContextBagMirrors()
{
	FYIInventoryMirrorService::OnRep_NetContextBagMirrors(*this);
}

void UYIInventoryComponent::OnRep_LockedBagItems()
{
	FYIInventoryMirrorService::OnRep_LockedBagItems(*this);
}

void UYIInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, NetBagItems, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, NetBagGridSize, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, NetBagRevision, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, NetBagDescriptors, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, ActiveBagId, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, ActiveBagContexts, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, NetContextBagMirrors, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UYIInventoryComponent, LockedBagItems, COND_OwnerOnly);
}

// -------- Net-safe bag mutations --------

bool UYIInventoryComponent::MoveItem(int32 Index, FIntPoint NewPos)
{
	return FYIInventoryMutationService::MoveItemInActiveBagByIndex(*this, Index, NewPos);
}

void UYIInventoryComponent::ServerMoveItem_Implementation(int32 Index, FIntPoint NewPos)
{
	MoveItem(Index, NewPos); // will take authority branch
}

bool UYIInventoryComponent::MoveItemInBag(const FGuid& BagId, const FGuid& ItemInstanceId, FIntPoint NewPos)
{
	return FYIInventoryMutationService::MoveItemInBag(*this, BagId, ItemInstanceId, NewPos);
}

void UYIInventoryComponent::ServerMoveItemInBag_Implementation(const FGuid& BagId, const FGuid& ItemInstanceId, FIntPoint NewPos)
{
	MoveItemInBag(BagId, ItemInstanceId, NewPos);
}

bool UYIInventoryComponent::MoveItemInBagAtCell(const FGuid& BagId, const FGuid& ItemInstanceId, FIntPoint DestCell, bool bAllowSingleOverlapSwap)
{
	return FYIInventoryMutationService::MoveItemInBagAtCell(*this, BagId, ItemInstanceId, DestCell, bAllowSingleOverlapSwap);
}

void UYIInventoryComponent::ServerMoveItemInBagAtCell_Implementation(const FGuid& BagId, const FGuid& ItemInstanceId, FIntPoint DestCell, bool bAllowSingleOverlapSwap)
{
	MoveItemInBagAtCell(BagId, ItemInstanceId, DestCell, bAllowSingleOverlapSwap);
}

bool UYIInventoryComponent::RotateItem(int32 Index)
{
	return FYIInventoryMutationService::RotateItemInActiveBagByIndex(*this, Index);
}

void UYIInventoryComponent::ServerRotateItem_Implementation(int32 Index)
{
	RotateItem(Index); // authority branch
}

bool UYIInventoryComponent::RotateItemInBag(const FGuid& BagId, const FGuid& ItemInstanceId)
{
	return FYIInventoryMutationService::RotateItemInBag(*this, BagId, ItemInstanceId);
}

void UYIInventoryComponent::ServerRotateItemInBag_Implementation(const FGuid& BagId, const FGuid& ItemInstanceId)
{
	RotateItemInBag(BagId, ItemInstanceId);
}

int32 UYIInventoryComponent::AddBagItem(const FYIBagItem& Item)
{
	return FYIInventoryItemIngressService::AddBagItem(*this, Item);
}

void UYIInventoryComponent::ServerAddBagItem_Implementation(const FYIItemInstanceNet& NetItem, FIntPoint Pos, FIntPoint Size)
{
	FYIInventoryItemIngressService::ServerAddBagItem(*this, NetItem, Pos, Size);
}

bool UYIInventoryComponent::RemoveItem(int32 Index)
{
	return FYIInventoryMutationService::RemoveItemFromActiveBagByIndex(*this, Index);
}

bool UYIInventoryComponent::RemoveItemFromBag(const FGuid& BagId, const FGuid& ItemInstanceId)
{
	return FYIInventoryMutationService::RemoveItemFromBag(*this, BagId, ItemInstanceId);
}

void UYIInventoryComponent::ServerRemoveItem_Implementation(int32 Index)
{
	RemoveItem(Index);
}

void UYIInventoryComponent::ServerRemoveItemFromBag_Implementation(const FGuid& BagId, const FGuid& ItemInstanceId)
{
	RemoveItemFromBag(BagId, ItemInstanceId);
}

bool UYIInventoryComponent::TransferItemBetweenBagsById(const FGuid& SourceBagId, const FGuid& ItemInstanceId, const FGuid& DestBagId, int32 Count, int32& OutDestIndex)
{
	return FYIInventoryMutationService::TransferItemBetweenBagsById(*this, SourceBagId, ItemInstanceId, DestBagId, Count, OutDestIndex);
}

void UYIInventoryComponent::ServerTransferItemBetweenBagsById_Implementation(
	const FGuid& SourceBagId,
	const FGuid& ItemInstanceId,
	const FGuid& DestBagId,
	int32 Count)
{
	int32 IgnoredDestIndex = INDEX_NONE;
	TransferItemBetweenBagsById(SourceBagId, ItemInstanceId, DestBagId, Count, IgnoredDestIndex);
}

bool UYIInventoryComponent::TransferItemBetweenBagsAtCellById(
	const FGuid& SourceBagId,
	const FGuid& ItemInstanceId,
	const FGuid& DestBagId,
	FIntPoint DestCell,
	int32 Count,
	bool bAllowSingleOverlapSwap)
{
	return FYIInventoryMutationService::TransferItemBetweenBagsAtCellById(*this, SourceBagId, ItemInstanceId, DestBagId, DestCell, Count, bAllowSingleOverlapSwap);
}

void UYIInventoryComponent::ServerTransferItemBetweenBagsAtCellById_Implementation(
	const FGuid& SourceBagId,
	const FGuid& ItemInstanceId,
	const FGuid& DestBagId,
	FIntPoint DestCell,
	int32 Count,
	bool bAllowSingleOverlapSwap)
{
	TransferItemBetweenBagsAtCellById(SourceBagId, ItemInstanceId, DestBagId, DestCell, Count, bAllowSingleOverlapSwap);
}

bool UYIInventoryComponent::SwapItemIntoBagCellById(const FGuid& SourceBagId, const FGuid& ItemInstanceId, const FGuid& DestBagId, FIntPoint DestCell)
{
	return TransferItemBetweenBagsAtCellById(SourceBagId, ItemInstanceId, DestBagId, DestCell, 0, true);
}

void UYIInventoryComponent::ServerSwapItemIntoBagCellById_Implementation(
	const FGuid& SourceBagId,
	const FGuid& ItemInstanceId,
	const FGuid& DestBagId,
	FIntPoint DestCell)
{
	SwapItemIntoBagCellById(SourceBagId, ItemInstanceId, DestBagId, DestCell);
}

bool UYIInventoryComponent::CombineItemInBag(const FGuid& BagId, const FGuid& ItemInstanceId)
{
	return FYIInventoryMutationService::CombineItemInBag(*this, BagId, ItemInstanceId);
}

void UYIInventoryComponent::ServerCombineItemInBag_Implementation(const FGuid& BagId, const FGuid& ItemInstanceId)
{
	CombineItemInBag(BagId, ItemInstanceId);
}

bool UYIInventoryComponent::SplitStackInBag(const FGuid& BagId, const FGuid& ItemInstanceId, int32 Amount, FIntPoint DesiredPos)
{
	return FYIInventoryMutationService::SplitStackInBag(*this, BagId, ItemInstanceId, Amount, DesiredPos);
}

void UYIInventoryComponent::ServerSplitStackInBag_Implementation(
	const FGuid& BagId,
	const FGuid& ItemInstanceId,
	int32 Amount,
	FIntPoint DesiredPos)
{
	SplitStackInBag(BagId, ItemInstanceId, Amount, DesiredPos);
}

FYIInventoryOpResult UYIInventoryComponent::RequestMoveItem(const FYIInventoryMoveItemRequest& Request)
{
	FYIInventoryMoveItemRequest EffectiveRequest = Request;
	if (!EffectiveRequest.RequestId.IsValid())
	{
		EffectiveRequest.RequestId = FGuid::NewGuid();
	}

	if (!EffectiveRequest.ItemRef.Bag.BagId.IsValid() || !EffectiveRequest.ItemRef.Item.ItemInstanceId.IsValid())
	{
		FYIInventoryOpResult Result = YIInventory_MakeOpResultRejected(EYIInventoryOpError::InvalidRef, &EffectiveRequest.ItemRef);
		Result.TransactionId = EffectiveRequest.RequestId;
		Result.OpKind = EYIInventoryOpKind::Move;
		return Result;
	}

	FYIInventoryOpResult Result;
	YIInventory_PopulateResultIds(Result, EffectiveRequest.ItemRef);
	Result.TransactionId = EffectiveRequest.RequestId;
	Result.OpKind = EYIInventoryOpKind::Move;
	Result.SourceBagRevision = GetBagRuntimeRevisionById(EffectiveRequest.ItemRef.Bag.BagId);

	if (!(GetOwner() && GetOwner()->HasAuthority()))
	{
		ServerRequestMoveItem(EffectiveRequest);
		Result.bRequestAccepted = true;
		Result.bSucceeded = false;
		Result.Error = EYIInventoryOpError::None;
		return Result;
	}

	if (EffectiveRequest.ExpectedSourceBagRevision != INDEX_NONE &&
		Result.SourceBagRevision != INDEX_NONE &&
		Result.SourceBagRevision != EffectiveRequest.ExpectedSourceBagRevision)
	{
		Result.Error = EYIInventoryOpError::RevisionMismatch;
		return Result;
	}

	const bool bOpResult = EffectiveRequest.bUseExactCell
		? MoveItemInBagAtCell(EffectiveRequest.ItemRef.Bag.BagId, EffectiveRequest.ItemRef.Item.ItemInstanceId, EffectiveRequest.TargetCell, EffectiveRequest.bAllowSingleOverlapSwap)
		: MoveItemInBag(EffectiveRequest.ItemRef.Bag.BagId, EffectiveRequest.ItemRef.Item.ItemInstanceId, EffectiveRequest.TargetCell);

	Result.bRequestAccepted = bOpResult;
	Result.bSucceeded = bOpResult && (!GetOwner() || GetOwner()->HasAuthority());
	Result.Error = bOpResult ? EYIInventoryOpError::None : EYIInventoryOpError::ValidationFailed;
	Result.SourceBagRevision = GetBagRuntimeRevisionById(EffectiveRequest.ItemRef.Bag.BagId);
	return Result;
}

FYIInventoryOpResult UYIInventoryComponent::RequestRotateItem(const FYIInventoryRotateItemRequest& Request)
{
	FYIInventoryRotateItemRequest EffectiveRequest = Request;
	if (!EffectiveRequest.RequestId.IsValid())
	{
		EffectiveRequest.RequestId = FGuid::NewGuid();
	}

	if (!EffectiveRequest.ItemRef.Bag.BagId.IsValid() || !EffectiveRequest.ItemRef.Item.ItemInstanceId.IsValid())
	{
		FYIInventoryOpResult Result = YIInventory_MakeOpResultRejected(EYIInventoryOpError::InvalidRef, &EffectiveRequest.ItemRef);
		Result.TransactionId = EffectiveRequest.RequestId;
		Result.OpKind = EYIInventoryOpKind::Rotate;
		return Result;
	}

	FYIInventoryOpResult Result;
	YIInventory_PopulateResultIds(Result, EffectiveRequest.ItemRef);
	Result.TransactionId = EffectiveRequest.RequestId;
	Result.OpKind = EYIInventoryOpKind::Rotate;
	Result.SourceBagRevision = GetBagRuntimeRevisionById(EffectiveRequest.ItemRef.Bag.BagId);
	if (!(GetOwner() && GetOwner()->HasAuthority()))
	{
		ServerRequestRotateItem(EffectiveRequest);
		Result.bRequestAccepted = true;
		Result.bSucceeded = false;
		Result.Error = EYIInventoryOpError::None;
		return Result;
	}
	if (EffectiveRequest.ExpectedSourceBagRevision != INDEX_NONE &&
		Result.SourceBagRevision != INDEX_NONE &&
		Result.SourceBagRevision != EffectiveRequest.ExpectedSourceBagRevision)
	{
		Result.Error = EYIInventoryOpError::RevisionMismatch;
		return Result;
	}

	const bool bOpResult = RotateItemInBag(EffectiveRequest.ItemRef.Bag.BagId, EffectiveRequest.ItemRef.Item.ItemInstanceId);
	Result.bRequestAccepted = bOpResult;
	Result.bSucceeded = bOpResult && (!GetOwner() || GetOwner()->HasAuthority());
	Result.Error = bOpResult ? EYIInventoryOpError::None : EYIInventoryOpError::ValidationFailed;
	Result.SourceBagRevision = GetBagRuntimeRevisionById(EffectiveRequest.ItemRef.Bag.BagId);
	return Result;
}

FYIInventoryOpResult UYIInventoryComponent::RequestRemoveItem(const FYIInventoryRemoveItemRequest& Request)
{
	FYIInventoryRemoveItemRequest EffectiveRequest = Request;
	if (!EffectiveRequest.RequestId.IsValid())
	{
		EffectiveRequest.RequestId = FGuid::NewGuid();
	}

	if (!EffectiveRequest.ItemRef.Bag.BagId.IsValid() || !EffectiveRequest.ItemRef.Item.ItemInstanceId.IsValid())
	{
		FYIInventoryOpResult Result = YIInventory_MakeOpResultRejected(EYIInventoryOpError::InvalidRef, &EffectiveRequest.ItemRef);
		Result.TransactionId = EffectiveRequest.RequestId;
		Result.OpKind = EYIInventoryOpKind::Remove;
		return Result;
	}

	FYIInventoryOpResult Result;
	YIInventory_PopulateResultIds(Result, EffectiveRequest.ItemRef);
	Result.TransactionId = EffectiveRequest.RequestId;
	Result.OpKind = EYIInventoryOpKind::Remove;
	Result.SourceBagRevision = GetBagRuntimeRevisionById(EffectiveRequest.ItemRef.Bag.BagId);
	if (!(GetOwner() && GetOwner()->HasAuthority()))
	{
		ServerRequestRemoveItem(EffectiveRequest);
		Result.bRequestAccepted = true;
		Result.bSucceeded = false;
		Result.Error = EYIInventoryOpError::None;
		return Result;
	}
	if (EffectiveRequest.ExpectedSourceBagRevision != INDEX_NONE &&
		Result.SourceBagRevision != INDEX_NONE &&
		Result.SourceBagRevision != EffectiveRequest.ExpectedSourceBagRevision)
	{
		Result.Error = EYIInventoryOpError::RevisionMismatch;
		return Result;
	}

	const bool bOpResult = RemoveItemFromBag(EffectiveRequest.ItemRef.Bag.BagId, EffectiveRequest.ItemRef.Item.ItemInstanceId);
	Result.bRequestAccepted = bOpResult;
	Result.bSucceeded = bOpResult && (!GetOwner() || GetOwner()->HasAuthority());
	Result.Error = bOpResult ? EYIInventoryOpError::None : EYIInventoryOpError::ValidationFailed;
	Result.SourceBagRevision = GetBagRuntimeRevisionById(EffectiveRequest.ItemRef.Bag.BagId);
	return Result;
}

FYIInventoryOpResult UYIInventoryComponent::RequestTransferItem(const FYIInventoryTransferItemRequest& Request)
{
	FYIInventoryTransferItemRequest EffectiveRequest = Request;
	if (!EffectiveRequest.RequestId.IsValid())
	{
		EffectiveRequest.RequestId = FGuid::NewGuid();
	}

	if (!EffectiveRequest.ItemRef.Bag.BagId.IsValid() || !EffectiveRequest.ItemRef.Item.ItemInstanceId.IsValid() || !EffectiveRequest.DestBagId.IsValid())
	{
		FYIInventoryOpResult Result = YIInventory_MakeOpResultRejected(EYIInventoryOpError::InvalidRef, &EffectiveRequest.ItemRef);
		Result.TransactionId = EffectiveRequest.RequestId;
		Result.OpKind = EYIInventoryOpKind::Transfer;
		return Result;
	}

	FYIInventoryOpResult Result;
	YIInventory_PopulateResultIds(Result, EffectiveRequest.ItemRef);
	Result.TransactionId = EffectiveRequest.RequestId;
	Result.OpKind = EYIInventoryOpKind::Transfer;
	Result.SourceBagRevision = GetBagRuntimeRevisionById(EffectiveRequest.ItemRef.Bag.BagId);
	Result.DestBagRevision = GetBagRuntimeRevisionById(EffectiveRequest.DestBagId);

	if (!(GetOwner() && GetOwner()->HasAuthority()))
	{
		ServerRequestTransferItem(EffectiveRequest);
		Result.bRequestAccepted = true;
		Result.bSucceeded = false;
		Result.Error = EYIInventoryOpError::None;
		return Result;
	}

	{
		if (EffectiveRequest.ExpectedSourceBagRevision != INDEX_NONE &&
			Result.SourceBagRevision != INDEX_NONE &&
			Result.SourceBagRevision != EffectiveRequest.ExpectedSourceBagRevision)
		{
			Result.Error = EYIInventoryOpError::RevisionMismatch;
			return Result;
		}
		if (EffectiveRequest.ExpectedDestBagRevision != INDEX_NONE &&
			Result.DestBagRevision != INDEX_NONE &&
			Result.DestBagRevision != EffectiveRequest.ExpectedDestBagRevision)
		{
			Result.Error = EYIInventoryOpError::RevisionMismatch;
			return Result;
		}
	}

	bool bOpResult = false;
	if (EffectiveRequest.bUseExactCell)
	{
		bOpResult = TransferItemBetweenBagsAtCellById(
			EffectiveRequest.ItemRef.Bag.BagId,
			EffectiveRequest.ItemRef.Item.ItemInstanceId,
			EffectiveRequest.DestBagId,
			EffectiveRequest.DestCell,
			EffectiveRequest.Count,
			EffectiveRequest.bAllowSingleOverlapSwap);
	}
	else
	{
		int32 IgnoredDestIndex = INDEX_NONE;
		bOpResult = TransferItemBetweenBagsById(
			EffectiveRequest.ItemRef.Bag.BagId,
			EffectiveRequest.ItemRef.Item.ItemInstanceId,
			EffectiveRequest.DestBagId,
			EffectiveRequest.Count,
			IgnoredDestIndex);
	}

	Result.bRequestAccepted = bOpResult;
	Result.bSucceeded = bOpResult && (!GetOwner() || GetOwner()->HasAuthority());
	Result.Error = bOpResult ? EYIInventoryOpError::None : EYIInventoryOpError::ValidationFailed;
	Result.SourceBagRevision = GetBagRuntimeRevisionById(EffectiveRequest.ItemRef.Bag.BagId);
	Result.DestBagRevision = GetBagRuntimeRevisionById(EffectiveRequest.DestBagId);
	return Result;
}

FYIInventoryOpResult UYIInventoryComponent::RequestSplitStack(const FYIInventorySplitStackRequest& Request)
{
	FYIInventorySplitStackRequest EffectiveRequest = Request;
	if (!EffectiveRequest.RequestId.IsValid())
	{
		EffectiveRequest.RequestId = FGuid::NewGuid();
	}

	if (!EffectiveRequest.ItemRef.Bag.BagId.IsValid() || !EffectiveRequest.ItemRef.Item.ItemInstanceId.IsValid() || EffectiveRequest.Amount <= 0)
	{
		FYIInventoryOpResult Result = YIInventory_MakeOpResultRejected(EYIInventoryOpError::InvalidRequest, &EffectiveRequest.ItemRef);
		Result.TransactionId = EffectiveRequest.RequestId;
		Result.OpKind = EYIInventoryOpKind::Split;
		return Result;
	}

	FYIInventoryOpResult Result;
	YIInventory_PopulateResultIds(Result, EffectiveRequest.ItemRef);
	Result.TransactionId = EffectiveRequest.RequestId;
	Result.OpKind = EYIInventoryOpKind::Split;
	Result.SourceBagRevision = GetBagRuntimeRevisionById(EffectiveRequest.ItemRef.Bag.BagId);
	if (!(GetOwner() && GetOwner()->HasAuthority()))
	{
		ServerRequestSplitStack(EffectiveRequest);
		Result.bRequestAccepted = true;
		Result.bSucceeded = false;
		Result.Error = EYIInventoryOpError::None;
		return Result;
	}
	if (EffectiveRequest.ExpectedSourceBagRevision != INDEX_NONE &&
		Result.SourceBagRevision != INDEX_NONE &&
		Result.SourceBagRevision != EffectiveRequest.ExpectedSourceBagRevision)
	{
		Result.Error = EYIInventoryOpError::RevisionMismatch;
		return Result;
	}

	const bool bOpResult = SplitStackInBag(EffectiveRequest.ItemRef.Bag.BagId, EffectiveRequest.ItemRef.Item.ItemInstanceId, EffectiveRequest.Amount, EffectiveRequest.DesiredPos);
	Result.bRequestAccepted = bOpResult;
	Result.bSucceeded = bOpResult && (!GetOwner() || GetOwner()->HasAuthority());
	Result.Error = bOpResult ? EYIInventoryOpError::None : EYIInventoryOpError::ValidationFailed;
	Result.SourceBagRevision = GetBagRuntimeRevisionById(EffectiveRequest.ItemRef.Bag.BagId);
	return Result;
}

FYIInventoryOpResult UYIInventoryComponent::RequestCombineItem(const FYIInventoryCombineItemRequest& Request)
{
	FYIInventoryCombineItemRequest EffectiveRequest = Request;
	if (!EffectiveRequest.RequestId.IsValid())
	{
		EffectiveRequest.RequestId = FGuid::NewGuid();
	}

	if (!EffectiveRequest.ItemRef.Bag.BagId.IsValid() || !EffectiveRequest.ItemRef.Item.ItemInstanceId.IsValid())
	{
		FYIInventoryOpResult Result = YIInventory_MakeOpResultRejected(EYIInventoryOpError::InvalidRef, &EffectiveRequest.ItemRef);
		Result.TransactionId = EffectiveRequest.RequestId;
		Result.OpKind = EYIInventoryOpKind::Combine;
		return Result;
	}

	FYIInventoryOpResult Result;
	YIInventory_PopulateResultIds(Result, EffectiveRequest.ItemRef);
	Result.TransactionId = EffectiveRequest.RequestId;
	Result.OpKind = EYIInventoryOpKind::Combine;
	Result.SourceBagRevision = GetBagRuntimeRevisionById(EffectiveRequest.ItemRef.Bag.BagId);
	if (!(GetOwner() && GetOwner()->HasAuthority()))
	{
		ServerRequestCombineItem(EffectiveRequest);
		Result.bRequestAccepted = true;
		Result.bSucceeded = false;
		Result.Error = EYIInventoryOpError::None;
		return Result;
	}
	if (EffectiveRequest.ExpectedSourceBagRevision != INDEX_NONE &&
		Result.SourceBagRevision != INDEX_NONE &&
		Result.SourceBagRevision != EffectiveRequest.ExpectedSourceBagRevision)
	{
		Result.Error = EYIInventoryOpError::RevisionMismatch;
		return Result;
	}

	const bool bOpResult = CombineItemInBag(EffectiveRequest.ItemRef.Bag.BagId, EffectiveRequest.ItemRef.Item.ItemInstanceId);
	Result.bRequestAccepted = bOpResult;
	Result.bSucceeded = bOpResult && (!GetOwner() || GetOwner()->HasAuthority());
	Result.Error = bOpResult ? EYIInventoryOpError::None : EYIInventoryOpError::ValidationFailed;
	Result.SourceBagRevision = GetBagRuntimeRevisionById(EffectiveRequest.ItemRef.Bag.BagId);
	return Result;
}

void UYIInventoryComponent::ServerRequestMoveItem_Implementation(FYIInventoryMoveItemRequest Request)
{
	const FYIInventoryOpResult Result = RequestMoveItem(Request);
	ClientReceiveInventoryOpResult(Result);
}

void UYIInventoryComponent::ServerRequestRotateItem_Implementation(FYIInventoryRotateItemRequest Request)
{
	const FYIInventoryOpResult Result = RequestRotateItem(Request);
	ClientReceiveInventoryOpResult(Result);
}

void UYIInventoryComponent::ServerRequestRemoveItem_Implementation(FYIInventoryRemoveItemRequest Request)
{
	const FYIInventoryOpResult Result = RequestRemoveItem(Request);
	ClientReceiveInventoryOpResult(Result);
}

void UYIInventoryComponent::ServerRequestTransferItem_Implementation(FYIInventoryTransferItemRequest Request)
{
	const FYIInventoryOpResult Result = RequestTransferItem(Request);
	ClientReceiveInventoryOpResult(Result);
}

void UYIInventoryComponent::ServerRequestSplitStack_Implementation(FYIInventorySplitStackRequest Request)
{
	const FYIInventoryOpResult Result = RequestSplitStack(Request);
	ClientReceiveInventoryOpResult(Result);
}

void UYIInventoryComponent::ServerRequestCombineItem_Implementation(FYIInventoryCombineItemRequest Request)
{
	const FYIInventoryOpResult Result = RequestCombineItem(Request);
	ClientReceiveInventoryOpResult(Result);
}

void UYIInventoryComponent::ClientReceiveInventoryOpResult_Implementation(const FYIInventoryOpResult& Result)
{
	OnInventoryOpResultReceived.Broadcast(Result);
}

void UYIInventoryComponent::HandleBagItemAdded(int32 Index, FYIBagItem Item)
{
	FYIInventoryEventBridgeService::HandleBagItemAdded(*this, Index, Item);
}

void UYIInventoryComponent::HandleBagItemRemoved(int32 Index, FYIBagItem Item)
{
	FYIInventoryEventBridgeService::HandleBagItemRemoved(*this, Index, Item);
}

void UYIInventoryComponent::HandleBagItemMoved(int32 Index, FIntPoint NewPos)
{
	FYIInventoryEventBridgeService::HandleBagItemMoved(*this, Index, NewPos);
}

void UYIInventoryComponent::HandleBagItemRotated(int32 Index)
{
	FYIInventoryEventBridgeService::HandleBagItemRotated(*this, Index);
}

void UYIInventoryComponent::HandleBagItemTransferred(UYIInventoryBag* Src, UYIInventoryBag* Dest, int32 SrcIdx, int32 DestIdx)
{
	FYIInventoryEventBridgeService::HandleBagItemTransferred(*this, Src, Dest, SrcIdx, DestIdx);
}

// -------- UI helpers --------

/* UI screen helpers moved to YOLOInventoryUI (UYIInventoryUIScreenLibrary). */





