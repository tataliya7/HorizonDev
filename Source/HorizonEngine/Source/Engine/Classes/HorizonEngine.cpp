#include "HorizonEngine.h"
#include "RenderSystem.h"

// TODO
#include "Streamline.h"

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

        RenderSystem* renderSystem = GetSubsystem<RenderSystem>();
        renderSystem->Tick(deltaTimeInSeconds);
    }

    void HorizonEngine::RegisterAndInitializeSubsystems()
    {
        #if HORIZON_EXPERIMENTAL_STREAMLINE
        // TODO:
        streamlineContext = new StreamlineContext();
        streamlineContext->Init();
        #endif

        RenderSystem* renderSystem = subsystemRegistry.RegisterSubsystem<RenderSystem>();
        renderSystem->Init();

        #if HORIZON_EXPERIMENTAL_STREAMLINE
        streamlineContext->Test(renderSystem->GetRenderBackend());
        #endif
    }
}