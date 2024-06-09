#pragma once

#include "RenderBackendCommon.h"
#include "RenderBackendCommands.h"
#include "RenderBackendInterface.h"

namespace Horizon
{
    struct RenderBackendCommandContainer
    {
        uint32 numCommands = 0;
        std::vector<RenderBackendCommandType> types = {};
        std::vector<void*> commands = {};
    };

    class RenderBackendCommandListBase
    {
    public:
        RenderBackendCommandListBase(MemoryArena* arena) : arena(arena) {}
        virtual ~RenderBackendCommandListBase() {}
        template <typename T>
        T* AllocateCommand(RenderBackendCommandType type, uint64 size = sizeof(T))
        {
            void* data = AllocateCommandInternal(size);
            container.types.push_back(type);
            container.commands.push_back(data);
            container.numCommands++;
            return static_cast<T*>(data);
        }
        RenderBackendCommandContainer* GetCommandContainer()
        {
            return &container;
        }
    private:
        void* AllocateCommandInternal(uint64 size)
        {
            void* data = HE_ARENA_ALLOC(arena, size);
            return data;
        }
        MemoryArena* arena;
        RenderBackendCommandContainer container;
    };

    /**
     * Render command list encodes high level commands.
     * The commands are stateless, which allows for fully parallel recording.
     */
    class RenderBackendCommandList : public RenderBackendCommandListBase
    {
    public:
        RenderBackendCommandList(MemoryArena* arena) : RenderBackendCommandListBase(arena) {}

        // Copy commands
        void CopyBuffer(RenderBackendBufferHandle srcBuffer, uint64 srcOffset, RenderBackendBufferHandle dstBuffer, uint64 dstOffset, uint64 bytes);

        void CopyTexture2D(RenderBackendTextureHandle srcTexture, const Offset2D& srcOffset, uint32 srcMipLevel, RenderBackendTextureHandle dstTexture, const Offset2D& dstOffset, uint32 dstMipLevel, const Extent2D extent);

        void ClearTextureUAV(const RenderBackendTextureUAVDesc& uav, const RenderBackendTextureClearValue& clearColor);

        void Transitions(const RenderBackendBarrier* transitions, uint32 numTransitions);

        void BeginDebugLabel(const char* name, const Vector4& color);

        void EndDebugLabel();

        // Compute commands
        void Dispatch(RenderBackendShaderHandle computeShader, const RenderBackendShaderArguments& shaderArguments, uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ);

        void DispatchIndirect(RenderBackendShaderHandle computeShader, const RenderBackendShaderArguments& shaderArguments, RenderBackendBufferHandle argumentBuffer, uint64 argumentBufferOffset);

        // Graphics commands
        void SetViewports(const RenderBackendViewport* viewports, uint32 numViewports);
        void SetScissors(const RenderBackendScissor* scissors, uint32 numScissors);
        void SetStencilReference(uint32 stencilReference);
        void BeginRenderPass(const RenderBackendRenderPassInfo& renderPassInfo);
        void EndRenderPass();
        void Draw(RenderBackendShaderHandle vertexShader, RenderBackendShaderHandle pixelShader, const RenderBackendGraphicsPipelineState& graphicsPipelineState, const RenderBackendShaderArguments& shaderArguments, uint32 numVertices, uint32 numInstances, uint32 firstVertex, uint32 firstInstance, RenderBackendPrimitiveTopology topology);
        void DrawIndexed(RenderBackendShaderHandle vertexShader, RenderBackendShaderHandle pixelShader, const RenderBackendGraphicsPipelineState& graphicsPipelineState, const RenderBackendShaderArguments& shaderArguments, RenderBackendBufferHandle indexBuffer, uint32 numIndices, uint32 numInstances, uint32 firstIndex, int32 vertexOffset, uint32 firstInstance, RenderBackendPrimitiveTopology topology);
        void DrawIndirect(RenderBackendShaderHandle vertexShader, RenderBackendShaderHandle pixelShader, const RenderBackendGraphicsPipelineState& graphicsPipelineState, const RenderBackendShaderArguments& shaderArguments, RenderBackendBufferHandle indexBuffer, RenderBackendBufferHandle argumentBuffer, uint64 argumentBufferOffset, uint32 numDraws, RenderBackendPrimitiveTopology topology);
        void DrawIndexedIndirect(RenderBackendShaderHandle vertexShader, RenderBackendShaderHandle pixelShader, const RenderBackendGraphicsPipelineState& graphicsPipelineState, const RenderBackendShaderArguments& shaderArguments, RenderBackendBufferHandle indexBuffer, RenderBackendBufferHandle argumentBuffer, uint64 argumentBufferOffset, uint32 numDraws, RenderBackendPrimitiveTopology topology);

        // Mesh shading commands
        void DispatchMesh(RenderBackendShaderHandle amplificationShader, RenderBackendShaderHandle meshShader, RenderBackendShaderHandle pixelShader, const RenderBackendGraphicsPipelineState& graphicsPipelineState, const RenderBackendShaderArguments& shaderArguments, uint32 threadGroupCountX, uint32 threadGroupCountY, uint32 threadGroupCountZ, RenderBackendPrimitiveTopology topology);
        void DispatchMeshIndirect(RenderBackendShaderHandle amplificationShader, RenderBackendShaderHandle meshShader, RenderBackendShaderHandle pixelShader, const RenderBackendGraphicsPipelineState& graphicsPipelineState, const RenderBackendShaderArguments& shaderArguments, RenderBackendBufferHandle argumentBuffer, uint64 argumentBufferOffset, uint32 numDraws, RenderBackendPrimitiveTopology topology);

        void BeginTimingQuery(RenderBackendTimingQueryHeapHandle timingQueryHeap, uint32 region);
        void EndTimingQuery(RenderBackendTimingQueryHeapHandle timingQueryHeap, uint32 region);
        void ResolveTimingQueryResults(RenderBackendTimingQueryHeapHandle timingQueryHeap, uint32 regionStart, uint32 regionCount, RenderBackendBufferHandle buffer, uint64 offset);

        // Ray tracing commands
        void BuildRayTracingTopLevelAccelerationStructure(RenderBackendRayTracingAccelerationStructureHandle tlas);
        void BuildRayTracingBottomLevelAccelerationStructure(RenderBackendRayTracingAccelerationStructureHandle blas);
        void UpdateRayTracingTopLevelAccelerationStructure(RenderBackendRayTracingAccelerationStructureHandle srcTLAS, RenderBackendRayTracingAccelerationStructureHandle dstTLAS);
        void UpdateRayTracingBottomLevelAccelerationStructure(RenderBackendRayTracingAccelerationStructureHandle srcBLAS, RenderBackendRayTracingAccelerationStructureHandle dstBLAS);
        void DispatchRays(RenderBackendRayTracingPipelineStateHandle pipelineState, RenderBackendBufferHandle shaderBindingTable, const RenderBackendShaderArguments& shaderArguments, uint32 width, uint32 height, uint32 depth);

        void DispatchSuperSampling(
            void* context,
            RenderBackendDispatchSuperSamplingCallback callback,
            RenderBackendTextureHandle output,
            RenderBackendTextureHandle color,
            RenderBackendTextureHandle depth,
            RenderBackendTextureHandle motionVectors,
            RenderBackendTextureHandle exposure);
    };
}