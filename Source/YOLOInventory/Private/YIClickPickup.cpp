#include "YIClickPickup.h"

#include "YIInventoryComponent.h"
#include "YIInventoryBag.h"
#include "YIInventoryBlueprintLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

AYIClickPickup::AYIClickPickup()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AYIClickPickup::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoEnableInput && !HasAuthority())
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			EnableInput(PC);
		}
	}

	// Bind click (works when input is enabled for the local PC)
	OnClicked.AddDynamic(this, &AYIClickPickup::HandleClicked);
}

void AYIClickPickup::HandleClicked(AActor* /*TouchedActor*/, FKey /*Button*/)
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		ServerPickup(PC);
	}
}

void AYIClickPickup::ServerPickup_Implementation(APlayerController* PC)
{
	// Implementation body
	if (!HasAuthority() || !IsValid(PC))
	{
		return;
	}

	APawn* Pawn = PC->GetPawn();
	if (!Pawn)
	{
		return;
	}

	UYIInventoryComponent* InvComp = Pawn->FindComponentByClass<UYIInventoryComponent>();
	if (!InvComp || !InvComp->EquippedBag)
	{
		return;
	}

	// Convert net-safe instance back to full instance and add to bag
	FYIItemInstance Full;
	Full.Definition = ItemInstance.Definition;
	Full.Count = ItemInstance.Count;
	Full.CustomStackKey = ItemInstance.CustomStackKey;
	Full.bRotated = ItemInstance.bRotated;
	Full.Affixes = ItemInstance.Affixes;
	for (const FYIAttributeKV& KV : ItemInstance.Attributes)
	{
		Full.Attributes.Add(KV.Name, KV.Value);
	}

	if (UYIInventoryBlueprintLibrary::AddItemInstanceToBag(InvComp->EquippedBag, Full))
	{
		Destroy();
	}
}
