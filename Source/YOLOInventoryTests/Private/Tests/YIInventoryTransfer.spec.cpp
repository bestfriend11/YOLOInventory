#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GameplayTagsManager.h"
#include "UObject/Package.h"
#include "YIInventoryBag.h"
#include "YIInventoryBlueprintLibrary.h"
#include "YIItemDefinition.h"
#include "YIItemFragments.h"
#include "YIGridContainer.h"

namespace InventoryTransferHelpers
{
	static UYIItemDefinition* MakeDef(const FIntPoint Size = FIntPoint(1,1), bool bAllowRotation = true, bool bAllowStacking = true, int32 MaxStack = 99, bool bUniquePerType = false, const FGameplayTag ItemType = FGameplayTag())
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
		Def->ItemType = ItemType;
		return Def;
	}

	static FYIBagItem MakeBagItem(UYIItemDefinition* Def, int32 Count, const FIntPoint Pos)
	{
		FYIBagItem I;
		I.Item.Definition = Def;
		I.Item.Count = Count;
		I.Pos = Pos;
		I.Size = Def ? Def->GetEffectiveDefaultSize() : FIntPoint(1,1);
		return I;
	}

	static FYIItemInstance MakeInstance(UYIItemDefinition* Def, int32 Count = 1, bool bRotated = false)
	{
		FYIItemInstance Inst;
		Inst.Definition = Def;
		Inst.Count = Count;
		Inst.bRotated = bRotated;
		return Inst;
	}
}

BEGIN_DEFINE_SPEC(FYIInventoryTransferSpec, "YOLOInventory.Transfer", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
END_DEFINE_SPEC(FYIInventoryTransferSpec)

void FYIInventoryTransferSpec::Define()
{
	It("transfers between bags with merge, partial moves, and no loss", [this]()
	{
		using namespace InventoryTransferHelpers;
		UYIInventoryBag* Src = NewObject<UYIInventoryBag>(GetTransientPackage());
		UYIInventoryBag* Dest = NewObject<UYIInventoryBag>(GetTransientPackage());
		Src->GridSize = Dest->GridSize = FIntPoint(5,5);

		UYIItemDefinition* Def = MakeDef(FIntPoint(1,1), true, true, 5);
		// Dest already has 2
		const int32 DestExisting = Dest->AddBagItem(MakeBagItem(Def, 2, FIntPoint(0,0)));
		const int32 SrcIdx = Src->AddBagItem(MakeBagItem(Def, 3, FIntPoint(0,0)));
		TestTrue(TEXT("Setup src/dest items"), DestExisting != INDEX_NONE && SrcIdx != INDEX_NONE);

		int32 OutDest = INDEX_NONE;
		// Full move (Count <=0 means move all)
		bool bOk = UYIInventoryBlueprintLibrary::TransferItemBetweenBags(Src, Dest, SrcIdx, 0, OutDest);
		TestTrue(TEXT("Full transfer succeeded"), bOk);
		TestEqual(TEXT("Dest index valid"), OutDest, DestExisting);
		TestEqual(TEXT("Merged count capped"), Dest->Items[DestExisting].Item.Count, 5);
		TestEqual(TEXT("Source removed after move"), Src->Items.Num(), 0);

		// Partial transfer leaves remainder
		const int32 SrcIdx2 = Src->AddBagItem(MakeBagItem(Def, 5, FIntPoint(1,1)));
		TestTrue(TEXT("Re-add src stack"), SrcIdx2 != INDEX_NONE);
		OutDest = INDEX_NONE;
		bOk = UYIInventoryBlueprintLibrary::TransferItemBetweenBags(Src, Dest, SrcIdx2, 2, OutDest);
		TestTrue(TEXT("Partial transfer succeeded"), bOk);
		TestTrue(TEXT("Dest index valid"), OutDest != INDEX_NONE);
		TestEqual(TEXT("Partial removed from source"), Src->Items[SrcIdx2].Item.Count, 3);
	});

	It("blocks transfers when destination has no space or unique-type conflict", [this]()
	{
		using namespace InventoryTransferHelpers;
		UYIInventoryBag* Src = NewObject<UYIInventoryBag>(GetTransientPackage());
		UYIInventoryBag* Dest = NewObject<UYIInventoryBag>(GetTransientPackage());
		Src->GridSize = Dest->GridSize = FIntPoint(2,2);

		UYIItemDefinition* BigDef = MakeDef(FIntPoint(2,2), true, false, 1);
		const int32 DestIdx = Dest->AddBagItem(MakeBagItem(BigDef, 1, FIntPoint(0,0))); // fills dest
		const int32 SrcIdx = Src->AddBagItem(MakeBagItem(BigDef, 1, FIntPoint(0,0)));
		TestTrue(TEXT("Setup big items"), DestIdx != INDEX_NONE && SrcIdx != INDEX_NONE);

		int32 OutDest = INDEX_NONE;
		bool bOk = UYIInventoryBlueprintLibrary::TransferItemBetweenBags(Src, Dest, SrcIdx, 0, OutDest);
		TestFalse(TEXT("Transfer blocked by lack of space"), bOk);
		TestEqual(TEXT("Source item untouched"), Src->Items[SrcIdx].Item.Count, 1);

		// Unique per type conflict
		FGameplayTag UniqueTag = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("YOLOInventory.Test.UniqueType"));
		UYIItemDefinition* UniqueDefA = MakeDef(FIntPoint(1,1), true, true, 5, true, UniqueTag);
		UYIItemDefinition* UniqueDefB = MakeDef(FIntPoint(1,1), true, true, 5, true, UniqueTag);
		Dest->Items.Reset(); Src->Items.Reset();
		const int32 DestUnique = Dest->AddBagItem(MakeBagItem(UniqueDefA, 1, FIntPoint(0,0)));
		const int32 SrcUnique = Src->AddBagItem(MakeBagItem(UniqueDefB, 1, FIntPoint(1,0)));
		TestTrue(TEXT("Setup unique items"), DestUnique != INDEX_NONE && SrcUnique != INDEX_NONE);
		OutDest = INDEX_NONE;
		bOk = UYIInventoryBlueprintLibrary::TransferItemBetweenBags(Src, Dest, SrcUnique, 0, OutDest);
		TestFalse(TEXT("Transfer blocked by unique type"), bOk);
		TestEqual(TEXT("Source unique still present"), Src->Items.Num(), 1);
	});

	It("honors destination acceptance rules without losing source items", [this]()
	{
		using namespace InventoryTransferHelpers;
		UYIInventoryBag* Src = NewObject<UYIInventoryBag>(GetTransientPackage());
		UYIInventoryBag* Dest = NewObject<UYIInventoryBag>(GetTransientPackage());
		Src->GridSize = Dest->GridSize = FIntPoint(4,4);

		const FGameplayTag AllowedTag = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("YOLOInventory.Test.Accepted"));
		const FGameplayTag BlockedTag = UGameplayTagsManager::Get().AddNativeGameplayTag(FName("YOLOInventory.Test.Blocked"));

		Dest->bEnforceAcceptanceRules = true;
		Dest->AllowedItemTypes.AddTag(AllowedTag);

		UYIItemDefinition* Def = MakeDef(FIntPoint(1,1), true, true, 20, false, BlockedTag);
		const int32 SrcIdx = Src->AddBagItem(MakeBagItem(Def, 7, FIntPoint(0,0)));
		TestTrue(TEXT("Setup source item"), SrcIdx != INDEX_NONE);

		int32 OutDest = INDEX_NONE;
		const bool bOk = UYIInventoryBlueprintLibrary::TransferItemBetweenBags(Src, Dest, SrcIdx, 3, OutDest);
		TestFalse(TEXT("Transfer blocked by acceptance rules"), bOk);
		TestEqual(TEXT("Destination untouched"), Dest->Items.Num(), 0);
		TestEqual(TEXT("Source stack count unchanged"), Src->Items[SrcIdx].Item.Count, 7);
	});

	It("supports grid container add/transfer without loss", [this]()
	{
		using namespace InventoryTransferHelpers;
		UYIGridContainer* A = NewObject<UYIGridContainer>(GetTransientPackage());
		UYIGridContainer* B = NewObject<UYIGridContainer>(GetTransientPackage());
		A->GridSize = B->GridSize = FIntPoint(3,3);

		UYIItemDefinition* StackDef = MakeDef(FIntPoint(1,1), true, true, 4);
		FYIItemInstance Inst = MakeInstance(StackDef, 5); // will split 4+1
		TestTrue(TEXT("AddItem merges and splits within capacity"), A->AddItem_Implementation(Inst));
		TestEqual(TEXT("Container A stack count"), A->Items.Num(), 2); // one full (4) one remainder (1)
		int32 FullIdx = (A->Items[0].Instance.Count == 4) ? 0 : 1;
		int32 PartialIdx = 1 - FullIdx;
		TestEqual(TEXT("Full stack is 4"), A->Items[FullIdx].Instance.Count, 4);
		TestEqual(TEXT("Remainder is 1"), A->Items[PartialIdx].Instance.Count, 1);

		// Transfer part of full stack to B (count=2)
		const FGuid MoveId = A->Items[FullIdx].Instance.InstanceId;
		const int32 BeforeCount = A->Items[FullIdx].Instance.Count;
		TScriptInterface<IYIContainerInterface> BIntf; BIntf.SetObject(B); BIntf.SetInterface(Cast<IYIContainerInterface>(B));
		TScriptInterface<IYIContainerInterface> AIntf; AIntf.SetObject(A); AIntf.SetInterface(Cast<IYIContainerInterface>(A));
		bool bOk = A->TransferTo_Implementation(BIntf, MoveId, 2);
		TestTrue(TEXT("TransferTo succeeded"), bOk);
		TestEqual(TEXT("Source reduced by 2"), A->Items[FullIdx].Instance.Count, BeforeCount - 2);
		TestEqual(TEXT("Dest has one entry"), B->Items.Num(), 1);
		TestEqual(TEXT("Dest count equals moved amount"), B->Items[0].Instance.Count, 2);

		// Transfer fails when destination cannot fit
		// Clear previous entries so the big item can actually occupy the whole grid
		B->Items.Reset();
		UYIItemDefinition* BigDef = MakeDef(FIntPoint(3,3), true, false, 1);
		FYIItemInstance BigInst = MakeInstance(BigDef, 1);
		TestTrue(TEXT("Place big in B fills grid"), B->AddItem_Implementation(BigInst));
		FYIItemInstance Another = MakeInstance(BigDef, 1);
		const int32 BeforeSrcCount = A->Items[PartialIdx].Instance.Count;
		bOk = A->TransferTo_Implementation(BIntf, A->Items[PartialIdx].Instance.InstanceId, 1);
		TestFalse(TEXT("Transfer blocked (no space)"), bOk);
		TestEqual(TEXT("Source unchanged on failed transfer"), A->Items[PartialIdx].Instance.Count, BeforeSrcCount);
	});
}

#endif // WITH_AUTOMATION_TESTS
