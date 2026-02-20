#include "YIItemBlueprintLibrary.h"
#include "YIItemRegistrySubsystem.h"
#include "YIItemDefinition.h"
#include "Engine/Engine.h"
#include "YIContainerInterface.h"
#include "YIGridContainer.h"

UYIItemDefinition* UYIItemBlueprintLibrary::GetItemDefinitionByCode(int64 Code)
{
	if (UEngine* Engine = GEngine)
	{
		if (UYIItemRegistrySubsystem* Sys = Engine->GetEngineSubsystem<UYIItemRegistrySubsystem>())
		{
			return Sys->GetByCode(Code);
		}
	}
	return nullptr;
}

FYIItemInstance UYIItemBlueprintLibrary::MakeItemInstanceByCode(int64 Code, int32 Count)
{
	FYIItemInstance Out;
	Out.Count = Count;
	Out.Definition = GetItemDefinitionByCode(Code);
	// Size is derived from the Definition->DefaultSize at runtime; default rotation false
	return Out;
}

bool UYIItemBlueprintLibrary::AddItemByCode(const TScriptInterface<IYIContainerInterface>& Container, int64 Code, int32 Count)
{
	if (!Container.GetInterface()) return false;
	FYIItemInstance Item = MakeItemInstanceByCode(Code, Count);
	return IYIContainerInterface::Execute_AddItem(Container.GetObject(), Item);
}

bool UYIItemBlueprintLibrary::Grid_MoveItem(UYIGridContainer* Grid, const FGuid& InstanceId, FIntPoint NewPos)
{
	return Grid ? Grid->MoveItem(InstanceId, NewPos) : false;
}

bool UYIItemBlueprintLibrary::Grid_RotateItem(UYIGridContainer* Grid, const FGuid& InstanceId)
{
	return Grid ? Grid->RotateItem(InstanceId) : false;
}

bool UYIItemBlueprintLibrary::Grid_CombineStacks(UYIGridContainer* Grid, const FGuid& A, const FGuid& B)
{
	return Grid ? Grid->CombineStacks(A, B) : false;
}

bool UYIItemBlueprintLibrary::Grid_SplitStack(UYIGridContainer* Grid, const FGuid& InstanceId, int32 Amount)
{
	return Grid ? Grid->SplitStack(InstanceId, Amount) : false;
}
