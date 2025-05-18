#pragma once

#include "RenderBackendCommon.h"
#include "RenderBackendHandles.h"
#include "RenderBackendTypes.h"

namespace Horizon
{
    class RenderBackendCommandList;

    using PhysicalDeviceID = uint32;

    enum class RenderBackendType
    {
        Unknown,
        Direct3D12,
        Vulkan,
    };

    enum class RenderBackendFeature
    {
        HardwareRayTracing,
    };

    struct RenderBackendVulkanInfo
    {
        void* device;
        void* instance;
        void* physicalDevice;
        uint32 computeQueueIndex;
        uint32 computeQueueFamily;
        uint32 graphicsQueueIndex;
        uint32 graphicsQueueFamily;
        uint32 opticalFlowQueueIndex;
        uint32 opticalFlowQueueFamily;
    };

    struct RenderBackendDesc
    {
        RenderBackendType              type;
        const char*                    applicationName;
        uint32                         applicationVersion;
        const char*                    engineName;
        uint32                         engineVersion;
        bool                           enableDebugLayer;
        const RenderBackendFeature*    features;
        uint32                         featureCount;
    };

    /**
     * Render backend interface.
     */
    class RenderBackend
    {
    public:

        /**
         * TBD.
         */
        virtual RenderBackendType GetType() const { return RenderBackendType::Unknown; }

        /**
         * TBD.
         */
        virtual void Tick() = 0;

        /**
         * TBD.
         */
        virtual void CreateRenderDevices(PhysicalDeviceID* physicalDeviceIDs, uint32 numDevices, uint32* outDeviceMasks) = 0;

        /**
         * TBD.
         */
        virtual void DestroyRenderDevices() = 0;

        /**
         * TBD.
         */
        virtual void FlushRenderDevices() = 0;

        /**
         * TBD.
         */
        virtual RenderBackendDeviceContext GetNativeDevice() = 0;

        virtual void GetRenderBackendVulkanInfo(RenderBackendVulkanInfo* vulkanInfo) {}

        /**
         * TBD.
         */
        virtual RenderBackendSwapChainHandle CreateSwapChain(const RenderBackendSwapChainDesc* desc) = 0;

        /**
         * TBD.
         */
        virtual void DestroySwapChain(RenderBackendSwapChainHandle swapChain) = 0;

        /**
         * TBD.
         */
        virtual void ResizeSwapChain(RenderBackendSwapChainHandle swapChain, uint32* width, uint32* height) = 0;

        /**
         * TBD.
         */
        virtual bool PresentSwapChain(RenderBackendSwapChainHandle swapChain) = 0;

        /**
         * TBD.
         */
        virtual RenderBackendTextureHandle GetActiveSwapChainBuffer(RenderBackendSwapChainHandle swapChain) = 0;

        /**
         * TBD.
         */
        virtual RenderBackendBufferHandle CreateBuffer(const RenderBackendBufferDesc* desc, const void* data, const char* name) = 0;

        /**
         * TBD.
         */
        virtual void DestroyBuffer(RenderBackendBufferHandle buffer) = 0;

        /**
         * TBD.
         */
        virtual void ResizeBuffer(RenderBackendBufferHandle buffer, uint64 size) = 0;

        /**
         * TBD.
         */
        virtual void MapBuffer(RenderBackendBufferHandle buffer, void** data) = 0;

        /**
         * TBD.
         */
        virtual void UnmapBuffer(RenderBackendBufferHandle buffer) = 0;

        /**
         * TBD.
         */
        virtual RenderBackendTextureHandle CreateTexture(const RenderBackendTextureDesc* desc, const void* data, const char* name) = 0;

        /**
         * TBD.
         */
        virtual void DestroyTexture(RenderBackendTextureHandle texture) = 0;

        /**
         * TBD.
         */
        virtual void UploadTexture(RenderBackendTextureHandle handle, const RenderBackendTextureUploadDataDesc& data) = 0;

        /**
         * TBD.
         */
        virtual void GetTextureReadbackData(RenderBackendTextureHandle texture, void** data) = 0;

        /** TODO: Do we need to create SRV/UAV explicitly? */
        //virtual RenderBackendTextureSRVHandle CreateTextureSRV(const RenderBackendTextureSRVDesc* desc, const char* name) = 0;
        //virtual RenderBackendTextureUAVHandle CreateTextureUAV(const RenderBackendTextureUAVDesc* desc, const char* name) = 0;

        /**
         * TBD.
         */
        virtual int32 GetTextureSRVBindlessResourceDescriptorIndex(RenderBackendTextureHandle handle) = 0;

        /**
         * TBD.
         */
        virtual int32 GetTextureSRVBindlessResourceDescriptorIndex(RenderBackendTextureHandle handle, uint32 mipLevel) = 0;

        /**
         * TBD.
         */
        virtual int32 GetTextureUAVBindlessResourceDescriptorIndex(RenderBackendTextureHandle handle, uint32 mipLevel) = 0;

        /**
         * TBD.
         */
        virtual int32 GetBufferCBVBindlessResourceDescriptorIndex(RenderBackendBufferHandle buffer) = 0;

        /**
         * TBD.
         */
        virtual int32 GetBufferSRVBindlessResourceDescriptorIndex(RenderBackendBufferHandle buffer) = 0;

        /**
         * TBD.
         */
        virtual int32 GetBufferUAVBindlessResourceDescriptorIndex(RenderBackendBufferHandle buffer) = 0;

        /**
         * TBD.
         */
        virtual int32 GetAccelerationStructureSRVBindlessResourceDescriptorIndex(RenderBackendRayTracingAccelerationStructureHandle accelerationStructure) = 0;

        virtual RenderBackendTextureViewHandle CreateTextureView(RenderBackendTextureHandle textureHandle, const RenderBackendTextureViewDesc* desc, int32* descriptor) = 0;

        /**
         * TBD.
         */
        virtual RenderBackendSamplerHandle CreateSampler(const RenderBackendSamplerDesc* desc, const char* name) = 0;

        /**
         * TBD.
         */
        virtual void DestroySampler(RenderBackendSamplerHandle sampler) = 0;

        /**
         * TBD.
         */
        virtual RenderBackendShaderHandle CreateShader(const RenderBackendShaderDesc* desc, const char* name) = 0;

        /**
         * TBD.
         */
        virtual void DestroyShader(RenderBackendShaderHandle shader) = 0;

#if 0
        /**
         * Experimental.
         */
        virtual RenderBackendShaderModuleHandle CreateShaderModule(const RenderBackendShaderDesc* desc, const char* name) = 0;

        /**
         * Experimental.
         */
        virtual void DestroyShaderModule(RenderBackendShaderModuleHandle shaderModule) = 0;
#endif

        /**
         * TBD.
         */
        virtual RenderBackendTimingQueryHeapHandle CreateTimingQueryHeap(const RenderBackendTimingQueryHeapDesc* desc, const char* name) = 0;

        /**
         * TBD.
         */
        virtual void DestroyTimingQueryHeap(RenderBackendTimingQueryHeapHandle timingQueryHeap) = 0;
        //virtual bool GetTimingQueryHeapResults(RenderBackendTimingQueryHeapHandle timingQueryHeap, uint32 regionStart, uint32 regionCount, void* results) = 0;

        /**
         * TBD.
         */
        virtual void SubmitCommandLists(RenderBackendCommandList** commandLists, uint32 numCommandLists, RenderBackendSwapChainHandle swapChain) = 0;

        /**
         * TBD.
         */
        virtual RenderBackendRayTracingAccelerationStructureHandle CreateRayTracingBottomLevelAccelerationStructure(const RenderBackendRayTracingBottomLevelAccelerationStructureDesc* desc, const char* name) = 0;

        /**
         * TBD.
         */
        virtual RenderBackendRayTracingAccelerationStructureHandle CreateRayTracingTopLevelAccelerationStructure(const RenderBackendRayTracingTopLevelAccelerationStructureDesc* desc, const char* name) = 0;

        /**
         * TBD.
         */
        virtual RenderBackendRayTracingPipelineStateHandle CreateRayTracingPipelineState(const RenderBackendRayTracingPipelineStateDesc* desc, const char* name) = 0;

        /**
         * TBD.
         */
        virtual RenderBackendBufferHandle CreateRayTracingShaderBindingTable(const RenderBackendRayTracingShaderBindingTableDesc* desc, const char* name) = 0;

        void UpdateBuffer(RenderBackendBufferHandle handle, uint64 offset, const void* data, uint64 size)
        {
            void* bufferAllocation = nullptr;
            MapBuffer(handle, &bufferAllocation);
            memcpy(bufferAllocation, data, size); // TODO: offset
            UnmapBuffer(handle);
        }
    };

    RenderBackend* RenderBackendCreateInstance(const RenderBackendDesc* desc);

    void RenderBackendDestroyInstance(RenderBackend* backend);
}