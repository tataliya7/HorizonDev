#pragma once

#include "RendererCommon.h"
#include "RenderStatistics.h"

namespace Horizon
{
    class MeshRenderObject
    {

    };

    struct LightRenderObjectDescription
    {
        Vector3 color;
        Vector3 position;
        Vector3 direction;
        bool castRayTracingShadows;
        bool usedAsAtmosphericLight;
        float halfApexAngleInRadians;
        Vector3 atmosphericLightDiskColorFactor;
    };

    class LightRenderObject
    {
    public:
        LightRenderObject(const LightRenderObjectDescription& description);
        virtual ~LightRenderObject();

        Vector3 GetPhysicalLightColor() const
        {
            return color;
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

        bool CastRayTracingShadows() const
        {
            return castRayTracingShadows;
        }

        bool IsUsedAsAtmosphericLight() const
        {
            return usedAsAtmosphericLight;
        }

    //private:
        Vector3 color;
        Vector3 position;
        Vector3 direction;
        bool castRayTracingShadows;
        bool usedAsAtmosphericLight;
        float halfApexAngleInRadians;
        Vector3 atmosphericLightDiskColorFactor;
    };

    class SkyLightRenderObject
    {
    public:
        SkyLightRenderObject();
        virtual ~SkyLightRenderObject();
    private:
    };

    class LocalFogVolumeRenderObject
    {
    public:

    private:

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

    class SkyAtmosphereRenderObject
    {
    public:

        SkyAtmosphereRenderObject();

        ~SkyAtmosphereRenderObject();

        const AtmosphereParameters& GetAtmosphereParameters() const
        {
            return atmosphereParameters;
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

        RenderScene();
        virtual ~RenderScene();

        /**
         * Release this scene.
         */
        virtual void Release();

        void Render();

        /**
         * Adds a mesh to the scene.
         */
        virtual void AddMesh(MeshRenderObject* mesh);

        /**
         * Removes a mesh from the scene.
         */
        virtual void RemoveMesh(MeshRenderObject* mesh);

        /**
         * Adds a new light to the scene.
         */
        virtual void AddLight(LightRenderObject* light);

        /**
         * Removes a light from the scene.
         */
        virtual void RemoveLight(LightRenderObject* light);

        // virtual void HasSkyLight() = 0;
        //
        // virtual void SetSkyLight(FSkyLightSceneProxy* component);

        virtual bool HasAtmosphericLight() const;

        virtual LightRenderObject* GetAtmosphericLight() const;

        virtual bool HasActiveSkyAtmosphere() const;

        virtual SkyAtmosphereRenderObject* GetActiveSkyAtmosphere() const;

        virtual void AddSkyAtmosphere(SkyAtmosphereRenderObject* skyAtmosphere);

        virtual void RemoveSkyAtmosphere(SkyAtmosphereRenderObject* skyAtmosphere);

        virtual bool HasAnyLocalFogVolume() const;

        virtual void AddLocalFogVolume(LocalFogVolumeRenderObject* localFogVolume);

        virtual void RemoveLocalFogVolume(LocalFogVolumeRenderObject* localFogVolume);

        void GetRenderStatistics(RenderStatistics& statistics) const;

    private:

        std::vector<LightRenderObject*> lights;

        LightRenderObject* atmosphericLight;

        SkyAtmosphereRenderObject* activeSkyAtmosphere;

        std::vector<SkyAtmosphereRenderObject*> skyAtmospheres;

        std::vector<LocalFogVolumeRenderObject*> localFogVolumes;

        GPUScene gpuScene;

        //RayTracingScene rayTracingScene;

        RenderStatistics renderStatistics;

        //void UpdateGeometry();

        struct DrawCallInfo
        {
            RenderBackendBufferHandle vertexBuffers[4];
            RenderBackendBufferHandle indexBuffer;
            uint32 numVertices;
            uint32 numIndices;
            uint32 firstIndex;
            uint32 geometryIndex;
        };

        enum class MeshType
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
        // uint32 numGeometries = 0;
        // uint64 geometryBufferSize = 0;
        // std::vector<GeometryShaderParameters> geometries;
        // RenderBackendBufferHandle geometryUploadBuffer;
        // RenderBackendBufferHandle geometryBuffer;

        // Materials
        // uint32 numMaterials = 0;
        // uint64 materialBufferSize = 0;
        // std::vector<MaterialShaderParameters> materials;
        // RenderBackendBufferHandle materialUploadBuffer;
        // RenderBackendBufferHandle materialBuffer;

        // Lights
        // uint32 numLights;
        // LightShaderParameters lightData[RendererMaxLightCount];
        // LightInfo lightInfo[RendererMaxLightCount];
        // uint32 numCascadedShadowMaps = 0;
        // CascadedShadowMapShaderParameters cascadedShadowMapData[RendererMaxCascadedShadowMapCount];
        // uint32 numCubeShadowMaps = 0;
        // CubeShadowMapShaderParameters cubeShadowMapData[RendererMaxCubeShadowMapCount];
        // uint32 cubeShadowMapIndexToLightIndex[RendererMaxCubeShadowMapCount];
        // uint32 cascadedShadowMapIndexToLightIndex[RendererMaxCascadedShadowMapCount];

        RenderBackendBufferHandle lightDataBuffer;
        RenderBackendBufferHandle lightDataUploadBuffer;

        RenderBackendBufferHandle cascadedShadowMapBuffer;
        RenderBackendBufferHandle cascadedShadowMapUploadBuffer;

        RenderBackendBufferHandle localLightShadowMapBuffer;
        RenderBackendBufferHandle localLightShadowMapUploadBuffer;

        // Environment
        // RenderBackendTextureHandle environmentMap;
        // RenderBackendTextureHandle irradianceEnvironmentMap;
        // RenderBackendBufferHandle irradianceEnvironmentMapSH;
        // RenderBackendTextureHandle filteredEnvironmentMap;

        // Debug draw
        // std::vector<Vector3> debugDrawLinesVertices;
        // uint32 debugDrawLinesVertexBufferSize = 0;
        // RenderBackendBufferHandle debugDrawLinesVertexBuffer;
        // RenderBackendBufferHandle debugDrawLinesVertexUploadBuffer;
    };
}