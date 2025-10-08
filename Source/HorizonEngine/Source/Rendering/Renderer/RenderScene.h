#pragma once

#include "RendererCommon.h"
#include "RendererPrivate.h" // TODO
#include "RenderStatistics.h"
#include "ShaderRepository.h"
#include "RayTracing/RayTracingScene.h"

namespace Horizon
{
    struct Material
    {
        std::string name;

        // BxDF
        Vector4f baseColor = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
        float metallic = 0.0f;
        float roughness = 0.5f;
        float specular = 0.5f;
        float specularTint = 0.0f;
        float transmission = 0.0f;
        float transmissionRoughness = 0.0f;
        float clearcoat = 0.0f;
        float clearcoatRoughness = 0.0f;
        Vector4f emission = Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
        float emissionStrength = 1.0f;
        float alpha = 1.0f;

        // SSS
        Vector4f sssSurfaceAlbedo = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
        Vector4f sssMFP = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
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
        Vector4f baseColor = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
        float metallic = 0.0f;
        float roughness = 0.5f;
        float specular = 0.5f;
        float specularTint = 0.0f;
        float transmission = 0.0f;
        float transmissionRoughness = 0.0f;
        float clearcoat = 0.0f;
        float clearcoatRoughness = 0.0f;
        Vector4f emission = Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
        float emissionStrength = 1.0f;
        float alpha = 1.0f;

        // SSS
        Vector4f sssSurfaceAlbedo = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
        Vector4f sssMFP = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
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

        bool UseGPUSkinning() const
        {
            return useGPUSkinning;
        }

        bool castDynamicShadows : 1;

        bool useGPUSkinning = false;

        Matrix4x4f localToWorldMatrix;
        Matrix4x4f worldToLocalMatrix;

        uint32 vertexCount;
        uint32 indexCount;
        uint32 meshletCount;

        RenderBackendBufferHandle vertexBuffers[4];
        RenderBackendBufferHandle indexBuffer;
        RenderBackendBufferHandle vertexBuffer;
        RenderBackendBufferHandle meshletBuffer;
        RenderBackendBufferHandle meshletVertexBuffer;
        RenderBackendBufferHandle meshletTriangleBuffer;
        RenderBackendBufferHandle materialBuffer;
        RenderBackendBufferHandle materialIndexBuffer;

        uint32 relevantJointCountPerVertex;
        RenderBackendBufferHandle jointIndexBuffer;
        RenderBackendBufferHandle jointWeightBuffer;
        RenderBackendBufferHandle jointTransformBuffer;

        RenderBackendBufferHandle instanceUploadBuffer;
        RenderBackendBufferHandle transformUploadBuffer;

        uint32 transformMatrixCount = 0;
        uint64 transformBufferSize = 0;
        std::vector<Matrix4x4f> rowMajorTransforms;
        RenderBackendBufferHandle transformBufferRowMajor;
        RenderBackendBufferHandle transformBufferRowMajorUpload;

        RenderBackendRayTracingAccelerationStructureHandle bottomLevelAccelerationStructure;

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
        Vector3f color;
        Vector3f position;
        Vector3f direction;
        float radius;
        bool castDynamicShadows;
        uint32 shadowMapSize;
        uint32 shadowCascadeCount;
        float shadowCascadeSplitLambda;
        float shadowCascadeTransitionScale;
        float maxShadowDistance;
        float shadowFadeOutFactor;
        float shadowMapDepthBiasConstantFactor;
        float shadowMapDepthBiasSlopeFactor;
        bool enableScreenSpaceShadows;
        float screenSpaceShadowsSurfaceThickness;
        float screenSpaceShadowsShadowContrast;
        bool usedAsAtmosphericLight;
        float halfApexAngleInRadians;
        Vector3f atmosphericLightDiskColorFactor;
        bool enableLightShafts;
        float lightShaftsIntensity;
        Vector3f lightShaftsColor;
    };

    class LightRenderObject
    {
    public:
        LightRenderObject(const LightRenderObjectDescription& description);
        virtual ~LightRenderObject();

        bool IsDistantLight() const
        {
            return lightType == LightType::DistantLight;
        }

        bool IsLocalLight() const
        {
            return lightType == LightType::PointLight || lightType == LightType::SpotLight;
        }

        Vector3f GetPhysicalLightColor() const
        {
            return color;
        }

        Vector3f GetDirection() const
        {
            return direction;
        }

        Sphere GetBoundingSphere() const
        {
            return { position, radius };
        }

        float GetHalfApexAngleInRadians() const
        {
            return halfApexAngleInRadians;
        }

        Vector3f GetAtmosphericLightDiskColorFactor() const
        {
            return atmosphericLightDiskColorFactor;
        }

        bool CastDynamicShadows() const
        {
            return castDynamicShadows;
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

        bool IsLightShaftsEnabled() const
        {
            return enableLightShafts;
        }

        float GetLightShaftsIntensity() const
        {
            return lightShaftsIntensity;
        }

        Vector3f GetLightShaftsColor() const
        {
            return lightShaftsColor;
        }

        float GetCullDistance() const
        {
            return cullDistance;
        }
    //private:
        LightType lightType;
        Vector3f color;
        Vector3f position;
        Vector3f direction;
        Vector3f tangent;
        Matrix4x4f worldToLight;
        float radius;
        float cullDistance = 1000.0f;
        //float fadeRange = ;
        bool castDynamicShadows;
        uint32 shadowMapSize;
        uint32 shadowCascadeCount;
        float shadowCascadeSplitLambda;
        float shadowCascadeBlendScale;
        float maxShadowDistance;
        float shadowFadeOutFactor;
        float shadowMapDepthBiasConstantFactor;
        float shadowMapDepthBiasSlopeFactor;
        bool enableScreenSpaceShadows;
        float screenSpaceShadowsSurfaceThickness;
        float screenSpaceShadowsShadowContrast;

        bool usedAsAtmosphericLight;
        float halfApexAngleInRadians;
        Vector3f atmosphericLightDiskColorFactor;

        float enableLightShafts;
        float lightShaftsIntensity;
        Vector3f lightShaftsColor;
    };

    class SkyLightRenderObject
    {
    public:
        SkyLightRenderObject();
        virtual ~SkyLightRenderObject();

        uint32 cubemapSize = 0;

        RenderGraphPersistentTexture* environmentMapTexture;

    private:
    };

    class LocalFogVolumeRenderObject
    {
    public:

        LocalFogVolumeRenderObject();
        ~LocalFogVolumeRenderObject();

        Matrix4x4f transform;

        Vector3f scattering;
        Vector3f absorption;
        Vector3f emission;
    };

    struct AtmosphereParameters
    {
        float bottomRadius;
        float topRadius;
        Vector3f groundAlbedo;

        Vector3f rayleighScattering;
        float rayleighDensityExpScale;

        Vector3f mieScattering;
        Vector3f mieExtinction;
        Vector3f mieAbsorption;
        float miePhaseG;
        float mieDensityExpScale;

        float absorptionDensity0LayerWidth;
        float absorptionDensity0ConstantTerm;
        float absorptionDensity0LinearTerm;
        float absorptionDensity1ConstantTerm;
        float absorptionDensity1LinearTerm;
        Vector3f absorptionExtinction;
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

        Vector3f GetSkyLuminanceFactor() const
        {
            return skyLuminanceFactor;
        }

        void SetSkyLuminanceFactor(const Vector3f& newValue)
        {
            skyLuminanceFactor = newValue;
        }

    private:

        AtmosphereParameters atmosphereParameters;

        Vector3f skyLuminanceFactor;
    };

    class GlobalFogRenderObject
    {
    public:

        GlobalFogRenderObject();

        ~GlobalFogRenderObject();

        float distance;

        Vector3f scattering;
        Vector3f absorption;
        Vector3f emission;
        float phaseG;

    private:
    };

#define GPU_SCENE_GEOMETRY_DATA_FLAG_USE_GPU_SKINNING 1

    struct GPUSceneGeometryData
    {
        int vertexBuffer0;
        int vertexBuffer1;
        int vertexBuffer2;
        int vertexBuffer3;
        int vertexBuffer;
        int previousVertexBuffer0;
        int indexBuffer;
        int meshletBuffer;
        int meshletVertexBuffer;
        int meshletTriangleBuffer;
        int materialBuffer;
        int materialIndexBuffer;
        int jointIndexBuffer;
        int jointWeightBuffer;
        int jointTransformBuffer;
        uint32 vertexCount;
        uint32 indexCount;
        uint32 meshletCount;
        uint32 flags;
    };

    struct GPUSceneGeometryInstanceData
    {
        Matrix4x4f localToWorldMatrix;
        Matrix4x4f worldToLocalMatrix;
        Matrix4x4f previousLocalToWorldMatrix;
        Matrix4x4f previousWorldToLocalMatrix;
        uint32 geometryID;
    };

    struct GPUSceneMeshletData
    {
        uint32 vertexOffset;
        uint32 triangleOffset;
        uint32 vertexCount;
        uint32 triangleCount;
        Vector3f boundingBoxCenter;
        float padding0;
        Vector3f boundingBoxExtent;
        int isSkinned;
    };

    static constexpr uint32 VirtualGeometryVertexMaximumTextureCoordinateCount = 2;

    struct VirtualGeometryVertex
    {
        Vector3f position;
        float padding0;
        Vector3f normal;
        float padding1;
        Vector4f tangent;
        Vector2f textureCoordinates[VirtualGeometryVertexMaximumTextureCoordinateCount];
    };

    class GPUScene
    {
    public:
        // Geometries
        uint32 geometryCount = 0;
        uint32 geometryInstanceCount = 0;

        std::vector<GPUSceneGeometryData> geometryData;
        std::vector<GPUSceneGeometryInstanceData> geometryInstanceData;

        uint64 geometryDataBufferSize = 0;
        RenderBackendBufferHandle geometryDataUploadBuffer;
        RenderBackendBufferHandle geometryDataBuffer;

        uint64 geometryInstanceDataBufferSize = 0;
        RenderBackendBufferHandle geometryInstanceDataUploadBuffer;
        RenderBackendBufferHandle geometryInstanceDataBuffer;

        // Materials
        uint32 materialCount = 0;
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

        Matrix4x4f localToWorldMatrix;
        std::vector<GPUSceneMeshletData> meshlets;

        RenderScene(RenderBackend* renderBackend, ShaderRepository* shaderRepository);

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

        virtual bool HasActiveGlobalFog() const;

        virtual GlobalFogRenderObject* GetActiveGlobalFog() const;

        virtual void AddGlobalFog(GlobalFogRenderObject* globalFog);

        virtual void RemoveGlobalFog(GlobalFogRenderObject* globalFog);

        virtual bool HasAnyLocalFogVolume() const;

        virtual void AddLocalFogVolume(LocalFogVolumeRenderObject* localFogVolume);

        virtual void RemoveLocalFogVolume(LocalFogVolumeRenderObject* localFogVolume);

        void GetRenderStatistics(RenderStatistics& statistics) const;

        void UpdateGPUScene(RenderGraph& renderGraph, RenderBackendCommandList* commandList);

        bool ShouldUpdateRayTracingScene()
        {
            return rayTracingScene && true;
        }

        GPUScene* GetGPUScene() const
        {
            return gpuScene;
        }

        RayTracingScene* CreateRayTracingScene();

        RayTracingScene* GetRayTracingScene() const
        {
            return rayTracingScene;
        }
    //private:

        RenderBackend* renderBackend;

        ShaderRepository* shaderRepository;

        std::vector<MeshRenderObject*> meshes;

        std::vector<LightRenderObject*> lights;

        std::vector<SkyLightRenderObject*> skyLights;

        LightRenderObject* atmosphericLight;

        SkyAtmosphereRenderObject* activeSkyAtmosphere;

        std::vector<SkyAtmosphereRenderObject*> skyAtmospheres;

        std::vector<GlobalFogRenderObject*> globalFogs;

        GlobalFogRenderObject* activeGlobalFog;

        std::vector<LocalFogVolumeRenderObject*> localFogVolumes;

        GPUScene* gpuScene;

        RayTracingScene* rayTracingScene;

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

        RenderBackendBufferDescription instanceBufferDesc;
        RenderBackendBufferHandle instanceBuffer;
        RenderBackendBufferHandle instanceUploadBuffer;

        // Transforms
        RenderBackendBufferHandle transformBuffer;
        RenderBackendBufferHandle transformUploadBuffer;
        RenderBackendBufferHandle previousTransformBuffer;

        RenderBackendBufferHandle lightDataBuffer;
        RenderBackendBufferHandle lightDataUploadBuffer;

        RenderBackendBufferHandle distantLightDataBuffer;
        RenderBackendBufferHandle distantLightDataUploadBuffer;

        RenderBackendBufferHandle localLightShadowMapBuffer;
        RenderBackendBufferHandle localLightShadowMapUploadBuffer;

        // Environment maps
        RenderBackendTextureHandle environmentMapTexture;
        RenderBackendTextureHandle convolvedEnvironmentMapTexture;
        RenderBackendTextureHandle irradianceEnvironmentMapTexture;
        RenderBackendBufferHandle irradianceEnvironmentMapBuffer;
        RenderBackendBufferHandle irradianceEnvironmentMapBufferFast;

        // Debug draw
        // std::vector<Vector3f> debugDrawLinesVertices;
        // uint32 debugDrawLinesVertexBufferSize = 0;
        // RenderBackendBufferHandle debugDrawLinesVertexBuffer;
        // RenderBackendBufferHandle debugDrawLinesVertexUploadBuffer;
    };
}