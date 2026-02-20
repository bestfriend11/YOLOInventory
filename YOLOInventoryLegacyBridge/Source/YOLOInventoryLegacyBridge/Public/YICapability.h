#pragma once
#include "CoreMinimal.h"
#include "YIItemInstance.h"
#include "YICapability.generated.h"

USTRUCT(BlueprintType)
struct YOLOINVENTORYLEGACYBRIDGE_API FYICapabilityContext
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Context")
	TMap<FName,float> Attributes;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class YOLOINVENTORYLEGACYBRIDGE_API UYICapability : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, Category="YOLOInventory|Capability")
	void OnApplyToInstance(UPARAM(ref) FYIItemInstance& Instance, const FYICapabilityContext& Context) const;

	UFUNCTION(BlueprintNativeEvent, Category="YOLOInventory|Capability|UI")
	FText GetDisplayText(const FYICapabilityContext& Context) const;
};
