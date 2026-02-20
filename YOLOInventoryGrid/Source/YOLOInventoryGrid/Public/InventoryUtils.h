#pragma once
#include "CoreMinimal.h"

// Small utility helpers for YOLOInventory
FORCEINLINE bool RectsOverlap(const FIntPoint& APos, const FIntPoint& ASize, const FIntPoint& BPos, const FIntPoint& BSize)
{
    return !(APos.X + ASize.X <= BPos.X || BPos.X + BSize.X <= APos.X ||
             APos.Y + ASize.Y <= BPos.Y || BPos.Y + BSize.Y <= APos.Y);
}