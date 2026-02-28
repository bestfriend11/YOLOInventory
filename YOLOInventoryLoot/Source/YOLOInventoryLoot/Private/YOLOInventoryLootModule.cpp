#include "Modules/ModuleManager.h"
#include "YIItemFeatureResolverRegistry.h"
#include "YILootPolicyResolver.h"

class FYOLOInventoryLootModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		LootPolicyResolver = MakeShared<FYIDefaultLootPolicyResolver, ESPMode::ThreadSafe>();
		FYIItemFeatureResolverRegistry::Get().RegisterResolver(LootPolicyResolver.ToSharedRef());
	}

	virtual void ShutdownModule() override
	{
		if (LootPolicyResolver.IsValid())
		{
			FYIItemFeatureResolverRegistry::Get().UnregisterResolver(LootPolicyResolver->GetResolverKey(), LootPolicyResolver.Get());
			LootPolicyResolver.Reset();
		}
	}

private:
	TSharedPtr<FYIDefaultLootPolicyResolver, ESPMode::ThreadSafe> LootPolicyResolver;
};

IMPLEMENT_MODULE(FYOLOInventoryLootModule, YOLOInventoryLoot)
