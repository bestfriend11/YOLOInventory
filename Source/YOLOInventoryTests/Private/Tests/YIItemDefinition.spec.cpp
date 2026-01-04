#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GameplayTagsManager.h"
#include "UObject/Package.h"
#include "YIItemDefinition.h"
#include "YIInventoryBag.h"
#include "YIInventoryBlueprintLibrary.h"
#include "YIRarityPalette.h"
#include "YIInventoryTypes.h"

// Local helpers
namespace ItemDefHelpers
{
	static UYIItemDefinition* MakeDef(const FIntPoint Size = FIntPoint(1,1), bool bAllowRotation = true, bool bAllowStacking = true, int32 MaxStack = 99)
	{
		UYIItemDefinition* Def = NewObject<UYIItemDefinition>(GetTransientPackage());
		Def->DefaultSize = Size;
		Def->bAllowRotation = bAllowRotation;
		Def->bAllowStacking = bAllowStacking;
		Def->MaxStackCount = MaxStack;
		return Def;
	}

	static FYIBagItem MakeBagItem(UYIItemDefinition* Def, int32 Count, const FIntPoint Pos)
	{
		FYIBagItem BagItem;
		BagItem.Item.Definition = Def;
		BagItem.Item.Count = Count;
		BagItem.Pos = Pos;
		BagItem.Size = Def ? Def->DefaultSize : FIntPoint(1,1);
		return BagItem;
	}
}

BEGIN_DEFINE_SPEC(FYIItemDefinitionSpec, "YOLOInventory.ItemDefinition", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
END_DEFINE_SPEC(FYIItemDefinitionSpec)

void FYIItemDefinitionSpec::Define()
{
	It("honors defaults and rotation rules", [this]()
	{
		using namespace ItemDefHelpers;
		UYIItemDefinition* Def = NewObject<UYIItemDefinition>(GetTransientPackage());
		TestEqual(TEXT("Default size is 1x1"), Def->DefaultSize, FIntPoint(1,1));
		TestTrue(TEXT("Rotation allowed by default"), Def->bAllowRotation);
		TestTrue(TEXT("Stacking allowed by default"), Def->bAllowStacking);
		TestEqual(TEXT("Default MaxStackCount"), Def->MaxStackCount, 99);

		// Bag respects per-definition rotation rule
		UYIInventoryBag* Bag = NewObject<UYIInventoryBag>(GetTransientPackage());
		Bag->GridSize = FIntPoint(2,2);
		Bag->Items.Add({}); // seed slot 0
		Bag->Items[0].Item.Definition = MakeDef(FIntPoint(2,1), false /*bAllowRotation*/);
		Bag->Items[0].Size = Bag->Items[0].Item.Definition->DefaultSize;
		Bag->Items[0].Pos = FIntPoint(0,0);
		TestFalse(TEXT("Rotation blocked by definition flag"), Bag->RotateItem(0));

		Bag->Items[0].Item.Definition = MakeDef(FIntPoint(2,1), true);
		TestTrue(TEXT("Rotation allowed when enabled"), Bag->RotateItem(0));
	});

	It("obeys stacking rules and max stack clamp", [this]()
	{
		using namespace ItemDefHelpers;
		UYIInventoryBag* Bag = NewObject<UYIInventoryBag>(GetTransientPackage());
		Bag->GridSize = FIntPoint(5,5);

		UYIItemDefinition* NoStackDef = MakeDef(FIntPoint(1,1), true, false /*bAllowStacking*/, 5);
		const int32 A = Bag->AddBagItem(MakeBagItem(NoStackDef, 3, FIntPoint(0,0)));
		const int32 B = Bag->AddBagItem(MakeBagItem(NoStackDef, 4, FIntPoint(1,0)));
		TestTrue(TEXT("First non-stack added"), A != INDEX_NONE);
		TestTrue(TEXT("Second non-stack added separately"), B != INDEX_NONE && B != A);

		UYIItemDefinition* StackDef = MakeDef(FIntPoint(1,1), true, true, 3);
		const int32 S1 = Bag->AddBagItem(MakeBagItem(StackDef, 5, FIntPoint(2,0))); // should clamp to 3
		const int32 S2 = Bag->AddBagItem(MakeBagItem(StackDef, 2, FIntPoint(3,0))); // should merge
		TestTrue(TEXT("Stack added"), S1 != INDEX_NONE);
		TestEqual(TEXT("Stack clamped to MaxStackCount"), Bag->Items[S1].Item.Count, 3);
		TestEqual(TEXT("Merge into existing stack"), S2, S1);
		TestEqual(TEXT("Stack count capped after merge"), Bag->Items[S1].Item.Count, 3);
	});

	It("maps rarity tags via palette override then fallback names", [this]()
	{
		// Install a transient palette at the well-known path
		UPackage* PalPkg = CreatePackage(TEXT("/Game/YOLOInventory/RarityPalette_Default"));
		UYIRarityPalette* Palette = NewObject<UYIRarityPalette>(PalPkg, UYIRarityPalette::StaticClass(), FName(TEXT("RarityPalette_Default")), RF_Public | RF_Standalone);

		FGameplayTag CustomTag = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("YOLOInventory.Test.Rarity"));
		const FLinearColor CustomColor(0.9f, 0.1f, 0.7f, 1.f);
		FRarityPaletteEntry Entry;
		Entry.Tag = CustomTag;
		Entry.DisplayName = FText::FromString(TEXT("Test"));
		Entry.Color = CustomColor;
		Palette->Entries.Add(Entry);

		// Palette should override
		FLinearColor Out = UYIInventoryBlueprintLibrary::GetColorForRarityTag(CustomTag);
		TestTrue(TEXT("Palette override hit"), Out.Equals(CustomColor));

		// Fallback name mapping (ensure tag exists to avoid warnings)
		FGameplayTag CommonTag = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("Rarity.Common"));
		FLinearColor CommonColor = UYIInventoryBlueprintLibrary::GetColorForRarityTag(CommonTag);
		TestTrue(TEXT("Fallback maps Common"), CommonColor.Equals(YI_GetRarityColor(EYOLOItemRarity::Common)));
	});
}

#endif // WITH_AUTOMATION_TESTS
