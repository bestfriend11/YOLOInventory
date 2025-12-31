#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YIItemEditorLibrary.generated.h"

UCLASS()
class UYIItemEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
#if WITH_EDITOR
	UFUNCTION(CallInEditor, BlueprintCallable, Category="YOLOInventory|Editor")
	static bool EnsureUniqueCodes(bool bAutoFix = true);
#endif
};
