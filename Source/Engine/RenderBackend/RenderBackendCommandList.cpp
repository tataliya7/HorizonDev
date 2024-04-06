#include "RenderBackendCommandList.h"

namespace HE
{
    void RenderBackendCommandList::CopyTexture2D(RenderBackendTextureHandle srcTexture, const Offset2D& srcOffset, uint32 srcMipLevel, RenderBackendTextureHandle dstTexture, const Offset2D& dstOffset, uint32 dstMipLevel, const Extent2D extent)
    {
        RenderBackendCommandCopyTexture* command = AllocateCommand<RenderBackendCommandCopyTexture>(RenderBackendCommandCopyTexture::Type);
        command->srcTexture = srcTexture;
        command->srcOffset = {
            .x = srcOffset.x,
            .y = srcOffset.y,
            .z = 0,
        };
        command->srcSubresourceLayers = {
            .mipLevel = srcMipLevel,
            .firstLayer = 0,
            .arrayLayers = 1,
        };
        command->dstTexture = dstTexture;
        command->dstOffset = {
            .x = dstOffset.x,
            .y = dstOffset.y,
            .z = 0,
        };
        command->dstSubresourceLayers = {
            .mipLevel = dstMipLevel,
            .firstLayer = 0,
            .arrayLayers = 1,
        };
        command->extent = {
            .width = extent.width,
            .height = extent.height,
            .depth = 1,
        };
    }

    void RenderBackendCommandList::CopyBuffer(RenderBackendBufferHandle srcBuffer, uint64 srcOffset, RenderBackendBufferHandle dstBuffer, uint64 dstOffset, uint64 bytes)
    {
        RenderBackendCommandCopyBuffer* command = AllocateCommand<RenderBackendCommandCopyBuffer>(RenderBackendCommandCopyBuffer::Type);
        command->srcBuffer = srcBuffer;
        command->srcOffset = srcOffset;
        command->dstBuffer = dstBuffer;
        command->dstOffset = dstOffset;
        command->bytes = bytes;
    }

    void RenderBackendCommandList::UpdateBuffer(RenderBackendBufferHandle buffer, uint64 offset, const void* data, uint64 size)
    {
        RenderBackendCommandUpdateBuffer* command = AllocateCommand<RenderBackendCommandUpdateBuffer>(RenderBackendCommandUpdateBuffer::Type);
        command->buffer = buffer;
        command->offset = offset;
        command->data = data;
        command->size = size;
    }

    void RenderBackendCommandList::ClearTextureUAV(const RenderBackendTextureUAVDesc& uav, const RenderBackendTextureClearValue& clearColor)
    {
        RenderBackendCommandClearTextureUAV* command = AllocateCommand<RenderBackendCommandClearTextureUAV>(RenderBackendCommandClearTextureUAV::Type);
        command->uav = uav;
        command->clearValue = clearColor;
    }

    void RenderBackendCommandList::Dispatch(RenderBackendShaderHandle shader, const RenderBackendShaderArguments& shaderArguments, uint32 x, uint32 y, uint32 z)
    {
        RenderBackendCommandDispatch* command = AllocateCommand<RenderBackendCommandDispatch>(RenderBackendCommandDispatch::Type);
        command->shader = shader;
        command->threadGroupCountX = x;
        command->threadGroupCountY = y;
        command->threadGroupCountZ = z;
        memcpy(&command->shaderArguments, &shaderArguments, sizeof(RenderBackendShaderArguments));
    }

    void RenderBackendCommandList::Dispatch2D(RenderBackendShaderHandle shader, const RenderBackendShaderArguments& shaderArguments, uint32 x, uint32 y)
    {
        Dispatch(shader, shaderArguments, x, y, 1);
    }

    void RenderBackendCommandList::DispatchIndirect(RenderBackendShaderHandle shader, const RenderBackendShaderArguments& shaderArguments, RenderBackendBufferHandle argumentBuffer, uint64 argumentBufferOffset)
    {
        RenderBackendCommandDispatchIndirect* command = AllocateCommand<RenderBackendCommandDispatchIndirect>(RenderBackendCommandDispatchIndirect::Type);
        command->shader = shader;
        command->argumentBuffer = argumentBuffer;
        command->argumentBufferOffset = argumentBufferOffset;
        memcpy(&command->shaderArguments, &shaderArguments, sizeof(RenderBackendShaderArguments));
    }

    void RenderBackendCommandList::UpdateRayTracingBottomLevelAccelerationStructure(RenderBackendRayTracingAccelerationStructureHandle srcBLAS, RenderBackendRayTracingAccelerationStructureHandle dstBLAS)
    {
        RenderBackendCommandBuildBottomLevelAS* command = AllocateCommand<RenderBackendCommandBuildBottomLevelAS>(RenderBackendCommandBuildBottomLevelAS::Type);
        command->srcBLAS = srcBLAS;
        command->dstBLAS = dstBLAS;
    }

    void RenderBackendCommandList::UpdateRayTracingTopLevelAccelerationStructure(RenderBackendRayTracingAccelerationStructureHandle srcTLAS, RenderBackendRayTracingAccelerationStructureHandle dstTLAS)
    {
        RenderBackendCommandBuildTopLevelAS* command = AllocateCommand<RenderBackendCommandBuildTopLevelAS>(RenderBackendCommandBuildTopLevelAS::Type);
        command->srcTLAS = srcTLAS;
        command->dstTLAS = dstTLAS;
    }

    void RenderBackendCommandList::BuildRayTracingBottomLevelAccelerationStructure(RenderBackendRayTracingAccelerationStructureHandle blas)
    {
        RenderBackendCommandBuildBottomLevelAS* command = AllocateCommand<RenderBackendCommandBuildBottomLevelAS>(RenderBackendCommandBuildBottomLevelAS::Type);
        command->srcBLAS = RenderBackendRayTracingAccelerationStructureHandle::Null;
        command->dstBLAS = blas;
    }

    void RenderBackendCommandList::BuildRayTracingTopLevelAccelerationStructure(RenderBackendRayTracingAccelerationStructureHandle tlas)
    {
        RenderBackendCommandBuildTopLevelAS* command = AllocateCommand<RenderBackendCommandBuildTopLevelAS>(RenderBackendCommandBuildTopLevelAS::Type);
        command->srcTLAS = RenderBackendRayTracingAccelerationStructureHandle::Null;
        command->dstTLAS = tlas;
    }

    void RenderBackendCommandList::TraceRays(RenderBackendRayTracingPipelineStateHandle pipelineState, RenderBackendBufferHandle shaderBindingTable, const RenderBackendShaderArguments& shaderArguments, uint32 x, uint32 y, uint32 z)
    {
        RenderBackendCommandTraceRays* command = AllocateCommand<RenderBackendCommandTraceRays>(RenderBackendCommandTraceRays::Type);
        command->pipelineState = pipelineState;
        command->shaderBindingTable = shaderBindingTable;
        command->width = x;
        command->height = y;
        command->depth = z;
        memcpy(&command->shaderArguments, &shaderArguments, sizeof(RenderBackendShaderArguments));
    }

    void RenderBackendCommandList::SetViewports(RenderBackendViewport* viewports, uint32 numViewports)
    {
        RenderBackendCommandSetViewport* command = AllocateCommand<RenderBackendCommandSetViewport>(RenderBackendCommandSetViewport::Type);
        command->numViewports = numViewports;
        memcpy(command->viewports, viewports, numViewports * sizeof(RenderBackendViewport));
    }

    void RenderBackendCommandList::SetScissors(RenderBackendScissor* scissors, uint32 numScissors)
    {
        RenderBackendCommandSetScissor* command = AllocateCommand<RenderBackendCommandSetScissor>(RenderBackendCommandSetScissor::Type);
        command->numScissors = numScissors;
        memcpy(command->scissors, scissors, numScissors * sizeof(RenderBackendScissor));
    }

    void RenderBackendCommandList::SetStencilReference(uint32 stencilReference)
    {
        RenderBackendCommandSetStencilReference* command = AllocateCommand<RenderBackendCommandSetStencilReference>(RenderBackendCommandSetStencilReference::Type);
        command->stencilReference = stencilReference;
    }

    void RenderBackendCommandList::BeginRenderPass(const RenderBackendRenderPassInfo& renderPassInfo)
    {
        RenderBackendCommandBeginRenderPass* command = AllocateCommand<RenderBackendCommandBeginRenderPass>(RenderBackendCommandBeginRenderPass::Type);
        memcpy(command, &renderPassInfo, sizeof(RenderBackendRenderPassInfo));
    }

    void RenderBackendCommandList::EndRenderPass()
    {
        RenderBackendCommandEndRenderPass* command = AllocateCommand<RenderBackendCommandEndRenderPass>(RenderBackendCommandEndRenderPass::Type);
    }

    void RenderBackendCommandList::Transitions(RenderBackendBarrier* transitions, uint32 numTransitions)
    {
        RenderBackendCommandTransitions* command = AllocateCommand<RenderBackendCommandTransitions>(RenderBackendCommandTransitions::Type, sizeof(RenderBackendCommandTransitions) + numTransitions * sizeof(RenderBackendBarrier));
        command->numTransitions = numTransitions;
        command->transitions = (RenderBackendBarrier*)(((uint8*)command) + sizeof(RenderBackendCommandTransitions));
        memcpy(command->transitions, transitions, numTransitions * sizeof(RenderBackendBarrier));
    }

    void RenderBackendCommandList::Draw(
        RenderBackendShaderHandle shader,
        const RenderBackendGraphicsPipelineState& graphicsPipelineState,
        const RenderBackendShaderArguments& shaderArguments,
        uint32 numVertices,
        uint32 numInstances,
        uint32 firstVertex,
        uint32 firstInstance,
        RenderBackendPrimitiveTopology topology)
    {
        RenderBackendCommandDraw* command = AllocateCommand<RenderBackendCommandDraw>(RenderBackendCommandDraw::Type);
        command->shader = shader;
        command->pipelineState = graphicsPipelineState,
        command->numVertices = numVertices;
        command->numInstances = numInstances;
        command->firstVertex = firstVertex;
        command->firstInstance = firstInstance;
        command->topology = topology;
        command->indexBuffer = RenderBackendBufferHandle::Null;
        memcpy(&command->shaderArguments, &shaderArguments, sizeof(RenderBackendShaderArguments));
    }

    void RenderBackendCommandList::DrawIndexed(
        RenderBackendShaderHandle shader,
        const RenderBackendGraphicsPipelineState& graphicsPipelineState,
        const RenderBackendShaderArguments& shaderArguments,
        RenderBackendBufferHandle indexBuffer,
        uint32 numIndices,
        uint32 numInstances,
        uint32 firstIndex,
        int32 vertexOffset,
        uint32 firstInstance,
        RenderBackendPrimitiveTopology topology)
    {
        RenderBackendCommandDraw* command = AllocateCommand<RenderBackendCommandDraw>(RenderBackendCommandDraw::Type);
        command->shader = shader;
        command->pipelineState = graphicsPipelineState,
        command->indexBuffer = indexBuffer;
        command->numIndices = numIndices;
        command->numInstances = numInstances;
        command->firstIndex = firstIndex;
        command->vertexOffset = vertexOffset;
        command->firstInstance = firstInstance;
        command->topology = topology;
        memcpy(&command->shaderArguments, &shaderArguments, sizeof(RenderBackendShaderArguments));
    }

    void RenderBackendCommandList::DrawIndirect(
        RenderBackendShaderHandle shader,
        const RenderBackendGraphicsPipelineState& graphicsPipelineState,
        const RenderBackendShaderArguments& shaderArguments,
        RenderBackendBufferHandle indexBuffer,
        RenderBackendBufferHandle argumentBuffer,
        uint64 argumentBufferOffset,
        uint32 numDraws,
        RenderBackendPrimitiveTopology topology)
    {
        RenderBackendCommandDrawIndirect* command = AllocateCommand<RenderBackendCommandDrawIndirect>(RenderBackendCommandDrawIndirect::Type);
        command->shader = shader;
        command->pipelineState = graphicsPipelineState,
        command->indexBuffer = indexBuffer;
        command->argumentBuffer = argumentBuffer;
        command->argumentBufferOffset = argumentBufferOffset;
        command->numDraws = numDraws;
        command->topology = topology;
        memcpy(&command->shaderArguments, &shaderArguments, sizeof(RenderBackendShaderArguments));
    }

    void RenderBackendCommandList::DrawIndexedIndirect(
        RenderBackendShaderHandle shader,
        const RenderBackendGraphicsPipelineState& graphicsPipelineState,
        const RenderBackendShaderArguments& shaderArguments,
        RenderBackendBufferHandle indexBuffer,
        RenderBackendBufferHandle argumentBuffer,
        uint64 argumentBufferOffset,
        uint32 numDraws,
        RenderBackendPrimitiveTopology topology)
    {
        RenderBackendCommandDrawIndirect* command = AllocateCommand<RenderBackendCommandDrawIndirect>(RenderBackendCommandDrawIndirect::Type);
        command->shader = shader;
        command->pipelineState = graphicsPipelineState,
        command->indexBuffer = indexBuffer;
        command->argumentBuffer = argumentBuffer;
        command->argumentBufferOffset = argumentBufferOffset;
        command->numDraws = numDraws;
        command->topology = topology;
        memcpy(&command->shaderArguments, &shaderArguments, sizeof(RenderBackendShaderArguments));
    }

    void RenderBackendCommandList::DisptachMesh(RenderBackendShaderHandle shader, const RenderBackendGraphicsPipelineState& graphicsPipelineState, const RenderBackendShaderArguments& shaderArguments, uint32 x, uint32 y, uint32 z, RenderBackendPrimitiveTopology topology)
    {
        RenderBackendCommandDispatchMesh* command = AllocateCommand<RenderBackendCommandDispatchMesh>(RenderBackendCommandDispatchMesh::Type);
        command->shader = shader;
        command->pipelineState = graphicsPipelineState;
        command->topology = topology;
        command->threadGroupCountX = x;
        command->threadGroupCountY = y;
        command->threadGroupCountZ = z;
        memcpy(&command->shaderArguments, &shaderArguments, sizeof(RenderBackendShaderArguments));
    }

    void RenderBackendCommandList::DispatchMeshIndirect(RenderBackendShaderHandle shader, const RenderBackendGraphicsPipelineState& graphicsPipelineState, const RenderBackendShaderArguments& shaderArguments, RenderBackendBufferHandle argumentBuffer, uint64 argumentBufferOffset, uint32 numDraws, RenderBackendPrimitiveTopology topology)
    {
        RenderBackendCommandDispatchMeshIndirect* command = AllocateCommand<RenderBackendCommandDispatchMeshIndirect>(RenderBackendCommandDispatchMeshIndirect::Type);
        command->shader = shader;
        command->pipelineState = graphicsPipelineState;
        command->topology = topology;
        command->argumentBuffer = argumentBuffer;
        command->argumentBufferOffset = argumentBufferOffset;
        command->numDraws = numDraws;
        memcpy(&command->shaderArguments, &shaderArguments, sizeof(RenderBackendShaderArguments));
    }

    void RenderBackendCommandList::BeginTimingQuery(RenderBackendTimingQueryHeapHandle timingQueryHeap, uint32 region)
    {
        auto* command = AllocateCommand<RenderBackendCommandBeginTimingQuery>(RenderBackendCommandBeginTimingQuery::Type);
        command->timingQueryHeap = timingQueryHeap;
        command->region = region;
    }

    void RenderBackendCommandList::EndTimingQuery(RenderBackendTimingQueryHeapHandle timingQueryHeap, uint32 region)
    {
        auto* command = AllocateCommand<RenderBackendCommandEndTimingQuery>(RenderBackendCommandEndTimingQuery::Type);
        command->timingQueryHeap = timingQueryHeap;
        command->region = region;
    }

    void RenderBackendCommandList::ResolveTimingQueryResults(RenderBackendTimingQueryHeapHandle timingQueryHeap, uint32 regionStart, uint32 regionCount, RenderBackendBufferHandle buffer, uint64 offset)
    {
        auto* command = AllocateCommand<RenderBackendCommandResolveTimingQueryResults>(RenderBackendCommandResolveTimingQueryResults::Type);
        command->timingQueryHeap = timingQueryHeap;
        command->regionStart = regionStart;
        command->regionCount = regionCount;
        command->buffer = buffer;
        command->offset = offset;
    }

    void RenderBackendCommandList::BeginDebugLabel(const char* name, const Vector4& color)
    {
        auto* command = AllocateCommand<RenderBackendCommandBeginDebugLabel>(RenderBackendCommandBeginDebugLabel::Type);
        memcpy(&command->labelName, name, std::min(std::strlen(name) + 1, 255ull));
        command->color[0] = color[0];
        command->color[1] = color[1];
        command->color[2] = color[2];
        command->color[3] = color[3];
    }

    void RenderBackendCommandList::EndDebugLabel()
    {
        auto* command = AllocateCommand<RenderBackendCommandEndDebugLabel>(RenderBackendCommandEndDebugLabel::Type);
    }

    void RenderBackendCommandList::EvaluateDLSS(
        RenderBackendTextureHandle output,
        RenderBackendTextureHandle color,
        RenderBackendTextureHandle depth,
        RenderBackendTextureHandle motionVectors,
        uint32 targetWidth,
        uint32 targetHeight,
        uint32 frameIndex)
    {
        auto* command = AllocateCommand<RenderBackendCommandEvaluateDLSS>(RenderBackendCommandEvaluateDLSS::Type);
        command->output = output;
        command->color = color;
        command->depth = depth;
        command->motionVectors = motionVectors;
        command->targetWidth = targetWidth;
        command->targetHeight = targetHeight;
        command->frameIndex = frameIndex;
    }

    void RenderBackendCommandList::FSR2Dispatch(
        RenderBackendTextureHandle output,
        RenderBackendTextureHandle color,
        RenderBackendTextureHandle depth,
        RenderBackendTextureHandle motionVectors,
        uint32 renderWidth,
        uint32 renderHeight,
        uint32 targetWidth,
        uint32 targetHeight,
        float jitterOffsetX,
        float jitterOffsetY,
        float motionVectorScaleX,
        float motionVectorScaleY,
        bool reset,
        float deltaTime,
        bool enableSharpening,
        float sharpeness,
        float cameraFarPlane,
        float cameraNearPlane,
        float cameraFovAngleVertical)
    {
        auto* command = AllocateCommand<RenderBackendCommandFSR2Dispatch>(RenderBackendCommandFSR2Dispatch::Type);
        command->output = output;
        command->color = color;
        command->depth = depth;
        command->motionVectors = motionVectors;
        command->renderWidth = renderWidth;
        command->renderHeight = renderHeight;
        command->targetWidth = targetWidth;
        command->targetHeight = targetHeight;
        command->jitterOffsetX = jitterOffsetX;
        command->jitterOffsetY = jitterOffsetY;
        command->motionVectorScaleX = motionVectorScaleX;
        command->motionVectorScaleY = motionVectorScaleY;
        command->reset = reset;
        command->deltaTime = deltaTime;
        command->enableSharpening = enableSharpening;
        command->sharpeness = sharpeness;
        command->cameraFarPlane = cameraFarPlane;
        command->cameraNearPlane = cameraNearPlane;
        command->cameraFovAngleVertical = cameraFovAngleVertical;
    }
}