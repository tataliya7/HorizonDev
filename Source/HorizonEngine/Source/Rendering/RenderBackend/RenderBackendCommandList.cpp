#include "RenderBackendCommandList.h"

namespace Horizon
{
    void RenderBackendCommandList::CopyBuffer(
        RenderBackendBufferHandle srcBuffer,
        uint64 srcOffset,
        RenderBackendBufferHandle dstBuffer,
        uint64 dstOffset,
        uint64 bytes)
    {
        RenderBackendCommandCopyBuffer* command = AllocateCommand<RenderBackendCommandCopyBuffer>(RenderBackendCommandCopyBuffer::Type);
        command->srcBuffer = srcBuffer;
        command->srcOffset = srcOffset;
        command->dstBuffer = dstBuffer;
        command->dstOffset = dstOffset;
        command->bytes = bytes;
    }

    void RenderBackendCommandList::CopyTexture2D(
        RenderBackendTextureHandle srcTexture,
        const Offset2D& srcOffset,
        uint32 srcMipLevel,
        RenderBackendTextureHandle dstTexture,
        const Offset2D& dstOffset,
        uint32 dstMipLevel,
        const Extent2D extent)
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

    // void RenderBackendCommandList::UpdateBuffer(
    //     RenderBackendBufferHandle buffer,
    //     uint64 offset,
    //     const void* data,
    //     uint64 size)
    // {
    //     RenderBackendCommandUpdateBuffer* command = AllocateCommand<RenderBackendCommandUpdateBuffer>(RenderBackendCommandUpdateBuffer::Type);
    //     command->buffer = buffer;
    //     command->offset = offset;
    //     command->data = data;
    //     command->size = size;
    // }

    void RenderBackendCommandList::ClearBufferUAV(
        RenderBackendBufferHandle buffer,
        uint32 data)
    {
        RenderBackendCommandClearBufferUAV* command = AllocateCommand<RenderBackendCommandClearBufferUAV>(RenderBackendCommandClearBufferUAV::Type);
        command->buffer = buffer;
        command->data = data;
    }

    void RenderBackendCommandList::ClearTextureUAV(
        const RenderBackendTextureUAVDesc& uav,
        const RenderBackendTextureClearValue& clearColor)
    {
        RenderBackendCommandClearTextureUAV* command = AllocateCommand<RenderBackendCommandClearTextureUAV>(RenderBackendCommandClearTextureUAV::Type);
        command->uav = uav;
        command->clearValue = clearColor;
    }

    void RenderBackendCommandList::Transitions(
        const RenderBackendBarrier* transitions,
        uint32 transitionCount)
    {
        RenderBackendCommandTransitions* command = AllocateCommand<RenderBackendCommandTransitions>(RenderBackendCommandTransitions::Type, sizeof(RenderBackendCommandTransitions) + transitionCount * sizeof(RenderBackendBarrier));
        command->transitionCount = transitionCount;
        command->transitions = (RenderBackendBarrier*)(((uint8*)command) + sizeof(RenderBackendCommandTransitions));
        memcpy(command->transitions, transitions, transitionCount * sizeof(RenderBackendBarrier));
    }

    void RenderBackendCommandList::BeginTimingQuery(
        RenderBackendTimingQueryHeapHandle timingQueryHeap,
        uint32 region)
    {
        auto* command = AllocateCommand<RenderBackendCommandBeginTimingQuery>(RenderBackendCommandBeginTimingQuery::Type);
        command->timingQueryHeap = timingQueryHeap;
        command->region = region;
    }

    void RenderBackendCommandList::EndTimingQuery(
        RenderBackendTimingQueryHeapHandle timingQueryHeap,
        uint32 region)
    {
        auto* command = AllocateCommand<RenderBackendCommandEndTimingQuery>(RenderBackendCommandEndTimingQuery::Type);
        command->timingQueryHeap = timingQueryHeap;
        command->region = region;
    }

    void RenderBackendCommandList::ResolveTimingQueryResults(
        RenderBackendTimingQueryHeapHandle timingQueryHeap,
        uint32 regionStart,
        uint32 regionCount,
        RenderBackendBufferHandle buffer,
        uint64 offset)
    {
        auto* command = AllocateCommand<RenderBackendCommandResolveTimingQueryResults>(RenderBackendCommandResolveTimingQueryResults::Type);
        command->timingQueryHeap = timingQueryHeap;
        command->regionStart = regionStart;
        command->regionCount = regionCount;
        command->buffer = buffer;
        command->offset = offset;
    }

    void RenderBackendCommandList::BeginDebugLabel(
        const char* name,
        const Vector4& color)
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

    void RenderBackendCommandList::Dispatch(
        RenderBackendShaderHandle computeShader,
        const RenderBackendShaderConstants& shaderConstants,
        uint32 threadGroupCountX,
        uint32 threadGroupCountY,
        uint32 threadGroupCountZ)
    {
        RenderBackendCommandDispatch* command = AllocateCommand<RenderBackendCommandDispatch>(RenderBackendCommandDispatch::Type);
        command->computeShader = computeShader;
        command->threadGroupCountX = threadGroupCountX;
        command->threadGroupCountY = threadGroupCountY;
        command->threadGroupCountZ = threadGroupCountZ;
        memcpy(&command->shaderConstants, &shaderConstants, sizeof(RenderBackendShaderConstants));
    }

    void RenderBackendCommandList::DispatchIndirect(
        RenderBackendShaderHandle computeShader,
        const RenderBackendShaderConstants& shaderConstants,
        RenderBackendBufferHandle argumentBuffer,
        uint64 argumentBufferOffset)
    {
        RenderBackendCommandDispatchIndirect* command = AllocateCommand<RenderBackendCommandDispatchIndirect>(RenderBackendCommandDispatchIndirect::Type);
        command->computeShader = computeShader;
        command->argumentBuffer = argumentBuffer;
        command->argumentBufferOffset = argumentBufferOffset;
        memcpy(&command->shaderConstants, &shaderConstants, sizeof(RenderBackendShaderConstants));
    }

    void RenderBackendCommandList::SetViewports(
        const RenderBackendViewport* viewports,
        uint32 viewportCount)
    {
        RenderBackendCommandSetViewport* command = AllocateCommand<RenderBackendCommandSetViewport>(RenderBackendCommandSetViewport::Type);
        command->viewportCount = viewportCount;
        memcpy(command->viewports, viewports, viewportCount * sizeof(RenderBackendViewport));
    }

    void RenderBackendCommandList::SetScissors(
        const RenderBackendScissor* scissors,
        uint32 scissorCount)
    {
        RenderBackendCommandSetScissor* command = AllocateCommand<RenderBackendCommandSetScissor>(RenderBackendCommandSetScissor::Type);
        command->scissorCount = scissorCount;
        memcpy(command->scissors, scissors, scissorCount * sizeof(RenderBackendScissor));
    }

    void RenderBackendCommandList::SetStencilReference(
        uint32 stencilReference)
    {
        RenderBackendCommandSetStencilReference* command = AllocateCommand<RenderBackendCommandSetStencilReference>(RenderBackendCommandSetStencilReference::Type);
        command->stencilReference = stencilReference;
    }

    void RenderBackendCommandList::BeginRenderPass(
        const RenderBackendRenderPassInfo& renderPassInfo)
    {
        RenderBackendCommandBeginRenderPass* command = AllocateCommand<RenderBackendCommandBeginRenderPass>(RenderBackendCommandBeginRenderPass::Type);
        memcpy(command, &renderPassInfo, sizeof(RenderBackendRenderPassInfo));
    }

    void RenderBackendCommandList::EndRenderPass()
    {
        RenderBackendCommandEndRenderPass* command = AllocateCommand<RenderBackendCommandEndRenderPass>(RenderBackendCommandEndRenderPass::Type);
    }

    void RenderBackendCommandList::Draw(
        RenderBackendShaderHandle vertexShader,
        RenderBackendShaderHandle pixelShader,
        const RenderBackendGraphicsPipelineState& graphicsPipelineState,
        const RenderBackendShaderConstants& shaderConstants,
        uint32 vertexCount,
        uint32 instanceCount,
        uint32 firstVertex,
        uint32 firstInstance,
        RenderBackendPrimitiveTopology topology)
    {
        RenderBackendCommandDraw* command = AllocateCommand<RenderBackendCommandDraw>(RenderBackendCommandDraw::Type);
        command->vertexShader = vertexShader;
        command->pixelShader = pixelShader;
        command->pipelineState = graphicsPipelineState;
        command->draw.vertexCount = vertexCount;
        command->draw.instanceCount = instanceCount;
        command->draw.firstVertex = firstVertex;
        command->draw.firstInstance = firstInstance;
        command->topology = topology;
        command->indexBuffer = RenderBackendBufferHandle::Null;
        memcpy(&command->shaderConstants, &shaderConstants, sizeof(RenderBackendShaderConstants));
    }

    void RenderBackendCommandList::DrawIndexed(
        RenderBackendShaderHandle vertexShader,
        RenderBackendShaderHandle pixelShader,
        const RenderBackendGraphicsPipelineState& graphicsPipelineState,
        const RenderBackendShaderConstants& shaderConstants,
        RenderBackendBufferHandle indexBuffer,
        uint32 indexCount,
        uint32 instanceCount,
        uint32 firstIndex,
        int32 vertexOffset,
        uint32 firstInstance,
        RenderBackendPrimitiveTopology topology)
    {
        RenderBackendCommandDraw* command = AllocateCommand<RenderBackendCommandDraw>(RenderBackendCommandDraw::Type);
        command->vertexShader = vertexShader;
        command->pixelShader = pixelShader;
        command->pipelineState = graphicsPipelineState;
        command->indexBuffer = indexBuffer;
        command->drawIndexed.indexCount = indexCount;
        command->drawIndexed.instanceCount = instanceCount;
        command->drawIndexed.firstIndex = firstIndex;
        command->drawIndexed.vertexOffset = vertexOffset;
        command->drawIndexed.firstInstance = firstInstance;
        command->topology = topology;
        memcpy(&command->shaderConstants, &shaderConstants, sizeof(RenderBackendShaderConstants));
    }

    void RenderBackendCommandList::DrawIndirect(
        RenderBackendShaderHandle vertexShader,
        RenderBackendShaderHandle pixelShader,
        const RenderBackendGraphicsPipelineState& graphicsPipelineState,
        const RenderBackendShaderConstants& shaderConstants,
        RenderBackendBufferHandle indexBuffer,
        RenderBackendBufferHandle argumentBuffer,
        uint64 argumentBufferOffset,
        uint32 drawCount,
        RenderBackendPrimitiveTopology topology)
    {
        RenderBackendCommandDrawIndirect* command = AllocateCommand<RenderBackendCommandDrawIndirect>(RenderBackendCommandDrawIndirect::Type);
        command->vertexShader = vertexShader;
        command->pixelShader = pixelShader;
        command->pipelineState = graphicsPipelineState;
        command->indexBuffer = indexBuffer;
        command->argumentBuffer = argumentBuffer;
        command->argumentBufferOffset = argumentBufferOffset;
        command->drawCount = drawCount;
        command->topology = topology;
        memcpy(&command->shaderConstants, &shaderConstants, sizeof(RenderBackendShaderConstants));
    }

    void RenderBackendCommandList::DrawIndexedIndirect(
        RenderBackendShaderHandle vertexShader,
        RenderBackendShaderHandle pixelShader,
        const RenderBackendGraphicsPipelineState& graphicsPipelineState,
        const RenderBackendShaderConstants& shaderConstants,
        RenderBackendBufferHandle indexBuffer,
        RenderBackendBufferHandle argumentBuffer,
        uint64 argumentBufferOffset,
        uint32 drawCount,
        RenderBackendPrimitiveTopology topology)
    {
        RenderBackendCommandDrawIndirect* command = AllocateCommand<RenderBackendCommandDrawIndirect>(RenderBackendCommandDrawIndirect::Type);
        command->vertexShader = vertexShader;
        command->pixelShader = pixelShader;
        command->pipelineState = graphicsPipelineState;
        command->indexBuffer = indexBuffer;
        command->argumentBuffer = argumentBuffer;
        command->argumentBufferOffset = argumentBufferOffset;
        command->drawCount = drawCount;
        command->topology = topology;
        memcpy(&command->shaderConstants, &shaderConstants, sizeof(RenderBackendShaderConstants));
    }

    void RenderBackendCommandList::DispatchMesh(
        RenderBackendShaderHandle amplificationShader,
        RenderBackendShaderHandle meshShader,
        RenderBackendShaderHandle pixelShader,
        const RenderBackendGraphicsPipelineState& graphicsPipelineState,
        const RenderBackendShaderConstants& shaderConstants,
        uint32 threadGroupCountX,
        uint32 threadGroupCountY,
        uint32 threadGroupCountZ,
        RenderBackendPrimitiveTopology topology)
    {
        RenderBackendCommandDispatchMesh* command = AllocateCommand<RenderBackendCommandDispatchMesh>(RenderBackendCommandDispatchMesh::Type);
        command->amplificationShader = amplificationShader;
        command->meshShader = meshShader;
        command->pixelShader = pixelShader;
        command->pipelineState = graphicsPipelineState;
        command->topology = topology;
        command->threadGroupCountX = threadGroupCountX;
        command->threadGroupCountY = threadGroupCountY;
        command->threadGroupCountZ = threadGroupCountZ;
        memcpy(&command->shaderConstants, &shaderConstants, sizeof(RenderBackendShaderConstants));
    }

    void RenderBackendCommandList::DispatchMeshIndirect(
        RenderBackendShaderHandle amplificationShader,
        RenderBackendShaderHandle meshShader,
        RenderBackendShaderHandle pixelShader,
        const RenderBackendGraphicsPipelineState& graphicsPipelineState,
        const RenderBackendShaderConstants& shaderConstants,
        RenderBackendBufferHandle argumentBuffer,
        uint64 argumentBufferOffset,
        uint32 drawCount,
        RenderBackendPrimitiveTopology topology)
    {
        RenderBackendCommandDispatchMeshIndirect* command = AllocateCommand<RenderBackendCommandDispatchMeshIndirect>(RenderBackendCommandDispatchMeshIndirect::Type);
        command->amplificationShader = amplificationShader;
        command->meshShader = meshShader;
        command->pixelShader = pixelShader;
        command->pipelineState = graphicsPipelineState;
        command->topology = topology;
        command->argumentBuffer = argumentBuffer;
        command->argumentBufferOffset = argumentBufferOffset;
        command->drawCount = drawCount;
        memcpy(&command->shaderConstants, &shaderConstants, sizeof(RenderBackendShaderConstants));
    }

    void RenderBackendCommandList::BuildRayTracingBottomLevelAccelerationStructure(
        RenderBackendRayTracingAccelerationStructureHandle blas)
    {
        RenderBackendCommandBuildBottomLevelAS* command = AllocateCommand<RenderBackendCommandBuildBottomLevelAS>(RenderBackendCommandBuildBottomLevelAS::Type);
        command->srcBLAS = RenderBackendRayTracingAccelerationStructureHandle::Null;
        command->dstBLAS = blas;
    }

    void RenderBackendCommandList::BuildRayTracingTopLevelAccelerationStructure(
        RenderBackendRayTracingAccelerationStructureHandle tlas)
    {
        RenderBackendCommandBuildTopLevelAS* command = AllocateCommand<RenderBackendCommandBuildTopLevelAS>(RenderBackendCommandBuildTopLevelAS::Type);
        command->srcTLAS = RenderBackendRayTracingAccelerationStructureHandle::Null;
        command->dstTLAS = tlas;
    }

    void RenderBackendCommandList::UpdateRayTracingTopLevelAccelerationStructure(
        RenderBackendRayTracingAccelerationStructureHandle srcTLAS,
        RenderBackendRayTracingAccelerationStructureHandle dstTLAS)
    {
        RenderBackendCommandBuildTopLevelAS* command = AllocateCommand<RenderBackendCommandBuildTopLevelAS>(RenderBackendCommandBuildTopLevelAS::Type);
        command->srcTLAS = srcTLAS;
        command->dstTLAS = dstTLAS;
    }

    void RenderBackendCommandList::UpdateRayTracingBottomLevelAccelerationStructure(
        RenderBackendRayTracingAccelerationStructureHandle srcBLAS,
        RenderBackendRayTracingAccelerationStructureHandle dstBLAS)
    {
        RenderBackendCommandBuildBottomLevelAS* command = AllocateCommand<RenderBackendCommandBuildBottomLevelAS>(RenderBackendCommandBuildBottomLevelAS::Type);
        command->srcBLAS = srcBLAS;
        command->dstBLAS = dstBLAS;
    }

    void RenderBackendCommandList::DispatchRays(
        RenderBackendRayTracingPipelineStateHandle pipelineState,
        RenderBackendBufferHandle shaderBindingTable,
        const RenderBackendShaderConstants& shaderConstants,
        uint32 width,
        uint32 height,
        uint32 depth)
    {
        RenderBackendCommandDispatchRays* command = AllocateCommand<RenderBackendCommandDispatchRays>(RenderBackendCommandDispatchRays::Type);
        command->pipelineState = pipelineState;
        command->shaderBindingTable = shaderBindingTable;
        command->width = width;
        command->height = height;
        command->depth = depth;
        memcpy(&command->shaderConstants, &shaderConstants, sizeof(RenderBackendShaderConstants));
    }

    void RenderBackendCommandList::DispatchSuperSampling(
        void* context,
        RenderBackendDispatchSuperSamplingCallback callback,
        RenderBackendTextureHandle output,
        RenderBackendTextureHandle color,
        RenderBackendTextureHandle depth,
        RenderBackendTextureHandle motionVectors,
        RenderBackendTextureHandle exposure)
    {
        RenderBackendCommandDispatchSuperSampling* command = AllocateCommand<RenderBackendCommandDispatchSuperSampling>(RenderBackendCommandDispatchSuperSampling::Type);
        command->context = context;
        command->callback = callback;
        command->output = output;
        command->color = color;
        command->depth = depth;
        command->motionVectors = motionVectors;
        command->exposure = exposure;
    }
}