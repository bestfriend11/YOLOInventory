#include "Modules/ModuleManager.h"
#include "YIItemFeatureResolverRegistry.h"
#include "YIWorldPickupPolicyResolver.h"

class FYOLOInventoryWorldModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		PickupPolicyResolver = MakeShared<FYIDefaultWorldPickupPolicyResolver, ESPMode::ThreadSafe>();
		FYIItemFeatureResolverRegistry::Get().RegisterResolver(PickupPolicyResolver.ToSharedRef());
	}

	virtual void ShutdownModule() override
	{
		if (PickupPolicyResolver.IsValid())
		{
			FYIItemFeatureResolverRegistry::Get().UnregisterResolver(PickupPolicyResolver->GetResolverKey(), PickupPolicyResolver.Get());
			PickupPolicyResolver.Reset();
		}
	}

private:
	TSharedPtr<FYIDefaultWorldPickupPolicyResolver, ESPMode::ThreadSafe> PickupPolicyResolver;
};

IMPLEMENT_MODULE(FYOLOInventoryWorldModule, YOLOInventoryWorld)
