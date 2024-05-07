#pragma once

#include "RenderBackendCommon.h"
#include "RenderBackendHandles.h"
#include "RenderBackendTypes.h"

namespace Horizon
{
    struct RenderBackendSuperSamplingTextureResource
    {
        /** VkImage or ID3D12Resource */
        void* texture;

        /** vkDeviceMemory or nullptr */
        void* memory;

        /** VkImageView or nullptr */
        void* view;

        /** Width in pixels */
        uint32_t width;

        /** Height in pixels */
        uint32_t height;

        /** Number of mip-map levels */
        uint32_t mipLevels;

        /** Number of arrays */
        uint32_t arrayLayers;

        /** Native format */
        uint32_t format;

        /** VkImageLayout or D3D12_RESOURCE_STATES */
        uint32_t state;

        /** VkImageCreateFlags */
        uint32_t flags;

        /** VkImageUsageFlags */
        uint32_t usage;
    };

    struct RenderBackendSuperSamplingDescription
    {
        uint32 viewportHandle;
        void* device;
        void* physicalDevice;
        void* commandList;

        RenderBackendSuperSamplingTextureResource output;
        RenderBackendSuperSamplingTextureResource color;
        RenderBackendSuperSamplingTextureResource depth;
        RenderBackendSuperSamplingTextureResource motionVectors;

        bool reset;
        uint32 renderWidth;
        uint32 renderHeight;
        uint32 targetWidth;
        uint32 targetHeight;
        float jitterOffsetX;
        float jitterOffsetY;
        float motionVectorScaleX;
        float motionVectorScaleY;
        float deltaTime;
        float preExposure;
        float cameraFarClippingPlane;
        float cameraNearClippingPlane;
        float cameraFovAngleVertical;
        bool enableSharpening;
        float sharpeness;
    };

    class RenderBackendCommandList;

    enum class RenderBackendType
    {
        Unknown,
        D3D12,
        Vulkan,
    };

    using PhysicalDeviceID = uint32;

    /**
     * TBD.
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

        /** TODO: Do we need to create SRV/UAV explicitly?
        //virtual RenderBackendTextureSRVHandle CreateTextureSRV(const RenderBackendTextureSRVDesc* desc, const char* name) = 0;
        //virtual RenderBackendTextureUAVHandle CreateTextureUAV(const RenderBackendTextureUAVDesc* desc, const char* name) = 0;

        /**
         * TBD.
         */
        virtual int32 GetTextureSRVBindlessDescriptorIndex(RenderBackendTextureHandle handle) = 0;
        
        /**
         * TBD.
         */
        virtual int32 GetTextureUAVBindlessDescriptorIndex(RenderBackendTextureHandle handle, uint32 mipLevel) = 0;
        
        /**
         * TBD.
         */
        virtual int32 GetBufferBindlessDescriptorIndex(RenderBackendBufferHandle buffer) = 0;

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
        virtual RenderBackendRayTracingAccelerationStructureHandle CreateRayTracingBottomLevelAccelerationStructure(const RenderBackendRayTracingBottomLevelAccelerationDesc* desc, const char* name) = 0;

        /**
         * TBD.
         */
        virtual RenderBackendRayTracingAccelerationStructureHandle CreateRayTracingTopLevelAccelerationStructure(const RenderBackendRayTracingTopLevelAccelerationDesc* desc, const char* name) = 0;
        
        /**
         * TBD.
         */
        virtual RenderBackendRayTracingPipelineStateHandle CreateRayTracingPipelineState(const RenderBackendRayTracingPipelineStateDesc* desc, const char* name) = 0;

        /**
         * TBD.
         */
        virtual RenderBackendBufferHandle CreateRayTracingShaderBindingTable(const RenderBackendRayTracingShaderBindingTableDesc* desc, const char* name) = 0;
    };
}
