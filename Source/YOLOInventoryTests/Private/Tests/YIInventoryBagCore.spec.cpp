#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/Package.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "YIItemFragments.h"
#include "InventoryUtils.h"

namespace BagCoreHelpers
{
// Helpers (prefixed to avoid Unity merge collisions)
static UYIItemDefinition* MakeDefCore(const FIntPoint Size, bool bAllowRotation = true, bool bAllowStacking = true, int32 MaxStack = 99, bool bUniquePerType = false)
{
	UYIItemDefinition* Def = NewObject<UYIItemDefinition>(GetTransientPackage());
	if (FInstancedStruct* LayoutStruct = Def->FindOrAddDefinitionFragmentByStruct(FYIItemLayoutDefinitionFragment::StaticStruct()))
	{
		if (FYIItemLayoutDefinitionFragment* Layout = LayoutStruct->GetMutablePtr<FYIItemLayoutDefinitionFragment>())
		{
			Layout->DefaultSize = Size;
			Layout->bAllowRotation = bAllowRotation;
		}
	}
	if (FInstancedStruct* StackingStruct = Def->FindOrAddDefinitionFragmentByStruct(FYIItemStackingDefinitionFragment::StaticStruct()))
	{
		if (FYIItemStackingDefinitionFragment* Stacking = StackingStruct->GetMutablePtr<FYIItemStackingDefinitionFragment>())
		{
			Stacking->bAllowStacking = bAllowStacking;
			Stacking->MaxStackCount = MaxStack;
		}
	}
	Def->bUniquePerType = bUniquePerType;
	return Def;
}

static FYIBagItem MakeItemCore(UYIItemDefinition* Def, const FIntPoint Pos, int32 Count = 1, int64 CustomStackKey = 0)
{
	FYIBagItem Item;
	Item.Item.Definition = Def;
	Item.Item.Count = Count;
	Item.Item.CustomStackKey = CustomStackKey;
	Item.Pos = Pos;
	Item.Size = Def ? Def->GetEffectiveDefaultSize() : FIntPoint(1,1);
	return Item;
}
}

BEGIN_DEFINE_SPEC(FYIInventoryBagCoreSpec, "YOLOInventory.Bag.Core", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
END_DEFINE_SPEC(FYIInventoryBagCoreSpec)

void FYIInventoryBagCoreSpec::Define()
{
	It("computes effective sizes with clamping and rounding", [this]()
	{
		UYIInventoryBag* Bag = NewObject<UYIInventoryBag>(GetTransientPackage());
		const FIntPoint Base(3,2);

		Bag->MinifyScale = 0.4f; // clamps to >=0.1 and rounds to at least 1x1
		TestEqual(TEXT("Scale 0.4 rounds down but min 1"), Bag->GetEffectiveSize(Base), FIntPoint(1,1));

		Bag->MinifyScale = 0.8f;
		TestEqual(TEXT("Scale 0.8 rounds halves"), Bag->GetEffectiveSize(Base), FIntPoint(2,2));

		Bag->MinifyScale = 2.5f; // clamps to 1.0
		TestEqual(TEXT("Scale clamps to 1.0"), Bag->GetEffectiveSize(Base), Base);
	});

	It("validates placement, overlap, and list mode", [this]()
	{
		UYIInventoryBag* Bag = NewObject<UYIInventoryBag>(GetTransientPackage());
		Bag->GridSize = FIntPoint(4,4);

		using namespace BagCoreHelpers;

		Bag->Items.Add(MakeItemCore(MakeDefCore(FIntPoint(2,2)), FIntPoint(0,0))); // occupies (0,0)-(2,2)
		TestFalse(TEXT("Rejects out of bounds"), Bag->CanPlaceAt(FIntPoint(3,3), FIntPoint(2,2)));
		TestFalse(TEXT("Rejects overlap"), Bag->CanPlaceAt(FIntPoint(1,1), FIntPoint(2,2)));
		TestTrue(TEXT("Accepts free spot"), Bag->CanPlaceAt(FIntPoint(2,2), FIntPoint(2,2)));

		Bag->MinifyScale = 0.5f; // makes 2x2 items effectively 1x1
		TestTrue(TEXT("Effective size shrink enables placement"), Bag->CanPlaceAt(FIntPoint(1,1), FIntPoint(2,2)));

		// List mode ignores spatial rules
		Bag->GridSize = FIntPoint(0,0);
		TestTrue(TEXT("List mode ignores bounds/overlap"), Bag->CanPlaceAt(FIntPoint(100,100), FIntPoint(99,99)));
	});

	It("supports CanPlaceAtIgnoring and MoveItem respecting overlaps", [this]()
	{
		UYIInventoryBag* Bag = NewObject<UYIInventoryBag>(GetTransientPackage());
		Bag->GridSize = FIntPoint(4,4);
		using namespace BagCoreHelpers;
		Bag->Items.Add(MakeItemCore(MakeDefCore(FIntPoint(2,2)), FIntPoint(0,0))); // Index 0
		Bag->Items.Add(MakeItemCore(MakeDefCore(FIntPoint(2,2)), FIntPoint(2,0))); // Index 1

		TestTrue(TEXT("Ignoring self allows placement"), Bag->CanPlaceAtIgnoring(FIntPoint(0,0), FIntPoint(2,2), 0));
		TestFalse(TEXT("Cannot place overlapping other"), Bag->CanPlaceAtIgnoring(FIntPoint(1,0), FIntPoint(2,2), 0));

		// Move into occupied space should fail
		TestFalse(TEXT("Move blocked by overlap"), Bag->MoveItem(0, FIntPoint(2,0)));
		TestTrue(TEXT("Move into free space succeeds"), Bag->MoveItem(0, FIntPoint(0,2)));
	});

	It("rotates items only when allowed and fitting", [this]()
	{
		UYIInventoryBag* Bag = NewObject<UYIInventoryBag>(GetTransientPackage());
		Bag->GridSize = FIntPoint(3,3);

		using namespace BagCoreHelpers;

		UYIItemDefinition* NoRotateDef = MakeDefCore(FIntPoint(2,1), false /*bAllowRotation*/);
		Bag->Items.Add(MakeItemCore(NoRotateDef, FIntPoint(0,0)));
		TestFalse(TEXT("Definition forbids rotation"), Bag->RotateItem(0));

		UYIItemDefinition* RotDef = MakeDefCore(FIntPoint(2,1), true);
		Bag->Items[0] = MakeItemCore(RotDef, FIntPoint(1,1)); // place near edge
		TestTrue(TEXT("Rotation fits inside grid"), Bag->RotateItem(0));
		TestEqual(TEXT("Size swapped"), Bag->Items[0].Size, FIntPoint(1,2));

		// Place another item to block rotation
		Bag->Items.Add(MakeItemCore(RotDef, FIntPoint(0,1))); // overlap target area after rotation back
		TestFalse(TEXT("Blocked rotation due to overlap"), Bag->RotateItem(0));
	});

	It("merges, respects max stack, and enforces unique-per-type", [this]()
	{
		UYIInventoryBag* Bag = NewObject<UYIInventoryBag>(GetTransientPackage());
		Bag->GridSize = FIntPoint(5,5);
		using namespace BagCoreHelpers;
		UYIItemDefinition* StackDef = MakeDefCore(FIntPoint(1,1), true, true, 5);

		const int32 First = Bag->AddBagItem(MakeItemCore(StackDef, FIntPoint(0,0), 3));
		TestTrue(TEXT("First stack added"), First != INDEX_NONE);
		const int32 MergeIdx = Bag->AddBagItem(MakeItemCore(StackDef, FIntPoint(1,0), 4));
		TestEqual(TEXT("Merged into existing stack"), MergeIdx, First);
		TestEqual(TEXT("Stack clamped to max"), Bag->Items[First].Item.Count, 5);

		// Unique per type: second add should be rejected
		Bag->Items.Reset();
		UYIItemDefinition* UniqueDefA = MakeDefCore(FIntPoint(1,1), true, true, 5, true);
		UniqueDefA->ItemType = FGameplayTag::RequestGameplayTag(TEXT("YOLOInventory.Test.Unique"), false);
		const int32 UniqueFirst = Bag->AddBagItem(MakeItemCore(UniqueDefA, FIntPoint(2,0), 1));
		TestTrue(TEXT("Unique item added"), UniqueFirst != INDEX_NONE);

		UYIItemDefinition* UniqueDefB = MakeDefCore(FIntPoint(1,1), true, true, 5, true);
		UniqueDefB->ItemType = UniqueDefA->ItemType; // same type should block
		const int32 UniqueSecond = Bag->AddBagItem(MakeItemCore(UniqueDefB, FIntPoint(3,0), 1));
		TestEqual(TEXT("Second of same type rejected"), UniqueSecond, INDEX_NONE);
	});

	It("splits and combines stacks with removal when empty", [this]()
	{
		UYIInventoryBag* Bag = NewObject<UYIInventoryBag>(GetTransientPackage());
		Bag->GridSize = FIntPoint(4,4);
		using namespace BagCoreHelpers;
		UYIItemDefinition* Def = MakeDefCore(FIntPoint(1,1), true, true, 5);

		const int32 Stack = Bag->AddBagItem(MakeItemCore(Def, FIntPoint(0,0), 5));
		TestTrue(TEXT("Stack added"), Stack != INDEX_NONE);

		const int32 Split = Bag->SplitStack(Stack, 2, FIntPoint(1,0));
		TestTrue(TEXT("Split created new stack"), Split != INDEX_NONE);
		TestEqual(TEXT("Source count reduced"), Bag->Items[Stack].Item.Count, 3);
		TestEqual(TEXT("New stack count matches split"), Bag->Items[Split].Item.Count, 2);

		// Invalid split (too much)
		const int32 BadSplit = Bag->SplitStack(Stack, 99, FIntPoint(2,0));
		TestEqual(TEXT("Split blocked when exceeding count"), BadSplit, INDEX_NONE);

		// Combine should remove emptied stack
		Bag->Items[Split].Item.Count = 1;
		Bag->Items[Stack].Item.Count = 4; // room for 1
		TestTrue(TEXT("Combine consumes and removes"), Bag->CombineStacks(Stack, Split));
		TestEqual(TEXT("Combined count capped"), Bag->Items[Stack].Item.Count, 5);
		TestTrue(TEXT("Removed stack no longer valid"), !Bag->Items.IsValidIndex(Split));
	});

	It("drops items when minify scale increases and keeps non-overlapping", [this]()
	{
		UYIInventoryBag* Bag = NewObject<UYIInventoryBag>(GetTransientPackage());
		Bag->GridSize = FIntPoint(4,4);
		Bag->MinifyScale = 0.5f; // start shrunk to allow tight placement
		TArray<FYIBagItem> Dropped;

		using namespace BagCoreHelpers;

		UYIItemDefinition* LargeDef = MakeDefCore(FIntPoint(4,4));
		Bag->Items.Add(MakeItemCore(LargeDef, FIntPoint(0,0)));            // fits at scale 0.5 (effective 2x2)
		Bag->Items.Add(MakeItemCore(LargeDef, FIntPoint(2,2)));            // also fits at scale 0.5

		Bag->ApplyMinifyScale(1.0f, Dropped); // expand sizes; second should be out of bounds and dropped

		TestEqual(TEXT("One item kept"), Bag->Items.Num(), 1);
		TestEqual(TEXT("One item dropped"), Dropped.Num(), 1);
		TestEqual(TEXT("Kept item at original slot"), Bag->Items[0].Pos, FIntPoint(0,0));
	});

	It("finds fits in grid and list mode", [this]()
	{
		UYIInventoryBag* Bag = NewObject<UYIInventoryBag>(GetTransientPackage());
		FIntPoint Out;

		Bag->GridSize = FIntPoint(3,3);
		using namespace BagCoreHelpers;
		Bag->Items.Add(MakeItemCore(MakeDefCore(FIntPoint(2,2)), FIntPoint(0,0)));
		TestTrue(TEXT("Find free cell"), Bag->FindFirstFit(FIntPoint(1,1), Out));
		TestEqual(TEXT("Free cell position"), Out, FIntPoint(2,0));

		Bag->GridSize = FIntPoint(0,0); // list mode
		Bag->Items.Reset();
		TestTrue(TEXT("List mode always fits"), Bag->FindFirstFit(FIntPoint(5,5), Out));
		TestEqual(TEXT("List mode position uses count"), Out, FIntPoint(0,0));
		Bag->Items.Add(BagCoreHelpers::MakeItemCore(BagCoreHelpers::MakeDefCore(FIntPoint(1,1)), FIntPoint(0,0)));
		TestTrue(TEXT("List mode count increments position"), Bag->FindFirstFit(FIntPoint(1,1), Out));
		TestEqual(TEXT("List mode next row index"), Out, FIntPoint(0,1));
	});

	It("auto-packs items largest-first to reduce fragmentation", [this]()
	{
		UYIInventoryBag* Bag = NewObject<UYIInventoryBag>(GetTransientPackage());
		Bag->GridSize = FIntPoint(3,3);
		using namespace BagCoreHelpers;
		UYIItemDefinition* One = MakeDefCore(FIntPoint(1,1));
		UYIItemDefinition* Two = MakeDefCore(FIntPoint(2,1));
		UYIItemDefinition* Three = MakeDefCore(FIntPoint(2,2));

		// Add in scrambled order; expect largest (2x2) placed first at origin, then 2x1, then 1x1
		Bag->Items.Add(MakeItemCore(One, FIntPoint(2,2)));
		Bag->Items.Add(MakeItemCore(Two, FIntPoint(0,2)));
		Bag->Items.Add(MakeItemCore(Three, FIntPoint(1,1)));

		Bag->AutoPack();

		TestEqual(TEXT("Largest (2x2) packed at origin"), Bag->Items[0].Pos, FIntPoint(0,0));
		TestEqual(TEXT("Largest size"), Bag->Items[0].Size, Three->GetEffectiveDefaultSize());
		// With first-fit after sorting, 2x1 drops to row 2, then 1x1 fills earliest free (2,0)
		TestEqual(TEXT("Next (2x1) packed row-major after big"), Bag->Items[1].Pos, FIntPoint(0,2));
		TestEqual(TEXT("Smallest (1x1) fills earliest free slot"), Bag->Items[2].Pos, FIntPoint(2,0));

		// Confirm no overlaps using effective sizes
		const FIntPoint Eff0 = Bag->GetEffectiveSize(Bag->Items[0].Size);
		const FIntPoint Eff1 = Bag->GetEffectiveSize(Bag->Items[1].Size);
		const FIntPoint Eff2 = Bag->GetEffectiveSize(Bag->Items[2].Size);
		TestFalse(TEXT("0 vs 1 overlap"), RectsOverlap(Bag->Items[0].Pos, Eff0, Bag->Items[1].Pos, Eff1));
		TestFalse(TEXT("0 vs 2 overlap"), RectsOverlap(Bag->Items[0].Pos, Eff0, Bag->Items[2].Pos, Eff2));
		TestFalse(TEXT("1 vs 2 overlap"), RectsOverlap(Bag->Items[1].Pos, Eff1, Bag->Items[2].Pos, Eff2));
	});

	It("validates RectsOverlap utility edges", [this]()
	{
		TestFalse(TEXT("Touching edges not overlap"), RectsOverlap(FIntPoint(0,0), FIntPoint(1,1), FIntPoint(1,0), FIntPoint(1,1)));
		TestTrue(TEXT("Proper overlap"), RectsOverlap(FIntPoint(0,0), FIntPoint(2,2), FIntPoint(1,1), FIntPoint(2,2)));
		TestFalse(TEXT("Separated rectangles"), RectsOverlap(FIntPoint(0,0), FIntPoint(1,1), FIntPoint(2,2), FIntPoint(1,1)));
	});
}

#endif // WITH_AUTOMATION_TESTS
