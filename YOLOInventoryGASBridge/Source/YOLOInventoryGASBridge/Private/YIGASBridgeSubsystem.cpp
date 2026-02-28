#include "YIGASBridgeSubsystem.h"

namespace
{
	static bool IsRequestContextValid(const FYIGASBridgeRequestContext& Context)
	{
		return Context.ItemRef.Bag.BagId.IsValid() && Context.ItemRef.Item.ItemInstanceId.IsValid();
	}
}

FYIGASBridgeOpResult UYIGASBridgeSubsystem::RequestApplyItemGrants(const FYIGASBridgeGrantRequest& Request)
{
	if (!IsRequestContextValid(Request.Context))
	{
		return MakeRejectedResult(
			EYIGASBridgeOpKind::ApplyItemGrants,
			EYIGASBridgeOpError::InvalidRequest,
			Request.Context.RequestId,
			NSLOCTEXT("YOLOInventory", "GASBridge_InvalidApply", "Invalid apply-grant request context."));
	}

	return ApplyItemGrants_Internal(Request);
}

FYIGASBridgeOpResult UYIGASBridgeSubsystem::RequestRemoveItemGrants(const FYIGASBridgeGrantRequest& Request)
{
	if (!IsRequestContextValid(Request.Context))
	{
		return MakeRejectedResult(
			EYIGASBridgeOpKind::RemoveItemGrants,
			EYIGASBridgeOpError::InvalidRequest,
			Request.Context.RequestId,
			NSLOCTEXT("YOLOInventory", "GASBridge_InvalidRemove", "Invalid remove-grant request context."));
	}

	return RemoveItemGrants_Internal(Request);
}

FYIGASBridgeOpResult UYIGASBridgeSubsystem::RequestActivateItem(const FYIGASBridgeActivateRequest& Request)
{
	if (!IsRequestContextValid(Request.Context))
	{
		return MakeRejectedResult(
			EYIGASBridgeOpKind::ActivateItem,
			EYIGASBridgeOpError::InvalidRequest,
			Request.Context.RequestId,
			NSLOCTEXT("YOLOInventory", "GASBridge_InvalidActivate", "Invalid activate-item request context."));
	}

	return ActivateItem_Internal(Request);
}

FYIGASBridgeOpResult UYIGASBridgeSubsystem::BuildDescriptionTokens(const FYIGASBridgeDescriptionRequest& Request, TArray<FYIGASBridgeDescriptionToken>& OutTokens) const
{
	if (!IsRequestContextValid(Request.Context))
	{
		return MakeRejectedResult(
			EYIGASBridgeOpKind::BuildDescriptionTokens,
			EYIGASBridgeOpError::InvalidRequest,
			Request.Context.RequestId,
			NSLOCTEXT("YOLOInventory", "GASBridge_InvalidDescription", "Invalid description-token request context."));
	}

	return BuildDescriptionTokens_Internal(Request, OutTokens);
}

FYIGASBridgeOpResult UYIGASBridgeSubsystem::ApplyItemGrants_Internal(const FYIGASBridgeGrantRequest& Request)
{
	return MakeRejectedResult(
		EYIGASBridgeOpKind::ApplyItemGrants,
		EYIGASBridgeOpError::NotImplemented,
		Request.Context.RequestId,
		NSLOCTEXT("YOLOInventory", "GASBridge_ApplyNotImpl", "Apply-item-grants is not implemented yet."));
}

FYIGASBridgeOpResult UYIGASBridgeSubsystem::RemoveItemGrants_Internal(const FYIGASBridgeGrantRequest& Request)
{
	return MakeRejectedResult(
		EYIGASBridgeOpKind::RemoveItemGrants,
		EYIGASBridgeOpError::NotImplemented,
		Request.Context.RequestId,
		NSLOCTEXT("YOLOInventory", "GASBridge_RemoveNotImpl", "Remove-item-grants is not implemented yet."));
}

FYIGASBridgeOpResult UYIGASBridgeSubsystem::ActivateItem_Internal(const FYIGASBridgeActivateRequest& Request)
{
	return MakeRejectedResult(
		EYIGASBridgeOpKind::ActivateItem,
		EYIGASBridgeOpError::NotImplemented,
		Request.Context.RequestId,
		NSLOCTEXT("YOLOInventory", "GASBridge_ActivateNotImpl", "Activate-item is not implemented yet."));
}

FYIGASBridgeOpResult UYIGASBridgeSubsystem::BuildDescriptionTokens_Internal(const FYIGASBridgeDescriptionRequest& Request, TArray<FYIGASBridgeDescriptionToken>& OutTokens) const
{
	OutTokens.Reset();
	return MakeRejectedResult(
		EYIGASBridgeOpKind::BuildDescriptionTokens,
		EYIGASBridgeOpError::NotImplemented,
		Request.Context.RequestId,
		NSLOCTEXT("YOLOInventory", "GASBridge_DescNotImpl", "Description-token generation is not implemented yet."));
}

FYIGASBridgeOpResult UYIGASBridgeSubsystem::MakeRejectedResult(
	EYIGASBridgeOpKind OpKind,
	EYIGASBridgeOpError Error,
	const FGuid& RequestId,
	const FText& Message)
{
	FYIGASBridgeOpResult Result;
	Result.bRequestAccepted = false;
	Result.bSucceeded = false;
	Result.OpKind = OpKind;
	Result.Error = Error;
	Result.RequestId = RequestId;
	Result.Message = Message;
	return Result;
}

