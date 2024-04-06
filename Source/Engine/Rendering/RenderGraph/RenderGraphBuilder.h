#pragma once

#include "Rendering/RenderGraph/RenderGraphCommon.h"
#include "Rendering/RenderGraph/RenderGraphHandles.h"
#include "Rendering/RenderGraph/RenderGraphResources.h"

namespace HE
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
        RenderGraphTextureHandle ReadTexture(RenderGraphTextureHandle handle, RenderBackendResourceState initalState, const RenderGraphTextureSubresourceRange& range = RenderGraphTextureSubresourceRange::WholeRange);
        RenderGraphTextureHandle WriteTexture(RenderGraphTextureHandle handle, RenderBackendResourceState initalState, const RenderGraphTextureSubresourceRange& range = RenderGraphTextureSubresourceRange::WholeRange);
        RenderGraphTextureHandle ReadWriteTexture(RenderGraphTextureHandle handle, RenderBackendResourceState initalState, const RenderGraphTextureSubresourceRange& range = RenderGraphTextureSubresourceRange::WholeRange);
        RenderGraphBufferHandle ReadBuffer(RenderGraphBufferHandle handle, RenderBackendResourceState initalState);
        RenderGraphBufferHandle WriteBuffer(RenderGraphBufferHandle handle, RenderBackendResourceState initalState);
        RenderGraphBufferHandle ReadWriteBuffer(RenderGraphBufferHandle handle, RenderBackendResourceState initalState);
        void BindColorTarget(
            uint32 slot,
            RenderGraphTextureHandle handle,
            RenderBackendRenderTargetLoadOp loadOp,
            RenderBackendRenderTargetStoreOp storeOp,
            uint32 mipLevel = 0,
            uint32 arraylayer = 0);
        void BindDepthTarget(
            RenderGraphTextureHandle handle,
            RenderBackendRenderTargetLoadOp depthLoadOp,
            RenderBackendRenderTargetStoreOp depthStoreOp,
            uint32 mipLevel = 0,
            uint32 arraylayer = 0);
        void BindDepthStencilTarget(
            RenderGraphTextureHandle handle,
            RenderBackendRenderTargetLoadOp depthLoadOp,
            RenderBackendRenderTargetStoreOp depthStoreOp,
            RenderBackendRenderTargetLoadOp stencilLoadOp,
            RenderBackendRenderTargetStoreOp stencilStoreOp,
            uint32 mipLevel = 0,
            uint32 arraylayer = 0);
    private:
        RenderGraph* const renderGraph;
        RenderGraphPass* const pass;
    };
}