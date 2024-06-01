#include "HorizonEngine.h"
#include "RenderSystem.h"

namespace Horizon
{
    HorizonEngine* HorizonEngine::Instance = nullptr;

    HorizonEngine::HorizonEngine()
    {
        assert(Instance == nullptr);
        Instance = this;
    }

    HorizonEngine::~HorizonEngine()
    {
        assert(Instance == this);
        Instance = nullptr;
    }

    void HorizonEngine::RegisterAndInitializeSubsystems()
    {
        RenderSystem* renderSystem = subsystemRegistry.RegisterSubsystem<RenderSystem>();
        renderSystem->Init();
    }
}