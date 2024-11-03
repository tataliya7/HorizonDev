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
        int32 GetTextureSRVBindlessResourceDescriptorIndex(RenderGraphTextureHandle handle) const;
        int32 GetTextureSRVBindlessResourceDescriptorIndex(RenderGraphTextureHandle handle, uint32 mipLevel) const;
        int32 GetTextureUAVBindlessResourceDescriptorIndex(RenderGraphTextureHandle handle, uint32 mipLevel) const;
        int32 GetBufferCBVBindlessResourceDescriptorIndex(RenderGraphBufferHandle handle) const;
        int32 GetBufferSRVBindlessResourceDescriptorIndex(RenderGraphBufferHandle handle) const;
        int32 GetBufferUAVBindlessResourceDescriptorIndex(RenderGraphBufferHandle handle) const;
    private:
        RenderGraph* const renderGraph;
        RenderGraphPass* const pass;
    };
}