#pragma once

#include "Core/CoreModule.h"
#include "Entity/EntityComponents.h"
#include "RenderBackend/RenderBackendModule.h"

namespace HE
{
    class Scene;
    class RenderEngine;

    enum class DebugViewMode
    {
        Lit,
        Wireframe,
        Illuminance,
        WorldSpaceNormal,
        PrimitiveID,
        MaterialID,
        MotionVectors,
        AmbientOcclusion,
        ShadowMask,
        SurfelGISurfel,
        SurfelGIHeatmap,
    };

    class SceneView
    {
    public:
        SceneView() = default;
        virtual ~SceneView() = default;
        Scene* scene;
        RenderEngine* renderEngine;
        CameraComponent camera;
        bool cameraCut;
        DebugViewMode debugViewMode;
        float deltaTime;
        uint32 frameIndex;
        uint32 targetWidth;
        uint32 targetHeight;
        RenderBackendDeviceMask deviceMask;
        RenderBackendSwapChainHandle swapChain;
        RenderBackendTextureDesc targetDesc;
        RenderBackendTextureHandle target;
        RenderBackendTextureDesc captureTargetDescs[8];
        RenderBackendTextureHandle captureTargets[8];

        RenderBackendRayTracingAccelerationStructureHandle GetRayTracingScene() const;
    };
}