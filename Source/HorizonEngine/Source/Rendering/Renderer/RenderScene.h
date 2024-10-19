#pragma once

#include "RendererCommon.h"
#include "RenderStatistics.h"

#include "RendererPrivate.h" // TODO

namespace Horizon
{
    struct Material
    {
        std::string name;

        // BxDF
        Vector4 baseColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        float metallic = 0.0f;
        float roughness = 0.5f;
        float specular = 0.5f;
        float specularTint = 0.0f;
        float transmission = 0.0f;
        float transmissionRoughness = 0.0f;
        float clearcoat = 0.0f;
        float clearcoatRoughness = 0.0f;
        Vector4 emission = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
        float emissionStrength = 1.0f;
        float alpha = 1.0f;

        // SSS
        Vector4 sssSurfaceAlbedo = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        Vector4 sssMFP = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        float secondRoughness = 0.5f;
        float lobeMix = 0.0f;

        enum TextureSlot
        {
            BaseColorMap,
            MetallicRoughnessMap,
            SpecularGlossinessMap,
            NormalMap,
            EmissiveMap,
            Count
        };

        struct TextureMap
        {
            std::string path;
            bool used = false;
            RenderBackendTextureHandle gpuTexture;
        };
        TextureMap textures[16];
        bool useMetallicRoughnessWorkflow = false;
    };

    class MaterialRenderObject
    {
    public:
        std::string name;

        // BxDF
        Vector4 baseColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        float metallic = 0.0f;
        float roughness = 0.5f;
        float specular = 0.5f;
        float specularTint = 0.0f;
        float transmission = 0.0f;
        float transmissionRoughness = 0.0f;
        float clearcoat = 0.0f;
        float clearcoatRoughness = 0.0f;
        Vector4 emission = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
        float emissionStrength = 1.0f;
        float alpha = 1.0f;

        // SSS
        Vector4 sssSurfaceAlbedo = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        Vector4 sssMFP = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        float secondRoughness = 0.5f;
        float lobeMix = 0.0f;

        enum TextureSlot
        {
            BaseColorMap,
            MetallicRoughnessMap,
            SpecularGlossinessMap,
            NormalMap,
            EmissiveMap,
            Count
        };

        struct TextureMap
        {
            std::string path;
            bool used = false;
            RenderBackendTextureHandle gpuTexture;
        };
        TextureMap textures[16];
        bool useMetallicRoughnessWorkflow = false;
    };

    class MeshRenderObject
    {
    public:

        bool castDynamicShadow : 1;

        Matrix4x4 localToWorldMatrix;
        Matrix4x4 worldToLocalMatrix;

        uint32 vertexCount;
        uint32 indexCount;

        RenderBackendBufferHandle vertexBuffers[4];
        RenderBackendBufferHandle indexBuffer;
        RenderBackendBufferHandle materialBuffer;
        RenderBackendBufferHandle materialIndexBuffer;

        std::string name;

        float cullDistance;

    private:

    };

    enum class LightType
    {
        DistantLight,
        PointLight,
        SpotLight,
    };

    struct LightRenderObjectDescription
    {
        LightType lightType;
        Vector3 color;
        Vector3 position;
        Vector3 direction;
        float radius;
        bool castRayTracingShadows;
        uint32 shadowMapSize;
        uint32 shadowCascadeCount;
        float shadowCascadeSplitLambda;
        float maxShadowDistance;
        float shadowMapDepthBiasConstantFactor;
        float shadowMapDepthBiasSlopeFactor;
        bool usedAsAtmosphericLight;
        float halfApexAngleInRadians;
        Vector3 atmosphericLightDiskColorFactor;
    };

    struct DistantLightRenderData
    {
        Vector3 direction;
        Vector3 tangent;
        Vector3 color;
    };

    struct LocalLightRenderData
    {
        Vector3 position;
        float radius;
        Vector3 direction;
        Vector3 tangent;
        Vector3 color;
    };

    class LightRenderObject
    {
    public:
        LightRenderObject(const LightRenderObjectDescription& description);
        virtual ~LightRenderObject();

        bool IsLocalLight() const
        {
            return lightType == LightType::PointLight || lightType == LightType::SpotLight;
        }

        void SetupDistantLightRenderData(DistantLightRenderData& outRenderData) const
        {
            outRenderData.direction = direction;
            outRenderData.tangent = tangent;
            outRenderData.color = color;
        }

        void SetupLocalLightRenderData(LocalLightRenderData& outRenderData) const
        {
            outRenderData.position = position;
            outRenderData.radius = radius;
            outRenderData.direction = direction;
            outRenderData.tangent = tangent;
            outRenderData.color = color;
        }

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

        uint32 GetShadowCascadeCount() const
        {
            return shadowCascadeCount;
        }

        uint32 GetShadowMapSize() const
        {
            return shadowMapSize;
        }

        float GetMaxShadowDistance() const
        {
            return maxShadowDistance;
        }

        float GetShadowCascadeSplitLambda() const
        {
            return shadowCascadeSplitLambda;
        }

    //private:
        LightType lightType;
        Vector3 color;
        Vector3 position;
        Vector3 direction;
        Vector3 tangent;
        float radius;
        bool castRayTracingShadows;
        uint32 shadowMapSize;
        uint32 shadowCascadeCount;
        float shadowCascadeSplitLambda;
        float maxShadowDistance;
        float shadowMapDepthBiasConstantFactor;
        float shadowMapDepthBiasSlopeFactor;
        bool usedAsAtmosphericLight;
        float halfApexAngleInRadians;
        Vector3 atmosphericLightDiskColorFactor;
    };

    class SkyLightRenderObject
    {
    public:
        SkyLightRenderObject();
        virtual ~SkyLightRenderObject();

        RenderGraphPersistentTexture environmentMapTexture;

    private:
    };

    class LocalFogVolumeRenderObject
    {
    public:

        LocalFogVolumeRenderObject();
        ~LocalFogVolumeRenderObject();

        Matrix4x4 transform;

        Vector3 scattering;
        Vector3 absorption;
        Vector3 emission;
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

    struct GPUSceneGeometryData
    {
        int vertexBuffer0;
        int vertexBuffer1;
        int vertexBuffer2;
        int vertexBuffer3;
        int previousVertexBuffer0;
        int indexBuffer;
        int materialBuffer;
        int materialIndexBuffer;
        uint32 vertexCount;
        uint32 indexCount;
    };

    struct GPUSceneGeometryInstanceData
    {
        Matrix4x4 localToWorldMatrix;
        Matrix4x4 worldToLocalMatrix;
        Matrix4x4 previousLocalToWorldMatrix;
        Matrix4x4 previousWorldToLocalMatrix;
        uint32 geometryID;
    };

    class GPUScene
    {
    public:
        // Geometries
        uint32 numGeometries = 0;

        std::vector<GPUSceneGeometryData> geometryData;
        std::vector<GPUSceneGeometryInstanceData> geometryInstanceData;

        uint64 geometryDataBufferSize = 0;
        RenderBackendBufferHandle geometryDataUploadBuffer;
        RenderBackendBufferHandle geometryDataBuffer;

        uint64 geometryInstanceDataBufferSize = 0;
        RenderBackendBufferHandle geometryInstanceDataUploadBuffer;
        RenderBackendBufferHandle geometryInstanceDataBuffer;

        // Materials
        uint32 numMaterials = 0;
        uint64 materialBufferSize = 0;
        std::vector<MaterialShaderParameters> materials;
        RenderBackendBufferHandle materialUploadBuffer;
        RenderBackendBufferHandle materialBuffer;

        // Lights
        uint64 lightDataBufferSize = 0;
        RenderBackendBufferHandle lightDataUploadBuffer;
        RenderBackendBufferHandle lightDataBuffer;
    };

    class RenderScene
    {
    public:

        RenderScene(RenderBackend* renderBackend);

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

        /**
         * Adds a new sky light to the scene.
         */
        virtual void AddSkyLight(SkyLightRenderObject* skyLight);

        /**
         * Removes a sky light from the scene.
         */
        virtual void RemoveSkyLight(SkyLightRenderObject* skyLight);

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

        void UpdateGPUScene(RenderBackendCommandList* commandList);

        GPUScene* GetGPUScene() const
        {
            return gpuScene;
        }

    //private:

        RenderBackend* renderBackend;

        std::vector<MeshRenderObject*> meshes;

        std::vector<LightRenderObject*> lights;

        std::vector<SkyLightRenderObject*> skyLights;

        LightRenderObject* atmosphericLight;

        SkyAtmosphereRenderObject* activeSkyAtmosphere;

        std::vector<SkyAtmosphereRenderObject*> skyAtmospheres;

        std::vector<LocalFogVolumeRenderObject*> localFogVolumes;

        GPUScene* gpuScene;

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

        RenderBackendBufferHandle distantLightDataBuffer;
        RenderBackendBufferHandle distantLightDataUploadBuffer;

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