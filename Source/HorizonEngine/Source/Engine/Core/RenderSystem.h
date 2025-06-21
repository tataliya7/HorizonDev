#pragma once

#include "Engine/Core/Subsystem.h"
#include "Rendering/RenderingModule.h"

struct ImGuiContext;

namespace Horizon
{
    class RenderSystem final : public Subsystem
    {
    public:

        RenderSystem();
        ~RenderSystem();

        /** Subsystem Interface: TBD. */
        void Init() override;

        /** Subsystem Interface: TBD. */
        void Exit() override;

        void Tick(float deltaTimeInSeconds);

        RenderBackend* GetRenderBackend() const
        {
            return renderBackend;
        }

        ShaderCollection* GetShaderLibrary() const
        {
            return shaderLibrary;
        }

        RenderGraphResourcePool* GetRenderGraphResourcePool() const
        {
            return renderGraphResourcePool;
        }

        RasterizationRenderer* CreateRenderer();

        void RenderSceneView(RasterizationRenderer* renderer, SceneView* sceneView);

        void BeginDrawUI(ImGuiContext* context);

        void EndDrawUI();

        void UpdateImGuiData(RenderBackendCommandList* commandList);

        void DrawUI(RenderBackendCommandList& commandList, RenderBackendTextureHandle output);

        void RenderUserInterface(RenderGraph& renderGraph, const SceneView& view);

        // RenderScene* CreateRenderScene();
        //
        // void RenderScene(SceneView* view) override;
        //
        // void DrawUI(RenderBackendCommandList& commandList, RenderBackendTextureHandle output);
        //
        // bool IsHardwareRayTracingEnabled() const
        // {
        //     return hardwareRayTracingEnabled;
        // }
        // bool hardwareRayTracingEnabled = false;
        //
        // void SetShouldUpdateRayTracingScene(bool value = true) { shouldUpdateRayTracingScene = value; }
        // bool ShouldUpdateRayTracingScene() const { return shouldUpdateRayTracingScene; }
        //
        // const RenderSettings& GetRenderSettings() const;
        //
        // RenderBackendGPUProfiler* gpuProfiler;
        //
        // const RenderSystemDefaultResources& GetDefaultResources() const
        // {
        //     return defaultResources;
        // }
        //
        // RenderGraphResourcePool* GetRenderGraphResourcePool()
        // {
        //     return renderGraphResourcePool;
        // }
        //
        // RenderGraphResourcePool* renderGraphResourcePool;
        //
        // RenderSystemDefaultResources defaultResources;
        //
        // void InitializeDefaultResources(RenderBackendCommandList* commandList);
        // void ReleaseDefaultResources();
        //
        // ShaderLibrary* GetShaderLibrary()
        // {
        //     return shaderLibrary;
        // }
        //
        // Vector3f TransformPosition(const Matrix4x4f& Transform, const Vector3f& Position)
        // {
        //     Vector4f p = Vector4f(Position.x, Position.y, Position.z, 1.0f);
        //     p = Transform * p;
        //     return Vector3f(p.x, p.y, p.z);
        // }
        //
        // void DrawWireframeFrustum(const Matrix4x4f& transform, const Box& box, const Vector4f& color, bool foreground = false)
        // {
        //
        // }
        //
        // void DrawWireframeBox(const Matrix4x4f& transform, const Box& box, const Vector4f& color, bool foreground = false)
        // {
        //     Vector3f B[2], P, Q;
        //     int32 i, j;
        //
        //     B[0] = Box.Min;
        //     B[1] = Box.Max;
        //
        //     for (i = 0; i < 2; i++)
        //     {
        //         for (j = 0; j < 2; j++)
        //         {
        //             P.x = B[i].x; Q.x = B[i].x;
        //             P.y = B[j].y; Q.y = B[j].y;
        //             P.z = B[0].z; Q.z = B[1].z;
        //             P = TransformPosition(Transform, P); Q = TransformPosition(Transform, Q);
        //             DrawLine(P, Q, Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
        //
        //             P.y = B[i].y; Q.y = B[i].y;
        //             P.z = B[j].z; Q.z = B[j].z;
        //             P.x = B[0].x; Q.x = B[1].x;
        //             P = TransformPosition(Transform, P); Q = TransformPosition(Transform, Q);
        //             DrawLine(P, Q, Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
        //
        //             P.z = B[i].z; Q.z = B[i].z;
        //             P.x = B[j].x; Q.x = B[j].x;
        //             P.y = B[0].y; Q.y = B[1].y;
        //             P = TransformPosition(Transform, P); Q = TransformPosition(Transform, Q);
        //             DrawLine(P, Q, Color, DepthPriority, Thickness, DepthBias, bScreenSpace);
        //         }
        //     }
        // }
        //
        // void DrawLine(const Vector3f& Start, const Vector3f& End, const Vector4f& Color, bool bScreenSpace = false)
        // {
        //     debugDrawLinesVertices.push_back(Start);
        //     debugDrawLinesVertices.push_back(End);
        // }

        // TODO: refactor
        RenderBackendGPUProfiler* gpuProfiler;

        RenderBackendType renderBackendType = RenderBackendType::Direct3D12;
        bool enableDebugLayer = true;
        bool enableHardwareRayTracing = true;

    private:

        RenderBackend* renderBackend;
        ShaderCollection* shaderLibrary;
        RenderGraphResourcePool* renderGraphResourcePool;

        // TODO: rename
        RendererDefaultResources* rendererDefaultResources;

        //
        // void CompileShaders_Deprecated();
        //
        // void AddLight(const SceneView& view, LightComponent& lightComponent, const CameraComponent& camera);
        //
        // void UpdateRenderData(SceneView* view, RenderBackendCommandList* commandList);
        // void UpdateSkyLight(EnvironmentLightComponent& skyLight);
        //
        // FrameAllocator* frameAllocator;
        // RenderBackend* renderBackend;
        // ShaderCompiler* shaderCompiler;
        // ShaderLibrary* shaderLibrary;
        //
        // RenderBackendTimingQueryHeapHandle timingQueryHeap;
        //
        // RenderBackendTextureHandle blueNoiseTexture;
        //
        // // UI
        // ImGuiContext* context;
        // RenderBackendTextureHandle defaultFontTexture;
        //
        uint32 frameInFlightCounter = 0;

        uint64 vertexBufferSize[3];
        uint64 currentVertexBufferDataSize[3];
        RenderBackendBufferHandle vertexBuffer[3];
        RenderBackendBufferHandle vertexBufferUpload[3];

        uint64 indexBufferSize[3];
        uint64 currentIndexBufferDataSize[3];
        RenderBackendBufferHandle indexBuffer[3];
        RenderBackendBufferHandle indexBufferUpload[3];

        uint32 totalDrawCommandCount = 0;

        uint64 drawDataBufferSize[3];
        uint64 currentDrawDataBufferDataSize[3];
        RenderBackendBufferHandle drawDataBuffer[3];
        RenderBackendBufferHandle drawDataBufferUpload[3];

        uint64 drawIndexedIndirectCommandBufferSize[3];
        uint64 currentDrawIndexedIndirectCommandBufferDataSize[3];
        RenderBackendBufferHandle drawIndexedIndirectCommandBuffer[3];
        RenderBackendBufferHandle drawIndexedIndirectCommandBufferUpload[3];

        //
        // bool shouldUpdateRayTracingScene = false;
        //
        // void UpdateRayTracingAccelerationStructures(SceneView* view, RenderBackendCommandList* commandList);
    };
}