#pragma once

#include "Engine/HorizonEngineModule.h"

namespace Horizon
{
    class TimeOfDayScheduler
    {
    public:
        TimeOfDayScheduler(Scene* scene);
        virtual ~TimeOfDayScheduler();

        void Tick(float deltaTime);

    private:
        Scene* scene;
        EntityHandle entityHandle;
        float timeOfDay = 12.0f;
        float loopTime = 3600.0f;

        TransformComponent* transform;
        LightComponent* sunLight;
        SkyLightComponent* skyLight;
        SkyAtmosphereComponent* skyAtmosphere;
        GlobalFogComponent* globalFog;
        VolumetricCloudComponent* volumetricCloud;
    };
}