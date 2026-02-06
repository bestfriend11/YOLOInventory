#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tests/NetTestHelpers.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "UObject/Package.h"
#include "YIInventoryComponent.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"

#if WITH_EDITOR

BEGIN_DEFINE_SPEC(FYIInventoryReplicationSpec, "YOLOInventory.Net.InventoryReplication", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
END_DEFINE_SPEC(FYIInventoryReplicationSpec)

void FYIInventoryReplicationSpec::Define()
{
	It("replicates bag state to owner and applies client move RPC on server", [this]()
	{
		using namespace UE::Net;

		FTestWorlds Worlds;
		TestTrue(TEXT("Client connected"), Worlds.CreateAndConnectClient());

		UWorld* ServerWorld = Worlds.Server.GetWorld();
		UWorld* ClientWorld = Worlds.Clients.Num() > 0 ? Worlds.Clients[0].GetWorld() : nullptr;
		TestTrue(TEXT("Worlds valid"), ServerWorld && ClientWorld);

		APlayerController* ServerPC = Worlds.GetServerPlayerControllerOfClient(0);
		TestNotNull(TEXT("Server PC"), ServerPC);

		APawn* ServerPawn = ServerWorld->SpawnActor<APawn>(APawn::StaticClass(), FTransform());
		TestNotNull(TEXT("Server pawn"), ServerPawn);
		ServerPawn->SetReplicates(true);
		ServerPawn->SetOwner(ServerPC);
		ServerPC->Possess(ServerPawn);

		UYIInventoryComponent* ServerInv = NewObject<UYIInventoryComponent>(ServerPawn);
		TestNotNull(TEXT("Server inventory component"), ServerInv);
		ServerInv->RegisterComponent();
		ServerInv->SetIsReplicated(true);

		UYIInventoryBag* Bag = ServerInv->CreateBag(TEXT("Bag"), FIntPoint(4, 4));
		ServerInv->OpenBag(Bag);
		TestNotNull(TEXT("Bag created"), Bag);

		UYIItemDefinition* Def = NewObject<UYIItemDefinition>(GetTransientPackage());
		Def->UniqueCode = 1234;
		Def->DefaultSize = FIntPoint(1, 1);
		FYIBagItem Item;
		Item.Item.Definition = Def;
		Item.Item.Count = 1;
		Item.Pos = FIntPoint(0, 0);
		Item.Size = FIntPoint(1, 1);
		const int32 AddedIdx = Bag->AddBagItem(Item);
		TestTrue(TEXT("Server item added"), AddedIdx != INDEX_NONE);
		ServerInv->SyncNetState();

		const bool bClientGotMirror = Worlds.TickAllUntil([&]()
		{
			APawn* ClientPawn = Worlds.FindReplicatedObjectOnClient<APawn>(ServerPawn, 0);
			if (!ClientPawn) return false;
			UYIInventoryComponent* ClientInv = ClientPawn->FindComponentByClass<UYIInventoryComponent>();
			if (!ClientInv) return false;
			UYIInventoryBag* ClientBag = ClientInv->GetBag();
			return ClientBag && ClientBag->Items.Num() == 1;
		}, 0.0166f, 120);
		TestTrue(TEXT("Client received bag mirror"), bClientGotMirror);

		APawn* ClientPawn = Worlds.FindReplicatedObjectOnClient<APawn>(ServerPawn, 0);
		UYIInventoryComponent* ClientInv = ClientPawn ? ClientPawn->FindComponentByClass<UYIInventoryComponent>() : nullptr;
		TestNotNull(TEXT("Client inventory component"), ClientInv);

		ClientInv->MoveItem(0, FIntPoint(1, 0));

		const bool bMoveReplicated = Worlds.TickAllUntil([&]()
		{
			if (!Bag || Bag->Items.Num() == 0) return false;
			if (Bag->Items[0].Pos != FIntPoint(1, 0)) return false;
			UYIInventoryBag* ClientBag = ClientInv->GetBag();
			return ClientBag && ClientBag->Items.Num() == 1 && ClientBag->Items[0].Pos == FIntPoint(1, 0);
		}, 0.0166f, 120);
		TestTrue(TEXT("Client move replicated to server and back"), bMoveReplicated);
	});
}

#endif // WITH_EDITOR

#endif // WITH_AUTOMATION_TESTS
