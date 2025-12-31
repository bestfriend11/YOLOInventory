#pragma once

#include "CoreMinimal.h"
#include "YIStackEntry.h"
#include "YIStackEntry_Examples.generated.h"

class UPhysicalMaterial;

class UTexture2D;
class USkeletalMesh;
class USoundBase;
class UFXSystemAsset;

UCLASS(BlueprintType, EditInlineNew)
class YOLOINVENTORY_API UYIUI_NameDesc : public UYIStackEntry_UI
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="UI") FText Name;
	UPROPERTY(EditAnywhere, Category="UI", meta=(MultiLine=true)) FText Description;
	UPROPERTY(EditAnywhere, Category="UI") TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditAnywhere, Category="UI") TSoftObjectPtr<USkeletalMesh> MeshSkeletal;
	UPROPERTY(EditAnywhere, Category="UI") TSoftObjectPtr<UStaticMesh> MeshStatic;
	UPROPERTY(EditAnywhere, Category="UI") TSoftObjectPtr<UFXSystemAsset> Effect;
	UPROPERTY(EditAnywhere, Category="UI") TSoftObjectPtr<USoundBase> DropSound;
	UPROPERTY(EditAnywhere, Category="UI") FText Tooltip;
};

UCLASS(BlueprintType, EditInlineNew)
class YOLOINVENTORY_API UYIAbility_GrantAbility : public UYIStackEntry_Ability
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="Ability") TSoftClassPtr<UObject> AbilityClass;
	UPROPERTY(EditAnywhere, Category="Ability") int32 Level = 1;
	UPROPERTY(EditAnywhere, Category="Ability") TMap<FName, float> Scalars;
};

// Additional UI-focused AV settings for sounds and effects
UCLASS(BlueprintType, EditInlineNew)
class YOLOINVENTORY_API UYIUI_AudioVisual : public UYIStackEntry_UI
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="UI|Audio") TSoftObjectPtr<USoundBase> PickupSound;
	UPROPERTY(EditAnywhere, Category="UI|Audio") TSoftObjectPtr<USoundBase> UseSound;
	UPROPERTY(EditAnywhere, Category="UI|VFX") TSoftObjectPtr<UFXSystemAsset> UseEffect;
	UPROPERTY(EditAnywhere, Category="UI|VFX") TSoftObjectPtr<UFXSystemAsset> ImpactEffect;
};

// Physics-related behavior that an item could enable when used/equipped
UCLASS(BlueprintType, EditInlineNew)
class YOLOINVENTORY_API UYIAbility_Physics : public UYIStackEntry_Ability
{
	GENERATED_BODY()
public:
	// Whether to simulate physics when equipped/spawned
	UPROPERTY(EditAnywhere, Category="Physics") bool bSimulatePhysics = false;
	// Optional overrides
	UPROPERTY(EditAnywhere, Category="Physics", meta=(EditCondition="bSimulatePhysics")) float MassScale = 1.f;
	UPROPERTY(EditAnywhere, Category="Physics", meta=(EditCondition="bSimulatePhysics")) float LinearDamping = 0.01f;
	UPROPERTY(EditAnywhere, Category="Physics", meta=(EditCondition="bSimulatePhysics")) float AngularDamping = 0.01f;
	UPROPERTY(EditAnywhere, Category="Physics", meta=(EditCondition="bSimulatePhysics")) TSoftObjectPtr<UPhysicalMaterial> PhysicalMaterial;
};

UCLASS(BlueprintType, EditInlineNew)
class YOLOINVENTORY_API UYIUpgrade_Path : public UYIStackEntry_Upgrade
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="Upgrade") TArray<FName> RequiredItems;
};

// Economy and durability tuning
UCLASS(BlueprintType, EditInlineNew)
class YOLOINVENTORY_API UYIEconomy_Market : public UYIStackEntry_Economy
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category="Economy") int32 Price = 0;
	UPROPERTY(EditAnywhere, Category="Economy") int32 SellValue = 0;
	UPROPERTY(EditAnywhere, Category="Economy") float Durability = 100.f;
};

UCLASS(BlueprintType, EditInlineNew)
class YOLOINVENTORY_API UYIEconomy_DurabilityRules : public UYIStackEntry_Economy
{
	GENERATED_BODY()
public:
	// Maximum durability for the item
	UPROPERTY(EditAnywhere, Category="Durability") float MaxDurability = 100.f;
	// Whether item breaks at zero
	UPROPERTY(EditAnywhere, Category="Durability") bool bBreakOnZero = true;
	// Optional cost to repair per point
	UPROPERTY(EditAnywhere, Category="Durability", meta=(ClampMin="0")) int32 RepairCostPerPoint = 1;
};
