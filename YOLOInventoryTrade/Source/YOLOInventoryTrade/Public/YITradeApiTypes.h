#pragma once

#include "CoreMinimal.h"
#include "YITradeSessionActor.h"
#include "YITradeApiTypes.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EYITradeOpError : uint8
{
	None UMETA(DisplayName="None"),
	InvalidRequest UMETA(DisplayName="Invalid Request"),
	InvalidOwner UMETA(DisplayName="Invalid Owner"),
	InvalidTarget UMETA(DisplayName="Invalid Target"),
	TooFar UMETA(DisplayName="Too Far"),
	NoSession UMETA(DisplayName="No Session"),
	NotParticipant UMETA(DisplayName="Not Participant"),
	ValidationFailed UMETA(DisplayName="Validation Failed"),
	AuthorityRequired UMETA(DisplayName="Authority Required")
};

UENUM(BlueprintType)
enum class EYITradeOpKind : uint8
{
	Unknown UMETA(DisplayName="Unknown"),
	Open UMETA(DisplayName="Open Trade"),
	Transfer UMETA(DisplayName="Transfer Trade Item"),
	Commit UMETA(DisplayName="Commit Trade"),
	Close UMETA(DisplayName="Close Trade")
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYTRADE_API FYITradeOpResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	bool bRequestAccepted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	EYITradeOpError Error = EYITradeOpError::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	EYITradeOpKind OpKind = EYITradeOpKind::Unknown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	FText Message;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYTRADE_API FYITradeOpenRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	bool bTargetIsNPC = false;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYTRADE_API FYITradeTransferRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	ETradeSide FromSide = ETradeSide::SideA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	ETradeSide ToSide = ETradeSide::SideB;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	int32 SourceIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	FIntPoint DestPos = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trade")
	int32 Count = 0;
};
