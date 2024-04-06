#pragma once

#include "Core/CoreModule.h"
#include "Rendering/RenderEngine.h"
#include "Rendering/RenderGraph/RenderGraph.h"
#include "Rendering/SceneView.h"
#include "Rendering/ShaderLibrary.h"
#include "Rendering/Renderer/RendererPrivate.h"

namespace HE
{
    class RayTracingScene
    {
    public:
        RenderBackendRayTracingAccelerationStructureHandle GetTLAS()
        {
            return tlas;
        }
        std::vector<RayTracingGeometry*> geometriesToBuild;
    private:
        RenderBackendRayTracingAccelerationStructureHandle tlas;

        RenderBackendBufferHandle instanceUploadBuffer;
        RenderBackendBufferHandle transformUploadBuffer;

        //std::vector<RayTracingInstance> instances;
    };
}