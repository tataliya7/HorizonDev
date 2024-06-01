#include "RenderGraphResources.h"

namespace Horizon
{
    const RenderGraphTextureSubresourceRange RenderGraphTextureSubresourceRange::WholeRange = RenderGraphTextureSubresourceRange(0, RenderBackendTextureSubresourceRange::RemainingMipLevels, 0, RenderBackendTextureSubresourceRange::RemainingArrayLayers);

    RenderGraphResourcePool::RenderGraphResourcePool(RenderBackend* backend)
        : backend(backend)
        , tickCount(0)
    {

    }

    RenderGraphResourcePool::~RenderGraphResourcePool()
    {
        for (RenderGraphPersistentTexture* texture : allocatedTextures)
        {
            if (texture->IsValid())
            {
                backend->DestroyTexture(texture->GetHandle());
            }
        }

        for (RenderGraphPersistentBuffer* buffer : allocatedBuffers)
        {
            if (buffer->IsValid())
            {
                backend->DestroyBuffer(buffer->GetHandle());
            }
        }
    }

    void RenderGraphResourcePool::Tick()
    {
        for (RenderGraphPersistentTexture* texture : allocatedTextures)
        {
            texture->active = false;
        }
        for (RenderGraphPersistentBuffer* buffer : allocatedBuffers)
        {
            buffer->active = false;
        }

        // TODO: destroy unused resources here

        
        tickCount++;
    }

    RenderGraphPersistentTexture* RenderGraphResourcePool::CacheTexture(RenderBackendTextureHandle handle, const RenderBackendTextureDesc& desc, const char* name)
    {
        RenderGraphPersistentTexture* persistentTexture = new RenderGraphPersistentTexture(name, desc, handle);
        persistentTexture->active = true;
        allocatedTextures.push_back(persistentTexture);
        return persistentTexture;
    }

    RenderGraphPersistentBuffer* RenderGraphResourcePool::CacheBuffer(RenderBackendBufferHandle handle, const RenderBackendBufferDesc& desc, const char* name)
    {
        RenderGraphPersistentBuffer* persistentBuffer = new RenderGraphPersistentBuffer(name, desc, handle);
        persistentBuffer->active = true;
        allocatedBuffers.push_back(persistentBuffer);
        return persistentBuffer;
    }

    RenderGraphPersistentTexture* RenderGraphResourcePool::AllocateTexture(const RenderBackendTextureDesc& desc, const char* name)
    {
        for (RenderGraphPersistentTexture* texture : allocatedTextures)
        {
            if (texture->active)
            {
                continue;
            }

            if (texture->desc == desc)
            {
                texture->active = true;
                texture->name = name;
                return texture;
            }
        }

        RenderBackendTextureHandle textureHandle = backend->CreateTexture(&desc, nullptr, name);

        RenderGraphPersistentTexture* persistentTexture = new RenderGraphPersistentTexture(name, desc, textureHandle);

        allocatedTextures.push_back(persistentTexture);

        return persistentTexture;
    }

    RenderGraphPersistentBuffer* RenderGraphResourcePool::AllocateBuffer(const RenderBackendBufferDesc& desc, const char* name)
    {
        for (RenderGraphPersistentBuffer* buffer : allocatedBuffers)
        {
            if (buffer->active)
            {
                continue;
            }

            if (buffer->desc == desc)
            {
                buffer->active = true;
                buffer->name = name;
                return buffer;
            }
        }

        RenderBackendBufferHandle bufferHandle = backend->CreateBuffer(&desc, nullptr, name);

        RenderGraphPersistentBuffer* persistentBuffer = new RenderGraphPersistentBuffer(name, desc, bufferHandle);

        allocatedBuffers.push_back(persistentBuffer);

        return persistentBuffer;
    }

    void RenderGraphResourcePool::ReleaseTexture(RenderGraphPersistentTexture* texture)
    {
        for (uint32 index = 0; index < (uint32)allocatedTextures.size(); index++)
        {
            if (allocatedTextures[index] == texture)
            {
                allocatedTextures[index]->active = false;
                break;
            }
        }
    }

    void RenderGraphResourcePool::ReleaseBuffer(RenderGraphPersistentBuffer* buffer)
    {
        for (uint32 index = 0; index < (uint32)allocatedBuffers.size(); index++)
        {
            if (allocatedBuffers[index] == buffer)
            {
                allocatedBuffers[index]->active = false;
                break;
            }
        }
    }
}