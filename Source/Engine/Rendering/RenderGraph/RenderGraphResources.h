#pragma once

#include "Rendering/RenderGraph/RenderGraphCommon.h"
#include "Rendering/RenderGraph/RenderGraphHandles.h"
#include "Rendering/RenderGraph/RenderGraphNode.h"

namespace HE
{
    class RenderGraphPass;

    enum class RenderGraphResourceType
    {
        Texture,
        Buffer,
    };

    struct RenderGraphTextureSubresource
    {
        RenderGraphTextureSubresource() : level(0), layer(0) {}
        RenderGraphTextureSubresource(uint32 level, uint32 layer) : level(level), layer(layer) {}
        uint32 level;
        uint32 layer;
    };

    struct RenderGraphTextureSubresourceLayout
    {
        RenderGraphTextureSubresourceLayout(uint32 numMipLevels, uint32 numArrayLayers)
            : numMipLevels(numMipLevels), numArrayLayers(numArrayLayers) {}
        uint32 GetSubresourceCount() const
        {
            return numMipLevels * numArrayLayers;
        }
        inline uint32 GetSubresourceIndex(const RenderGraphTextureSubresource& subresource) const
        {
            return subresource.layer + (subresource.level * numArrayLayers);
        }
        uint32 numMipLevels;
        uint32 numArrayLayers;
    };

    struct RenderGraphTextureSubresourceRange
    {
        static const RenderGraphTextureSubresourceRange WholeRange;
        static RenderGraphTextureSubresourceRange Create(uint32 baseMipLevel, uint32 numMipLevels, uint32 baseArrayLayer, uint32 numArrayLayers)
        {
            return RenderGraphTextureSubresourceRange(baseMipLevel, numMipLevels, baseArrayLayer, numArrayLayers);
        }
        static RenderGraphTextureSubresourceRange CreateForAllSubresources()
        {
            return RenderGraphTextureSubresourceRange(0, RENDER_BACKEND_REMAINING_MIP_LEVELS, 0, RENDER_BACKEND_REMAINING_ARRAY_LAYERS);
        }
        static RenderGraphTextureSubresourceRange CreateForMipLevel(uint32 mipLevel)
        {
            return RenderGraphTextureSubresourceRange(mipLevel, 1, 0, RENDER_BACKEND_REMAINING_ARRAY_LAYERS);
        }
        static RenderGraphTextureSubresourceRange CreateFromLayout(const RenderGraphTextureSubresourceLayout& layout)
        {
            return RenderGraphTextureSubresourceRange(0, layout.numMipLevels, 0, layout.numArrayLayers);
        }
        RenderGraphTextureSubresourceRange(uint32 baseMipLevel, uint32 numMipLevels, uint32 baseArrayLayer, uint32 numArrayLayers)
            : baseMipLevel(baseMipLevel), numMipLevels(numMipLevels), baseArrayLayer(baseArrayLayer), numArrayLayers(numArrayLayers) {}
        RenderGraphTextureSubresource GetMinSubresource() const
        {
            return RenderGraphTextureSubresource(baseMipLevel, baseArrayLayer);
        }
        RenderGraphTextureSubresource GetMaxSubresource() const
        {
            return RenderGraphTextureSubresource(baseMipLevel + numMipLevels, baseArrayLayer + numArrayLayers);
        }
        uint32 baseMipLevel;
        uint32 numMipLevels;
        uint32 baseArrayLayer;
        uint32 numArrayLayers;
    };

    class RenderGraphResource : public RenderGraphNode
    {
    public:
        RenderGraphResource(const char* name, RenderGraphResourceType type)
            : RenderGraphNode(name, RenderGraphNodeType::Resource)
            , type(type) {}
        RenderGraphResourceType GetType() const
        {
            return type;
        }
        bool IsImported() const
        {
            return imported;
        }
        bool IsExported() const
        {
            return exported;
        }
    protected:
        bool imported = false;
        bool exported = false;
        bool transient = false;
        bool usedByAsyncComputePass = false;
        RenderBackendResourceState initialState = RenderBackendResourceState::Undefined;
        RenderBackendResourceState finalState = RenderBackendResourceState::Undefined;
        RenderGraphPass* firstPass = nullptr;
        RenderGraphPass* lastPass = nullptr;
    private:
        friend class RenderGraph;
        friend class RenderGraphBuilder;
        const RenderGraphResourceType type;
    };

    using RenderGraphTextureDesc = RenderBackendTextureDesc;

    class RenderGraphTexture final : public RenderGraphResource
    {
    public:
        RenderGraphTexture(const char* name, const RenderGraphTextureDesc& desc)
            : RenderGraphResource(name, RenderGraphResourceType::Texture)
            , desc(desc)
            , subresourceLayout(desc.mipLevels, desc.arrayLayers) {}
        const RenderGraphTextureDesc& GetDesc() const
        {
            return desc;
        }
        RenderBackendTextureHandle GetRenderBackendTexture() const
        {
            return texture;
        }
        RenderGraphTextureSubresourceLayout GetSubresourceLayout() const
        {
            return subresourceLayout;
        }
        RenderGraphTextureSubresourceRange GetSubresourceRange() const
        {
            return RenderGraphTextureSubresourceRange::CreateFromLayout(subresourceLayout);
        }
        bool HasRenderBackendTexture() const
        {
            return texture != RenderBackendTextureHandle::Null;
        }
    private:
        friend class RenderGraph;
        void SetRenderBackendTexture(RenderBackendTextureHandle texture, RenderBackendResourceState initialState)
        {
            this->texture = texture;
            this->initialState = initialState;
            this->tempState = initialState;
            this->finalState = initialState;
        }
        const RenderGraphTextureDesc desc;
        RenderBackendResourceState tempState = RenderBackendResourceState::Undefined;
        RenderGraphTextureSubresourceLayout subresourceLayout;
        RenderBackendTextureHandle texture = RenderBackendTextureHandle::Null;
    };

    enum class RenderGraphResourceViewType
    {
        TextureSRV,
        TextureUAV,
        BufferSRV,
        BufferUAV,
    };

    class RenderGraphResourceView
    {
    public:
        RenderGraphResourceViewType GetType() const
        {
            return type;
        }
    protected:
        RenderGraphResourceView(const char* name, RenderGraphResourceViewType type)
            : name(name), type(type) {}
    private:
        const char* name;
        const RenderGraphResourceViewType type;
    };

    class RenderGraphShaderResourceView : public RenderGraphResourceView
    {
    public:
        //uint32 GetDescripotrIndex() const { return static_cast<FRHIShaderResourceView*>(FRDGResource::GetRHI()); }
    protected:
        RenderGraphShaderResourceView(const char* name, RenderGraphResourceViewType type) : RenderGraphResourceView(name, type) {}
    };

    class RenderGraphUnorderedAccessView : public RenderGraphResourceView
    {
    public:
        //uint32 GetDescripotrIndex() const { return static_cast<FRHIShaderResourceView*>(FRDGResource::GetRHI()); }
    protected:
        RenderGraphUnorderedAccessView(const char* name, RenderGraphResourceViewType type) : RenderGraphResourceView(name, type) {}
    };

    struct RenderGraphTextureSRVDesc
    {
        static RenderGraphTextureSRVDesc Create(RenderGraphTextureHandle texture, uint32 baseMipLevel, uint32 mipLevelCount, uint32 baseArrayLayer, uint32 arrayLayerCount)
        {
            return RenderGraphTextureSRVDesc(texture, baseMipLevel, mipLevelCount, baseArrayLayer, arrayLayerCount);
        }
        static RenderGraphTextureSRVDesc CreateForMipLevel(RenderGraphTextureHandle texture, uint32 mipLevel, uint32 baseArrayLayer = 0, uint32 arrayLayerCount = 1)
        {
            return RenderGraphTextureSRVDesc(texture, mipLevel, 1, baseArrayLayer, arrayLayerCount);
        }
        RenderGraphTextureSRVDesc(RenderGraphTextureHandle texture, uint32 baseMipLevel, uint32 numMipLevels, uint32 baseArrayLayer, uint32 numArrayLayers)
            : texture(texture)
            , baseMipLevel(baseMipLevel)
            , numMipLevels(numMipLevels)
            , baseArrayLayer(baseArrayLayer)
            , numArrayLayers(numArrayLayers) {}
        RenderGraphTextureHandle texture;
        uint32 baseMipLevel;
        uint32 numMipLevels;
        uint32 baseArrayLayer;
        uint32 numArrayLayers;
    };

    class RenderGraphTextureSRV final : public RenderGraphShaderResourceView
    {
    public:
        const RenderGraphTextureSRVDesc& GetDesc() const
        {
            return desc;
        }
        RenderGraphTextureSubresourceRange GetSubresourceRange() const
        {
            return RenderGraphTextureSubresourceRange::Create(desc.baseMipLevel, desc.numMipLevels, desc.baseArrayLayer, desc.numArrayLayers);
        }
    private:
        friend class RenderGraph;
        RenderGraphTextureSRV(const char* name, const RenderGraphTextureSRVDesc& desc)
            : RenderGraphShaderResourceView(name, RenderGraphResourceViewType::TextureSRV), desc(desc) {}
        const RenderGraphTextureSRVDesc desc;
    };

    struct RenderGraphTextureUAVDesc
    {
        static RenderGraphTextureUAVDesc Create(RenderGraphTextureHandle texture, uint32 mipLevel = 0)
        {
            return RenderGraphTextureUAVDesc(texture, mipLevel);
        }
        RenderGraphTextureUAVDesc() = default;
        RenderGraphTextureUAVDesc(RenderGraphTextureHandle texture, uint32 mipLevel)
            : texture(texture), mipLevel(mipLevel) {}
        RenderGraphTextureHandle texture = RenderGraphTextureHandle::Null;
        uint32 mipLevel = 0;
    };

    class RenderGraphTextureUAV final : public RenderGraphUnorderedAccessView
    {
    public:
        const RenderGraphTextureUAVDesc& GetDesc() const
        {
            return desc;
        }
        RenderGraphTextureSubresourceRange GetSubresourceRange() const
        {
            return RenderGraphTextureSubresourceRange::CreateForMipLevel(desc.mipLevel);
        }
    private:
        friend class RenderGraph;
        RenderGraphTextureUAV(const char* name, const RenderGraphTextureUAVDesc& desc)
            : RenderGraphUnorderedAccessView(name, RenderGraphResourceViewType::TextureUAV), desc(desc) {}
        const RenderGraphTextureUAVDesc desc;
    };

    using RenderGraphBufferDesc = RenderBackendBufferDesc;

    class RenderGraphBuffer final : public RenderGraphResource
    {
    public:
        RenderGraphBuffer(const char* name, const RenderGraphBufferDesc& desc)
            : RenderGraphResource(name, RenderGraphResourceType::Buffer)
            , desc(desc) {}
        const RenderGraphBufferDesc& GetDesc() const
        {
            return desc;
        }
        RenderBackendBufferHandle GetRenderBackendBuffer() const
        {
            return buffer;
        }

        bool HasRenderBackendBuffer() const
        {
            return buffer != RenderBackendBufferHandle::Null;
        }
    private:
        friend class RenderGraph;
        void SetRenderBackendBuffer(RenderBackendBufferHandle buffer, RenderBackendResourceState initialState)
        {
            this->imported = true;
            this->buffer = buffer;
            this->initialState = initialState;
            this->finalState = initialState;
        }
        const RenderGraphBufferDesc desc;
        RenderBackendBufferHandle buffer;
    };

    struct RenderGraphBufferUAVDesc
    {
        static RenderGraphBufferUAVDesc Create(RenderGraphBufferHandle buffer)
        {
            return RenderGraphBufferUAVDesc(buffer);
        }
        RenderGraphBufferUAVDesc() = default;
        RenderGraphBufferUAVDesc(RenderGraphBufferHandle buffer) : buffer(buffer) {}
        RenderGraphBufferHandle buffer = RenderGraphBufferHandle::Null;
    };

    class RenderGraphBufferUAV final : public RenderGraphUnorderedAccessView
    {
    public:
        const RenderGraphBufferUAVDesc& GetDesc() const
        {
            return desc;
        }
    private:
        friend class RenderGraph;
        RenderGraphBufferUAV(const char* name, const RenderGraphBufferUAVDesc& desc)
            : RenderGraphUnorderedAccessView(name, RenderGraphResourceViewType::BufferUAV), desc(desc) {}
        const RenderGraphBufferUAVDesc desc;
    };

    struct RenderGraphPersistentTexture
    {
        bool active;
        std::string name;
        RenderBackendTextureHandle texture;
        RenderBackendTextureDesc desc;
        RenderBackendResourceState initialState;

        bool IsValid() const
        {
            return active && texture.IsValid();
        }
    };

    struct RenderGraphPersistentBuffer
    {
        bool active;
        std::string name;
        RenderBackendBufferHandle buffer;
        RenderBackendBufferDesc desc;
        RenderBackendResourceState initialState;

        bool IsValid() const
        {
            return active && buffer.IsValid();
        }
    };

    class RenderGraphResourcePool
    {
    public:
        void Tick();
        void CacheTexture(const RenderGraphPersistentTexture& texture);
        void CacheBuffer(const RenderGraphPersistentBuffer& buffer);
        void ReleaseTexture(RenderBackendTextureHandle texture);
        void ReleaseBuffer(RenderBackendBufferHandle buffer);
        RenderBackendTextureHandle FindOrCreateTexture(RenderBackend* backend, const RenderBackendTextureDesc* desc, const char* name);
        RenderBackendBufferHandle FindOrCreateBuffer(RenderBackend* backend, const RenderBackendBufferDesc* desc, const char* name);
    private:
        friend class RenderGraph;
        std::vector<RenderGraphPersistentTexture> allocatedTextures;
        std::vector<RenderGraphPersistentBuffer> allocatedBuffers;
        uint32 frameCounter = 0;
    };

    extern RenderGraphResourcePool* GRenderGraphResourcePool;
}