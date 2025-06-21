#pragma once

#include "RenderGraphCommon.h"
#include "RenderGraphHandles.h"
#include "RenderGraphResources.h"

namespace Horizon
{
    class RenderGraph;
    class RenderGraphPass;

    class RenderGraphBuilder
    {
    public:
        RenderGraphBuilder(RenderGraph* renderGraph, RenderGraphPass* pass)
            : renderGraph(renderGraph), pass(pass) {}
        ~RenderGraphBuilder() = default;
        RenderGraphTextureHandle CreateTransientTexture(const RenderGraphTextureDesc& desc, const char* name);
        RenderGraphBufferHandle CreateTransientBuffer(const RenderGraphBufferDesc& desc, const char* name);
        RenderGraphTextureHandle ReadTexture(RenderGraphTextureHandle handle, RenderBackendResourceState initialState);
        RenderGraphTextureHandle WriteTexture(RenderGraphTextureHandle handle, RenderBackendResourceState initialState);
        RenderGraphTextureHandle WriteTexture(RenderGraphTextureHandle handle, RenderBackendResourceState initialState, RenderBackendResourceState finalState);
        RenderGraphTextureHandle ReadWriteTexture(RenderGraphTextureHandle handle, RenderBackendResourceState initialState);
        RenderGraphBufferHandle ReadBuffer(RenderGraphBufferHandle handle, RenderBackendResourceState initialState);
        RenderGraphBufferHandle WriteBuffer(RenderGraphBufferHandle handle, RenderBackendResourceState initialState);
        RenderGraphBufferHandle ReadWriteBuffer(RenderGraphBufferHandle handle, RenderBackendResourceState initialState);

        void SetBindlessResourceSRV(uint32 slot, RenderBackendBufferHandle buffer);

        void SetBindlessResourceUAV(uint32 slot, RenderBackendBufferHandle buffer);

        void SetBindlessResourceSRV(uint32 slot, RenderGraphBufferHandle buffer);

        void SetBindlessResourceUAV(uint32 slot, RenderGraphBufferHandle buffer);

        void SetBindlessResourceSRV(uint32 slot, RenderGraphTextureHandle texture);

        void SetBindlessResourceUAV(uint32 slot, RenderGraphTextureHandle texture, uint32 mipLevel);

        void SetShaderConstantValue(uint32 slot, int32 value);

        void SetShaderConstantValue(uint32 slot, uint32 value);

        void SetShaderConstantValue(uint32 slot, float value);

        void SetRenderTargetBinding(
            uint32 slot,
            RenderGraphTextureHandle handle,
            RenderBackendRenderPassLoadOperation loadOperation,
            RenderBackendRenderPassStoreOperation storeOperation,
            uint32 mipLevel = 0);

        void SetDepthStencilBinding(
            RenderGraphTextureHandle handle,
            RenderBackendRenderPassLoadOperation depthLoadOperation,
            RenderBackendRenderPassStoreOperation depthStoreOperation,
            RenderBackendRenderPassLoadOperation stencilLoadOperation,
            RenderBackendRenderPassStoreOperation stencilStoreOperation,
            RenderBackendDepthStencilAccessType depthStencilAccessType);

        void SetRenderArea(int32 x, int32 y, uint32 width, uint32 height);

        void SetAllowUAVWrites(bool value);

    private:
        RenderGraph* const renderGraph;
        RenderGraphPass* const pass;
    };
}