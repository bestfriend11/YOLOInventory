#include "IYOLOInventoryEditorCoreModule.h"

class FYOLOInventoryEditorCoreModule : public IYOLOInventoryEditorCoreModule
{
public:
    virtual void StartupModule() override {}
    virtual void ShutdownModule() override
    {
        BagDashboardFactory.Unbind();
        GeneratorDashboardFactory.Unbind();
    }

    virtual void RegisterBagDashboardFactory(FYICreateBagDashboardBridge InFactory) override
    {
        BagDashboardFactory = MoveTemp(InFactory);
    }

    virtual void ClearBagDashboardFactory() override
    {
        BagDashboardFactory.Unbind();
    }

    virtual bool HasBagDashboardFactory() const override
    {
        return BagDashboardFactory.IsBound();
    }

    virtual TSharedRef<IYIBagDashboardBridge> CreateBagDashboardBridge() override
    {
        checkf(BagDashboardFactory.IsBound(), TEXT("YOLOInventoryEditorCore: Bag dashboard factory is not registered."));
        return BagDashboardFactory.Execute();
    }

    virtual void RegisterGeneratorDashboardFactory(FYICreateGeneratorDashboardBridge InFactory) override
    {
        GeneratorDashboardFactory = MoveTemp(InFactory);
    }

    virtual void ClearGeneratorDashboardFactory() override
    {
        GeneratorDashboardFactory.Unbind();
    }

    virtual bool HasGeneratorDashboardFactory() const override
    {
        return GeneratorDashboardFactory.IsBound();
    }

    virtual TSharedRef<IYIGeneratorDashboardBridge> CreateGeneratorDashboardBridge() override
    {
        checkf(GeneratorDashboardFactory.IsBound(), TEXT("YOLOInventoryEditorCore: Generator dashboard factory is not registered."));
        return GeneratorDashboardFactory.Execute();
    }

private:
    FYICreateBagDashboardBridge BagDashboardFactory;
    FYICreateGeneratorDashboardBridge GeneratorDashboardFactory;
};

IMPLEMENT_MODULE(FYOLOInventoryEditorCoreModule, YOLOInventoryEditorCore)
