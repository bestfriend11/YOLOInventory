#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YIEquipmentApiTypes.generated.h"

class UYIInventoryComponent;

UENUM(BlueprintType)
enum class EYIEquipmentOpError : uint8
{
	None UMETA(DisplayName="None"),
	InvalidRequest UMETA(DisplayName="Invalid Request"),
	InvalidInventory UMETA(DisplayName="Invalid Inventory"),
	InvalidIndex UMETA(DisplayName="Invalid Index"),
	InvalidSlot UMETA(DisplayName="Invalid Slot"),
	NoSpace UMETA(DisplayName="No Space"),
	Locked UMETA(DisplayName="Locked"),
	ValidationFailed UMETA(DisplayName="Validation Failed"),
	AuthorityRequired UMETA(DisplayName="Authority Required")
};

UENUM(BlueprintType)
enum class EYIEquipmentOpKind : uint8
{
	Unknown UMETA(DisplayName="Unknown"),
	Equip UMETA(DisplayName="Equip"),
	Unequip UMETA(DisplayName="Unequip")
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYEQUIPMENT_API FYIEquipmentOpResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	bool bRequestAccepted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	EYIEquipmentOpError Error = EYIEquipmentOpError::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	EYIEquipmentOpKind OpKind = EYIEquipmentOpKind::Unknown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	FGuid ItemInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	FText Message;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYEQUIPMENT_API FYIEquipFromInventoryRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	TObjectPtr<UYIInventoryComponent> SourceInventory = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	int32 SourceIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	FGameplayTag RequestedSlotTag;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYEQUIPMENT_API FYIUnequipToInventoryRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	TObjectPtr<UYIInventoryComponent> DestInventory = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
	FGameplayTag SlotTag;
};
