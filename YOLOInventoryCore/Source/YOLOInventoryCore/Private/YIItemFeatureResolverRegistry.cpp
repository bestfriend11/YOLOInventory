#include "YIItemFeatureResolverRegistry.h"

namespace YIItemFeatureKeys
{
	const FName TradePolicy(TEXT("TradePolicy"));
	const FName EquipPolicy(TEXT("EquipPolicy"));
	const FName LootPolicy(TEXT("LootPolicy"));
	const FName PickupPolicy(TEXT("PickupPolicy"));
}

FYIItemFeatureResolverRegistry& FYIItemFeatureResolverRegistry::Get()
{
	static FYIItemFeatureResolverRegistry Registry;
	return Registry;
}

bool FYIItemFeatureResolverRegistry::RegisterResolver(const TSharedRef<IYIItemFeatureResolver, ESPMode::ThreadSafe>& Resolver, bool bReplaceExisting)
{
	const FName ResolverKey = Resolver->GetResolverKey();
	if (ResolverKey.IsNone())
	{
		return false;
	}

	FWriteScopeLock WriteLock(ResolverLock);
	if (!bReplaceExisting && ResolversByKey.Contains(ResolverKey))
	{
		return false;
	}

	ResolversByKey.Add(ResolverKey, Resolver);
	return true;
}

bool FYIItemFeatureResolverRegistry::UnregisterResolver(FName ResolverKey, const IYIItemFeatureResolver* ExpectedResolver)
{
	if (ResolverKey.IsNone())
	{
		return false;
	}

	FWriteScopeLock WriteLock(ResolverLock);
	TSharedPtr<IYIItemFeatureResolver, ESPMode::ThreadSafe>* ExistingResolver = ResolversByKey.Find(ResolverKey);
	if (!ExistingResolver)
	{
		return false;
	}

	if (ExpectedResolver && ExistingResolver->Get() != ExpectedResolver)
	{
		return false;
	}

	ResolversByKey.Remove(ResolverKey);
	return true;
}

TSharedPtr<IYIItemFeatureResolver, ESPMode::ThreadSafe> FYIItemFeatureResolverRegistry::FindResolver(FName ResolverKey) const
{
	if (ResolverKey.IsNone())
	{
		return nullptr;
	}

	FReadScopeLock ReadLock(ResolverLock);
	if (const TSharedPtr<IYIItemFeatureResolver, ESPMode::ThreadSafe>* Found = ResolversByKey.Find(ResolverKey))
	{
		return *Found;
	}
	return nullptr;
}

void FYIItemFeatureResolverRegistry::Reset()
{
	FWriteScopeLock WriteLock(ResolverLock);
	ResolversByKey.Reset();
}

