#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "YIItemInstance.h"
#include "YIContainerInterface.generated.h"

UINTERFACE(BlueprintType)
class YOLOINVENTORYLEGACYBRIDGE_API UYIContainerInterface : public UInterface
{
	GENERATED_BODY()
};

class YOLOINVENTORYLEGACYBRIDGE_API IYIContainerInterface
{
	GENERATED_BODY()
public:
	// Add instance (or count) to container; returns true if succeeded
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="YOLOInventory|Container")
	bool AddItem(const FYIItemInstance& Instance);

	// Remove by InstanceId (or StackId and Count)
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="YOLOInventory|Container")
	bool RemoveItemById(const FGuid& InstanceId, int32 Count);

	// Transfer between containers
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="YOLOInventory|Container")
	bool TransferTo(const TScriptInterface<IYIContainerInterface>& Other, const FGuid& InstanceId, int32 Count);
};
