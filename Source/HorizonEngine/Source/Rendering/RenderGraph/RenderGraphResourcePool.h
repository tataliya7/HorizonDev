#pragma once

#include "RenderGraphCommon.h"
#include "RenderGraphHandles.h"
#include "RenderGraphNode.h"

namespace Horizon
{
    class RenderGraphPersistentTexture
    {
    public:

        RenderGraphPersistentTexture()
        {

        }

        RenderGraphPersistentTexture(const char* name, RenderBackend* backend, const RenderBackendTextureDesc& desc, RenderBackendTextureHandle handle)
            : active(false)
            , name(name)
            , backend(backend)
            , desc(desc)
            , handle(handle)
        {
            renderTargetViews.resize(desc.mipLevelCount);
        }

        bool IsValid() const
        {
            return handle.IsValid();
        }
        const char* GetName() const
        {
            return name.c_str();
        }
        RenderBackendTextureHandle GetHandle() const
        {
            return handle;
        }
        const RenderBackendTextureDesc& GetDesc() const
        {
            return desc;
        }

        RenderBackendTextureViewHandle FindOrCreateRenderTargetView(uint32 mipLevel)
        {
            if (!renderTargetViews[mipLevel])
            {
                RenderBackendTextureViewDesc textureViewDesc = RenderBackendTextureViewDesc::CreateRenderTargetView(mipLevel);
                renderTargetViews[mipLevel] = backend->CreateTextureView(handle, &textureViewDesc, nullptr);
            }
            return renderTargetViews[mipLevel];
        }

    private:
        friend class RenderGraphResourcePool;
        bool active;
        std::string name;
        RenderBackend* backend;
        RenderBackendTextureDesc desc;
        RenderBackendTextureHandle handle;

        std::vector<RenderBackendTextureViewHandle> renderTargetViews;
    };

    class RenderGraphPersistentBuffer
    {
    public:
        RenderGraphPersistentBuffer(const char* name, const RenderBackendBufferDesc& desc, RenderBackendBufferHandle handle)
            : active(false)
            , name(name)
            , desc(desc)
            , handle(handle)
        {

        }
        bool IsValid() const
        {
            return handle.IsValid();
        }
        const char* GetName() const
        {
            return name.c_str();
        }
        RenderBackendBufferHandle GetHandle() const
        {
            return handle;
        }
        const RenderBackendBufferDesc& GetDesc() const
        {
            return desc;
        }
    private:
        friend class RenderGraphResourcePool;
        bool active;
        std::string name;
        RenderBackendBufferDesc desc;
        RenderBackendBufferHandle handle;
    };

    class RenderGraphResourcePool
    {
    public:
        RenderGraphResourcePool(RenderBackend* backend);
        ~RenderGraphResourcePool();
        void Tick();
        RenderGraphPersistentTexture* CacheTexture(RenderBackendTextureHandle handle, const RenderBackendTextureDesc& desc, const char* name);
        RenderGraphPersistentBuffer* CacheBuffer(RenderBackendBufferHandle handle, const RenderBackendBufferDesc& desc, const char* name);
        RenderGraphPersistentTexture* AllocateTexture(const RenderBackendTextureDesc& desc, const char* name);
        RenderGraphPersistentBuffer* AllocateBuffer(const RenderBackendBufferDesc& desc, const char* name);
        void ReleaseTexture(RenderGraphPersistentTexture* texture);
        void ReleaseBuffer(RenderGraphPersistentBuffer* buffer);
    private:
        friend class RenderGraph;
        RenderBackend* backend;
        uint32 tickCount;
        std::vector<RenderGraphPersistentTexture*> allocatedTextures;
        std::vector<RenderGraphPersistentBuffer*> allocatedBuffers;
    };
}
