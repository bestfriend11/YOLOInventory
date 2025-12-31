#include "YIItemRegistrySubsystem.h"
#include "YIItemDefinition.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"

void UYIItemRegistrySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bIndexed = false;
}

void UYIItemRegistrySubsystem::BuildIndex(bool bForce)
{
	if (bIndexed && !bForce) return;
	CodeToAsset.Reset();

	FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> Assets;
	ARM.Get().GetAssetsByClass(UYIItemDefinition::StaticClass()->GetClassPathName(), Assets, true);
	for (const FAssetData& AD : Assets)
	{
		int64 Code = 0;
		// Load the asset to read UniqueCode reliably
		if (UYIItemDefinition* Def = Cast<UYIItemDefinition>(AD.GetAsset()))
		{
			Code = Def->UniqueCode;
		}
		if (Code != 0)
		{
			CodeToAsset.FindOrAdd(Code) = TSoftObjectPtr<UYIItemDefinition>(AD.ToSoftObjectPath());
		}
	}
	bIndexed = true;
}

UYIItemDefinition* UYIItemRegistrySubsystem::GetByCode(int64 Code)
{
	if (!bIndexed) BuildIndex(false);
	if (const TSoftObjectPtr<UYIItemDefinition>* Ptr = CodeToAsset.Find(Code))
	{
		return Ptr->LoadSynchronous();
	}
	return nullptr;
}

bool UYIItemRegistrySubsystem::EnsureUniqueCodes(bool bAutoFix)
{
	BuildIndex(true);
	TSet<int64> Seen;
	bool bOk = true;
	for (const TPair<int64,TSoftObjectPtr<UYIItemDefinition>>& P : CodeToAsset)
	{
		if (P.Key == 0 || Seen.Contains(P.Key))
		{
			bOk = false;
			if (bAutoFix)
			{
#if WITH_EDITOR
				UYIItemDefinition* Def = P.Value.LoadSynchronous();
				if (Def)
				{
					int64 NewCode = 0;
					do { NewCode = (int64)FMath::RandRange(100000, INT32_MAX) * 1000ll + (int64)FMath::RandRange(0,999); } while (Seen.Contains(NewCode));
					Def->Modify();
					Def->UniqueCode = NewCode;
					Seen.Add(NewCode);
				}
#endif
			}
		}
		else
		{
			Seen.Add(P.Key);
		}
	}
	return bOk;
}
