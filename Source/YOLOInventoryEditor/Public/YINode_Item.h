#pragma once

#include "CoreMinimal.h"
#include "YINode_Base.h"
#include "YIStackEntry.h"

#include "YINode_Item.generated.h"


UCLASS()
class YOLOINVENTORYEDITOR_API UYINode_Item : public UYINode_Base
{
	GENERATED_BODY()
public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override { return NSLOCTEXT("YOLOInventory", "ItemNode", "Item"); }

	// Stacks
	UPROPERTY(EditAnywhere, Instanced, Category="Stacks") TArray<TObjectPtr<UYIStackEntry_UI>> UIStack;
	UPROPERTY(EditAnywhere, Instanced, Category="Stacks") TArray<TObjectPtr<UYIStackEntry_Ability>> AbilityStack;
	UPROPERTY(EditAnywhere, Instanced, Category="Stacks") TArray<TObjectPtr<UYIStackEntry_Upgrade>> UpgradeStack;
	UPROPERTY(EditAnywhere, Instanced, Category="Stacks") TArray<TObjectPtr<UYIStackEntry_Economy>> EconomyStack;

	// Helper to add a new entry of a subclass to a given stack
	UFUNCTION() UObject* AddStackEntry(TSubclassOf<UYIStackEntry> EntryClass, FName StackName);
	UFUNCTION() bool RemoveStackEntry(int32 Index, FName StackName);
	UFUNCTION() bool MoveStackEntryUp(int32 Index, FName StackName);
	UFUNCTION() bool MoveStackEntryDown(int32 Index, FName StackName);
	UFUNCTION() bool DuplicateStackEntry(int32 Index, FName StackName);

	// Notify helpers
	UFUNCTION() void RefreshGraph();
};

