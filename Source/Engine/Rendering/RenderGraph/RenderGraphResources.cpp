#include "Rendering/RenderGraph/RenderGraphResources.h"

namespace HE
{
    const RenderGraphTextureSubresourceRange RenderGraphTextureSubresourceRange::WholeRange = RenderGraphTextureSubresourceRange(0, RENDER_BACKEND_REMAINING_MIP_LEVELS, 0, RENDER_BACKEND_REMAINING_ARRAY_LAYERS);

    RenderGraphResourcePool* GRenderGraphResourcePool = new RenderGraphResourcePool();

    void RenderGraphResourcePool::Tick()
    {
        for (auto& persistentTexture : allocatedTextures)
        {
            persistentTexture.active = false;
        }
        for (auto& allocatedBuffer : allocatedBuffers)
        {
            allocatedBuffer.active = false;
        }
        frameCounter++;
    }

    void RenderGraphResourcePool::CacheTexture(const RenderGraphPersistentTexture& texture)
    {
        for (auto& persistentTexture : allocatedTextures)
        {
            if (persistentTexture.texture == texture.texture)
            {
                return;
            }
        }

        RenderGraphPersistentTexture persistentTexture = {
            .active = false,
            .name = texture.name,
            .texture = texture.texture,
            .desc = texture.desc,
            .initialState = texture.initialState,
        };
        allocatedTextures.push_back(persistentTexture);
    }

    void RenderGraphResourcePool::CacheBuffer(const RenderGraphPersistentBuffer& buffer)
    {
        for (auto& persistentBuffer : allocatedBuffers)
        {
            if (persistentBuffer.buffer == buffer.buffer)
            {
                return;
            }
        }

        RenderGraphPersistentBuffer persistentBuffer = {
            .active = false,
            .name = buffer.name,
            .buffer = buffer.buffer,
            .desc = buffer.desc,
            .initialState = buffer.initialState,
        };
        allocatedBuffers.push_back(persistentBuffer);
    }

    void RenderGraphResourcePool::ReleaseTexture(RenderBackendTextureHandle texture)
    {
        for (uint32 index = 0; index < allocatedTextures.size(); index++)
        {
            if (allocatedTextures[index].texture == texture)
            {
                allocatedTextures.erase(allocatedTextures.begin() + index);
                break;
            }
        }
    }

    void RenderGraphResourcePool::ReleaseBuffer(RenderBackendBufferHandle buffer)
    {
        for (uint32 index = 0; index < allocatedBuffers.size(); index++)
        {
            if (allocatedBuffers[index].buffer == buffer)
            {
                allocatedBuffers.erase(allocatedBuffers.begin() + index);
                break;
            }
        }
    }

    RenderBackendTextureHandle RenderGraphResourcePool::FindOrCreateTexture(RenderBackend* backend, const RenderBackendTextureDesc* desc, const char* name)
    {
        for (auto& persistentTexture : allocatedTextures)
        {
            if (persistentTexture.active)
            {
                continue;
            }
            if (persistentTexture.desc == *desc)
            {
                persistentTexture.active = true;
                return persistentTexture.texture;
            }
        }

        uint32 deviceMask = ~0u;
        RenderBackendTextureHandle texture = backend->CreateTexture(deviceMask, desc, nullptr, name);

        RenderGraphPersistentTexture persistentTexture = {
            .active = true,
            .texture = texture,
            .desc = *desc,
            .initialState = desc->initialState,
        };
        allocatedTextures.emplace_back(persistentTexture);
        return allocatedTextures.back().texture;
    }

    RenderBackendBufferHandle RenderGraphResourcePool::FindOrCreateBuffer(RenderBackend* backend, const RenderBackendBufferDesc* desc, const char* name)
    {
        for (auto& persistentBuffer : allocatedBuffers)
        {
            if (persistentBuffer.active)
            {
                continue;
            }
            if (persistentBuffer.desc == *desc)
            {
                persistentBuffer.active = true;
                return persistentBuffer.buffer;
            }
        }

        uint32 deviceMask = ~0u;
        RenderBackendBufferHandle buffer = backend->CreateBuffer(deviceMask, desc, nullptr, name);

        RenderGraphPersistentBuffer persistentBuffer = {
            .active = true,
            .buffer = buffer,
            .desc = *desc,
            .initialState = RenderBackendResourceState::Undefined,
        };
        allocatedBuffers.emplace_back(persistentBuffer);
        return allocatedBuffers.back().buffer;
    }
}