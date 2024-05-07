#pragma once

#include "Core/CoreModule.h"

namespace Horizon
{
    class GPUScene
    {

    };

    class RenderScene : public RenderSceneInterface
    {
    public:
        LightComponent* GetAtmosphericLight()
        {
            return atmosphericLight;
        }
        SkyAtmosphereRenderProxy* GetActiveSkyAtmosphere() const
        {
            return skyAtmosphere;
        }
        bool HasSkyAtmosphere() const
        {

        }
    private:
        LightComponent* atmosphericLight = nullptr;
        SkyAtmosphereRenderProxy* skyAtmosphere = nullptr;
        RenderBackendRayTracingAccelerationStructureHandle rayTracingScene;
        RenderBackendRayTracingAccelerationStructureHandle bottomLevelAS;

        int64 updateCounter = 0;

        GPUScene gpuScene;

        void UpdateGeometry();

        struct DrawCallInfo
        {
            RenderBackendBufferHandle vertexBuffers[4];
            RenderBackendBufferHandle indexBuffer;
            uint32 numVertices;
            uint32 numIndices;
            uint32 firstIndex;
            uint32 geometryIndex;
        };

        enum class Mesh
        {
            Opaque,
            Translucency,
            EditorPickingProxy,
            EditorSelection,
        };

        std::vector<DrawCallInfo> drawList;

        RenderBackendBufferDesc instanceBufferDesc;
        RenderBackendBufferHandle instanceBuffer;
        RenderBackendBufferHandle instanceUploadBuffer;

        // Transforms
        uint32 numTransforms = 0;
        uint64 transformBufferSize = 0;
        std::vector<Matrix4x4> transforms;
        std::vector<Matrix4x4> rowMajorTransforms;
        RenderBackendBufferHandle transformBuffer;
        RenderBackendBufferHandle transformUploadBuffer;

        RenderBackendBufferHandle previousTransformBuffer;

        RenderBackendBufferHandle transformBufferRowMajor;

        // Geometries
        uint32 numGeometries = 0;
        uint64 geometryBufferSize = 0;
        std::vector<GeometryShaderParameters> geometries;
        RenderBackendBufferHandle geometryUploadBuffer;
        RenderBackendBufferHandle geometryBuffer;

        // Materials
        uint32 numMaterials = 0;
        uint64 materialBufferSize = 0;
        std::vector<MaterialShaderParameters> materials;
        RenderBackendBufferHandle materialUploadBuffer;
        RenderBackendBufferHandle materialBuffer;

        // Lights
        uint32 numLights;
        LightShaderParameters lightData[RendererMaxLightCount];
        LightInfo lightInfo[RendererMaxLightCount];
        uint32 numCascadedShadowMaps = 0;
        CascadedShadowMapShaderParameters cascadedShadowMapData[RendererMaxCascadedShadowMapCount];
        uint32 numCubeShadowMaps = 0;
        CubeShadowMapShaderParameters cubeShadowMapData[RendererMaxCubeShadowMapCount];
        uint32 cubeShadowMapIndexToLightIndex[RendererMaxCubeShadowMapCount];
        uint32 cascadedShadowMapIndexToLightIndex[RendererMaxCascadedShadowMapCount];

        RenderBackendBufferHandle lightDataBuffer;
        RenderBackendBufferHandle lightDataUploadBuffer;

        RenderBackendBufferHandle cascadedShadowMapBuffer;
        RenderBackendBufferHandle cascadedShadowMapUploadBuffer;

        RenderBackendBufferHandle cubeShadowMapBuffer;
        RenderBackendBufferHandle cubeShadowMapUploadBuffer;

        // Environment
        RenderBackendTextureHandle environmentMap;
        RenderBackendTextureHandle irradianceEnvironmentMap;
        RenderBackendBufferHandle irradianceEnvironmentMapSH;
        RenderBackendTextureHandle filteredEnvironmentMap;

        // Debug draw
        std::vector<Vector3> debugDrawLinesVertices;

        uint32 debugDrawLinesVertexBufferSize = 0;
        RenderBackendBufferHandle debugDrawLinesVertexBuffer;
        RenderBackendBufferHandle debugDrawLinesVertexUploadBuffer;
    };
}
