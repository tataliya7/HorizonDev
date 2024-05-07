#pragma once

#include "RenderBackendCommon.h"
#include "RenderBackendConfig.h"
#include "RenderBackendHandles.h"
#include "RenderBackendTypes.h"

namespace Horizon
{
    enum class RenderBackendCommandQueueType : uint8
    {
        None = 0,
        Copy = (1 << 0),
        Compute = (1 << 1),
        Graphics = (1 << 2),
        All = Copy | Compute | Graphics,
    };

    enum class RenderBackendCommandType
    {
        CopyBuffer,
        CopyTexture,
        UpdateBuffer,
        UpdateTexture,
        ClearTexture,
        Barriers,
        Transitions,
        BeginTiming,
        EndTiming,
        ResolveTimingQueryResults,
        Dispatch,
        DispatchIndirect,
        BuildBottomLevelAS,
        BuildTopLevelAS,
        DispatchRays,
        SetViewport,
        SetScissor,
        SetStencilReference,
        BeginRenderPass,
        EndRenderPass,
        Draw,
        DrawIndirect,
        DispatchMesh,
        DispatchMeshIndirect,
        BeginDebugLabel,
        EndDebugLabel,
        DispatchSuperSampling,
        Count,
    };

    template<RenderBackendCommandType commandType, RenderBackendCommandQueueType queueType>
    struct RenderBackendCommand
    {
        static const RenderBackendCommandType Type = commandType;
        static const RenderBackendCommandQueueType QueueType = queueType;
    };

    struct RenderBackendCommandCopyBuffer : RenderBackendCommand<RenderBackendCommandType::CopyBuffer, RenderBackendCommandQueueType::All>
    {
        RenderBackendBufferHandle srcBuffer;
        uint64 srcOffset;
        RenderBackendBufferHandle dstBuffer;
        uint64 dstOffset;
        uint64 bytes;
    };

    struct RenderBackendCommandCopyTexture : RenderBackendCommand<RenderBackendCommandType::CopyTexture, RenderBackendCommandQueueType::All>
    {
        RenderBackendTextureHandle srcTexture;
        RenderBackendOffset3D srcOffset;
        RenderBackendTextureSubresourceLayers srcSubresourceLayers;
        RenderBackendTextureHandle dstTexture;
        RenderBackendOffset3D dstOffset;
        RenderBackendTextureSubresourceLayers dstSubresourceLayers;
        RenderBackendExtent3D extent;
    };

    struct RenderBackendCommandUpdateBuffer : RenderBackendCommand<RenderBackendCommandType::UpdateBuffer, RenderBackendCommandQueueType::All>
    {
        RenderBackendBufferHandle buffer;
        uint64 offset;
        const void* data;
        uint64 size;
    };

    struct RenderBackendCommandUpdateTexture : RenderBackendCommand<RenderBackendCommandType::UpdateTexture, RenderBackendCommandQueueType::All>
    {
        RenderBackendTextureHandle texture;
    };

    struct RenderBackendCommandBarriers : RenderBackendCommand<RenderBackendCommandType::Barriers, RenderBackendCommandQueueType::All>
    {
        uint32 numBarriers;
    };

    struct RenderBackendCommandClearTextureUAV : RenderBackendCommand<RenderBackendCommandType::ClearTexture, RenderBackendCommandQueueType::All>
    {
        RenderBackendTextureUAVDesc uav;
        RenderBackendTextureClearValue clearValue;
    };

    struct RenderBackendCommandTransitions : RenderBackendCommand<RenderBackendCommandType::Transitions, RenderBackendCommandQueueType::All>
    {
        uint32 numTransitions;
        RenderBackendBarrier* transitions;
    };

    struct RenderBackendCommandBeginTimingQuery : RenderBackendCommand<RenderBackendCommandType::BeginTiming, RenderBackendCommandQueueType::All>
    {
        RenderBackendTimingQueryHeapHandle timingQueryHeap;
        uint32 region;
    };

    struct RenderBackendCommandEndTimingQuery : RenderBackendCommand<RenderBackendCommandType::EndTiming, RenderBackendCommandQueueType::All>
    {
        RenderBackendTimingQueryHeapHandle timingQueryHeap;
        uint32 region;
    };

    struct RenderBackendCommandResolveTimingQueryResults : RenderBackendCommand<RenderBackendCommandType::ResolveTimingQueryResults, RenderBackendCommandQueueType::Graphics>
    {
        RenderBackendTimingQueryHeapHandle timingQueryHeap;
        uint32 regionStart;
        uint32 regionCount;
        RenderBackendBufferHandle buffer;
        uint64 offset;
    };

    struct RenderBackendCommandDispatch : RenderBackendCommand<RenderBackendCommandType::Dispatch, RenderBackendCommandQueueType::Compute>
    {
        RenderBackendShaderHandle shader;
        RenderBackendShaderArguments shaderArguments;
        uint32 threadGroupCountX;
        uint32 threadGroupCountY;
        uint32 threadGroupCountZ;
    };

    struct RenderBackendCommandDispatchIndirect : RenderBackendCommand<RenderBackendCommandType::DispatchIndirect, RenderBackendCommandQueueType::Compute>
    {
        RenderBackendShaderHandle shader;
        RenderBackendShaderArguments shaderArguments;
        RenderBackendBufferHandle argumentBuffer;
        uint64 argumentBufferOffset;
    };

    struct RenderBackendCommandDispatchRays : RenderBackendCommand<RenderBackendCommandType::DispatchRays, RenderBackendCommandQueueType::Graphics>
    {
        RenderBackendRayTracingPipelineStateHandle pipelineState;
        RenderBackendBufferHandle shaderBindingTable;
        RenderBackendShaderArguments shaderArguments;
        uint32 width;
        uint32 height;
        uint32 depth;
    };

    struct RenderBackendCommandBuildBottomLevelAS : RenderBackendCommand<RenderBackendCommandType::BuildBottomLevelAS, RenderBackendCommandQueueType::Compute>
    {
        RenderBackendRayTracingAccelerationStructureHandle srcBLAS;
        RenderBackendRayTracingAccelerationStructureHandle dstBLAS;
    };

    struct RenderBackendCommandBuildTopLevelAS : RenderBackendCommand<RenderBackendCommandType::BuildTopLevelAS, RenderBackendCommandQueueType::Compute>
    {
        RenderBackendRayTracingAccelerationStructureHandle srcTLAS;
        RenderBackendRayTracingAccelerationStructureHandle dstTLAS;
    };

    struct RenderBackendCommandSetViewport : RenderBackendCommand<RenderBackendCommandType::SetViewport, RenderBackendCommandQueueType::Graphics>
    {
        uint32 numViewports;
        RenderBackendViewport viewports[RenderBackendMaxViewportCount];
    };

    struct RenderBackendCommandSetScissor : RenderBackendCommand<RenderBackendCommandType::SetScissor, RenderBackendCommandQueueType::Graphics>
    {
        uint32 numScissors;
        RenderBackendScissor scissors[RenderBackendMaxViewportCount];
    };

    struct RenderBackendCommandSetStencilReference : RenderBackendCommand<RenderBackendCommandType::SetStencilReference, RenderBackendCommandQueueType::Graphics>
    {
        uint32 stencilReference;
    };

    struct RenderBackendCommandBeginRenderPass : RenderBackendCommand<RenderBackendCommandType::BeginRenderPass, RenderBackendCommandQueueType::Graphics>
    {
        RenderBackendRenderPassInfo renderPassInfo;
    };

    struct RenderBackendCommandEndRenderPass : RenderBackendCommand<RenderBackendCommandType::EndRenderPass, RenderBackendCommandQueueType::Graphics>
    {

    };

    struct RenderBackendCommandDraw : RenderBackendCommand<RenderBackendCommandType::Draw, RenderBackendCommandQueueType::Graphics>
    {
        RenderBackendShaderHandle shader;
        RenderBackendGraphicsPipelineState pipelineState;
        RenderBackendShaderArguments shaderArguments;
        RenderBackendBufferHandle indexBuffer;
        union
        {
            struct
            {
                uint32 numVertices;
                uint32 numInstances;
                uint32 firstVertex;
                uint32 firstInstance;
            };
            struct
            {
                uint32 numIndices;
                uint32 numInstances;
                uint32 firstIndex;
                int32 vertexOffset;
                uint32 firstInstance;
            };
        };
        RenderBackendPrimitiveTopology topology;
    };

    struct RenderBackendCommandDrawIndirect : RenderBackendCommand<RenderBackendCommandType::DrawIndirect, RenderBackendCommandQueueType::Graphics>
    {
        RenderBackendShaderHandle shader;
        RenderBackendGraphicsPipelineState pipelineState;
        RenderBackendShaderArguments shaderArguments;
        RenderBackendBufferHandle indexBuffer;
        RenderBackendBufferHandle argumentBuffer;
        uint64 argumentBufferOffset;
        uint32 numDraws;
        RenderBackendPrimitiveTopology topology;
    };

    struct RenderBackendCommandDispatchMesh : RenderBackendCommand<RenderBackendCommandType::DispatchMesh, RenderBackendCommandQueueType::Graphics>
    {
        RenderBackendShaderHandle shader;
        RenderBackendGraphicsPipelineState pipelineState;
        RenderBackendShaderArguments shaderArguments;
        RenderBackendPrimitiveTopology topology;
        uint32 threadGroupCountX;
        uint32 threadGroupCountY;
        uint32 threadGroupCountZ;
    };

    struct RenderBackendCommandDispatchMeshIndirect : RenderBackendCommand<RenderBackendCommandType::DispatchMeshIndirect, RenderBackendCommandQueueType::Graphics>
    {
        RenderBackendShaderHandle shader;
        RenderBackendGraphicsPipelineState pipelineState;
        RenderBackendShaderArguments shaderArguments;
        RenderBackendPrimitiveTopology topology;
        RenderBackendBufferHandle argumentBuffer;
        uint64 argumentBufferOffset;
        uint32 numDraws;
    };

    struct RenderBackendCommandBeginDebugLabel: RenderBackendCommand<RenderBackendCommandType::BeginDebugLabel, RenderBackendCommandQueueType::All>
    {
        char labelName[256];
        float color[4];
    };

    struct RenderBackendCommandEndDebugLabel : RenderBackendCommand<RenderBackendCommandType::EndDebugLabel, RenderBackendCommandQueueType::All>
    {

    };

    struct RenderBackendCommandDispatchSuperSampling : RenderBackendCommand<RenderBackendCommandType::DispatchSuperSampling, RenderBackendCommandQueueType::Compute>
    {
        RenderBackendTextureHandle output;
        RenderBackendTextureHandle color;
        RenderBackendTextureHandle depth;
        RenderBackendTextureHandle motionVectors;
        uint32 renderWidth;
        uint32 renderHeight;
        uint32 targetWidth;
        uint32 targetHeight;
        uint32 viewID;
        float jitterOffsetX;
        float jitterOffsetY;
        float motionVectorScaleX;
        float motionVectorScaleY;
        bool reset;
        float deltaTime;
        bool enableSharpening;
        float sharpeness;
        float cameraFarPlane;
        float cameraNearPlane;
        float cameraFovAngleVertical;
    };
}