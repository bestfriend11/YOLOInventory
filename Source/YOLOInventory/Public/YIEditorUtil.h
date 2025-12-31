#pragma once

#include "CoreMinimal.h"

namespace YI
{
	inline bool SwapIfInRange(TArray<UObject*>& Arr, int32 A, int32 B)
	{
		if (Arr.IsValidIndex(A) && Arr.IsValidIndex(B))
		{
			Arr.Swap(A, B);
			return true;
		}
		return false;
	}
}
