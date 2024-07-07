#include "RenderGraphRegistry.h"
#include "RenderGraph.h"

namespace Horizon
{
    RenderGraphTexture* RenderGraphRegistry::GetTexture(RenderGraphTextureHandle handle) const
    {
        return renderGraph->textures[handle.GetIndex()];
    }

    RenderGraphBuffer* RenderGraphRegistry::GetBuffer(RenderGraphBufferHandle handle) const
    {
        return renderGraph->buffers[handle.GetIndex()];
    }

    RenderGraphTexture* RenderGraphRegistry::GetImportedTexture(RenderBackendTextureHandle handle) const
    {
        if (renderGraph->importedTextures.find(handle.ToUnit64()) == renderGraph->importedTextures.end())
        {
            return nullptr;
        }
        return GetTexture(renderGraph->importedTextures[handle.ToUnit64()]);
    }

    RenderGraphBuffer* RenderGraphRegistry::GetImportedBuffer(RenderBackendBufferHandle handle) const
    {
        if (renderGraph->importedBuffers.find(handle.ToUnit64()) == renderGraph->importedBuffers.end())
        {
            return nullptr;
        }
        return GetBuffer(renderGraph->importedBuffers[handle.ToUnit64()]);
    }

    const RenderBackendTextureDesc& RenderGraphRegistry::GetTextureDesc(RenderGraphTextureHandle handle) const
    {
        return GetTexture(handle)->GetDesc();
    }

    const RenderBackendBufferDesc& RenderGraphRegistry::GetBufferDesc(RenderGraphBufferHandle handle) const
    {
        return GetBuffer(handle)->GetDesc();
    }

    RenderBackendTextureHandle RenderGraphRegistry::GetRenderBackendTextureHandle(RenderGraphTextureHandle handle) const
    {
        return GetTexture(handle)->GetRenderBackendTextureHandle();
    }

    RenderBackendBufferHandle RenderGraphRegistry::GetRenderBackendBufferHandle(RenderGraphBufferHandle handle) const
    {
        return GetBuffer(handle)->GetRenderBackendBufferHandle();
    }

    int32 RenderGraphRegistry::GetTextureSRVBindlessResourceDescriptorIndex(
        RenderGraphTextureHandle handle,
        const RenderBackendTextureSubresourceRange& subresourceRange) const
    {
        RenderBackendTextureHandle renderBackendTextureHandle = GetRenderBackendTextureHandle(handle);
        return renderGraph->renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(renderBackendTextureHandle, subresourceRange);
    }

    int32 RenderGraphRegistry::GetTextureUAVBindlessResourceDescriptorIndex(
        RenderGraphTextureHandle handle,
        uint32 mipLevel) const
    {
        RenderBackendTextureHandle renderBackendTextureHandle = GetRenderBackendTextureHandle(handle);
        return renderGraph->renderBackend->GetTextureUAVBindlessResourceDescriptorIndex(renderBackendTextureHandle, mipLevel);
    }

    int32 RenderGraphRegistry::GetBufferCBVBindlessResourceDescriptorIndex(RenderGraphBufferHandle handle) const
    {
        RenderBackendBufferHandle renderBackendBufferHandle = GetRenderBackendBufferHandle(handle);
        return renderGraph->renderBackend->GetBufferCBVBindlessResourceDescriptorIndex(renderBackendBufferHandle);
    }

    int32 RenderGraphRegistry::GetBufferSRVBindlessResourceDescriptorIndex(RenderGraphBufferHandle handle) const
    {
        RenderBackendBufferHandle renderBackendBufferHandle = GetRenderBackendBufferHandle(handle);
        return renderGraph->renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(renderBackendBufferHandle);
    }

    int32 RenderGraphRegistry::GetBufferUAVBindlessResourceDescriptorIndex(RenderGraphBufferHandle handle) const
    {
        RenderBackendBufferHandle renderBackendBufferHandle = GetRenderBackendBufferHandle(handle);
        return renderGraph->renderBackend->GetBufferUAVBindlessResourceDescriptorIndex(renderBackendBufferHandle);
    }
}