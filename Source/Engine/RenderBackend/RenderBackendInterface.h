#pragma once

#include "RenderBackend/RenderBackendCommon.h"

namespace HE
{
    class RenderBackendCommandList;

    enum class RenderBackendType
    {
        Unknown,
        D3D12,
        Vulkan,
    };

    class RenderBackend
    {
    public:
        virtual RenderBackendType GetType() const { return RenderBackendType::Unknown; }
        virtual void Tick() = 0;
        virtual void CreateRenderDevices(PhysicalDeviceID* physicalDeviceIDs, uint32 numDevices, uint32* outDeviceMasks) = 0;
        virtual void DestroyRenderDevices() = 0;
        virtual void FlushRenderDevices() = 0;
        virtual RenderBackendSwapChainHandle CreateSwapChain(uint32 deviceMask, const RenderBackendSwapChainDesc* desc) = 0;
        virtual void DestroySwapChain(RenderBackendSwapChainHandle swapChain) = 0;
        virtual void ResizeSwapChain(RenderBackendSwapChainHandle swapChain, uint32* width, uint32* height) = 0;
        virtual bool PresentSwapChain(RenderBackendSwapChainHandle swapChain) = 0;
        virtual RenderBackendTextureHandle GetActiveSwapChainBuffer(RenderBackendSwapChainHandle swapChain) = 0;
        virtual RenderBackendBufferHandle CreateBuffer(uint32 deviceMask, const RenderBackendBufferDesc* desc, const void* data, const char* name) = 0;
        virtual void ResizeBuffer(RenderBackendBufferHandle buffer, uint64 size) = 0;
        virtual void MapBuffer(RenderBackendBufferHandle buffer, void** data) = 0;
        virtual void UnmapBuffer(RenderBackendBufferHandle buffer) = 0;
        virtual void UpdateBuffer(RenderBackendBufferHandle buffer, uint64 offset, const void* data, uint64 size) = 0;
        virtual void DestroyBuffer(RenderBackendBufferHandle buffer) = 0;
        virtual RenderBackendTextureHandle CreateTexture(uint32 deviceMask, const RenderBackendTextureDesc* desc, const void* data, const char* name) = 0;
        virtual void DestroyTexture(RenderBackendTextureHandle texture) = 0;
        virtual void UploadTexture(RenderBackendTextureHandle handle, const RenderBackendTextureUploadDataDesc& data) = 0;
        virtual void GetTextureReadbackData(RenderBackendTextureHandle texture, void** data) = 0;
        virtual RenderBackendTextureSRVHandle CreateTextureSRV(uint32 deviceMask, const RenderBackendTextureSRVDesc* desc, const char* name) = 0;
        virtual RenderBackendTextureUAVHandle CreateTextureUAV(uint32 deviceMask, const RenderBackendTextureUAVDesc* desc, const char* name) = 0;
        virtual int32 GetTextureSRVDescriptorIndex(uint32 deviceMask, RenderBackendTextureHandle srv) = 0;
        virtual int32 GetTextureUAVDescriptorIndex(uint32 deviceMask, RenderBackendTextureHandle uav) = 0;
        virtual int32 GetBufferUAVDescriptorIndex(uint32 deviceMask, RenderBackendBufferHandle uav) = 0;
        virtual RenderBackendSamplerHandle CreateSampler(uint32 deviceMask, const RenderBackendSamplerDesc* desc, const char* name) = 0;
        virtual void DestroySampler(RenderBackendSamplerHandle sampler) = 0;
        virtual RenderBackendShaderHandle CreateShader(uint32 deviceMask, const RenderBackendShaderDesc* desc, const char* name) = 0;
        virtual void DestroyShader(RenderBackendShaderHandle shader) = 0;
        virtual RenderBackendTimingQueryHeapHandle CreateTimingQueryHeap(uint32 deviceMask, const RenderBackendTimingQueryHeapDesc* desc, const char* name) = 0;
        virtual void DestroyTimingQueryHeap(RenderBackendTimingQueryHeapHandle timingQueryHeap) = 0;
        //virtual bool GetTimingQueryHeapResults(RenderBackendTimingQueryHeapHandle timingQueryHeap, uint32 regionStart, uint32 regionCount, void* results) = 0;
        virtual RenderBackendOcclusionQueryHeapHandle CreateOcclusionQueryHeap(uint32 deviceMask, const RenderBackendOcclusionQueryHeapDesc* desc, const char* name) = 0;
        virtual void DestroyOcclusionQueryHeap(RenderBackendOcclusionQueryHeapHandle occlusionQueryHeap) = 0;
        virtual void SubmitCommandLists(RenderBackendCommandList** commandLists, uint32 numCommandLists, RenderBackendSwapChainHandle swapChain) = 0;
        virtual RenderBackendRayTracingAccelerationStructureHandle CreateRayTracingBottomLevelAccelerationStructure(uint32 deviceMask, const RenderBackendRayTracingBottomLevelAccelerationDesc* desc, const char* name) = 0;
        virtual RenderBackendRayTracingAccelerationStructureHandle CreateRayTracingTopLevelAccelerationStructure(uint32 deviceMask, const RenderBackendRayTracingTopLevelAccelerationDesc* desc, const char* name) = 0;
        virtual RenderBackendRayTracingPipelineStateHandle CreateRayTracingPipelineState(uint32 deviceMask, const RenderBackendRayTracingPipelineStateDesc* desc, const char* name) = 0;
        virtual RenderBackendBufferHandle CreateRayTracingShaderBindingTable(uint32 deviceMask, const RenderBackendRayTracingShaderBindingTableDesc* desc, const char* name) = 0;
    };

    extern RenderBackend* GRenderBackend;
}