#pragma once

#include "Core/CoreModule.h"

namespace HE
{
    class SceneView;

    class EngineSubsystem
    {
    public:
        EngineSubsystem(const std::string& name) : name(name) {}
        virtual ~EngineSubsystem() = default;
        virtual bool IsCustom() const = 0;
        virtual void Init(void* data) = 0;
        virtual void Exit() = 0;
        virtual void BeginDrawUI() = 0;
        virtual void EndDrawUI() = 0;
        virtual void RenderScene(SceneView* view) = 0;

        bool IsHardwareRayTracingEnabled() const
        {
            return hardwareRayTracingEnabled;
        }
        const std::string& GetName() const
        {
            return name;
        }
        bool hardwareRayTracingEnabled = false;
    private:
        std::string name;
    };
}