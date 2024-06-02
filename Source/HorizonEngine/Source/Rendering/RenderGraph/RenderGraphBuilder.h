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
        RenderGraphTextureHandle ReadTexture(RenderGraphTextureHandle handle, RenderBackendResourceState initalState);
        RenderGraphTextureHandle WriteTexture(RenderGraphTextureHandle handle, RenderBackendResourceState initalState);
        RenderGraphTextureHandle ReadWriteTexture(RenderGraphTextureHandle handle, RenderBackendResourceState initalState);
        RenderGraphBufferHandle ReadBuffer(RenderGraphBufferHandle handle, RenderBackendResourceState initalState);
        RenderGraphBufferHandle WriteBuffer(RenderGraphBufferHandle handle, RenderBackendResourceState initalState);
        RenderGraphBufferHandle ReadWriteBuffer(RenderGraphBufferHandle handle, RenderBackendResourceState initalState);
        void BindRenderTarget(
            uint32 slot,
            RenderGraphTextureHandle handle,
            RenderBackendRenderPassBeginningAccessType loadOp,
            RenderBackendRenderPassEndingAccessType storeOp,
            uint32 mipLevel = 0,
            uint32 arraylayer = 0);
        void BindDepthStencil(
            RenderGraphTextureHandle handle,
            RenderBackendRenderPassBeginningAccessType depthLoadOp,
            RenderBackendRenderPassEndingAccessType depthStoreOp,
            uint32 mipLevel = 0,
            uint32 arraylayer = 0);
        void BindDepthStencilTarget(
            RenderGraphTextureHandle handle,
            RenderBackendRenderPassBeginningAccessType depthLoadOp,
            RenderBackendRenderPassEndingAccessType depthStoreOp,
            RenderBackendRenderPassBeginningAccessType stencilLoadOp,
            RenderBackendRenderPassEndingAccessType stencilStoreOp,
            uint32 mipLevel = 0,
            uint32 arraylayer = 0);
    private:
        RenderGraph* const renderGraph;
        RenderGraphPass* const pass;
    };
}