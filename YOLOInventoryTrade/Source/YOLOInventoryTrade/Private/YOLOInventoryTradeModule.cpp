#include "Modules/ModuleManager.h"
#include "YIItemFeatureResolverRegistry.h"
#include "YITradePolicyResolver.h"

class FYOLOInventoryTradeModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		TradePolicyResolver = MakeShared<FYIDefaultTradePolicyResolver, ESPMode::ThreadSafe>();
		FYIItemFeatureResolverRegistry::Get().RegisterResolver(TradePolicyResolver.ToSharedRef());
	}

	virtual void ShutdownModule() override
	{
		if (TradePolicyResolver.IsValid())
		{
			FYIItemFeatureResolverRegistry::Get().UnregisterResolver(TradePolicyResolver->GetResolverKey(), TradePolicyResolver.Get());
			TradePolicyResolver.Reset();
		}
	}

private:
	TSharedPtr<FYIDefaultTradePolicyResolver, ESPMode::ThreadSafe> TradePolicyResolver;
};

IMPLEMENT_MODULE(FYOLOInventoryTradeModule, YOLOInventoryTrade)
