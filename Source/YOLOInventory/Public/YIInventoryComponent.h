#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YIInventoryComponent.generated.h"

class UYIInventoryBag;

UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class YOLOINVENTORY_API UYIInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UYIInventoryComponent();

	// Currently equipped (open) bag
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory")
	TObjectPtr<UYIInventoryBag> EquippedBag;

	// All bags owned by this component
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory")
	TArray<TObjectPtr<UYIInventoryBag>> Bags;

	// Events
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBagOpened, UYIInventoryBag*, Bag);
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBagClosed, UYIInventoryBag*, Bag);

	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnBagOpened OnBagOpened;

	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnBagClosed OnBagClosed;

	// Create a new bag (owned by this component)
	UFUNCTION(BlueprintCallable, Category="Inventory")
	UYIInventoryBag* CreateBag(FName BagName, FIntPoint GridSize);

	// Open/close bag (fire events)
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void OpenBag(UYIInventoryBag* Bag);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	void CloseBag(UYIInventoryBag* Bag);

	// Quick accessors
	UFUNCTION(BlueprintCallable, Category="Inventory")
	UYIInventoryBag* GetBag() const { return EquippedBag; }

	// Add an item to a bag; returns success
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool AddItemToBag(UYIInventoryBag* Bag, TSoftObjectPtr<class UYIItemDefinition> ItemDef, int32 Count = 1);

	// Remove a bag owned by this component
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool RemoveBag(UYIInventoryBag* Bag);
};
