#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/Package.h"
#include "InventoryGridWidget.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"

namespace GridTestHelpers
{
	static UYIItemDefinition* MakeDef(const FIntPoint Size = FIntPoint(1,1))
	{
		UYIItemDefinition* Def = NewObject<UYIItemDefinition>(GetTransientPackage());
		Def->DefaultSize = Size;
		return Def;
	}

	static FYIBagItem MakeBagItem(UYIItemDefinition* Def, int32 Count, const FIntPoint Pos)
	{
		FYIBagItem Item;
		Item.Item.Definition = Def;
		Item.Item.Count = Count;
		Item.Pos = Pos;
		Item.Size = Def ? Def->DefaultSize : FIntPoint(1,1);
		return Item;
	}
}

BEGIN_DEFINE_SPEC(FYIInventoryGridWidgetSpec, "YOLOInventory.UI.GridWidget", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
END_DEFINE_SPEC(FYIInventoryGridWidgetSpec)

void FYIInventoryGridWidgetSpec::Define()
{
	It("moves drag within same bag and places at target cell", [this]()
	{
		using namespace GridTestHelpers;
		UInventoryGridWidget* Grid = NewObject<UInventoryGridWidget>(GetTransientPackage());
		Grid->Bag = NewObject<UYIInventoryBag>(GetTransientPackage());
		Grid->Bag->GridSize = FIntPoint(3,3);
		Grid->Bag->Items.Add(MakeBagItem(MakeDef(), 1, FIntPoint(0,0)));

		TestTrue(TEXT("Begin drag succeeds"), Grid->BeginDragFromCell(FIntPoint(0,0)));
		TestTrue(TEXT("Drag marked active"), Grid->IsItemDragActive());
		TestEqual(TEXT("Bag item removed during drag"), Grid->Bag->Items.Num(), 0);

		TestTrue(TEXT("Drop onto empty cell succeeds"), Grid->DropDraggedItemAtCell(FIntPoint(1,1)));
		TestFalse(TEXT("Drag reset after drop"), Grid->IsItemDragActive());
		TestEqual(TEXT("Bag has one item after drop"), Grid->Bag->Items.Num(), 1);
		if (Grid->Bag->Items.Num() == 1)
		{
			TestEqual(TEXT("Item placed at target cell"), Grid->Bag->Items[0].Pos, FIntPoint(1,1));
		}
	});

	It("displaces a single victim and continues dragging it without loss", [this]()
	{
		using namespace GridTestHelpers;
		UInventoryGridWidget* Grid = NewObject<UInventoryGridWidget>(GetTransientPackage());
		Grid->Bag = NewObject<UYIInventoryBag>(GetTransientPackage());
		Grid->Bag->GridSize = FIntPoint(3,3);
		auto* Def = MakeDef();
		Grid->Bag->Items.Add(MakeBagItem(Def, 1, FIntPoint(0,0))); // to drag
		Grid->Bag->Items.Add(MakeBagItem(Def, 1, FIntPoint(1,0))); // victim

		TestTrue(TEXT("Begin drag from first item"), Grid->BeginDragFromCell(FIntPoint(0,0)));
		TestEqual(TEXT("Only victim remains in bag"), Grid->Bag->Items.Num(), 1);

		TestTrue(TEXT("Drop onto victim cell (displace)"), Grid->DropDraggedItemAtCell(FIntPoint(1,0)));
		TestTrue(TEXT("Drag stays active with displaced item"), Grid->IsItemDragActive());
		TestEqual(TEXT("Bag now has dragged item at victim slot"), Grid->Bag->Items[0].Pos, FIntPoint(1,0));

		// Now place displaced victim elsewhere
		TestTrue(TEXT("Place displaced item elsewhere"), Grid->DropDraggedItemAtCell(FIntPoint(2,0)));
		TestFalse(TEXT("Drag reset after placing victim"), Grid->IsItemDragActive());
		TestEqual(TEXT("Bag holds two items again"), Grid->Bag->Items.Num(), 2);
	});

	It("transfers across grids without loss", [this]()
	{
		using namespace GridTestHelpers;
		UInventoryGridWidget* Src = NewObject<UInventoryGridWidget>(GetTransientPackage());
		UInventoryGridWidget* Dest = NewObject<UInventoryGridWidget>(GetTransientPackage());
		Src->Bag = NewObject<UYIInventoryBag>(GetTransientPackage());
		Dest->Bag = NewObject<UYIInventoryBag>(GetTransientPackage());
		Src->Bag->GridSize = Dest->Bag->GridSize = FIntPoint(3,3);
		auto* Def = MakeDef();
		Src->Bag->Items.Add(MakeBagItem(Def, 2, FIntPoint(0,0)));

		TestTrue(TEXT("Start drag on source"), Src->BeginDragFromCell(FIntPoint(0,0)));
		TestTrue(TEXT("Drop into dest grid"), Dest->DropDraggedItemAtCell(FIntPoint(1,1)));
		TestFalse(TEXT("Drag reset after cross-bag drop"), Src->IsItemDragActive());
		TestEqual(TEXT("Source emptied"), Src->Bag->Items.Num(), 0);
		TestEqual(TEXT("Dest received item"), Dest->Bag->Items.Num(), 1);
		if (Dest->Bag->Items.Num() == 1)
		{
			TestEqual(TEXT("Dest item at target cell"), Dest->Bag->Items[0].Pos, FIntPoint(1,1));
			TestEqual(TEXT("Dest count preserved"), Dest->Bag->Items[0].Item.Count, 2);
		}
	});

	It("restores item on cancel when drag removed it from bag", [this]()
	{
		using namespace GridTestHelpers;
		UInventoryGridWidget* Grid = NewObject<UInventoryGridWidget>(GetTransientPackage());
		Grid->Bag = NewObject<UYIInventoryBag>(GetTransientPackage());
		Grid->Bag->GridSize = FIntPoint(2,2);
		auto* Def = MakeDef();
		Grid->Bag->Items.Add(MakeBagItem(Def, 1, FIntPoint(0,0)));

		TestTrue(TEXT("Start drag"), Grid->BeginDragFromCell(FIntPoint(0,0)));
		TestEqual(TEXT("Bag emptied during drag"), Grid->Bag->Items.Num(), 0);
		Grid->CancelDrag();
		TestFalse(TEXT("Drag inactive after cancel"), Grid->IsItemDragActive());
		TestEqual(TEXT("Item restored after cancel"), Grid->Bag->Items.Num(), 1);
		if (Grid->Bag->Items.Num() == 1)
		{
			TestEqual(TEXT("Item restored at original pos when possible"), Grid->Bag->Items[0].Pos, FIntPoint(0,0));
		}
	});
}

#endif // WITH_AUTOMATION_TESTS
