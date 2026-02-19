#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YIInventoryDemoActor.generated.h"

class UYIInventoryBag;
class UYIItemDefinition;

UCLASS(Blueprintable)
class YOLOINVENTORYCONTAINERS_API AYIInventoryDemoActor : public AActor
{
	GENERATED_BODY()
public:
	AYIInventoryDemoActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category="YOLOInventory|Demo")
	UYIInventoryBag* BagA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Instanced, Category="YOLOInventory|Demo")
	UYIInventoryBag* BagB;

	// Create or reset demo bags with given grid sizes
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Demo")
	void InitializeBags(FIntPoint GridA, FIntPoint GridB);

	// Add an item to the specified bag; returns index or -1
	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Demo")
	int32 AddItemToBag(UYIInventoryBag* Bag, UYIItemDefinition* Definition, int32 Count);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Demo")
	bool TransferFromAToB(int32 Index, int32 Count, int32& OutDestIndex);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Demo")
	bool TransferFromBToA(int32 Index, int32 Count, int32& OutDestIndex);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Demo")
	void AutoPackA();

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Demo")
	void AutoPackB();
};

