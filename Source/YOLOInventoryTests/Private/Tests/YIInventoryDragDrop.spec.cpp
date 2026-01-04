#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/Package.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "InventoryUtils.h"

// Helper: make a transient item definition with specified size/stacking
static UYIItemDefinition* MakeDragDef(FIntPoint Size, bool bAllowRotation = true, bool bAllowStacking = true, int32 MaxStack = 99)
{
    UYIItemDefinition* Def = NewObject<UYIItemDefinition>(GetTransientPackage());
    Def->DisplayName = FText::FromString(TEXT("TestDef"));
    Def->DefaultSize = Size;
    Def->bAllowRotation = bAllowRotation;
    Def->bAllowStacking = bAllowStacking;
    Def->MaxStackCount = MaxStack;
    return Def;
}

// Helper: add a new stack to a bag at a position for a given def+count
static int32 AddItemToBag(UYIInventoryBag* Bag, UYIItemDefinition* Def, int32 Count, FIntPoint Pos)
{
    FYIBagItem Item;
    Item.Item.Definition = Def; // soft pointer from raw
    Item.Item.Count = Count;
    Item.Pos = Pos;
    Item.Size = Def ? Def->DefaultSize : FIntPoint(1,1);
    return Bag->AddBagItem(Item);
}

BEGIN_DEFINE_SPEC(FYIInventoryDragDropSpec, "YOLOInventory.DragDrop", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
    UYIInventoryBag* BagA = nullptr;
    UYIInventoryBag* BagB = nullptr;
    UYIItemDefinition* Def_2x1 = nullptr;
    UYIItemDefinition* Def_1x2 = nullptr;
END_DEFINE_SPEC(FYIInventoryDragDropSpec)

void FYIInventoryDragDropSpec::Define()
{
    BeforeEach([this]()
    {
        BagA = NewObject<UYIInventoryBag>(GetTransientPackage());
        BagA->GridSize = FIntPoint(5,5);
        BagA->bAllowRotation = true;
        BagB = NewObject<UYIInventoryBag>(GetTransientPackage());
        BagB->GridSize = FIntPoint(5,5);
        BagB->bAllowRotation = true;

        Def_2x1 = MakeDragDef(FIntPoint(2,1), true);
        Def_1x2 = MakeDragDef(FIntPoint(1,2), true);
    });

    It("moves items within the same bag to valid empty cells and blocks overlaps/out-of-bounds", [this]()
    {
        // Place two items
        const int32 IdxA = AddItemToBag(BagA, Def_2x1, 1, FIntPoint(0,0));
        const int32 IdxB = AddItemToBag(BagA, Def_1x2, 1, FIntPoint(3,0));
        TestTrue(TEXT("IdxA valid"), IdxA != INDEX_NONE);
        TestTrue(TEXT("IdxB valid"), IdxB != INDEX_NONE);

        // Valid move: move A to (0,2)
        bool bMoved = BagA->MoveItem(IdxA, FIntPoint(0,2));
        TestTrue(TEXT("A moved to empty cell"), bMoved);
        if (ensure(BagA->Items.IsValidIndex(IdxA)))
        {
            TestEqual(TEXT("A new Pos"), BagA->Items[IdxA].Pos, FIntPoint(0,2));
        }

        // Invalid move: into overlap with B
        bMoved = BagA->MoveItem(IdxA, FIntPoint(3,0));
        TestFalse(TEXT("Move blocked due to overlap"), bMoved);

        // Invalid move: out of bounds
        bMoved = BagA->MoveItem(IdxA, FIntPoint(4,4)); // 2x1 won't fit
        TestFalse(TEXT("Move blocked out-of-bounds"), bMoved);
    });

    It("transfers items across bags by remove+add semantics (simulating drag between bags)", [this]()
    {
        const int32 SrcIdx = AddItemToBag(BagA, Def_2x1, 1, FIntPoint(0,0));
        TestTrue(TEXT("Src index valid"), SrcIdx != INDEX_NONE);
        const int32 DestIdxBefore = BagB->Items.Num();

        // Simulate drag-drop: attempt to add to B first, then remove from A if succeeded
        FYIBagItem Copy = BagA->Items[SrcIdx];
        // Choose a target position in B (first-fit if needed)
        if (!BagB->CanPlaceAt(Copy.Pos, Copy.Size))
        {
            FIntPoint Fit;
            const bool bFound = BagB->FindFirstFit(Copy.Size, Fit);
            TestTrue(TEXT("Found first-fit in dest"), bFound);
            Copy.Pos = Fit;
        }
        const int32 NewIdx = BagB->AddBagItem(Copy);
        TestTrue(TEXT("Item added to dest"), NewIdx != INDEX_NONE);
        const bool bRemoved = BagA->RemoveItem(SrcIdx);
        TestTrue(TEXT("Source removed after transfer"), bRemoved);

        // Validate counts
        TestEqual(TEXT("Dest gained one"), BagB->Items.Num(), DestIdxBefore + 1);
        TestEqual(TEXT("Source now empty"), BagA->Items.Num(), 0);
    });

    It("supports repeated alternating moves of two items without corrupting layout", [this]()
    {
        const int32 A = AddItemToBag(BagA, Def_2x1, 1, FIntPoint(0,0));
        const int32 B = AddItemToBag(BagA, Def_1x2, 1, FIntPoint(2,0));
        TestTrue(TEXT("A valid"), A != INDEX_NONE);
        TestTrue(TEXT("B valid"), B != INDEX_NONE);

        // Sequence of valid moves that swap regions without overlap
        const TArray<FIntPoint> TargetsA = { FIntPoint(0,2), FIntPoint(3,3), FIntPoint(0,0) };
        const TArray<FIntPoint> TargetsB = { FIntPoint(2,2), FIntPoint(0,3), FIntPoint(2,0) };

        for (int32 i=0;i<TargetsA.Num();++i)
        {
            // Move A then B; ensure each result is valid
            TestTrue(FString::Printf(TEXT("Move A #%d"), i), BagA->MoveItem(A, TargetsA[i]));
            TestTrue(FString::Printf(TEXT("Move B #%d"), i), BagA->MoveItem(B, TargetsB[i]));

            // Validate no overlaps after each pair
            const FYIBagItem& IA = BagA->Items[A];
            const FYIBagItem& IB = BagA->Items[B];
            const FIntPoint EffA = BagA->GetEffectiveSize(IA.Size);
            const FIntPoint EffB = BagA->GetEffectiveSize(IB.Size);
            const bool bOverlap = RectsOverlap(IA.Pos, EffA, IB.Pos, EffB);
            TestFalse(TEXT("No overlap after moves"), bOverlap);
        }
    });

    AfterEach([this]()
    {
        BagA = nullptr;
        BagB = nullptr;
        Def_2x1 = nullptr;
        Def_1x2 = nullptr;
    });
}

#endif // WITH_AUTOMATION_TESTS
