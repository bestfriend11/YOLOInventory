#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YIInventoryCoreTypes.h"
#include "YIGASBridgeApiTypes.generated.h"

class AActor;
class UAbilitySystemComponent;

UENUM(BlueprintType)
enum class EYIGASBridgeOpKind : uint8
{
	Unknown UMETA(DisplayName="Unknown"),
	ApplyItemGrants UMETA(DisplayName="Apply Item Grants"),
	RemoveItemGrants UMETA(DisplayName="Remove Item Grants"),
	ActivateItem UMETA(DisplayName="Activate Item"),
	BuildDescriptionTokens UMETA(DisplayName="Build Description Tokens")
};

UENUM(BlueprintType)
enum class EYIGASBridgeOpError : uint8
{
	None UMETA(DisplayName="None"),
	InvalidRequest UMETA(DisplayName="Invalid Request"),
	MissingItem UMETA(DisplayName="Missing Item"),
	MissingASC UMETA(DisplayName="Missing AbilitySystemComponent"),
	NotAuthoritative UMETA(DisplayName="Not Authoritative"),
	BlockedByTags UMETA(DisplayName="Blocked By Tags"),
	CooldownActive UMETA(DisplayName="Cooldown Active"),
	NotImplemented UMETA(DisplayName="Not Implemented")
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYGASBRIDGE_API FYIGASBridgeOpResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	bool bRequestAccepted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	bool bSucceeded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	EYIGASBridgeOpKind OpKind = EYIGASBridgeOpKind::Unknown;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	EYIGASBridgeOpError Error = EYIGASBridgeOpError::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	FText Message;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYGASBRIDGE_API FYIGASBridgeRequestContext
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	FGuid RequestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	FYIInventoryItemRef ItemRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	TObjectPtr<UAbilitySystemComponent> TargetASC = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	FGameplayTagContainer ContextTags;

	/** Evaluated level context (from player/item/encounter). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	int32 EvaluationLevel = 1;

	/** Optional additional scaling factor for difficulty/world context. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	float DifficultyScale = 1.0f;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYGASBRIDGE_API FYIGASBridgeGrantRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	FYIGASBridgeRequestContext Context;

	/** True for equip/apply path, false for unequip/remove path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	bool bApply = true;

	/** Optional source stack count for scalable calculations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge", meta=(ClampMin="1"))
	int32 StackCount = 1;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYGASBRIDGE_API FYIGASBridgeActivateRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	FYIGASBridgeRequestContext Context;

	/** Optional trigger tag (e.g. Item.Use.Primary). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	FGameplayTag ActivationTag;

	/** Optional consume count for use requests. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge", meta=(ClampMin="1"))
	int32 ConsumeCount = 1;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYGASBRIDGE_API FYIGASBridgeDescriptionRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	FYIGASBridgeRequestContext Context;
};

USTRUCT(BlueprintType)
struct YOLOINVENTORYGASBRIDGE_API FYIGASBridgeDescriptionToken
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	FName Key = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	FText Label;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	FText ValueText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GAS Bridge")
	FLinearColor Color = FLinearColor::White;
};

