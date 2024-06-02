#include "HorizonEngine.h"
#include "RenderSystem.h"

namespace Horizon
{
    HorizonEngine* HorizonEngine::Instance = nullptr;

    void InitializeEngine()
    {
        HorizonEngine::Instance = new HorizonEngine();
        HorizonEngine::Instance->RegisterAndInitializeSubsystems();
    }

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

    void HorizonEngine::Tick(float deltaTimeInSeconds)
    {
        GArena->Reset();


    }

    void HorizonEngine::RegisterAndInitializeSubsystems()
    {
        RenderSystem* renderSystem = subsystemRegistry.RegisterSubsystem<RenderSystem>();
        renderSystem->Init();
    }
}