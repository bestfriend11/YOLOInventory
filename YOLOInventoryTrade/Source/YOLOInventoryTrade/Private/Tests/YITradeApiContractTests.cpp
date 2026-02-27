#include "YITradeInteractionComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FYITradeApiContract_AddOfferInvalidOwnerTest,
	"YOLO.Inventory.API.Contracts.Trade.AddOffer.InvalidOwner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FYITradeApiContract_RemoveOfferInvalidOwnerTest,
	"YOLO.Inventory.API.Contracts.Trade.RemoveOffer.InvalidOwner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FYITradeApiContract_SetResourceInvalidOwnerTest,
	"YOLO.Inventory.API.Contracts.Trade.SetResource.InvalidOwner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FYITradeApiContract_ShopOpenInvalidOwnerTest,
	"YOLO.Inventory.API.Contracts.Shop.Open.InvalidOwner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYITradeApiContract_AddOfferInvalidOwnerTest::RunTest(const FString& Parameters)
{
	UYITradeInteractionComponent* Trade = NewObject<UYITradeInteractionComponent>(GetTransientPackage());
	TestNotNull(TEXT("Trade interaction component is created"), Trade);

	const FYITradeAddOfferRequest Request;
	const FYITradeOpResult Result = Trade->RequestTradeAddOfferEx(Request);

	TestEqual(TEXT("Op kind is AddOffer"), Result.OpKind, EYITradeOpKind::AddOffer);
	TestEqual(TEXT("Invalid owner error is returned"), Result.Error, EYITradeOpError::InvalidOwner);
	TestFalse(TEXT("Request is rejected"), Result.bRequestAccepted);
	TestFalse(TEXT("Operation did not succeed"), Result.bSucceeded);
	TestTrue(TEXT("Request id is generated"), Result.RequestId.IsValid());
	return true;
}

bool FYITradeApiContract_RemoveOfferInvalidOwnerTest::RunTest(const FString& Parameters)
{
	UYITradeInteractionComponent* Trade = NewObject<UYITradeInteractionComponent>(GetTransientPackage());
	TestNotNull(TEXT("Trade interaction component is created"), Trade);

	const FYITradeRemoveOfferRequest Request;
	const FYITradeOpResult Result = Trade->RequestTradeRemoveOfferEx(Request);

	TestEqual(TEXT("Op kind is RemoveOffer"), Result.OpKind, EYITradeOpKind::RemoveOffer);
	TestEqual(TEXT("Invalid owner error is returned"), Result.Error, EYITradeOpError::InvalidOwner);
	TestFalse(TEXT("Request is rejected"), Result.bRequestAccepted);
	TestFalse(TEXT("Operation did not succeed"), Result.bSucceeded);
	TestTrue(TEXT("Request id is generated"), Result.RequestId.IsValid());
	return true;
}

bool FYITradeApiContract_SetResourceInvalidOwnerTest::RunTest(const FString& Parameters)
{
	UYITradeInteractionComponent* Trade = NewObject<UYITradeInteractionComponent>(GetTransientPackage());
	TestNotNull(TEXT("Trade interaction component is created"), Trade);

	const FYITradeSetResourceRequest Request;
	const FYITradeOpResult Result = Trade->RequestTradeSetResourceEx(Request);

	TestEqual(TEXT("Op kind is SetResource"), Result.OpKind, EYITradeOpKind::SetResource);
	TestEqual(TEXT("Invalid owner error is returned"), Result.Error, EYITradeOpError::InvalidOwner);
	TestFalse(TEXT("Request is rejected"), Result.bRequestAccepted);
	TestFalse(TEXT("Operation did not succeed"), Result.bSucceeded);
	TestTrue(TEXT("Request id is generated"), Result.RequestId.IsValid());
	return true;
}

bool FYITradeApiContract_ShopOpenInvalidOwnerTest::RunTest(const FString& Parameters)
{
	UYITradeInteractionComponent* Trade = NewObject<UYITradeInteractionComponent>(GetTransientPackage());
	TestNotNull(TEXT("Trade interaction component is created"), Trade);

	const FYIShopOpenRequest Request;
	const FYIShopOpResult Result = Trade->RequestShopOpenEx(Request);

	TestEqual(TEXT("Op kind is Open"), Result.OpKind, EYIShopOpKind::Open);
	TestEqual(TEXT("Invalid request error is returned"), Result.Error, EYIShopOpError::InvalidRequest);
	TestFalse(TEXT("Request is rejected"), Result.bRequestAccepted);
	TestFalse(TEXT("Operation did not succeed"), Result.bSucceeded);
	TestTrue(TEXT("Request id is generated"), Result.RequestId.IsValid());
	return true;
}

#endif
