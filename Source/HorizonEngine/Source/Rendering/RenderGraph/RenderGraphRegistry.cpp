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
}