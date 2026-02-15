#include "YIEquipmentSchemaAsset.h"

#include "Algo/Sort.h"

void UYIEquipmentSchemaAsset::SortSlotDefinitions()
{
	Algo::Sort(SlotDefinitions, [](const FYIEquipmentSlotDefinition& A, const FYIEquipmentSlotDefinition& B)
	{
		if (!A.SlotTag.IsValid() || !B.SlotTag.IsValid())
		{
			return A.SlotTag.IsValid();
		}
		return A.SlotTag.ToString() < B.SlotTag.ToString();
	});
}

