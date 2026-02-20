#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "YIContainerInterface.h"
#include "YIGridContainer.generated.h"

USTRUCT(BlueprintType)
struct YOLOINVENTORYLEGACYBRIDGE_API FYIGridEntry
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FYIItemInstance Instance;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint Pos = FIntPoint::ZeroValue;
};

UCLASS(BlueprintType)
class YOLOINVENTORYLEGACYBRIDGE_API UYIGridContainer : public UObject, public IYIContainerInterface
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid")
	FIntPoint GridSize = FIntPoint(8,8);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Grid")
	TArray<FYIGridEntry> Items;

	// IYIContainerInterface
	virtual bool AddItem_Implementation(const FYIItemInstance& Instance) override;
	virtual bool RemoveItemById_Implementation(const FGuid& InstanceId, int32 Count) override;
	virtual bool TransferTo_Implementation(const TScriptInterface<IYIContainerInterface>& Other, const FGuid& InstanceId, int32 Count) override;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Grid")
	bool FindFirstFit(const FYIItemInstance& Instance, FIntPoint& OutPos) const;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Grid")
	bool CanPlaceAt(const FIntPoint& Pos, const FYIItemInstance& Instance) const;

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Grid")
	bool MoveItem(const FGuid& InstanceId, const FIntPoint& NewPos);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Grid")
	bool RotateItem(const FGuid& InstanceId);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Grid")
	bool CombineStacks(const FGuid& A, const FGuid& B);

	UFUNCTION(BlueprintCallable, Category="YOLOInventory|Grid")
	bool SplitStack(const FGuid& InstanceId, int32 Amount);
};
