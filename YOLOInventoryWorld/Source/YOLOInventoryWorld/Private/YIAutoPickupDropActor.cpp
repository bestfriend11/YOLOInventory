#include "YIAutoPickupDropActor.h"

#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "YIInventoryComponent.h"
#include "YIInventoryBag.h"
#include "YIItemDefinition.h"
#include "YIWorldLootBlueprintLibrary.h"

AYIAutoPickupDropActor::AYIAutoPickupDropActor()
{
	PrimaryActorTick.bCanEverTick = true;

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	SetRootComponent(VisualRoot);

	if (MeshComponent)
	{
		MeshComponent->SetupAttachment(VisualRoot);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		MeshComponent->SetSimulatePhysics(false);
		MeshComponent->SetEnableGravity(false);
	}

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->SetupAttachment(VisualRoot);
	PickupSphere->SetSphereRadius(PickupRadius);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionObjectType(ECC_WorldDynamic);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PickupSphere->SetGenerateOverlapEvents(true);
}

void AYIAutoPickupDropActor::BeginPlay()
{
	Super::BeginPlay();

	if (PickupSphere)
	{
		PickupSphere->SetSphereRadius(PickupRadius);
		PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &AYIAutoPickupDropActor::HandlePickupOverlap);
	}

	if (VisualRoot)
	{
		BaseVisualRelativeLocation = VisualRoot->GetRelativeLocation();
	}

	if (SpawnScaleDuration > 0.f)
	{
		SetActorScale3D(FVector::ZeroVector);
	}

	if (HasAuthority())
	{
		ApplyDefaultItemIfNeeded();
	}
}

void AYIAutoPickupDropActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	TimeAccumulator += DeltaSeconds;

	// Cosmetic-only motion; runs on all instances for smooth visuals.
	if (VisualRoot)
	{
		const float BobOffset = BobAmplitude > 0.f
			? FMath::Sin(TimeAccumulator * BobFrequency * UE_TWO_PI) * BobAmplitude
			: 0.f;
		VisualRoot->SetRelativeLocation(BaseVisualRelativeLocation + FVector(0.f, 0.f, BobOffset));

		if (!FMath::IsNearlyZero(SpinYawDegPerSecond))
		{
			FRotator R = VisualRoot->GetRelativeRotation();
			R.Yaw = FMath::Fmod(R.Yaw + SpinYawDegPerSecond * DeltaSeconds, 360.f);
			VisualRoot->SetRelativeRotation(R);
		}
	}

	if (SpawnScaleDuration > 0.f && SpawnTimeElapsed < SpawnScaleDuration)
	{
		SpawnTimeElapsed = FMath::Min(SpawnTimeElapsed + DeltaSeconds, SpawnScaleDuration);
		const float Alpha = SpawnTimeElapsed / SpawnScaleDuration;
		const float EaseOut = 1.f - FMath::Square(1.f - Alpha);
		SetActorScale3D(FVector(EaseOut));
	}
}

void AYIAutoPickupDropActor::SetDropItem(UYIItemDefinition* InDefinition, int32 InCount)
{
	if (!HasAuthority() || !InDefinition)
	{
		return;
	}

	ItemDefinition = InDefinition;
	ItemCode = InDefinition->UniqueCode;
	Count = FMath::Max(1, InCount);
	ItemInstance.Definition = InDefinition;
	ItemInstance.Count = Count;
	if (!ItemInstance.InstanceId.IsValid())
	{
		ItemInstance.InstanceId = FGuid::NewGuid();
	}
	if (!ItemInstance.StackId.IsValid())
	{
		ItemInstance.StackId = FGuid::NewGuid();
	}
	RefreshVisuals();
}

void AYIAutoPickupDropActor::HandlePickupOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bAutoPickupOnOverlap || !OtherActor)
	{
		return;
	}

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	if (Now < OverlapRetryUnlockAt)
	{
		return;
	}

	OverlapRetryUnlockAt = Now + RetryCooldownSeconds;
	TryPickupActor(OtherActor);
}

bool AYIAutoPickupDropActor::TryPickupActor(AActor* PickerActor)
{
	if (!HasAuthority() || !IsValid(PickerActor))
	{
		return false;
	}

	UYIInventoryComponent* Inventory = nullptr;
	if (!ResolveInventoryComponent(PickerActor, Inventory) || !Inventory)
	{
		OnPickupFailed.Broadcast(PickerActor);
		return false;
	}

	UYIInventoryBag* TargetBag = Inventory->GetBag();
	if (!TargetBag)
	{
		OnPickupFailed.Broadcast(PickerActor);
		return false;
	}

	if (UYIWorldLootBlueprintLibrary::PickupItemActorIntoBag(this, TargetBag, this))
	{
		OnPickedUp.Broadcast(PickerActor, Inventory);
		return true;
	}

	OnPickupFailed.Broadcast(PickerActor);
	return false;
}

bool AYIAutoPickupDropActor::ResolveInventoryComponent(AActor* CandidateActor, UYIInventoryComponent*& OutInventory) const
{
	OutInventory = nullptr;
	if (!CandidateActor)
	{
		return false;
	}

	APawn* Pawn = Cast<APawn>(CandidateActor);
	if (!Pawn)
	{
		Pawn = Cast<APawn>(CandidateActor->GetOwner());
	}
	if (!Pawn)
	{
		Pawn = CandidateActor->GetInstigator();
	}

	if (!Pawn)
	{
		return false;
	}

	if (bRequirePlayerControlledPawn && !Pawn->IsPlayerControlled())
	{
		return false;
	}

	OutInventory = Pawn->FindComponentByClass<UYIInventoryComponent>();
	return OutInventory != nullptr;
}

void AYIAutoPickupDropActor::ApplyDefaultItemIfNeeded()
{
	if (!HasAuthority())
	{
		return;
	}

	const bool bHasCode = ItemCode != 0;
	const bool bHasInstanceDef = ItemInstance.Definition.ToSoftObjectPath().IsValid();
	if (bHasCode || bHasInstanceDef)
	{
		RefreshVisuals();
		return;
	}

	UYIItemDefinition* Def = ItemDefinition.LoadSynchronous();
	if (!Def)
	{
		RefreshVisuals();
		return;
	}

	ItemCode = Def->UniqueCode;
	Count = FMath::Max(1, ItemCount);
	ItemInstance.Definition = Def;
	ItemInstance.Count = Count;
	if (!ItemInstance.InstanceId.IsValid())
	{
		ItemInstance.InstanceId = FGuid::NewGuid();
	}
	if (!ItemInstance.StackId.IsValid())
	{
		ItemInstance.StackId = FGuid::NewGuid();
	}
	RefreshVisuals();
}
