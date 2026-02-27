#include "YIInventoryComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FYIInventoryApiContract_MoveInvalidRefTest,
	"YOLO.Inventory.API.Contracts.Inventory.Move.InvalidRef",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FYIInventoryApiContract_TransferInvalidRefTest,
	"YOLO.Inventory.API.Contracts.Inventory.Transfer.InvalidRef",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYIInventoryApiContract_MoveInvalidRefTest::RunTest(const FString& Parameters)
{
	UYIInventoryComponent* Inventory = NewObject<UYIInventoryComponent>(GetTransientPackage());
	TestNotNull(TEXT("Inventory component is created"), Inventory);

	const FYIInventoryMoveItemRequest Request;
	const FYIInventoryOpResult Result = Inventory->RequestMoveItem(Request);

	TestEqual(TEXT("Op kind is Move"), Result.OpKind, EYIInventoryOpKind::Move);
	TestEqual(TEXT("Invalid ref error is returned"), Result.Error, EYIInventoryOpError::InvalidRef);
	TestFalse(TEXT("Request is rejected"), Result.bRequestAccepted);
	TestFalse(TEXT("Operation did not succeed"), Result.bSucceeded);
	TestTrue(TEXT("Transaction id is generated"), Result.TransactionId.IsValid());
	return true;
}

bool FYIInventoryApiContract_TransferInvalidRefTest::RunTest(const FString& Parameters)
{
	UYIInventoryComponent* Inventory = NewObject<UYIInventoryComponent>(GetTransientPackage());
	TestNotNull(TEXT("Inventory component is created"), Inventory);

	const FYIInventoryTransferItemRequest Request;
	const FYIInventoryOpResult Result = Inventory->RequestTransferItem(Request);

	TestEqual(TEXT("Op kind is Transfer"), Result.OpKind, EYIInventoryOpKind::Transfer);
	TestEqual(TEXT("Invalid ref error is returned"), Result.Error, EYIInventoryOpError::InvalidRef);
	TestFalse(TEXT("Request is rejected"), Result.bRequestAccepted);
	TestFalse(TEXT("Operation did not succeed"), Result.bSucceeded);
	TestTrue(TEXT("Transaction id is generated"), Result.TransactionId.IsValid());
	return true;
}

#endif
