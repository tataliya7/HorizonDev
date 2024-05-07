#pragma once

#include "RenderGraphCommon.h"
#include "RenderGraphHandles.h"

namespace Horizon
{
    class RenderGraph;
    class RenderGraphPass;

    class RenderGraphRegistry
    {
    public:
        RenderGraphRegistry(RenderGraph* renderGraph, RenderGraphPass* pass)
            : renderGraph(renderGraph), pass(pass) {}
        RenderGraphTexture* GetTexture(RenderGraphTextureHandle handle) const;
        RenderGraphBuffer* GetBuffer(RenderGraphBufferHandle handle) const;
        RenderGraphTexture* GetImportedTexture(RenderBackendTextureHandle handle) const;
        RenderGraphBuffer* GetImportedBuffer(RenderBackendBufferHandle handle) const;
        const RenderBackendTextureDesc& GetTextureDesc(RenderGraphTextureHandle handle) const;
        const RenderBackendBufferDesc& GetBufferDesc(RenderGraphBufferHandle handle) const;
        RenderBackendTextureHandle GetRenderBackendTextureHandle(RenderGraphTextureHandle handle) const;
        RenderBackendBufferHandle GetRenderBackendBufferHandle(RenderGraphBufferHandle handle) const;
    private:
        RenderGraph* const renderGraph;
        RenderGraphPass* const pass;
    };
}