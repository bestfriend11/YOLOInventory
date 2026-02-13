#include "YIEquipmentLayoutAsset.h"

namespace
{
static bool YIEquipmentLayout_EntryLess(const FYIEquipmentSlotLayoutEntry& A, const FYIEquipmentSlotLayoutEntry& B)
{
	if (A.SortOrder != B.SortOrder)
	{
		return A.SortOrder < B.SortOrder;
	}

	if (A.Row != B.Row)
	{
		return A.Row < B.Row;
	}

	if (A.Column != B.Column)
	{
		return A.Column < B.Column;
	}

	return A.SlotTag.ToString() < B.SlotTag.ToString();
}
}

void UYIEquipmentLayoutAsset::SortSlotsByOrder()
{
	Slots.Sort(&YIEquipmentLayout_EntryLess);
}

void UYIEquipmentLayoutAsset::GetSortedSlots(TArray<FYIEquipmentSlotLayoutEntry>& OutSlots) const
{
	OutSlots = Slots;
	OutSlots.Sort(&YIEquipmentLayout_EntryLess);
}

