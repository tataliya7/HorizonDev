#pragma once

#include "RenderGraphCommon.h"

namespace Horizon
{
    class RenderGraphHandleBase
    {
    public:

        RenderGraphHandleBase()
            : index(InvalidIndex)
            , version(0)
        {

        }

        explicit RenderGraphHandleBase(uint32 index, uint32 version = 0)
            : index(index)
            , version(version)
        {

        }

        uint32 GetIndex() const
        {
            return index;
        }

        uint32 GetVersion() const
        {
            return version;
        }

        bool IsNull()  const
        {
            return index == InvalidIndex;
        }

        explicit operator bool() const
        {
            return !IsNull();
        }

        bool operator==(const RenderGraphHandleBase& rhs) const
        {
            return ((index == rhs.index) && (version == rhs.version));
        }

        bool operator!=(const RenderGraphHandleBase& rhs) const
        {
            return ((index != rhs.index) || (version != rhs.version));
        }

        RenderGraphHandleBase& operator++()
        {
            index++; return *this;
        }

        RenderGraphHandleBase& operator--()
        {
            index--; return *this;
        }

    private:

        static constexpr uint32 InvalidIndex = UINT32_MAX;

        uint32 index;

        uint32 version;
    };

    template<typename ObjectType>
    class RenderGraphHandle : public RenderGraphHandleBase
    {
    public:

        static const RenderGraphHandle Null;

        static RenderGraphHandle CreateNewVersion(RenderGraphHandle oldHandle)
        {
            assert(oldHandle.GetVersion() + 1 <= UINT32_MAX);
            return RenderGraphHandle(oldHandle.GetIndex(), oldHandle.GetVersion() + 1);
        }

        RenderGraphHandle() = default;

        explicit RenderGraphHandle(uint32 index, uint32 version = 0)
            : RenderGraphHandleBase(index, version)
        {

        }
    };

    template<typename ObjectType>
    const RenderGraphHandle<ObjectType> RenderGraphHandle<ObjectType>::Null = RenderGraphHandle<ObjectType>();

    class RenderGraphTexture;
    using RenderGraphTextureHandle = RenderGraphHandle<RenderGraphTexture>;

    class RenderGraphBuffer;
    using RenderGraphBufferHandle = RenderGraphHandle<RenderGraphBuffer>;

    class RenderGraphTextureSRV;
    using RenderGraphTextureSRVHandle = RenderGraphHandle<RenderGraphTextureSRV>;

    class RenderGraphTextureUAV;
    using RenderGraphTextureUAVHandle = RenderGraphHandle<RenderGraphTextureUAV>;

    class RenderGraphBufferUAV;
    using RenderGraphBufferUAVHandle = RenderGraphHandle<RenderGraphBufferUAV>;
}