#pragma once

#include "RendererCommon.h"
#include "RenderStatistics.h"

namespace Horizon
{
    class LightRenderProxy
    {
    public:
        LightRenderProxy();
        virtual ~LightRenderProxy();
    private:
    };

    class DistantLightRenderProxy : public LightRenderProxy
    {
    public:
        DistantLightRenderProxy();
        virtual ~DistantLightRenderProxy();

        Vector3 GetPhysicalLightColor() const
        {

        }

        Vector3 GetDirection() const
        {
            return direction;
        }

        float GetHalfApexAngleInRadians() const
        {
            return halfApexAngleInRadians;
        }

        Vector3 GetAtmosphericLightDiskColorFactor() const
        {
            return atmosphericLightDiskColorFactor;
        }

    private:
        Vector3 direction;
        float halfApexAngleInRadians;
        Vector3 atmosphericLightDiskColorFactor;
    };

    class SpotLightRenderProxy : public LightRenderProxy
    {
    public:
        SpotLightRenderProxy();
        virtual ~SpotLightRenderProxy();
    private:
    };

    class PointLightRenderProxy : public LightRenderProxy
    {
    public:
        PointLightRenderProxy();
        virtual ~PointLightRenderProxy();
    private:
    };

    class EnvironmentLight
    {

    };

    struct AtmosphereParameters
    {
        float bottomRadius;
        float topRadius;
        Vector3 groundAlbedo;

        Vector3 rayleighScattering;
        float rayleighDensityExpScale;

        Vector3 mieScattering;
        Vector3 mieExtinction;
        Vector3 mieAbsorption;
        float miePhaseG;
        float mieDensityExpScale;

        float absorptionDensity0LayerWidth;
        float absorptionDensity0ConstantTerm;
        float absorptionDensity0LinearTerm;
        float absorptionDensity1ConstantTerm;
        float absorptionDensity1LinearTerm;
        Vector3 absorptionExtinction;
    };

    class SkyAtmosphereRenderProxy
    {
    public:
        SkyAtmosphereRenderProxy();

        const AtmosphereParameters& GetAtmosphereParameters() const
        {

        }

        void SetAtmosphereParameters(const AtmosphereParameters& newValue)
        {
            atmosphereParameters = newValue;
        }

        Vector3 GetSkyLuminanceFactor() const
        {
            return skyLuminanceFactor;
        }

        void SetSkyLuminanceFactor(const Vector3& newValue)
        {
            skyLuminanceFactor = newValue;
        }

    private:
        AtmosphereParameters atmosphereParameters;
        Vector3 skyLuminanceFactor;
    };

    class GPUScene
    {

    };

    class RenderScene
    {
    public:

        bool HasAtmosphericLight() const
        {
            return atmosphericLight != nullptr;
        }

        DistantLightRenderProxy* GetAtmosphericLight() const
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

        /**
         * Release this scene.
         */
        virtual void Release() = 0;

        /**
         * Adds a mesh component to the scene.
         *
         * @param[in] component - mesh component to add to the scene.
         */
        virtual void AddMesh(MeshComponent* component) = 0;

        /**
         * Removes a mesh component from the scene.
         *
         * @param[in] component - mesh component to remove from the scene.
         */
        virtual void RemoveMesh(MeshComponent* component) = 0;

        /**
         * Adds a new light to the scene.
         *
         * @param [in] light - light to add to the scene.
         */
        virtual void AddLight(LightRenderProxy* light) = 0;

        /**
         * Removes a light component from the scene.
         *
         * @param [in] component light component to remove from the scene.
         */
        virtual void RemoveLight(LightComponent* component) = 0;

        virtual void HasSkyLight() = 0;

        virtual void SetSkyLight(FSkyLightSceneProxy* component) = 0;

        virtual void HasSkyAtmosphere() = 0;

        virtual void SetSkyAtmosphere(SkyAtmosphereRenderProxy* skyAtmosphere) = 0;

        void Render();

        void GetRenderStatistics(RenderStatistics& statistics) const;

    private:

        DistantLightRenderProxy* atmosphericLight = nullptr;
        SkyAtmosphereRenderProxy* skyAtmosphere = nullptr;

        RenderBackendRayTracingAccelerationStructureHandle rayTracingScene;
        RenderBackendRayTracingAccelerationStructureHandle bottomLevelAS;

        int64 updateCounter = 0;

        GPUScene gpuScene;
        RenderStatistics renderStatistics;

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
            EditorSelection,
            EditorPickingProxy,
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
