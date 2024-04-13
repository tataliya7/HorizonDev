#pragma once

#include "Core/CoreModule.h"
#include "Rendering/RenderEngine.h"
#include "Rendering/RenderGraph/RenderGraph.h"
#include "Rendering/SceneView.h"
#include "Rendering/ShaderLibrary.h"
#include "Rendering/Renderer/RendererPrivate.h"

#include <imgui.h>

namespace HE
{
    class Scene;
    class RenderBackend;
    class ShaderCompiler;
    class ShaderLibrary_Deprecated;

    struct SkyAtmosphereComponent;

    struct Box
    {
        Vector3 Min;
        Vector3 Max;
    };

    struct RealTimeRendererSettings;

    class RenderPipeline
    {
    public:
        virtual void SetupRenderGraph(RenderGraph& renderGraph, const SceneView& view) = 0;
    };

    class FrameAllocator : public MemoryArena
    {

    };

    struct RealTimeRendererRenderSettings
    {

    };

    struct PathTracingRendererRenderSettings
    {

    };

    class Renderer : public RenderEngine
    {
    public:
        Renderer();
        virtual ~Renderer();

        void Init(void* data) override;
        void Exit() override;
        bool IsCustom() const override { return false; }
        void RenderScene(SceneView* view) override;
        void BeginDrawUI() override;
        void EndDrawUI() override;

        void DrawUI(RenderBackendCommandList& commandList, RenderBackendTextureHandle output);

        void SetShouldUpdateRayTracingScene(bool value = true) { shouldUpdateRayTracingScene = value; }
        bool ShouldUpdateRayTracingScene() const { return shouldUpdateRayTracingScene; }

        RealTimeRendererSettings& GetRealTimeRendererSettings_Deprecated();

        RenderBackendGPUProfiler* gpuProfiler;

        ShaderLibrary_Deprecated* GetShaderLibrary()
        {
            return shaderLibrary;
        }

        int64 updateCounter = 0;

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

        RenderBackendRayTracingAccelerationStructureHandle rayTracingScene;
        RenderBackendRayTracingAccelerationStructureHandle bottomLevelAS;

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

        LightComponent* skyAtmosphereLight = nullptr;
        SkyAtmosphereComponent* skyAtmosphereComponent = nullptr;

        // Debug draw
        std::vector<Vector3> debugDrawLinesVertices;

        uint32 debugDrawLinesVertexBufferSize = 0;
        RenderBackendBufferHandle debugDrawLinesVertexBuffer;
        RenderBackendBufferHandle debugDrawLinesVertexUploadBuffer;

        Vector3 TransformPosition(const Matrix4x4& Transform, const Vector3& Position)
        {
            Vector4 p = Vector4(Position.x, Position.y, Position.z, 1.0f);
            p = Transform * p;
            return Vector3(p.x, p.y, p.z);
        }

        void DrawWireBox(
            const Matrix4x4& Transform,
            const Box& Box,
            const Vector4& Color,
            uint8 DepthPriority,
            float Thickness = 0.0f,
            float DepthBias = 0.0f,
            bool bScreenSpace = false)
        {
            Vector3    B[2], P, Q;
            int32 i, j;

            B[0] = Box.Min;
            B[1] = Box.Max;

            for (i = 0; i < 2; i++)
            {
                for (j = 0; j < 2; j++)
                {
                    P.x = B[i].x; Q.x = B[i].x;
                    P.y = B[j].y; Q.y = B[j].y;
                    P.z = B[0].z; Q.z = B[1].z;
                    P = TransformPosition(Transform, P); Q = TransformPosition(Transform, Q);
                    DrawLine(P, Q, Color, DepthPriority, Thickness, DepthBias, bScreenSpace);

                    P.y = B[i].y; Q.y = B[i].y;
                    P.z = B[j].z; Q.z = B[j].z;
                    P.x = B[0].x; Q.x = B[1].x;
                    P = TransformPosition(Transform, P); Q = TransformPosition(Transform, Q);
                    DrawLine(P, Q, Color, DepthPriority, Thickness, DepthBias, bScreenSpace);

                    P.z = B[i].z; Q.z = B[i].z;
                    P.x = B[j].x; Q.x = B[j].x;
                    P.y = B[0].y; Q.y = B[1].y;
                    P = TransformPosition(Transform, P); Q = TransformPosition(Transform, Q);
                    DrawLine(P, Q, Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
                }
            }
        }

        void DrawLine(
            const Vector3& Start,
            const Vector3& End,
            const Vector4& Color,
            uint8 DepthPriorityGroup,
            float Thickness = 0.0f,
            float DepthBias = 0.0f,
            bool bScreenSpace = false)
        {
            debugDrawLinesVertices.push_back(Start);
            debugDrawLinesVertices.push_back(End);
        }

    private:

        void AddLight(const SceneView& view, LightComponent& lightComponent, const CameraComponent& camera);

        void UpdateRenderData(SceneView* view, RenderBackendCommandList* commandList);
        void UpdateSkyLight(EnvironmentLightComponent& skyLight);

        FrameAllocator* frameAllocator;
        MemoryArena* arena;
        RenderBackend* renderBackend;
        ShaderCompiler* shaderCompiler;
        ShaderLibrary_Deprecated* shaderLibrary;
        RenderPipeline* renderPipeline;

        RenderBackendTimingQueryHeapHandle timingQueryHeap;

        // UI
        ImGuiContext* context;
        RenderBackendTextureHandle defaultFontTexture;
        RenderBackendShaderHandle imguiShader;

        uint32 frameInFlightCounter = 0;

        uint64 vertexBufferSize[3];
        uint64 currentVertexBufferDataSize[3];
        RenderBackendBufferHandle vertexBuffer[3];
        RenderBackendBufferHandle vertexBufferUpload[3];

        uint64 indexBufferSize[3];
        uint64 currentIndexBufferDataSize[3];
        RenderBackendBufferHandle indexBuffer[3];
        RenderBackendBufferHandle indexBufferUpload[3];

        bool shouldUpdateRayTracingScene = false;

        void UpdateRayTracingAccelerationStructures(SceneView* view, RenderBackendCommandList* commandList);
    };

    extern Renderer* GRenderer;
    extern void Texture2DGenerateMips(ShaderLibrary_Deprecated* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle textureHandle, uint32 width, uint32 height, uint32 numMipLevels);
}