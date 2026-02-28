#pragma once

#include "CoreMinimal.h"

class IYIItemFeatureResolver
{
public:
	virtual ~IYIItemFeatureResolver() = default;

	/** Stable key used by feature consumers to locate this resolver. */
	virtual FName GetResolverKey() const = 0;
};

namespace YIItemFeatureKeys
{
	YOLOINVENTORYCORE_API extern const FName TradePolicy;
	YOLOINVENTORYCORE_API extern const FName EquipPolicy;
	YOLOINVENTORYCORE_API extern const FName LootPolicy;
	YOLOINVENTORYCORE_API extern const FName PickupPolicy;
}

/**
 * Thread-safe resolver registry shared across feature modules.
 * Feature plugins register resolvers on startup and unregister on shutdown.
 */
class YOLOINVENTORYCORE_API FYIItemFeatureResolverRegistry
{
public:
	static FYIItemFeatureResolverRegistry& Get();

	bool RegisterResolver(const TSharedRef<IYIItemFeatureResolver, ESPMode::ThreadSafe>& Resolver, bool bReplaceExisting = true);
	bool UnregisterResolver(FName ResolverKey, const IYIItemFeatureResolver* ExpectedResolver = nullptr);
	TSharedPtr<IYIItemFeatureResolver, ESPMode::ThreadSafe> FindResolver(FName ResolverKey) const;
	void Reset();

	template<typename TResolverType>
	TSharedPtr<TResolverType, ESPMode::ThreadSafe> FindResolverTyped(FName ResolverKey) const
	{
		return StaticCastSharedPtr<TResolverType>(FindResolver(ResolverKey));
	}

private:
	mutable FRWLock ResolverLock;
	TMap<FName, TSharedPtr<IYIItemFeatureResolver, ESPMode::ThreadSafe>> ResolversByKey;
};

