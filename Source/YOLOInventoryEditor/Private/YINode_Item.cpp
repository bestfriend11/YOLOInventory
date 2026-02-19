#include "YINode_Item.h"
#include "YIEditorUtil.h"
#include "EdGraph/EdGraph.h"

void UYINode_Item::RefreshGraph()
{
#if WITH_EDITOR
	Modify();
	PostEditChange();
	if (UEdGraph* Graph = GetGraph())
	{
		Graph->NotifyGraphChanged();
	}
#endif
}

UObject* UYINode_Item::AddStackEntry(TSubclassOf<UYIStackEntry> EntryClass, FName StackName)
{
	if (!*EntryClass)
	{
		return nullptr;
	}
	UYIStackEntry* NewEntry = NewObject<UYIStackEntry>(this, EntryClass, NAME_None, RF_Transactional);
	if (StackName == TEXT("UI"))
	{
		if (UYIStackEntry_UI* Typed = Cast<UYIStackEntry_UI>(NewEntry)) { UIStack.Add(Typed); RefreshGraph(); return Typed; }
	}
	else if (StackName == TEXT("Ability"))
	{
		if (UYIStackEntry_Ability* Typed = Cast<UYIStackEntry_Ability>(NewEntry)) { AbilityStack.Add(Typed); RefreshGraph(); return Typed; }
	}
	else if (StackName == TEXT("Upgrade"))
	{
		if (UYIStackEntry_Upgrade* Typed = Cast<UYIStackEntry_Upgrade>(NewEntry)) { UpgradeStack.Add(Typed); RefreshGraph(); return Typed; }
	}
	else if (StackName == TEXT("Economy"))
	{
		if (UYIStackEntry_Economy* Typed = Cast<UYIStackEntry_Economy>(NewEntry)) { EconomyStack.Add(Typed); RefreshGraph(); return Typed; }
	}
	return nullptr;
}

bool UYINode_Item::RemoveStackEntry(int32 Index, FName StackName)
{
	bool bRemoved = false;
	if (StackName == TEXT("UI")) { if (UIStack.IsValidIndex(Index)) { UIStack.RemoveAt(Index); bRemoved = true; } }
	else if (StackName == TEXT("Ability")) { if (AbilityStack.IsValidIndex(Index)) { AbilityStack.RemoveAt(Index); bRemoved = true; } }
	else if (StackName == TEXT("Upgrade")) { if (UpgradeStack.IsValidIndex(Index)) { UpgradeStack.RemoveAt(Index); bRemoved = true; } }
	else if (StackName == TEXT("Economy")) { if (EconomyStack.IsValidIndex(Index)) { EconomyStack.RemoveAt(Index); bRemoved = true; } }
	if (bRemoved) { RefreshGraph(); }
	return bRemoved;
}

bool UYINode_Item::MoveStackEntryUp(int32 Index, FName StackName)
{
	int32 Other = Index - 1;
	bool bSwapped = false;
	if (StackName == TEXT("UI")) { bSwapped = YI::SwapIfInRange(*(TArray<UObject*>*)&UIStack, Index, Other); }
	else if (StackName == TEXT("Ability")) { bSwapped = YI::SwapIfInRange(*(TArray<UObject*>*)&AbilityStack, Index, Other); }
	else if (StackName == TEXT("Upgrade")) { bSwapped = YI::SwapIfInRange(*(TArray<UObject*>*)&UpgradeStack, Index, Other); }
	else if (StackName == TEXT("Economy")) { bSwapped = YI::SwapIfInRange(*(TArray<UObject*>*)&EconomyStack, Index, Other); }
	if (bSwapped) { RefreshGraph(); }
	return bSwapped;
}

bool UYINode_Item::MoveStackEntryDown(int32 Index, FName StackName)
{
	int32 Other = Index + 1;
	bool bSwapped = false;
	if (StackName == TEXT("UI")) { bSwapped = YI::SwapIfInRange(*(TArray<UObject*>*)&UIStack, Index, Other); }
	else if (StackName == TEXT("Ability")) { bSwapped = YI::SwapIfInRange(*(TArray<UObject*>*)&AbilityStack, Index, Other); }
	else if (StackName == TEXT("Upgrade")) { bSwapped = YI::SwapIfInRange(*(TArray<UObject*>*)&UpgradeStack, Index, Other); }
	else if (StackName == TEXT("Economy")) { bSwapped = YI::SwapIfInRange(*(TArray<UObject*>*)&EconomyStack, Index, Other); }
	if (bSwapped) { RefreshGraph(); }
	return bSwapped;
}

bool UYINode_Item::DuplicateStackEntry(int32 Index, FName StackName)
{
	if (StackName == TEXT("UI"))
	{
		if (!UIStack.IsValidIndex(Index)) return false;
		UYIStackEntry_UI* Src = UIStack[Index];
		if (!Src) return false;
		UYIStackEntry_UI* Dst = DuplicateObject<UYIStackEntry_UI>(Src, this);
		UIStack.Insert(Dst, Index + 1);
		RefreshGraph();
		return true;
	}
	if (StackName == TEXT("Ability"))
	{
		if (!AbilityStack.IsValidIndex(Index)) return false;
		UYIStackEntry_Ability* Src = AbilityStack[Index];
		if (!Src) return false;
		UYIStackEntry_Ability* Dst = DuplicateObject<UYIStackEntry_Ability>(Src, this);
		AbilityStack.Insert(Dst, Index + 1);
		RefreshGraph();
		return true;
	}
	if (StackName == TEXT("Upgrade"))
	{
		if (!UpgradeStack.IsValidIndex(Index)) return false;
		UYIStackEntry_Upgrade* Src = UpgradeStack[Index];
		if (!Src) return false;
		UYIStackEntry_Upgrade* Dst = DuplicateObject<UYIStackEntry_Upgrade>(Src, this);
		UpgradeStack.Insert(Dst, Index + 1);
		RefreshGraph();
		return true;
	}
	if (StackName == TEXT("Economy"))
	{
		if (!EconomyStack.IsValidIndex(Index)) return false;
		UYIStackEntry_Economy* Src = EconomyStack[Index];
		if (!Src) return false;
		UYIStackEntry_Economy* Dst = DuplicateObject<UYIStackEntry_Economy>(Src, this);
		EconomyStack.Insert(Dst, Index + 1);
		RefreshGraph();
		return true;
	}
	return false;
}
