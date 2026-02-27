#include "YIEquipmentComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FYIEquipmentApiContract_RequestEquipInvalidOwnerTest,
	"YOLO.Inventory.API.Contracts.Equipment.RequestEquip.InvalidOwner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FYIEquipmentApiContract_RequestUnequipInvalidOwnerTest,
	"YOLO.Inventory.API.Contracts.Equipment.RequestUnequip.InvalidOwner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FYIEquipmentApiContract_RequestEquipInvalidOwnerTest::RunTest(const FString& Parameters)
{
	UYIEquipmentComponent* Equipment = NewObject<UYIEquipmentComponent>(GetTransientPackage());
	TestNotNull(TEXT("Equipment component is created"), Equipment);

	const FYIEquipFromInventoryRequest Request;
	const FYIEquipmentOpResult Result = Equipment->RequestEquip(Request);

	TestEqual(TEXT("Op kind is Equip"), Result.OpKind, EYIEquipmentOpKind::Equip);
	TestEqual(TEXT("Invalid request error is returned"), Result.Error, EYIEquipmentOpError::InvalidRequest);
	TestFalse(TEXT("Request is rejected"), Result.bRequestAccepted);
	TestFalse(TEXT("Operation did not succeed"), Result.bSucceeded);
	TestTrue(TEXT("Request id is generated"), Result.RequestId.IsValid());
	return true;
}

bool FYIEquipmentApiContract_RequestUnequipInvalidOwnerTest::RunTest(const FString& Parameters)
{
	UYIEquipmentComponent* Equipment = NewObject<UYIEquipmentComponent>(GetTransientPackage());
	TestNotNull(TEXT("Equipment component is created"), Equipment);

	const FYIUnequipToInventoryRequest Request;
	const FYIEquipmentOpResult Result = Equipment->RequestUnequip(Request);

	TestEqual(TEXT("Op kind is Unequip"), Result.OpKind, EYIEquipmentOpKind::Unequip);
	TestEqual(TEXT("Invalid request error is returned"), Result.Error, EYIEquipmentOpError::InvalidRequest);
	TestFalse(TEXT("Request is rejected"), Result.bRequestAccepted);
	TestFalse(TEXT("Operation did not succeed"), Result.bSucceeded);
	TestTrue(TEXT("Request id is generated"), Result.RequestId.IsValid());
	return true;
}

#endif
