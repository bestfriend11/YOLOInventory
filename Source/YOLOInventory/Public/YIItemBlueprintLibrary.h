#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YIItemInstance.h"
#include "YIItemBlueprintLibrary.generated.h"

class UYIItemDefinition;

UCLASS()
class UYIItemBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Items")
	static UYIItemDefinition* GetItemDefinitionByCode(int64 Code);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Items")
	static FYIItemInstance MakeItemInstanceByCode(int64 Code, int32 Count);

	// Convenience: add item by code to a container
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Items")
	static bool AddItemByCode(const TScriptInterface<class IYIContainerInterface>& Container, int64 Code, int32 Count);

	// Grid helpers (safe no-op if not grid)
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Grid")
	static bool Grid_MoveItem(class UYIGridContainer* Grid, const FGuid& InstanceId, FIntPoint NewPos);
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Grid")
	static bool Grid_RotateItem(class UYIGridContainer* Grid, const FGuid& InstanceId);
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Grid")
	static bool Grid_CombineStacks(class UYIGridContainer* Grid, const FGuid& A, const FGuid& B);
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Grid")
	static bool Grid_SplitStack(class UYIGridContainer* Grid, const FGuid& InstanceId, int32 Amount);
};
