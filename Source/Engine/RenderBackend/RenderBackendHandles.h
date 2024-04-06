#pragma once

#include "RenderBackendCommon.h"

#include <limits>

namespace HE
{
    // TODO : fix auto cast to uint64

    class RenderBackendHandle
    {
    public:
        RenderBackendHandle() = default;
        RenderBackendHandle(uint64 value) : index((uint32)(value >> 32)), deviceMask((uint32)value) {}
        RenderBackendHandle(uint32 index, uint32 deviceMask) : index(index), deviceMask(deviceMask) {}
        FORCEINLINE bool IsValid()  const { return index != InvalidIndex; }
        FORCEINLINE operator bool() const { return IsValid(); }
        FORCEINLINE bool operator==(const RenderBackendHandle& rhs) const { return ((index == rhs.index) && (deviceMask == rhs.deviceMask)); }
        FORCEINLINE bool operator!=(const RenderBackendHandle& rhs) const { return ((index != rhs.index) || (deviceMask != rhs.deviceMask)); }
        FORCEINLINE uint64 ToUnit64() const { return uint64(index) << 32 | uint64(deviceMask); }
        FORCEINLINE RenderBackendHandle& operator=(const RenderBackendHandle& rhs) = default;
        FORCEINLINE RenderBackendHandle& operator++() { index++; return *this; }
        FORCEINLINE RenderBackendHandle& operator--() { index--; return *this; }
        FORCEINLINE uint32 GetIndex() const { return index; }
        FORCEINLINE uint32 GetDeviceMask() const { return deviceMask; }
    private:
        static const uint32 InvalidIndex = std::numeric_limits<uint32>::max();
        uint32 index = InvalidIndex;
        uint32 deviceMask = 0;
    };

    template<typename ObjectType>
    class RenderBackendHandleTyped : public RenderBackendHandle
    {
    public:
        static const RenderBackendHandleTyped Null;
        RenderBackendHandleTyped() = default;
        RenderBackendHandleTyped(uint64 value) : RenderBackendHandle(value) {}
        RenderBackendHandleTyped(uint32 index, uint32 deviceMask) : RenderBackendHandle(index, deviceMask) {}
    };

    template<typename ObjectType>
    const RenderBackendHandleTyped<ObjectType> RenderBackendHandleTyped<ObjectType>::Null = RenderBackendHandleTyped<ObjectType>();

    class RenderBackendSwapChain;
    using RenderBackendSwapChainHandle = RenderBackendHandleTyped<RenderBackendSwapChain>;

    class RenderBackendTexture;
    using RenderBackendTextureHandle = RenderBackendHandleTyped<RenderBackendTexture>;

    class RenderBackendTextureSRV;
    using RenderBackendTextureSRVHandle = RenderBackendHandleTyped<RenderBackendTextureSRV>;

    class RenderBackendTextureUAV;
    using RenderBackendTextureUAVHandle = RenderBackendHandleTyped<RenderBackendTextureUAV>;

    class RenderBackendBuffer;
    using RenderBackendBufferHandle = RenderBackendHandleTyped<RenderBackendBuffer>;

    class RenderBackendSampler;
    using RenderBackendSamplerHandle = RenderBackendHandleTyped<RenderBackendSampler>;

    class RenderBackendShader;
    using RenderBackendShaderHandle = RenderBackendHandleTyped<RenderBackendShader>;

    class RenderBackendTimingQueryHeap;
    using RenderBackendTimingQueryHeapHandle = RenderBackendHandleTyped<RenderBackendTimingQueryHeap>;

    class RenderBackendOcclusionQueryHeap;
    using RenderBackendOcclusionQueryHeapHandle = RenderBackendHandleTyped<RenderBackendOcclusionQueryHeap>;

    class RenderBackendAccelerationStructure;
    using RenderBackendRayTracingAccelerationStructureHandle = RenderBackendHandleTyped<RenderBackendAccelerationStructure>;

    class RenderBackendRayTracingPipelineState;
    using RenderBackendRayTracingPipelineStateHandle = RenderBackendHandleTyped<RenderBackendRayTracingPipelineState>;
}