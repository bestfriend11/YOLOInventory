#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YIInventoryUIScreenLibrary.generated.h"

class UUserWidget;
class UYIInventoryComponent;

UCLASS()
class YOLOINVENTORYUI_API UYIInventoryUIScreenLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Open or reuse an inventory screen for the local owning player and bind it to InventoryComponent. */
	UFUNCTION(BlueprintCallable, Category="YOLO Inventory|UI", meta=(WorldContext="WorldContextObject"))
	static UUserWidget* OpenInventoryScreenForComponent(UObject* WorldContextObject, UYIInventoryComponent* InventoryComponent, TSubclassOf<UUserWidget> ScreenClass = nullptr);

	/** Close the tracked inventory screen for this inventory component if one was opened through this library. */
	UFUNCTION(BlueprintCallable, Category="YOLO Inventory|UI")
	static void CloseInventoryScreenForComponent(UYIInventoryComponent* InventoryComponent);

	/** Close all tracked inventory screens for this inventory component. */
	UFUNCTION(BlueprintCallable, Category="YOLO Inventory|UI")
	static void CloseAllScreensForComponent(UYIInventoryComponent* InventoryComponent);
};

