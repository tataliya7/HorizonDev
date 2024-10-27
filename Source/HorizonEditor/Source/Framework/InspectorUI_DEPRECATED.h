#include "Engine/HorizonEngineModule.h"

namespace Horizon
{
    extern bool DrawComponentUI_TransformComponent(const char* lable, TransformComponent& component);
    extern bool DrawComponentUI_CameraComponent(const char* lable, CameraComponent& component);
    extern bool DrawComponentUI_LightComponent(const char* lable, LightComponent& component);
    extern bool DrawComponentUI_SkyLightComponent(const char* lable, SkyLightComponent& component);
    extern bool DrawComponentUI_SkyAtmosphereComponent(const char* lable, SkyAtmosphereComponent& component);
    extern bool DrawComponentUI_LocalFogVolumeComponent(const char* lable, LocalFogVolumeComponent& component);
}
