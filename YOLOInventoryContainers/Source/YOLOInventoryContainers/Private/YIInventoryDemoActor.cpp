#include "YIInventoryDemoActor.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "YIInventoryBlueprintLibrary.h"

AYIInventoryDemoActor::AYIInventoryDemoActor()
{
	PrimaryActorTick.bCanEverTick = false;
	BagA = CreateDefaultSubobject<UYIInventoryBag>(TEXT("BagA"));
	BagB = CreateDefaultSubobject<UYIInventoryBag>(TEXT("BagB"));
}

void AYIInventoryDemoActor::InitializeBags(FIntPoint GridA, FIntPoint GridB)
{
	if (BagA)
	{
		BagA->GridSize = GridA;
		BagA->Items.Reset();
		BagA->MarkPackageDirty(); BagA->OnChanged.Broadcast();
	}
	if (BagB)
	{
		BagB->GridSize = GridB;
		BagB->Items.Reset();
		BagB->MarkPackageDirty(); BagB->OnChanged.Broadcast();
	}
}

int32 AYIInventoryDemoActor::AddItemToBag(UYIInventoryBag* Bag, UYIItemDefinition* Definition, int32 Count)
{
	if (!Bag || !Definition) return INDEX_NONE;
	FIntPoint Pos;
	const FIntPoint ItemSize = Definition->GetEffectiveDefaultSize();
	if (!Bag->FindFirstFit(ItemSize, Pos)) return INDEX_NONE;
	FYIBagItem NewItem;
	NewItem.Item.Definition = TSoftObjectPtr<UYIItemDefinition>(Definition);
	NewItem.Item.Count = FMath::Max(1, Count);
	NewItem.Size = ItemSize;
	NewItem.Pos = Pos;
	return Bag->AddBagItem(NewItem);
}

bool AYIInventoryDemoActor::TransferFromAToB(int32 Index, int32 Count, int32& OutDestIndex)
{
	return UYIInventoryBlueprintLibrary::TransferItemBetweenBags(BagA, BagB, Index, Count, OutDestIndex);
}

bool AYIInventoryDemoActor::TransferFromBToA(int32 Index, int32 Count, int32& OutDestIndex)
{
	return UYIInventoryBlueprintLibrary::TransferItemBetweenBags(BagB, BagA, Index, Count, OutDestIndex);
}

void AYIInventoryDemoActor::AutoPackA()
{
	if (BagA) { BagA->AutoPack(); }
}

void AYIInventoryDemoActor::AutoPackB()
{
	if (BagB) { BagB->AutoPack(); }
}
