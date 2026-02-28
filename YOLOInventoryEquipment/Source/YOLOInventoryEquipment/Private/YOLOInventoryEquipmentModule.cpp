#include "Modules/ModuleManager.h"
#include "YIEquipmentPolicyResolver.h"
#include "YIItemFeatureResolverRegistry.h"

class FYOLOInventoryEquipmentModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		EquipPolicyResolver = MakeShared<FYIDefaultEquipPolicyResolver, ESPMode::ThreadSafe>();
		FYIItemFeatureResolverRegistry::Get().RegisterResolver(EquipPolicyResolver.ToSharedRef());
	}

	virtual void ShutdownModule() override
	{
		if (EquipPolicyResolver.IsValid())
		{
			FYIItemFeatureResolverRegistry::Get().UnregisterResolver(EquipPolicyResolver->GetResolverKey(), EquipPolicyResolver.Get());
			EquipPolicyResolver.Reset();
		}
	}

private:
	TSharedPtr<FYIDefaultEquipPolicyResolver, ESPMode::ThreadSafe> EquipPolicyResolver;
};

IMPLEMENT_MODULE(FYOLOInventoryEquipmentModule, YOLOInventoryEquipment)
