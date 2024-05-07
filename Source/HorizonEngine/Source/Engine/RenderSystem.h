#pragma once

#include "Core/CoreModule.h"
#include "Rendering/RenderGraph/RenderGraph.h"
#include "Rendering/SceneView.h"
#include "Rendering/ShaderLibrary.h"

#include <imgui.h>

namespace Horizon
{
    class Scene;
    class RenderBackend;
    class ShaderCompiler;
    class ShaderLibrary_DEPRECATED;

    struct SkyAtmosphereComponent;

    struct Box
    {
        Vector3 Min;
        Vector3 Max;
    };

    class FrameAllocator : public MemoryArena
    {

    };


    enum class ShaderPipelineID
    {
        PreIntegratedBRDF,
        EquirectangularToCubemap,
        DownsampleCubemap,
        DownsampleTexture2D,
        DownsampleTexture2D_PS,
        ComputeEnvironmentIrradiance,
        ComputeEnvironmentIrradianceSH,
        FilterEnvironmentMap,
        SharedMemoryComplexFFT,
        SharedMemoryComplexIFFT,
        SharedMemoryTwoForOneRealFFT,
        SharedMemoryTwoForOneRealIFFT,
        SharedMemoryComplexFFTConvolution,
        UIColorAndAlpha,
        Count,
    };

    class RenderSystemDefaultResources
    {
    public:
        RenderGraphTextureHandle ImportBlackDummyTexture2D(RenderGraph& renderGraph) const;
        RenderGraphTextureHandle ImportWhiteDummyTexture2D(RenderGraph& renderGraph) const;
        RenderBackendTextureHandle GetPreIntegratedBrdfLut() const;
    private:
        friend class RenderSystem;
        RenderGraphPersistentTexture* blackDummyTexture2D;
        RenderGraphPersistentTexture* whiteDummyTexture2D;

        static const uint32 PreIntegratedBrdfLutSize = 256;
        RenderBackendTextureHandle preIntegratedBrdfLut;

        RenderBackendSamplerHandle globalSamplerLinearWarp;
        RenderBackendSamplerHandle globalSamplerLinearClamp;
        RenderBackendSamplerHandle globalSamplerLinearBorder;
        RenderBackendSamplerHandle globalSamplerPointWarp;
        RenderBackendSamplerHandle globalSamplerPointClamp;
        RenderBackendSamplerHandle globalSamplerPointBorder;
        RenderBackendSamplerHandle globalSamplerComparisonGreaterLinearClamp;
        RenderBackendSamplerHandle globalSamplerComparisonLessLinearClamp;
    };

    class RenderSystem : public EngineSubsystem
    {
    public:
        RenderSystem();
        virtual ~RenderSystem();

        /** Subsystem Interface: TBD. */
        void Init() override;

        /** Subsystem Interface: TBD. */
        void Exit() override;

        RenderScene* CreateRenderScene();

        void RenderScene(SceneView* view) override;

        void BeginDrawUI();
        void EndDrawUI();

        void Tick();

        void DrawUI(RenderBackendCommandList& commandList, RenderBackendTextureHandle output);

        bool IsHardwareRayTracingEnabled() const
        {
            return hardwareRayTracingEnabled;
        }
        bool hardwareRayTracingEnabled = false;

        void SetShouldUpdateRayTracingScene(bool value = true) { shouldUpdateRayTracingScene = value; }
        bool ShouldUpdateRayTracingScene() const { return shouldUpdateRayTracingScene; }

        const RenderSettings& GetRenderSettings() const;

        RenderBackendGPUProfiler* gpuProfiler;

        const RenderSystemDefaultResources& GetDefaultResources() const
        {
            return defaultResources;
        }

        RenderGraphResourcePool* GetRenderGraphResourcePool()
        {
            return renderGraphResourcePool;
        }

        RenderGraphResourcePool* renderGraphResourcePool;

        RenderSystemDefaultResources defaultResources;

        void InitializeDefaultResources(RenderBackendCommandList* commandList);
        void ReleaseDefaultResources();

        ShaderLibrary_DEPRECATED* GetShaderLibrary()
        {
            return shaderLibrary;
        }

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

        void CompileShaders_DEPRECATED();

        void AddLight(const SceneView& view, LightComponent& lightComponent, const CameraComponent& camera);

        void UpdateRenderData(SceneView* view, RenderBackendCommandList* commandList);
        void UpdateSkyLight(EnvironmentLightComponent& skyLight);

        FrameAllocator* frameAllocator;
        MemoryArena* arena;
        RenderBackend* renderBackend;
        ShaderCompiler* shaderCompiler;
        ShaderLibrary_DEPRECATED* shaderLibrary;

        RenderBackendTimingQueryHeapHandle timingQueryHeap;

        RenderBackendTextureHandle blueNoiseTexture;

        // UI
        ImGuiContext* context;
        RenderBackendTextureHandle defaultFontTexture;
        RenderBackendShaderHandle imguiShader;

        uint64 vertexBufferSize[3];
        uint64 currentVertexBufferDataSize[3];
        RenderBackendBufferHandle vertexBuffer[3];
        RenderBackendBufferHandle vertexBufferUpload[3];

        uint64 indexBufferSize[3];
        uint64 currentIndexBufferDataSize[3];
        RenderBackendBufferHandle indexBuffer[3];
        RenderBackendBufferHandle indexBufferUpload[3];

        void RenderPreIntegratedBrdfLut(RenderBackendCommandList* commandList);

        bool shouldUpdateRayTracingScene = false;

        void UpdateRayTracingAccelerationStructures(SceneView* view, RenderBackendCommandList* commandList);
    };

    extern void Texture2DGenerateMips(ShaderLibrary_DEPRECATED* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle textureHandle, uint32 width, uint32 height, uint32 numMipLevels);
}