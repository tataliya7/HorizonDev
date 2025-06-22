#pragma once

#include "Direct3D12RenderBackendCommon.h"

#include <wrl/client.h>

#include <dxgi1_6.h>
#include <dxgiformat.h>
#include <dxgidebug.h>

#include <d3dcommon.h>
#include <d3d12.h>
#include <d3d12compatibility.h>
#include <d3d12sdklayers.h>
#include <d3d12shader.h>
#include <d3d12video.h>

#include <d3dx12/d3dx12.h>
#include <d3dx12/d3dx12_core.h>
#include <d3dx12/d3dx12_default.h>
#include <d3dx12/d3dx12_barriers.h>
#include <d3dx12/d3dx12_render_pass.h>
#include <d3dx12/d3dx12_state_object.h>
#include <d3dx12/d3dx12_root_signature.h>
#include <d3dx12/d3dx12_resource_helpers.h>
#include <d3dx12/d3dx12_pipeline_state_stream.h>
#include <d3dx12/d3dx12_property_format_table.h>
#include <d3dx12/d3dx12_check_feature_support.h>

#include "D3D12MemoryAllocator/D3D12MemAlloc.h"

namespace Horizon
{
    class D3D12RenderBackend;

    enum class D3D12CommandQueueType
    {
        Direct      = 0,
        Compute     = 1,
        Copy        = 2,
        Count       = 3,
        VideoEncode = 3,
        VideoDecode = 3,
    };

    inline D3D12_COMMAND_LIST_TYPE GetD3D12CommandListType(D3D12CommandQueueType queueType)
    {
        switch (queueType)
        {
        case D3D12CommandQueueType::Direct:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
        case D3D12CommandQueueType::Compute:
            return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        case D3D12CommandQueueType::Copy:
            return D3D12_COMMAND_LIST_TYPE_COPY;
        default: std::unreachable();
            return D3D12_COMMAND_LIST_TYPE_NONE;
        }
    }

    uint32 GetDepthStencilViewIndex(RenderBackendDepthStencilAccessType accessType)
    {
        switch (accessType)
        {
        case RenderBackendDepthStencilAccessType::DepthNoAccess_StencilNoAccess:
        case RenderBackendDepthStencilAccessType::DepthNoAccess_StencilWrite:
        case RenderBackendDepthStencilAccessType::DepthWrite_StencilNoAccess:
        case RenderBackendDepthStencilAccessType::DepthWrite_StencilWrite:
            return 0u;
        case RenderBackendDepthStencilAccessType::DepthReadOnly_StencilNoAccess:
        case RenderBackendDepthStencilAccessType::DepthReadOnly_StencilWrite:
            return 1u;
        case RenderBackendDepthStencilAccessType::DepthNoAccess_StencilReadOnly:
        case RenderBackendDepthStencilAccessType::DepthWrite_StencilReadOnly:
            return 2u;
        case RenderBackendDepthStencilAccessType::DepthReadOnly_StencilReadOnly:
            return 3u;
        default:
            std::unreachable();
            return ~0u;
        }
    }

    struct D3D12Adapter
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter> dxgiAdapter;

        uint32 adapterIndex;

        DXGI_ADAPTER_DESC1 desc;

        IDXGIAdapter* GetIDXGIAdapter()
        {
            return dxgiAdapter.Get();
        }
    };

    struct D3D12CommandList
    {
        D3D12CommandQueueType queueType;

        Microsoft::WRL::ComPtr<ID3D12CommandList> commandList;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> graphicsCommandList;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> graphicsCommandList4;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList6> graphicsCommandList6;
        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> graphicsCommandList7;

        ID3D12CommandList* GetID3D12CommandList()
        {
            return commandList.Get();
        }

        ID3D12GraphicsCommandList* GetID3D12GraphicsCommandList()
        {
            return graphicsCommandList.Get();
        }

        ID3D12GraphicsCommandList4* GetID3D12GraphicsCommandList4()
        {
            return graphicsCommandList4.Get();
        }

        ID3D12GraphicsCommandList6* GetID3D12GraphicsCommandList6()
        {
            return graphicsCommandList6.Get();
        }

        ID3D12GraphicsCommandList7* GetID3D12GraphicsCommandList7()
        {
            return graphicsCommandList7.Get();
        }
    };

    struct D3D12CommandAllocator
    {
        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
        D3D12CommandQueueType queueType;

        ID3D12CommandAllocator* GetID3D12CommandAllocator()
        {
            return allocator.Get();
        }
    };

    struct D3D12CommandQueue
    {
        D3D12CommandQueueType queueType;
        Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
        Microsoft::WRL::ComPtr<ID3D12Fence> fence;
        uint64 lastSignaledValue;

        ID3D12CommandQueue* GetID3D12CommandQueue()
        {
            return queue.Get();
        }

        uint64 SignalFence()
        {
            uint64 valueToSignal = ++lastSignaledValue;
            D3D12_CHECK(queue->Signal(fence.Get(), valueToSignal));
            return valueToSignal;
        }
    };

    struct D3D12SubmissionWorkload
    {
        struct Fence
        {
            Microsoft::WRL::ComPtr<ID3D12Fence> fence;
            uint64 value;

            Fence(Microsoft::WRL::ComPtr<ID3D12Fence>&& fence, uint64 value)
                : fence(std::move(fence))
                , value(value)
            {

            }
        };

        D3D12CommandQueue* commandQueue;

        uint64 completionFenceValue;

        std::vector<Fence> fencesToWait;
        std::vector<Fence> fencesToSignal;

        std::vector<D3D12CommandList*> commandListsToExecute;

        std::vector<D3D12CommandAllocator*> commandAllocatorsToRelease;
    };

    struct D3D12Buffer
    {
        std::string debugName;
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation;
        RenderBackendBufferCreateFlags flags;
        RenderBackendBufferDesc desc;
        D3D12_RESOURCE_DESC1 resourceDesc;
        D3D12_BARRIER_LAYOUT initialLayout;
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
        uint64 size;
        void* mappedData;

        D3D12_CPU_DESCRIPTOR_HANDLE descriptor;

        int bindlessResourceDescriptorIndexCBV;
        int bindlessResourceDescriptorIndexSRV;
        int bindlessResourceDescriptorIndexUAV;

        struct ShaderBindingTable
        {
            D3D12_GPU_VIRTUAL_ADDRESS_RANGE rayGenerationShaderRecord;
            D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE missShaderTable;
            D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE hitGroupTable;
            D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE callableShaderTable;
        };
        ShaderBindingTable shaderBindingTable;

        ID3D12Resource* GetID3D12Resource()
        {
            return resource.Get();
        }
    };

    struct D3D12ShaderResourceView
    {
        D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
        int bindlessIndex;
    };

    struct D3D12UnorderedAccessView
    {
        D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
        int bindlessIndex;
    };

    struct D3D12RenderTargetView
    {
        D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
    };

    struct D3D12DepthStencilView
    {
        D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
    };

    struct D3D12Texture
    {
        std::string debugName;

        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        Microsoft::WRL::ComPtr<D3D12MA::Allocation> allocation;

        uint32 width;
        uint32 height;
        uint32 depth;
        uint32 arraySize;
        uint32 mipLevels;
        DXGI_FORMAT format;

        RenderBackendTextureType t;
        RenderBackendResourceState initialState;

        bool isSwapChainBuffer;

        D3D12_CLEAR_VALUE clearValue;

        D3D12ShaderResourceView* shaderResourceView;

        std::vector<D3D12RenderTargetView*> renderTargetViews;
        D3D12DepthStencilView* depthStencilViews[4];
        std::vector<D3D12ShaderResourceView*> shaderResourceViews;
        std::vector<D3D12UnorderedAccessView*> unorderedAccessViews;

        UINT64 totalSize = 0;
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints;
        std::vector<UINT64> rowSizesInBytes;
        std::vector<UINT> numRows;

        D3D12ShaderResourceView* GetShaderResourceView(uint32 mipSlice)
        {
            return shaderResourceViews[mipSlice];
        }

        D3D12RenderTargetView* GetRenderTargetView(uint32 mipSlice)
        {
            //uint32 index = mipSlice * arraySize + arraySlice;
            //assert(index < (uint32)renderTargetViews.size());
            return renderTargetViews[mipSlice];
        }

        D3D12UnorderedAccessView* GetUnorderedAccessView(uint32 mipSlice)
        {
            return unorderedAccessViews[mipSlice];
        }

        D3D12DepthStencilView* GetDepthStencilView(uint32 index)
        {
            assert(index <= 3);
            return depthStencilViews[index];
        }

        ID3D12Resource* GetID3D12Resource()
        {
            return resource.Get();
        }
    };

    class D3D12BindlessDescriptorAllocator
    {

    };

    struct D3D12RenderPass
    {
        bool hasDepthStencil;
        uint32 numRenderTargets;
        DXGI_FORMAT renderTargetFormats[RenderBackendMaxRenderTargetCount];
        DXGI_FORMAT depthStencilViewFormat;
    };

    struct D3D12Sampler
    {
        D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
        int bindlessIndex;
    };

    struct D3D12RayTracingAccelerationStructure
    {
        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags;
        Microsoft::WRL::ComPtr<ID3D12Resource> accelerationStructureBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> scratchBuffer;
        Microsoft::WRL::ComPtr<ID3D12Resource> instanceBuffer;
        uint32 numInstances = 0;
        int bindlessIndex = -1;
        D3D12_CPU_DESCRIPTOR_HANDLE descriptor;
        std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometries;
    };

    struct D3D12TimingQueryHeap
    {
        Microsoft::WRL::ComPtr<ID3D12QueryHeap> queryHeap;
        uint32 maxQueryCount;

        ID3D12QueryHeap* GetID3D12QueryHeap()
        {
            return queryHeap.Get();
        }
    };

    struct D3D12DescriptorHeap
    {
        Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
        D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptorHandle;
        D3D12_GPU_DESCRIPTOR_HANDLE gpuDescriptorHandle;

        ID3D12DescriptorHeap* GetID3D12DescriptorHeap()
        {
            return descriptorHeap.Get();
        }
    };

    struct D3D12Shader
    {
        D3D12_SHADER_BYTECODE bytecode;
        std::wstring entryFunctionName;
        uint32 hash;
    };

    /**
     * Each individual program is something that can be bound as a unit on the GPU for execution.
     * Consisting of a grouping of shaders and/or fixed function operations that are launched as logical grouping on the GPU.
     */
    struct D3D12Program
    {

    };

    struct D3D12ComputePipelineState
    {
        Microsoft::WRL::ComPtr<ID3D12PipelineState> state;

        ID3D12PipelineState* GetID3D12PipelineState()
        {
            return state.Get();
        }
    };

    struct D3D12GraphicsPipelineStateDesc
    {
        D3D12_BLEND_DESC blendState;
        D3D12_RASTERIZER_DESC rasterizerState;
        D3D12_DEPTH_STENCIL_DESC depthStencilState;
    };

    struct D3D12GraphicsPipelineState
    {
        Microsoft::WRL::ComPtr<ID3D12PipelineState> stateObject;

        D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType;

        ID3D12PipelineState* GetID3D12PipelineState()
        {
            return stateObject.Get();
        }
    };

    struct D3D12RayTracingPipelineStateObject
    {
        Microsoft::WRL::ComPtr<ID3D12StateObject> stateObject;

        uint32 numMissShaders = 0;
        uint32 numHitGroups = 0;
        RenderBackendRayTracingPipelineStateDesc desc;

        ID3D12StateObject* GetID3D12StateObject()
        {
            return stateObject.Get();
        }
    };

    struct D3D12SwapChain
    {
        Microsoft::WRL::ComPtr<IDXGISwapChain1> dxgiSwapChain1;
        Microsoft::WRL::ComPtr<IDXGISwapChain2> dxgiSwapChain2;
        Microsoft::WRL::ComPtr<IDXGISwapChain3> dxgiSwapChain3;
        Microsoft::WRL::ComPtr<IDXGISwapChain4> dxgiSwapChain4;

        uint32 width;
        uint32 height;
        DXGI_FORMAT format;

        bool windowed;
        bool vsyncEnabled;

        // TODO ColorSpace colorSpace = ColorSpace::SRGB;

        uint32 numBuffers;
        RenderBackendTextureHandle buffers[RenderBackendMaxSwapChainBufferCount];
        Microsoft::WRL::ComPtr<ID3D12Fence> frameFences[RenderBackendMaxSwapChainBufferCount];

        IDXGISwapChain1* GetIDXGISwapChain1()
        {
            return dxgiSwapChain1.Get();
        }

        IDXGISwapChain2* GetIDXGISwapChain2()
        {
            return dxgiSwapChain2.Get();
        }

        IDXGISwapChain3* GetIDXGISwapChain3()
        {
            return dxgiSwapChain3.Get();
        }

        IDXGISwapChain4* GetIDXGISwapChain4()
        {
            return dxgiSwapChain4.Get();
        }

        ID3D12Fence* GetFrameFence(uint32 bufferIndex)
        {
            return frameFences[bufferIndex].Get();
        }

        uint32 GetBufferCount() const
        {
            return numBuffers;
        }

        uint32 GetCurrentBackBufferIndex() const
        {
            return dxgiSwapChain3->GetCurrentBackBufferIndex();
        }
    };

    struct D3D12RenderBackendHandleManager
    {
        std::vector<uint32> freeIndices;
        uint32 nextIndex;
        template <typename HandleType>
        HandleType Allocate(uint32 deviceMask = ~0u)
        {
            uint32 index = 0;
            if (!freeIndices.empty())
            {
                index = freeIndices.back();
                freeIndices.pop_back();
            }
            else
            {
                nextIndex++;
                index = nextIndex;
            }
            HandleType handle = HandleType(index, deviceMask);
            return handle;
        }
        template <typename HandleType>
        void Free(HandleType handle)
        {
            uint32 index = ((RenderBackendHandle)handle).GetIndex();
            freeIndices.push_back(index);
        }
    };

    class D3D12Device
    {
    public:

        D3D12Device(D3D12RenderBackend* backend)
            : backend(backend)
        {

        }

        ~D3D12Device()
        {
            // if (DXGIDebug != nullptr)
            // {
            //     DXGIDebug->ReportLiveObjects(
            //         GUID{ 0xe48ae283, 0xda80, 0x490b, { 0x87, 0xe6, 0x43, 0xe9, 0xa9, 0xcf, 0xda, 0x8 } }, // DXGI_DEBUG_ALL
            //         DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
            //     DXGIDebug.SafeRelease();
            // }
        }

        D3D12RenderBackend* backend;

        struct DeviceMask
        {
            UINT mask;
            UINT Get() const
            {
                return mask;
            }
        };
        DeviceMask mask;
        const DeviceMask& GetMask() const { return mask; }

        std::vector<D3D12SubmissionWorkload*> workloads;

        Microsoft::WRL::ComPtr<ID3D12CommandSignature> drawIndirectCommandSignature;
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> drawIndexedIndirectCommandSignature;
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> dispatchIndirectCommandSignature;
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> dispatchRaysIndirectCommandSignature;
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> dispatchMeshIndirectCommandSignature;

        ID3D12Device* GetID3D12Device()
        {
            return device.Get();
        }

        ID3D12Device5* GetDXRDevice()
        {
            return device5.Get();
        }

        ID3D12CommandSignature* GetDispatchIndirectCommandSignature()
        {
            return dispatchIndirectCommandSignature.Get();
        }

        ID3D12CommandSignature* GetDrawIndirectCommandSignature()
        {
            return drawIndirectCommandSignature.Get();
        }

        ID3D12CommandSignature* GetDrawIndexedIndirectCommandSignature()
        {
            return drawIndexedIndirectCommandSignature.Get();
        }

        ID3D12CommandSignature* GetDispatchMeshIndirectCommandSignature()
        {
            return dispatchMeshIndirectCommandSignature.Get();
        }

        D3D12Texture* GetTexture(RenderBackendTextureHandle handle)
        {
            uint32 index = GetRenderBackendHandleRepresentation(handle.GetIndex());
            return textures[index];
        }

        D3D12Buffer* GetBuffer(RenderBackendBufferHandle handle)
        {
            uint32 index = GetRenderBackendHandleRepresentation(handle.GetIndex());
            return buffers[index];
        }

        D3D12RayTracingAccelerationStructure* GetRayTracingAccelerationStructure(RenderBackendRayTracingAccelerationStructureHandle handle)
        {
            uint32 index = GetRenderBackendHandleRepresentation(handle.GetIndex());
            return accelerationStructures[index];
        }

        D3D12Shader* GetShader(RenderBackendShaderHandle handle)
        {
            uint32 index = GetRenderBackendHandleRepresentation(handle.GetIndex());
            return shaders[index];
        }

        D3D12TimingQueryHeap* GetTimingQueryHeap(RenderBackendTimingQueryHeapHandle handle)
        {
            uint32 index = GetRenderBackendHandleRepresentation(handle.GetIndex());
            return queryHeaps[index];
        }

        D3D12SwapChain* GetSwapChain(RenderBackendSwapChainHandle handle)
        {
            uint32 index = GetRenderBackendHandleRepresentation(handle.GetIndex());
            return swapChains[index];
        }

        uint32 CreateD3D12SwapChain(const RenderBackendSwapChainDesc* desc);

        uint32 AllocateBuffer()
        {
            uint32 bufferIndex = 0;
            if (!freeBuffers.empty())
            {
                bufferIndex = freeBuffers.back();
                freeBuffers.pop_back();
            }
            else
            {
                bufferIndex = (uint32)buffers.size();
                buffers.emplace_back(new D3D12Buffer());
            }
            return bufferIndex;
        }

        uint32 CreateD3D12Buffer(const RenderBackendBufferDesc* desc, const void* data, const char* name)
        {
            uint32 index = AllocateBuffer();
            D3D12Buffer* buffer = buffers[index];

            D3D12_RESOURCE_DESC1 resourceDesc =
            {
                .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
                .Alignment = 0,
                .Width = desc->size,
                .Height = 1,
                .DepthOrArraySize = 1,
                .MipLevels = 1,
                .Format = DXGI_FORMAT_UNKNOWN,
                .SampleDesc =
                {
                    .Count = 1,
                    .Quality = 0,
                },
                .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                .Flags = GetD3D12ResourceFlags(desc->flags)
            };

            D3D12MA::ALLOCATION_DESC allocationDesc =
            {
                .Flags = D3D12MA::ALLOCATION_FLAG_NONE,
                .HeapType = GetD3D12HeapType(desc->flags),
                .ExtraHeapFlags = D3D12_HEAP_FLAG_NONE, // TODO
                .CustomPool = nullptr,
                .pPrivateData = nullptr,
            };

            // Buffer resources must specify an initial layout of D3D12_BARRIER_LAYOUT_UNDEFINED.
            D3D12_BARRIER_LAYOUT initialLayout = D3D12_BARRIER_LAYOUT_UNDEFINED;

            assert(buffer->resource == nullptr);
            assert(buffer->allocation == nullptr);

            D3D12_CHECK(allocator->CreateResource3(
                &allocationDesc,
                &resourceDesc,
                initialLayout,
                nullptr,
                0,
                nullptr,
                &buffer->allocation,
                IID_PPV_ARGS(&buffer->resource)));
            D3D12_CHECK(buffer->resource->SetName(UTF8ToUTF16(name).c_str()));

            buffer->debugName = name;
            buffer->desc = *desc;
            buffer->resourceDesc = resourceDesc;
            buffer->initialLayout = initialLayout;
            buffer->gpuAddress = buffer->resource->GetGPUVirtualAddress();
            buffer->flags = desc->flags;
            buffer->size = desc->size;
            buffer->bindlessResourceDescriptorIndexCBV = -1;
            buffer->bindlessResourceDescriptorIndexSRV = -1;
            buffer->bindlessResourceDescriptorIndexUAV = -1;

            if (EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::Readback))
            {
                D3D12_CHECK(buffer->resource->Map(0, nullptr, &buffer->mappedData));
            }
            else if (EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::Upload)) // Always mapped
            {
                // It is valid to specify the CPU won't read any data by passing a range where End is less than or equal to Begin.
                D3D12_RANGE readRange =
                {
                    .Begin = 0,
                    .End = 0,
                };
                D3D12_CHECK(buffer->resource->Map(0, &readRange, &buffer->mappedData));
            }
            else
            {
                // The mapped data pointer is valid only if the resource is created with CPU access (upload or readback).
                buffer->mappedData = nullptr;
            }

            if (data != nullptr)
            {
                if (EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::Upload)) // Copy directly in mapped data
                {
                    assert(buffer->mappedData);
                    memcpy(buffer->mappedData, data, buffer->size);
                }
                else // Copy throught upload heap
                {
                    CopyWorkload copyWorkload = AllocateCopyWorkload(buffer->size);

                    memcpy(copyWorkload.uploadBuffer->mappedData, data, buffer->size);

                    copyWorkload.commandList->Reset(copyWorkload.commandAllocator.Get(), nullptr);
                    {
                        D3D12_BUFFER_BARRIER BufBarriers[] =
                        {
                            CD3DX12_BUFFER_BARRIER(
                                D3D12_BARRIER_SYNC_ALL,
                                D3D12_BARRIER_SYNC_ALL,
                                D3D12_BARRIER_ACCESS_NO_ACCESS,
                                D3D12_BARRIER_ACCESS_COPY_DEST,
                                buffer->GetID3D12Resource()
                            )
                        };

                        D3D12_BARRIER_GROUP BufBarrierGroups[] =
                        {
                            CD3DX12_BARRIER_GROUP(1, BufBarriers)
                        };

                        copyWorkload.commandList7->Barrier(1, BufBarrierGroups);
                    }

                    copyWorkload.commandList->CopyBufferRegion(
                        buffer->GetID3D12Resource(),
                        0,
                        copyWorkload.uploadBuffer->GetID3D12Resource(),
                        0,
                        buffer->size);

                    SubmitCopyWorkload(copyWorkload);
                }
            }

            if (EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::UniformBuffer))
            {
                D3D12_CONSTANT_BUFFER_VIEW_DESC cbcDesc =
                {
                    .BufferLocation = buffer->gpuAddress,
                    .SizeInBytes = UINT(buffer->size),
                };

                buffer->descriptor = resourceDescriptorAllocator.Allocate();
                device->CreateConstantBufferView(&cbcDesc, buffer->descriptor);

                buffer->bindlessResourceDescriptorIndexCBV = AllocateResourceDescriptorIndex();
                if (buffer->bindlessResourceDescriptorIndexCBV >= 0)
                {
                    assert(buffer->bindlessResourceDescriptorIndexCBV < D3D12_RENDER_BACKEND_BINDLESS_MAX_NUM_RESOURCE_DESCRIPTOERS);
                    D3D12_CPU_DESCRIPTOR_HANDLE rangeStart = resourceDescriptorHeap->cpuDescriptorHandle;
                    rangeStart.ptr += buffer->bindlessResourceDescriptorIndexCBV * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    device->CopyDescriptorsSimple(1, rangeStart, buffer->descriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                }
            }
            if (EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::ShaderResource))
            {
                bool isStructuredBuffer = EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::StructuredBuffer);

                D3D12_BUFFER_SRV bufferSRV =
                {
                    .FirstElement = 0,
                    .NumElements = isStructuredBuffer ? desc->elementCount : uint32(buffer->size / sizeof(uint32)),
                    .StructureByteStride = isStructuredBuffer ? desc->elementSize : 0,
                    .Flags = isStructuredBuffer ? D3D12_BUFFER_SRV_FLAG_NONE : D3D12_BUFFER_SRV_FLAG_RAW
                };

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc =
                {
                    .Format = isStructuredBuffer ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_R32_TYPELESS,
                    .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
                    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                    .Buffer = bufferSRV
                };

                buffer->descriptor = resourceDescriptorAllocator.Allocate();
                device->CreateShaderResourceView(buffer->GetID3D12Resource(), &srvDesc, buffer->descriptor);

                buffer->bindlessResourceDescriptorIndexSRV = AllocateResourceDescriptorIndex();
                if (buffer->bindlessResourceDescriptorIndexSRV >= 0)
                {
                    assert(buffer->bindlessResourceDescriptorIndexSRV < D3D12_RENDER_BACKEND_BINDLESS_MAX_NUM_RESOURCE_DESCRIPTOERS);
                    D3D12_CPU_DESCRIPTOR_HANDLE rangeStart = resourceDescriptorHeap->cpuDescriptorHandle;
                    rangeStart.ptr += buffer->bindlessResourceDescriptorIndexSRV * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    device->CopyDescriptorsSimple(1, rangeStart, buffer->descriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                }
            }
            if (EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::UnorderedAccess))
            {
                bool isStructuredBuffer = EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::StructuredBuffer);

                D3D12_BUFFER_UAV bufferUAV =
                {
                    .FirstElement = 0,
                    .NumElements = isStructuredBuffer ? desc->elementCount : uint32(buffer->size / sizeof(uint32)),
                    .StructureByteStride = isStructuredBuffer ? desc->elementSize : 0,
                    .CounterOffsetInBytes = 0,
                    .Flags = isStructuredBuffer ? D3D12_BUFFER_UAV_FLAG_NONE : D3D12_BUFFER_UAV_FLAG_RAW
                };

                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc =
                {
                    .Format = isStructuredBuffer ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_R32_TYPELESS,
                    .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
                    .Buffer = bufferUAV
                };

                buffer->descriptor = resourceDescriptorAllocator.Allocate();
                device->CreateUnorderedAccessView(buffer->GetID3D12Resource(), nullptr, &uavDesc, buffer->descriptor);

                buffer->bindlessResourceDescriptorIndexUAV = AllocateResourceDescriptorIndex();
                if (buffer->bindlessResourceDescriptorIndexUAV >= 0)
                {
                    assert(buffer->bindlessResourceDescriptorIndexUAV < D3D12_RENDER_BACKEND_BINDLESS_MAX_NUM_RESOURCE_DESCRIPTOERS);
                    D3D12_CPU_DESCRIPTOR_HANDLE rangeStart = resourceDescriptorHeap->cpuDescriptorHandle;
                    rangeStart.ptr += buffer->bindlessResourceDescriptorIndexUAV * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    device->CopyDescriptorsSimple(1, rangeStart, buffer->descriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                }
            }

            return index;
        }

        void ResizeD3D12Buffer(uint32 index, uint64 size)
        {
            D3D12Buffer* buffer = buffers[index];

            // TODO: Destroy the original buffer

            D3D12MA::ALLOCATION_DESC allocationDesc = {
                .Flags = D3D12MA::ALLOCATION_FLAG_NONE,
                .HeapType = GetD3D12HeapType(buffer->flags),
                .ExtraHeapFlags = D3D12_HEAP_FLAG_NONE,
                .CustomPool = nullptr,
                .pPrivateData = nullptr,
            };

            buffer->resourceDesc.Width = size;

            D3D12_CHECK(allocator->CreateResource3(
                &allocationDesc,
                &buffer->resourceDesc,
                buffer->initialLayout,
                nullptr,
                0,
                nullptr,
                &buffer->allocation,
                IID_PPV_ARGS(&buffer->resource)));
            D3D12_CHECK(buffer->resource->SetName(UTF8ToUTF16(buffer->debugName).c_str()));

            buffer->gpuAddress = buffer->resource->GetGPUVirtualAddress();
            buffer->size = size;
            buffer->bindlessResourceDescriptorIndexCBV = -1;
            buffer->bindlessResourceDescriptorIndexSRV = -1;
            buffer->bindlessResourceDescriptorIndexUAV = -1;

            if (EnumClassHasFlags(buffer->flags, RenderBackendBufferCreateFlags::Readback))
            {
                D3D12_CHECK(buffer->resource->Map(0, nullptr, &buffer->mappedData));
            }
            else if (EnumClassHasFlags(buffer->flags, RenderBackendBufferCreateFlags::Upload))
            {
                // It is valid to specify the CPU won't read any data by passing a range where End is less than or equal to Begin
                D3D12_RANGE readRange = {
                    .Begin = 0,
                    .End = 0,
                };
                D3D12_CHECK(buffer->resource->Map(0, &readRange, &buffer->mappedData));
            }
            else
            {
                // The mapped data pointer is valid only if the resource is created with CPU access (upload or readback)
                buffer->mappedData = nullptr;
            }

            // TODO:
            RenderBackendBufferDesc* desc = &buffer->desc;
            desc->elementCount = uint32(size / desc->elementSize);

            if (EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::UniformBuffer))
            {
                D3D12_CONSTANT_BUFFER_VIEW_DESC cbcDesc =
                {
                    .BufferLocation = buffer->gpuAddress,
                    .SizeInBytes = UINT(buffer->size),
                };

                buffer->descriptor = resourceDescriptorAllocator.Allocate();
                device->CreateConstantBufferView(&cbcDesc, buffer->descriptor);

                buffer->bindlessResourceDescriptorIndexCBV = AllocateResourceDescriptorIndex();
                if (buffer->bindlessResourceDescriptorIndexCBV >= 0)
                {
                    assert(buffer->bindlessResourceDescriptorIndexCBV < D3D12_RENDER_BACKEND_BINDLESS_MAX_NUM_RESOURCE_DESCRIPTOERS);
                    D3D12_CPU_DESCRIPTOR_HANDLE rangeStart = resourceDescriptorHeap->cpuDescriptorHandle;
                    rangeStart.ptr += buffer->bindlessResourceDescriptorIndexCBV * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    device->CopyDescriptorsSimple(1, rangeStart, buffer->descriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                }
            }
            if (EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::ShaderResource))
            {
                bool isStructuredBuffer = EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::StructuredBuffer);

                D3D12_BUFFER_SRV bufferSRV =
                {
                    .FirstElement = 0,
                    .NumElements = isStructuredBuffer ? desc->elementCount : uint32(buffer->size / sizeof(uint32)),
                    .StructureByteStride = isStructuredBuffer ? desc->elementSize : 0,
                    .Flags = isStructuredBuffer ? D3D12_BUFFER_SRV_FLAG_NONE : D3D12_BUFFER_SRV_FLAG_RAW
                };

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc =
                {
                    .Format = isStructuredBuffer ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_R32_TYPELESS,
                    .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
                    .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
                    .Buffer = bufferSRV
                };

                buffer->descriptor = resourceDescriptorAllocator.Allocate();
                device->CreateShaderResourceView(buffer->GetID3D12Resource(), &srvDesc, buffer->descriptor);

                buffer->bindlessResourceDescriptorIndexSRV = AllocateResourceDescriptorIndex();
                if (buffer->bindlessResourceDescriptorIndexSRV >= 0)
                {
                    assert(buffer->bindlessResourceDescriptorIndexSRV < D3D12_RENDER_BACKEND_BINDLESS_MAX_NUM_RESOURCE_DESCRIPTOERS);
                    D3D12_CPU_DESCRIPTOR_HANDLE rangeStart = resourceDescriptorHeap->cpuDescriptorHandle;
                    rangeStart.ptr += buffer->bindlessResourceDescriptorIndexSRV * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    device->CopyDescriptorsSimple(1, rangeStart, buffer->descriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                }
            }
            if (EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::UnorderedAccess))
            {
                bool isStructuredBuffer = EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::StructuredBuffer);

                D3D12_BUFFER_UAV bufferUAV =
                {
                    .FirstElement = 0,
                    .NumElements = isStructuredBuffer ? desc->elementCount : uint32(buffer->size / sizeof(uint32)),
                    .StructureByteStride = isStructuredBuffer ? desc->elementSize : 0,
                    .CounterOffsetInBytes = 0,
                    .Flags = isStructuredBuffer ? D3D12_BUFFER_UAV_FLAG_NONE : D3D12_BUFFER_UAV_FLAG_RAW
                };

                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc =
                {
                    .Format = isStructuredBuffer ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_R32_TYPELESS,
                    .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
                    .Buffer = bufferUAV
                };

                buffer->descriptor = resourceDescriptorAllocator.Allocate();
                device->CreateUnorderedAccessView(buffer->GetID3D12Resource(), nullptr, &uavDesc, buffer->descriptor);

                buffer->bindlessResourceDescriptorIndexUAV = AllocateResourceDescriptorIndex();
                if (buffer->bindlessResourceDescriptorIndexUAV >= 0)
                {
                    assert(buffer->bindlessResourceDescriptorIndexUAV < D3D12_RENDER_BACKEND_BINDLESS_MAX_NUM_RESOURCE_DESCRIPTOERS);
                    D3D12_CPU_DESCRIPTOR_HANDLE rangeStart = resourceDescriptorHeap->cpuDescriptorHandle;
                    rangeStart.ptr += buffer->bindlessResourceDescriptorIndexUAV * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    device->CopyDescriptorsSimple(1, rangeStart, buffer->descriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                }
            }
        }

        uint32 AllocateTexture()
        {
            uint32 textureIndex = 0;
            if (!freeTextures.empty())
            {
                textureIndex = freeTextures.back();
                freeTextures.pop_back();
            }
            else
            {
                textureIndex = (uint32)textures.size();
                textures.emplace_back(new D3D12Texture());
            }
            return textureIndex;
        }

        uint32 CreateD3D12Texture(const RenderBackendTextureDesc* desc, const void* data, const char* name)
        {
            uint32 index = AllocateTexture();
            D3D12Texture* texture = textures[index];

            D3D12_RESOURCE_DESC1 resourceDesc = {
                .Dimension = GetD3D12ResourceDimension(desc->type),
                .Alignment = 0,
                .Width = desc->width,
                .Height = desc->height,
                .DepthOrArraySize = (UINT16)((desc->type == RenderBackendTextureType::Texture3D) ? desc->depth : desc->arrayLayerCount),
                .MipLevels = (UINT16)desc->mipLevelCount,
                .Format = ConvertToDXGIFormat(desc->format),
                .SampleDesc = {
                    .Count = 1,
                    .Quality = 0,
                },
                .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
                .Flags = GetD3D12ResourceFlags(desc->flags)
            };

            D3D12MA::ALLOCATION_DESC allocationDesc =
            {
                .Flags = D3D12MA::ALLOCATION_FLAG_NONE,
                .HeapType = D3D12_HEAP_TYPE_DEFAULT,
                .ExtraHeapFlags = D3D12_HEAP_FLAG_NONE, // TODO
                .CustomPool = nullptr,
                .pPrivateData = nullptr,
            };

            D3D12_BARRIER_LAYOUT initialLayout = ConvertToD3D12BarrierLayout(desc->initialState);

            D3D12_CLEAR_VALUE optimizedClearValue = {};
            optimizedClearValue.Color[0] = desc->clearValue.colorValue.float32[0];
            optimizedClearValue.Color[1] = desc->clearValue.colorValue.float32[1];
            optimizedClearValue.Color[2] = desc->clearValue.colorValue.float32[2];
            optimizedClearValue.Color[3] = desc->clearValue.colorValue.float32[3];
            optimizedClearValue.DepthStencil.Depth = desc->clearValue.depthStencilValue.depth;
            optimizedClearValue.DepthStencil.Stencil = desc->clearValue.depthStencilValue.stencil; // TODO
            optimizedClearValue.Format = resourceDesc.Format;
            bool useClearValue = EnumClassHasFlags(desc->flags, RenderBackendTextureCreateFlags::RenderTarget) || EnumClassHasFlags(desc->flags, RenderBackendTextureCreateFlags::DepthStencil);

            D3D12_CHECK(allocator->CreateResource3(
                &allocationDesc,
                &resourceDesc,
                initialLayout,
                useClearValue ? &optimizedClearValue : nullptr,
                0,
                nullptr,
                &texture->allocation,
                IID_PPV_ARGS(&texture->resource)));

            D3D12_CHECK(texture->resource->SetName(UTF8ToUTF16(name).c_str()));

            texture->width = desc->width;
            texture->height = desc->height;
            texture->depth = desc->depth;
            texture->mipLevels = desc->mipLevelCount;
            texture->arraySize = desc->arrayLayerCount;
            texture->format = resourceDesc.Format;
            texture->initialState = desc->initialState;
            texture->isSwapChainBuffer = false;
            texture->clearValue = optimizedClearValue;

            //
            texture->t = desc->type;

            // Temp
            texture->debugName = name;

            uint32 numSubresources = desc->arrayLayerCount * std::max(1u, desc->mipLevelCount);
            texture->totalSize = 0;
            texture->footprints.resize(numSubresources);
            texture->rowSizesInBytes.resize(numSubresources);
            texture->numRows.resize(numSubresources);
            device10->GetCopyableFootprints1(
                &resourceDesc,
                0,
                numSubresources,
                0,
                texture->footprints.data(),
                texture->numRows.data(),
                texture->rowSizesInBytes.data(),
                &texture->totalSize);

            if (data != nullptr)
            {
#if 0
                std::vector<D3D12_SUBRESOURCE_DATA> subresourceData(texture->footprints.size());
                for (uint32 i = 0; i < numSubresources; i++)
                {
                    subresourceData[i] = {};
                    subresourceData[i].pData = data;
                    subresourceData[i].RowPitch = texture->width * RenderBackendGetTextureFormatDesc(desc->format).bytes;
                    if (desc->type == RenderBackendTextureType::Texture3D)
                    {
                        //subresourceData[i].SlicePitch = texture->height * subresourceData[i].RowPitch;
                    }
                }

                CopyWorkload copyWorkload = AllocateCopyWorkload(texture->totalSize);
                copyWorkload.commandList->Reset(copyWorkload.commandAllocator.Get(), nullptr);

                void* mappedData = copyWorkload.uploadBuffer->mappedData;
                for (uint32 subresourceIndex = 0; subresourceIndex < numSubresources; subresourceIndex++)
                {
                    D3D12_MEMCPY_DEST memcpyDest = {
                        .pData = (void*)((UINT64)mappedData + texture->footprints[subresourceIndex].Offset),
                        .RowPitch = (SIZE_T)texture->footprints[subresourceIndex].Footprint.RowPitch,
                        .SlicePitch = (SIZE_T)texture->footprints[subresourceIndex].Footprint.RowPitch * (SIZE_T)texture->numRows[subresourceIndex],
                    };
                    MemcpySubresource(&memcpyDest, &subresourceData[subresourceIndex], (SIZE_T)texture->rowSizesInBytes[subresourceIndex], texture->numRows[subresourceIndex], texture->footprints[subresourceIndex].Footprint.Depth);

                    CD3DX12_TEXTURE_COPY_LOCATION dstTextureCopyLocation(texture->GetID3D12Resource(), subresourceIndex);
                    CD3DX12_TEXTURE_COPY_LOCATION srcTextureCopyLocation(copyWorkload.uploadBuffer->GetID3D12Resource(), texture->footprints[subresourceIndex]);
                    copyWorkload.commandList->CopyTextureRegion(
                        &dstTextureCopyLocation,
                        0,
                        0,
                        0,
                        &srcTextureCopyLocation,
                        nullptr);
                }
                SubmitCopyWorkload(copyWorkload);
#else
                std::vector<D3D12_SUBRESOURCE_DATA> subresourceData(1);
                for (uint32 i = 0; i < 1; i++)
                {
                    subresourceData[i] = {};
                    subresourceData[i].pData = data;
                    subresourceData[i].RowPitch = texture->width * RenderBackendGetTextureFormatDesc(desc->format).bytes;
                    if (desc->type == RenderBackendTextureType::Texture3D)
                    {
                        //subresourceData[i].SlicePitch = texture->height * subresourceData[i].RowPitch;
                    }
                }

                CopyWorkload copyWorkload = AllocateCopyWorkload(std::max(texture->totalSize, UINT64(4)));
                copyWorkload.commandList->Reset(copyWorkload.commandAllocator.Get(), nullptr);

                {
                    D3D12_TEXTURE_BARRIER TexBarriers[] =
                    {
                        CD3DX12_TEXTURE_BARRIER(
                            D3D12_BARRIER_SYNC_ALL,
                            D3D12_BARRIER_SYNC_ALL,
                            D3D12_BARRIER_ACCESS_NO_ACCESS,
                            D3D12_BARRIER_ACCESS_COPY_DEST,
                            D3D12_BARRIER_LAYOUT_UNDEFINED,
                            D3D12_BARRIER_LAYOUT_COPY_DEST,
                            texture->GetID3D12Resource(),
                            CD3DX12_BARRIER_SUBRESOURCE_RANGE(0xffffffff),
                            D3D12_TEXTURE_BARRIER_FLAG_DISCARD)
                    };

                    D3D12_BARRIER_GROUP TexBarrierGroups[] =
                    {
                        CD3DX12_BARRIER_GROUP(1, TexBarriers)
                    };

                    // @todo Needs a buffer barrier here?

                    copyWorkload.commandList7->Barrier(1, TexBarrierGroups);
                    //copyWorkload.commandList7->Barrier(1, BufBarrierGroups);
                }

                void* mappedData = copyWorkload.uploadBuffer->mappedData;
                for (uint32 subresourceIndex = 0; subresourceIndex < 1; subresourceIndex++)
                {
                    D3D12_MEMCPY_DEST memcpyDest = {
                        .pData = (void*)((UINT64)mappedData + texture->footprints[subresourceIndex].Offset),
                        .RowPitch = (SIZE_T)texture->footprints[subresourceIndex].Footprint.RowPitch,
                        .SlicePitch = (SIZE_T)texture->footprints[subresourceIndex].Footprint.RowPitch * (SIZE_T)texture->numRows[subresourceIndex],
                    };
                    MemcpySubresource(&memcpyDest, &subresourceData[subresourceIndex], (SIZE_T)texture->rowSizesInBytes[subresourceIndex], texture->numRows[subresourceIndex], texture->footprints[subresourceIndex].Footprint.Depth);

                    CD3DX12_TEXTURE_COPY_LOCATION dstTextureCopyLocation(texture->GetID3D12Resource(), subresourceIndex);
                    CD3DX12_TEXTURE_COPY_LOCATION srcTextureCopyLocation(copyWorkload.uploadBuffer->GetID3D12Resource(), texture->footprints[subresourceIndex]);

                    copyWorkload.commandList->CopyTextureRegion(
                        &dstTextureCopyLocation,
                        0,
                        0,
                        0,
                        &srcTextureCopyLocation,
                        nullptr);
                }

                {
                    D3D12_TEXTURE_BARRIER TexBarriers[] =
                    {
                        CD3DX12_TEXTURE_BARRIER(
                            D3D12_BARRIER_SYNC_ALL,
                            D3D12_BARRIER_SYNC_ALL,
                            D3D12_BARRIER_ACCESS_COPY_DEST,
                            D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
                            D3D12_BARRIER_LAYOUT_COPY_DEST,
                            D3D12_BARRIER_LAYOUT_SHADER_RESOURCE,
                            texture->GetID3D12Resource(),
                            CD3DX12_BARRIER_SUBRESOURCE_RANGE(0xffffffff),
                            D3D12_TEXTURE_BARRIER_FLAG_NONE)
                    };

                    D3D12_BARRIER_GROUP TexBarrierGroups[] =
                    {
                        CD3DX12_BARRIER_GROUP(1, TexBarriers)
                    };

                    copyWorkload.commandList7->Barrier(1, TexBarrierGroups);
                }

                SubmitCopyWorkload(copyWorkload);
#endif
            }

            if (EnumClassHasFlags(desc->flags, RenderBackendTextureCreateFlags::ShaderResource))
            {
                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = texture->format;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

                switch (srvDesc.Format)
                {
                case DXGI_FORMAT_D16_UNORM:
                    srvDesc.Format = DXGI_FORMAT_R16_UNORM;
                    break;
                case DXGI_FORMAT_D32_FLOAT:
                    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
                    break;
                case DXGI_FORMAT_D24_UNORM_S8_UINT:
                    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                    break;
                case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
                    srvDesc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
                    break;
                }

                switch (desc->type)
                {
                case RenderBackendTextureType::Texture1D:
                {
                    if (texture->arraySize == 1)
                    {
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                        srvDesc.Texture1D.MostDetailedMip = 0;
                        srvDesc.Texture1D.MipLevels = texture->mipLevels;
                        srvDesc.Texture1D.ResourceMinLODClamp = 0.0f;
                    }
                    else
                    {
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                        srvDesc.Texture1DArray.FirstArraySlice = 0;
                        srvDesc.Texture1DArray.ArraySize = texture->arraySize;
                        srvDesc.Texture1DArray.MostDetailedMip = 0;
                        srvDesc.Texture1DArray.MipLevels = texture->mipLevels;
                        srvDesc.Texture1DArray.ResourceMinLODClamp = 0.0f;
                    }
                } break;
                case RenderBackendTextureType::Texture2D:
                {
                    uint32 planeSlice = 0;
                    if (texture->arraySize == 1)
                    {
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                        srvDesc.Texture2D.MostDetailedMip = 0;
                        srvDesc.Texture2D.MipLevels = texture->mipLevels;
                        srvDesc.Texture2D.PlaneSlice = planeSlice;
                        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
                    }
                    else
                    {
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                        srvDesc.Texture2DArray.FirstArraySlice = 0;
                        srvDesc.Texture2DArray.ArraySize = texture->arraySize;
                        srvDesc.Texture2DArray.MostDetailedMip = 0;
                        srvDesc.Texture2DArray.MipLevels = texture->mipLevels;
                        srvDesc.Texture2DArray.PlaneSlice = planeSlice;
                        srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
                    }
                } break;
                case RenderBackendTextureType::Texture3D:
                {
                    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                    srvDesc.Texture3D.MostDetailedMip = 0;
                    srvDesc.Texture3D.MipLevels = texture->mipLevels;
                    srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
                } break;
                case RenderBackendTextureType::TextureCube:
                {
                    if (texture->arraySize == 6)
                    {
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                        srvDesc.TextureCube.MostDetailedMip = 0;
                        srvDesc.TextureCube.MipLevels = texture->mipLevels;
                        srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
                    }
                    else
                    {
                        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                        srvDesc.TextureCubeArray.MostDetailedMip = 0;
                        srvDesc.TextureCubeArray.MipLevels = texture->mipLevels;
                        srvDesc.TextureCubeArray.First2DArrayFace = 0;
                        srvDesc.TextureCubeArray.NumCubes = texture->arraySize / 6;
                        srvDesc.TextureCubeArray.ResourceMinLODClamp = 0.0f;
                    }
                } break;
                }

                {
                    texture->shaderResourceView = new D3D12ShaderResourceView();
                    texture->shaderResourceView->descriptor = resourceDescriptorAllocator.Allocate();
                    device->CreateShaderResourceView(texture->GetID3D12Resource(), &srvDesc, texture->shaderResourceView->descriptor);

                    texture->shaderResourceView->bindlessIndex = AllocateResourceDescriptorIndex();
                    if (texture->shaderResourceView->bindlessIndex >= 0)
                    {
                        assert(texture->shaderResourceView->bindlessIndex < D3D12_RENDER_BACKEND_BINDLESS_MAX_NUM_RESOURCE_DESCRIPTOERS);
                        D3D12_CPU_DESCRIPTOR_HANDLE rangeStart = resourceDescriptorHeap->cpuDescriptorHandle;
                        rangeStart.ptr += texture->shaderResourceView->bindlessIndex * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                        device->CopyDescriptorsSimple(1, rangeStart, texture->shaderResourceView->descriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    }
                }

                texture->shaderResourceViews.resize(texture->mipLevels);
                for (uint32 mipLevel = 0; mipLevel < texture->mipLevels; mipLevel++)
                {
                    switch (desc->type)
                    {
                    case RenderBackendTextureType::Texture1D:
                    {
                        if (texture->arraySize == 1)
                        {
                            srvDesc.Texture1D.MostDetailedMip = mipLevel;
                            srvDesc.Texture1D.MipLevels = 1;
                        }
                        else
                        {
                            srvDesc.Texture1DArray.MostDetailedMip = mipLevel;
                            srvDesc.Texture1DArray.MipLevels = 1;
                        }
                    } break;
                    case RenderBackendTextureType::Texture2D:
                    {
                        uint32 planeSlice = 0;
                        if (texture->arraySize == 1)
                        {
                            srvDesc.Texture2D.MostDetailedMip = mipLevel;
                            srvDesc.Texture2D.MipLevels = 1;
                        }
                        else
                        {
                            srvDesc.Texture2DArray.MostDetailedMip = mipLevel;
                            srvDesc.Texture2DArray.MipLevels = 1;
                        }
                    } break;
                    case RenderBackendTextureType::Texture3D:
                    {
                        srvDesc.Texture3D.MostDetailedMip = mipLevel;
                        srvDesc.Texture3D.MipLevels = 1;
                    } break;
                    case RenderBackendTextureType::TextureCube:
                    {
                        if (texture->arraySize == 6)
                        {
                            srvDesc.TextureCube.MostDetailedMip = mipLevel;
                            srvDesc.TextureCube.MipLevels = 1;
                        }
                        else
                        {
                            srvDesc.TextureCubeArray.MostDetailedMip = mipLevel;
                            srvDesc.TextureCubeArray.MipLevels = 1;
                        }
                    } break;
                    }

                    texture->shaderResourceViews[mipLevel] = new D3D12ShaderResourceView();
                    texture->shaderResourceViews[mipLevel]->descriptor = resourceDescriptorAllocator.Allocate();
                    device->CreateShaderResourceView(texture->GetID3D12Resource(), &srvDesc, texture->shaderResourceViews[mipLevel]->descriptor);

                    texture->shaderResourceViews[mipLevel]->bindlessIndex = AllocateResourceDescriptorIndex();
                    if (texture->shaderResourceViews[mipLevel]->bindlessIndex >= 0)
                    {
                        assert(texture->shaderResourceViews[mipLevel]->bindlessIndex < D3D12_RENDER_BACKEND_BINDLESS_MAX_NUM_RESOURCE_DESCRIPTOERS);
                        D3D12_CPU_DESCRIPTOR_HANDLE rangeStart = resourceDescriptorHeap->cpuDescriptorHandle;
                        rangeStart.ptr += texture->shaderResourceViews[mipLevel]->bindlessIndex * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                        device->CopyDescriptorsSimple(1, rangeStart, texture->shaderResourceViews[mipLevel]->descriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    }
                }
            }

            if (EnumClassHasFlags(desc->flags, RenderBackendTextureCreateFlags::RenderTarget))
            {
                D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
                rtvDesc.Format = texture->format;

                texture->renderTargetViews.resize(texture->mipLevels);
                for (uint32 mipSlice = 0; mipSlice < texture->mipLevels; mipSlice++)
                {
                    //for (uint32 arraySlice = 0; arraySlice < texture->arraySize; arraySlice++)
                    {
                        switch (desc->type)
                        {
                        case RenderBackendTextureType::Texture1D:
                        {
                            if (texture->arraySize == 1)
                            {
                                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
                                rtvDesc.Texture1D.MipSlice = mipSlice;
                            }
                            else
                            {
                                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
                                rtvDesc.Texture1DArray.MipSlice = mipSlice;
                                rtvDesc.Texture1DArray.FirstArraySlice = 0;
                                rtvDesc.Texture1DArray.ArraySize = texture->arraySize;
                            }
                        } break;
                        case RenderBackendTextureType::Texture2D:
                        {
                            if (texture->arraySize == 1)
                            {
                                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                                rtvDesc.Texture2D.MipSlice = mipSlice;
                                rtvDesc.Texture2D.PlaneSlice = 0;
                            }
                            else
                            {
                                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                                rtvDesc.Texture2DArray.MipSlice = mipSlice;
                                rtvDesc.Texture2DArray.FirstArraySlice = 0;
                                rtvDesc.Texture2DArray.ArraySize = texture->arraySize;
                                rtvDesc.Texture2DArray.PlaneSlice = 0;
                            }
                        } break;
                        case RenderBackendTextureType::Texture3D:
                        {
                            rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
                            rtvDesc.Texture3D.MipSlice = mipSlice;
                            rtvDesc.Texture3D.FirstWSlice = 0;
                            rtvDesc.Texture3D.WSize = -1;
                        } break;
                        }

                        //uint32 rtvIndex = mipSlice * texture->arraySize + arraySlice;
                        uint32 rtvIndex = mipSlice;
                        texture->renderTargetViews[rtvIndex] = new D3D12RenderTargetView();
                        texture->renderTargetViews[rtvIndex]->descriptor = rtvDescriptorAllocator.Allocate();
                        device->CreateRenderTargetView(texture->GetID3D12Resource(), &rtvDesc, texture->renderTargetViews[rtvIndex]->descriptor);
                    }
                }
            }

            if (EnumClassHasFlags(desc->flags, RenderBackendTextureCreateFlags::DepthStencil))
            {
                D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
                dsvDesc.Format = texture->format;
                dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

                switch (desc->type)
                {
                case RenderBackendTextureType::Texture1D:
                {
                    if (texture->arraySize == 1)
                    {
                        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
                        dsvDesc.Texture1D.MipSlice = 0;
                    }
                    else
                    {
                        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
                        dsvDesc.Texture1DArray.MipSlice = 0;
                        dsvDesc.Texture1DArray.FirstArraySlice = 0;
                        dsvDesc.Texture1DArray.ArraySize = texture->arraySize;
                    }
                } break;
                case RenderBackendTextureType::Texture2D:
                {
                    if (texture->arraySize == 1)
                    {
                        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                        dsvDesc.Texture2D.MipSlice = 0;
                    }
                    else
                    {
                        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                        dsvDesc.Texture2DArray.MipSlice = 0;
                        dsvDesc.Texture2DArray.FirstArraySlice = 0;
                        dsvDesc.Texture2DArray.ArraySize = texture->arraySize;
                    }
                } break;
                }

                texture->depthStencilViews[0] = new D3D12DepthStencilView();
                texture->depthStencilViews[0]->descriptor = dsvDescriptorAllocator.Allocate();
                dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
                device->CreateDepthStencilView(texture->GetID3D12Resource(), &dsvDesc, texture->depthStencilViews[0]->descriptor);

                texture->depthStencilViews[1] = new D3D12DepthStencilView();
                texture->depthStencilViews[1]->descriptor = dsvDescriptorAllocator.Allocate();
                dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
                device->CreateDepthStencilView(texture->GetID3D12Resource(), &dsvDesc, texture->depthStencilViews[1]->descriptor);

                const bool hasStencil = false;
                if (hasStencil)
                {
                    texture->depthStencilViews[2] = new D3D12DepthStencilView();
                    texture->depthStencilViews[2]->descriptor = dsvDescriptorAllocator.Allocate();
                    dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
                    device->CreateDepthStencilView(texture->GetID3D12Resource(), &dsvDesc, texture->depthStencilViews[2]->descriptor);

                    texture->depthStencilViews[3] = new D3D12DepthStencilView();
                    texture->depthStencilViews[3]->descriptor = dsvDescriptorAllocator.Allocate();
                    dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
                    device->CreateDepthStencilView(texture->GetID3D12Resource(), &dsvDesc, texture->depthStencilViews[3]->descriptor);
                }
            }

            if (EnumClassHasFlags(desc->flags, RenderBackendTextureCreateFlags::UnorderedAccess))
            {
                D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
                uavDesc.Format = texture->format;

                texture->unorderedAccessViews.resize(texture->mipLevels);
                for (uint32 mipSlice = 0; mipSlice < texture->mipLevels; mipSlice++)
                {
                    switch (desc->type)
                    {
                    case RenderBackendTextureType::Texture1D:
                    {
                        if (texture->arraySize == 1)
                        {
                            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
                            uavDesc.Texture1D.MipSlice = mipSlice;
                        }
                        else
                        {
                            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
                            uavDesc.Texture1DArray.MipSlice = mipSlice;
                            uavDesc.Texture1DArray.FirstArraySlice = 0;
                            uavDesc.Texture1DArray.ArraySize = texture->arraySize;
                        }
                    } break;
                    case RenderBackendTextureType::Texture2D:
                    case RenderBackendTextureType::TextureCube:
                    {
                        if (texture->arraySize == 1)
                        {
                            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
                            uavDesc.Texture2D.MipSlice = mipSlice;
                            uavDesc.Texture2D.PlaneSlice = 0;
                        }
                        else
                        {
                            uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
                            uavDesc.Texture2DArray.MipSlice = mipSlice;
                            uavDesc.Texture2DArray.FirstArraySlice = 0;
                            uavDesc.Texture2DArray.ArraySize = texture->arraySize;
                            uavDesc.Texture2DArray.PlaneSlice = 0;
                        }
                    } break;
                    case RenderBackendTextureType::Texture3D:
                    {
                        uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
                        uavDesc.Texture3D.MipSlice = mipSlice;
                        uavDesc.Texture3D.FirstWSlice = 0;
                        uavDesc.Texture3D.WSize = -1;
                    } break;
                    }

                    texture->unorderedAccessViews[mipSlice] = new D3D12UnorderedAccessView();
                    texture->unorderedAccessViews[mipSlice]->descriptor = resourceDescriptorAllocator.Allocate();
                    device->CreateUnorderedAccessView(texture->GetID3D12Resource(), nullptr, &uavDesc, texture->unorderedAccessViews[mipSlice]->descriptor);

                    texture->unorderedAccessViews[mipSlice]->bindlessIndex = AllocateResourceDescriptorIndex();
                    if (texture->unorderedAccessViews[mipSlice]->bindlessIndex >= 0)
                    {
                        assert(texture->unorderedAccessViews[mipSlice]->bindlessIndex < D3D12_RENDER_BACKEND_BINDLESS_MAX_NUM_RESOURCE_DESCRIPTOERS);
                        D3D12_CPU_DESCRIPTOR_HANDLE rangeStart = resourceDescriptorHeap->cpuDescriptorHandle;
                        rangeStart.ptr += texture->unorderedAccessViews[mipSlice]->bindlessIndex * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                        device->CopyDescriptorsSimple(1, rangeStart, texture->unorderedAccessViews[mipSlice]->descriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    }
                }
            }
            return index;
        }

        uint32 AllocateSampler()
        {
            uint32 samplerIndex = 0;
            if (!freeSamplers.empty())
            {
                samplerIndex = freeSamplers.back();
                freeSamplers.pop_back();
            }
            else
            {
                samplerIndex = (uint32)samplers.size();
                samplers.emplace_back(new D3D12Sampler());
            }
            return samplerIndex;
        }

        uint32 AllocateRayTracingAccelerationStructure()
        {
            uint32 index = 0;
            if (!freeAccelerationStructures.empty())
            {
                index = freeAccelerationStructures.back();
                freeAccelerationStructures.pop_back();
            }
            else
            {
                index = (uint32)accelerationStructures.size();
                accelerationStructures.emplace_back(new D3D12RayTracingAccelerationStructure());
            }
            return index;
        }

        uint32 AllocateShader()
        {
            uint32 shaderIndex = 0;
            if (!freeShaders.empty())
            {
                shaderIndex = freeShaders.back();
                freeShaders.pop_back();
            }
            else
            {
                shaderIndex = (uint32)shaders.size();
                shaders.emplace_back(new D3D12Shader());
            }
            return shaderIndex;
        }

        uint32 CreateD3D12Sampler(const RenderBackendSamplerDesc* desc, const char* name)
        {
            uint32 index = AllocateSampler();
            D3D12Sampler* sampler = samplers[index];

            D3D12_SAMPLER_DESC samplerDesc = {
                .Filter = ConvertToD3D12Filter(desc->filter),
                .AddressU = ConvertToD3D12TextureAddressMode(desc->addressModeU),
                .AddressV = ConvertToD3D12TextureAddressMode(desc->addressModeV),
                .AddressW = ConvertToD3D12TextureAddressMode(desc->addressModeW),
                .MipLODBias = desc->mipLodBias,
                .MaxAnisotropy = desc->maxAnisotropy,
                .ComparisonFunc = ConvertToD3D12ComparisonFunc(desc->compareOp),
                .BorderColor = { 0.0f, 0.0f, 0.0f, 0.0f }, // TODO
                .MinLOD = desc->minLod,
                .MaxLOD = desc->maxLod,
            };

            sampler->descriptor = samplerDescriptorAllocator.Allocate();
            device->CreateSampler(&samplerDesc, sampler->descriptor);

            sampler->bindlessIndex = AllocateSamplerDescriptorIndex();

            if (sampler->bindlessIndex >= 0)
            {
                assert(sampler->bindlessIndex < D3D12_RENDER_BACKEND_BINDLESS_MAX_NUM_SAMPLER_DESCRIPTOERS);
                D3D12_CPU_DESCRIPTOR_HANDLE rangeStart = samplerDescriptorHeap->cpuDescriptorHandle;
                rangeStart.ptr += sampler->bindlessIndex * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
                device->CopyDescriptorsSimple(1, rangeStart, sampler->descriptor, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
            }

            return index;
        }

        inline void AllocateUAVBuffer(ID3D12Device* pDevice, UINT64 bufferSize, ID3D12Resource **ppResource, D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_COMMON, const wchar_t* resourceName = nullptr)
        {
            auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
            auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_RAYTRACING_ACCELERATION_STRUCTURE);
            pDevice->CreateCommittedResource(
                &uploadHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                initialResourceState,
                nullptr,
                IID_PPV_ARGS(ppResource));
            if (resourceName)
            {
                (*ppResource)->SetName(resourceName);
            }
        }

        inline void AllocateUploadBuffer(ID3D12Device* pDevice, void *pData, UINT64 datasize, ID3D12Resource **ppResource, const wchar_t* resourceName = nullptr)
        {
            auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
            auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(datasize);
            pDevice->CreateCommittedResource(
                &uploadHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(ppResource));
            if (resourceName)
            {
                (*ppResource)->SetName(resourceName);
            }
            void *pMappedData;
            (*ppResource)->Map(0, nullptr, &pMappedData);
            memcpy(pMappedData, pData, datasize);
            (*ppResource)->Unmap(0, nullptr);
        }

        uint32 CreateD3D12RayTracingBottomLevelAccelerationStructure(const RenderBackendRayTracingBottomLevelAccelerationStructureDesc* desc, const char* name)
        {
            uint32 index = AllocateRayTracingAccelerationStructure();
            D3D12RayTracingAccelerationStructure* accelerationStructure = accelerationStructures[index];
            {
                accelerationStructure->buildFlags = ConvertToD3D12RayTracingAccelerationStructureBuildFlags(desc->buildFlags);

                accelerationStructure->geometries.resize(desc->geometryCount);
                for (uint32 i = 0; i < desc->geometryCount; i++)
                {
                    const RenderBackendRayTracingGeometryDesc& geometryDesc = desc->geometryDescs[i];
                    D3D12_RAYTRACING_GEOMETRY_DESC& geometry = accelerationStructure->geometries[i];
                    geometry.Type = ConvertToD3D12RayTracingGeometryType(geometryDesc.type);
                    geometry.Flags = ConvertToD3D12RayTracingGeometryFlags(geometryDesc.flags);
                    if (geometry.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES)
                    {
                        geometry.Triangles.Transform3x4 = GetBuffer(geometryDesc.triangleDesc.transformBuffer)->GetID3D12Resource()->GetGPUVirtualAddress() + geometryDesc.triangleDesc.transformOffset;
                        geometry.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
                        geometry.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
                        geometry.Triangles.IndexCount = geometryDesc.triangleDesc.indexCount;
                        geometry.Triangles.VertexCount = geometryDesc.triangleDesc.vertexCount;
                        geometry.Triangles.IndexBuffer = GetBuffer(geometryDesc.triangleDesc.indexBuffer)->GetID3D12Resource()->GetGPUVirtualAddress() + geometryDesc.triangleDesc.indexOffset;
                        geometry.Triangles.VertexBuffer.StartAddress = GetBuffer(geometryDesc.triangleDesc.vertexBuffer)->GetID3D12Resource()->GetGPUVirtualAddress() + geometryDesc.triangleDesc.vertexOffset;
                        geometry.Triangles.VertexBuffer.StrideInBytes = geometryDesc.triangleDesc.vertexStride;
                    }
                    else if (geometry.Type == D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS)
                    {
                        //geometry.AABBs.AABBCount = ;
                        //geometry.AABBs.AABBs.StartAddress = ;
                        //geometry.AABBs.AABBs.StrideInBytes = ;
                    }
                }

                D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

                D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS bottomLevelAccelerationStructureInputs = {};
                bottomLevelAccelerationStructureInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
                bottomLevelAccelerationStructureInputs.Flags = buildFlags;
                bottomLevelAccelerationStructureInputs.NumDescs = desc->geometryCount;
                bottomLevelAccelerationStructureInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
                bottomLevelAccelerationStructureInputs.pGeometryDescs = accelerationStructure->geometries.data();

                D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelAccelerationStructurePrebuildInfo = {};
                GetDXRDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelAccelerationStructureInputs, &bottomLevelAccelerationStructurePrebuildInfo);

                AllocateUAVBuffer(device.Get(), bottomLevelAccelerationStructurePrebuildInfo.ResultDataMaxSizeInBytes, &accelerationStructure->accelerationStructureBuffer, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"BottomLevelAccelerationStructure");
                AllocateUAVBuffer(device.Get(), bottomLevelAccelerationStructurePrebuildInfo.ScratchDataSizeInBytes, &accelerationStructure->scratchBuffer, D3D12_RESOURCE_STATE_COMMON, L"ScratchBuffer");
            }
            return index;
        }

        uint32 CreateD3D12RayTracingTopLevelAccelerationStructure(const RenderBackendRayTracingTopLevelAccelerationStructureDesc* desc, const char* name)
        {
            uint32 index = AllocateRayTracingAccelerationStructure();
            D3D12RayTracingAccelerationStructure* accelerationStructure = accelerationStructures[index];
            {
                accelerationStructure->buildFlags = ConvertToD3D12RayTracingAccelerationStructureBuildFlags(desc->buildFlags);

                std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instances;
                for (uint32 i = 0; i < desc->instanceCount; i++)
                {
                    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
                    instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1; // TODO
                    instanceDesc.InstanceID = desc->instances[i].instanceID;
                    instanceDesc.InstanceMask = desc->instances[i].instanceMask;
                    instanceDesc.InstanceContributionToHitGroupIndex = desc->instances[i].instanceContributionToHitGroupIndex;
                    instanceDesc.Flags = desc->instances[i].instanceContributionToHitGroupIndex;
                    instanceDesc.AccelerationStructure = GetRayTracingAccelerationStructure(desc->instances[i].blas)->accelerationStructureBuffer->GetGPUVirtualAddress();
                    instances.emplace_back(instanceDesc);
                }
                AllocateUploadBuffer(device.Get(), instances.data(), instances.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC), &accelerationStructure->instanceBuffer, L"InstanceDescs");
                accelerationStructure->numInstances = desc->instanceCount;

                D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

                D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelAccelerationStructureInputs = {};
                topLevelAccelerationStructureInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
                topLevelAccelerationStructureInputs.Flags = buildFlags;
                topLevelAccelerationStructureInputs.NumDescs = desc->instanceCount;
                topLevelAccelerationStructureInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

                D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelAccelerationStructurePrebuildInfo = {};
                GetDXRDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelAccelerationStructureInputs, &topLevelAccelerationStructurePrebuildInfo);

                AllocateUAVBuffer(device.Get(), topLevelAccelerationStructurePrebuildInfo.ResultDataMaxSizeInBytes, &accelerationStructure->accelerationStructureBuffer, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, L"TopLevelAccelerationStructure");
                AllocateUAVBuffer(device.Get(), topLevelAccelerationStructurePrebuildInfo.ScratchDataSizeInBytes, &accelerationStructure->scratchBuffer, D3D12_RESOURCE_STATE_COMMON, L"ScratchBuffer");

                accelerationStructure->descriptor = resourceDescriptorAllocator.Allocate();

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = DXGI_FORMAT_UNKNOWN;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.RaytracingAccelerationStructure.Location = accelerationStructure->accelerationStructureBuffer->GetGPUVirtualAddress();
                device->CreateShaderResourceView(nullptr, &srvDesc, accelerationStructure->descriptor);

                accelerationStructure->bindlessIndex = AllocateResourceDescriptorIndex();
                if (accelerationStructure->bindlessIndex >= 0)
                {
                    assert(accelerationStructure->bindlessIndex < D3D12_RENDER_BACKEND_BINDLESS_MAX_NUM_RESOURCE_DESCRIPTOERS);
                    D3D12_CPU_DESCRIPTOR_HANDLE rangeStart = resourceDescriptorHeap->cpuDescriptorHandle;
                    rangeStart.ptr += accelerationStructure->bindlessIndex * device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    device->CopyDescriptorsSimple(1, rangeStart, accelerationStructure->descriptor, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                }
            }
            return index;
        }

        void InitD3D12BlendDesc(const RenderBackendColorBlendStateDescription& blendState, uint32 numRenderTargets, D3D12_BLEND_DESC& dstBlendState)
        {
            dstBlendState = {};
            dstBlendState.AlphaToCoverageEnable = FALSE;
            dstBlendState.IndependentBlendEnable = TRUE;
            for (uint32 i = 0; i < numRenderTargets; i++)
            {
                dstBlendState.RenderTarget[i].BlendEnable = blendState.targetBlends->blendEnable;
                dstBlendState.RenderTarget[i].LogicOpEnable = FALSE;
                dstBlendState.RenderTarget[i].SrcBlend = ConvertToD3D12Blend(blendState.targetBlends->srcColorBlendFactor);
                dstBlendState.RenderTarget[i].DestBlend = ConvertToD3D12Blend(blendState.targetBlends->dstColorBlendFactor);
                dstBlendState.RenderTarget[i].BlendOp = ConvertToD3D12BlendOp(blendState.targetBlends->colorBlendOp);
                dstBlendState.RenderTarget[i].SrcBlendAlpha = ConvertToD3D12Blend(blendState.targetBlends->srcAlphaBlendFactor);
                dstBlendState.RenderTarget[i].DestBlendAlpha = ConvertToD3D12Blend(blendState.targetBlends->dstAlphaBlendFactor);
                dstBlendState.RenderTarget[i].BlendOpAlpha = ConvertToD3D12BlendOp(blendState.targetBlends->alphaBlendOp);
                dstBlendState.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_CLEAR; // TODO
                dstBlendState.RenderTarget[i].RenderTargetWriteMask = ConvertToD3D12RenderTargetWriteMask(blendState.targetBlends->writeMask);
            }
        }

        void InitD3D12RasterizerDesc(const RenderBackendRasterizationStateDescription& rasterizationState, D3D12_RASTERIZER_DESC& dstRasterizationState)
        {
            dstRasterizationState = {};
            dstRasterizationState.FillMode = ConvertToD3D12FillMode(rasterizationState.fillMode);
            dstRasterizationState.CullMode = ConvertToD3D12CullMode(rasterizationState.cullMode);
            dstRasterizationState.FrontCounterClockwise = rasterizationState.frontFaceCounterClockwise;
            dstRasterizationState.DepthBias = (int32)rasterizationState.depthBiasConstantFactor;
            dstRasterizationState.DepthBiasClamp = rasterizationState.depthBiasClamp;
            dstRasterizationState.SlopeScaledDepthBias = rasterizationState.depthBiasSlopeFactor;
            dstRasterizationState.DepthClipEnable = !rasterizationState.depthClampEnable;
            dstRasterizationState.MultisampleEnable = FALSE;
            dstRasterizationState.AntialiasedLineEnable = FALSE;
            dstRasterizationState.ForcedSampleCount = 0;
            dstRasterizationState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
        }

        void InitD3D12DepthStencilDesc(const RenderBackendDepthStencilStateDescription& depthStencilState, D3D12_DEPTH_STENCIL_DESC& dstDepthStencilState)
        {
            dstDepthStencilState = {};
            dstDepthStencilState.DepthEnable = depthStencilState.depthTestEnable;
            dstDepthStencilState.DepthWriteMask = depthStencilState.depthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
            dstDepthStencilState.DepthFunc = ConvertToD3D12ComparisonFunc(depthStencilState.depthCompareFunction);
            dstDepthStencilState.StencilEnable = depthStencilState.stencilTestEnable;
            dstDepthStencilState.StencilReadMask = (UINT8)depthStencilState.stencilReadMask;
            dstDepthStencilState.StencilWriteMask = (UINT8)depthStencilState.stencilWriteMask;
            dstDepthStencilState.FrontFace.StencilFailOp = ConvertToD3D12StencilOp(depthStencilState.frontFaceStencilFailOp);
            dstDepthStencilState.FrontFace.StencilDepthFailOp = ConvertToD3D12StencilOp(depthStencilState.frontFaceStencilDepthFailOp);
            dstDepthStencilState.FrontFace.StencilPassOp = ConvertToD3D12StencilOp(depthStencilState.frontFaceStencilPassOp);
            dstDepthStencilState.FrontFace.StencilFunc = ConvertToD3D12ComparisonFunc(depthStencilState.frontFaceStencilCompareFunction);
            dstDepthStencilState.BackFace.StencilFailOp = ConvertToD3D12StencilOp(depthStencilState.backFaceStencilFailOp);
            dstDepthStencilState.BackFace.StencilDepthFailOp = ConvertToD3D12StencilOp(depthStencilState.backFaceStencilDepthFailOp);
            dstDepthStencilState.BackFace.StencilPassOp = ConvertToD3D12StencilOp(depthStencilState.backFaceStencilPassOp);
            dstDepthStencilState.BackFace.StencilFunc = ConvertToD3D12ComparisonFunc(depthStencilState.backFaceStencilCompareFunction);
        }

        uint32 CreateD3D12Shader(const RenderBackendShaderDesc* desc, const char* name)
        {
            uint32 index = AllocateShader();

            D3D12Shader* shader = shaders[index];

            shader->bytecode.pShaderBytecode = desc->code;
            shader->bytecode.BytecodeLength = desc->codeSize;
            shader->entryFunctionName = D3D12Utils::Widen(desc->entryFunctionName);
            shader->hash = CRC32(desc->code, desc->codeSize);

            return index;
        }

        void Tick()
        {
            for (D3D12SubmissionWorkload*& workload : workloads)
            {
                if (workload->commandQueue->fence->GetCompletedValue() >= workload->completionFenceValue)
                {
                    for (D3D12CommandAllocator* commandAllocator : workload->commandAllocatorsToRelease)
                    {
                        ReleaseCommandAllocator(commandAllocator);
                    }
                    delete workload;
                    workload = nullptr;
                }
            }
            workloads.erase(std::remove(workloads.begin(), workloads.end(), nullptr), workloads.end());
        }

        void WaitIdle() const
        {
#if 1 // TODO: why Nsight crash here?
            Microsoft::WRL::ComPtr<ID3D12Fence> fence;
            D3D12_CHECK(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
            D3D12_CHECK(fence->Signal(0));
            for (const auto& commandQueue : commandQueues)
            {
                if (commandQueue)
                {
                    D3D12_CHECK(commandQueue->GetID3D12CommandQueue()->Signal(fence.Get(), 1));
                    if (fence->GetCompletedValue() < 1)
                    {
                        D3D12_CHECK(fence->SetEventOnCompletion(1, NULL));
                    }
                    D3D12_CHECK(fence->Signal(0));
                }
            }
            fence->Release();
#endif
        }

        bool Init(D3D12RenderBackend* backend, D3D12Adapter* adapter);
        void Exit();

        inline uint32 GetRenderBackendHandleRepresentation(uint32 handle)
        {
            if (handleRepresentations.find(handle) == handleRepresentations.end())
            {
                assert(false);
            }
            return handleRepresentations[handle];
        }
        inline bool TryGetRenderBackendHandleRepresentation(uint32 handle, uint32* outValue)
        {
            if (handleRepresentations.find(handle) == handleRepresentations.end())
            {
                return false;
            }
            *outValue = handleRepresentations[handle];
            return true;
        }
        inline void SetRenderBackendHandleRepresentation(uint32 handle, uint32 value)
        {
            handleRepresentations[handle] = value;
        }
        inline bool RemoveRenderBackendHandleRepresentation(uint32 handle)
        {
            return handleRepresentations.erase(handle);
        }

        D3D12CommandQueue* GetCommandQueue(D3D12CommandQueueType type)
        {
            return commandQueues[(uint32)type];
        }

        Microsoft::WRL::ComPtr<ID3D12Device> device;
        Microsoft::WRL::ComPtr<ID3D12Device5> device5;
        Microsoft::WRL::ComPtr<ID3D12Device10> device10;

        D3D12CommandQueue* commandQueues[(uint32)D3D12CommandQueueType::Count];

        std::map<uint32, uint32> handleRepresentations;

        std::vector<D3D12SwapChain*> swapChains;

        std::vector<D3D12Buffer*> buffers;
        std::vector<uint32> freeBuffers;

        std::vector<D3D12Texture*> textures;
        std::vector<uint32> freeTextures;

        std::vector<D3D12Sampler*> samplers;
        std::vector<uint32> freeSamplers;

        std::vector<D3D12RayTracingAccelerationStructure*> accelerationStructures;
        std::vector<uint32> freeAccelerationStructures;

        std::vector<D3D12Shader*> shaders;
        std::vector<uint32> freeShaders;

        std::unordered_map<uint64, D3D12ComputePipelineState*> computePipelineStateMap;
        std::unordered_map<uint64, D3D12GraphicsPipelineState*> graphicsPipelineStateMap;
        std::vector<D3D12RayTracingPipelineStateObject*> rayTracingPipelineStateObjects;

        D3D12ComputePipelineState* FindOrCreateComputePipelineState(D3D12Shader* shader)
        {
            uint32 shaderHash = shader->hash;
            uint64 pipelineStateHash = shaderHash;

            if (computePipelineStateMap.find(pipelineStateHash) != computePipelineStateMap.end())
            {
                return computePipelineStateMap[pipelineStateHash];
            }

            D3D12ComputePipelineState* newComputePipelineState = new D3D12ComputePipelineState();

            D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc =
            {
                .pRootSignature = rootSignature.Get(),
                .CS = shader->bytecode,
                .NodeMask = mask.Get(),
                // .CachedPSO = ,
                .Flags = D3D12_PIPELINE_STATE_FLAG_NONE
            };

            D3D12_CHECK(device->CreateComputePipelineState(&computePipelineStateDesc, IID_PPV_ARGS(&newComputePipelineState->state)));

            computePipelineStateMap.emplace(pipelineStateHash, newComputePipelineState);
            return computePipelineStateMap[pipelineStateHash];
        }

        D3D12GraphicsPipelineState* FindOrCreateGraphicsPipelineState(
            D3D12Shader* vertexShader,
            D3D12Shader* pixelShader,
            D3D12Shader* amplificationShader,
            D3D12Shader* meshShader,
            const RenderBackendGraphicsPipelineStateDescription& pipelineState,
            D3D12RenderPass* renderPass,
            RenderBackendPrimitiveTopology topology)
        {
            bool useMeshShader = meshShader != nullptr;

            // TODO: move this after lookup PSO
            D3D12GraphicsPipelineStateDesc pipelineStateDesc = {};
            InitD3D12RasterizerDesc(pipelineState.rasterizationState, pipelineStateDesc.rasterizerState);
            InitD3D12DepthStencilDesc(pipelineState.depthStencilState, pipelineStateDesc.depthStencilState);
            InitD3D12BlendDesc(pipelineState.colorBlendState, renderPass->numRenderTargets, pipelineStateDesc.blendState);

            uint64 pipelineStateDescHash = CRC32(&pipelineStateDesc, sizeof(D3D12GraphicsPipelineStateDesc));
            uint32 renderPassHash = CRC32(renderPass, sizeof(D3D12RenderPass));

            uint32 vertexShaderHash = vertexShader ? vertexShader->hash : 0;
            uint32 pixelShaderHash = pixelShader ? pixelShader->hash : 0;
            uint32 amplificationShaderHash = amplificationShader ? amplificationShader->hash : 0;
            uint32 meshShaderHash = meshShader ? meshShader->hash : 0;

            uint64 values[] = { uint64(vertexShaderHash), uint64(pixelShaderHash), uint64(amplificationShaderHash), uint64(meshShaderHash), uint64(topology), pipelineStateDescHash };
            uint32 psoHash = CRC32(values, ArraySize(values) * sizeof(uint64));

            uint64 pipelineStateHash = (uint64(psoHash) << 32) | uint64(renderPassHash);

            if (graphicsPipelineStateMap.find(pipelineStateHash) != graphicsPipelineStateMap.end())
            {
                return graphicsPipelineStateMap[pipelineStateHash];
            }

            D3D12GraphicsPipelineState* newGraphicsPipelineState = new D3D12GraphicsPipelineState();

            D3D12_INPUT_LAYOUT_DESC nullInputLayout =
            {
                .pInputElementDescs = nullptr,
                .NumElements = 0
            };

            D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType = ConvertToD3D12PrimitiveTopologyType(topology);

            newGraphicsPipelineState->primitiveTopologyType = primitiveTopologyType;

            DXGI_SAMPLE_DESC dxgiSampleDesc =
            {
                .Count = 1,
                .Quality = 0
            };

            if (useMeshShader)
            {
                ID3D12Device2* device2 = nullptr;
                assert(SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&device2))));

                D3DX12_MESH_SHADER_PIPELINE_STATE_DESC meshShaderPipelineStateDesc =
                {
                    .pRootSignature = rootSignature.Get(),
                    .AS = amplificationShader ? amplificationShader->bytecode : D3D12_SHADER_BYTECODE(),
                    .MS = meshShader->bytecode,
                    .PS = pixelShader->bytecode, // TODO: allow no pixel shader
                    .BlendState = pipelineStateDesc.blendState,
                    .SampleMask = 0xFFFFFFFF,
                    .RasterizerState = pipelineStateDesc.rasterizerState,
                    .DepthStencilState = pipelineStateDesc.depthStencilState,
                    .PrimitiveTopologyType = primitiveTopologyType,
                    .NumRenderTargets = renderPass->numRenderTargets,
                    .RTVFormats = {},
                    .DSVFormat = renderPass->hasDepthStencil ? renderPass->depthStencilViewFormat : DXGI_FORMAT_UNKNOWN,
                    .SampleDesc = dxgiSampleDesc,
                    .NodeMask = mask.Get(),
                    // .CachedPSO = ,
                    .Flags = D3D12_PIPELINE_STATE_FLAG_NONE
                };

                for (uint32 i = 0; i < renderPass->numRenderTargets; i++)
                {
                    meshShaderPipelineStateDesc.RTVFormats[i] = renderPass->renderTargetFormats[i];
                }

                CD3DX12_PIPELINE_MESH_STATE_STREAM psoStream = CD3DX12_PIPELINE_MESH_STATE_STREAM(meshShaderPipelineStateDesc);

                D3D12_PIPELINE_STATE_STREAM_DESC streamDesc =
                {
                    streamDesc.SizeInBytes = sizeof(psoStream),
                    streamDesc.pPipelineStateSubobjectStream = &psoStream
                };

                D3D12_CHECK(device2->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&newGraphicsPipelineState->stateObject)));
            }
            else
            {
                D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc =
                {
                    .pRootSignature = rootSignature.Get(),
                    .VS = vertexShader->bytecode,
                    .PS = pixelShader->bytecode, // TODO: allow no pixel shader
                    // .DS = ,
                    // .HS = ,
                    // .GS = ,
                    // .StreamOutput = ,
                    .BlendState = pipelineStateDesc.blendState,
                    .SampleMask = 0xFFFFFFFF,
                    .RasterizerState = pipelineStateDesc.rasterizerState,
                    .DepthStencilState = pipelineStateDesc.depthStencilState,
                    .InputLayout = nullInputLayout,
                    .IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
                    .PrimitiveTopologyType = primitiveTopologyType,
                    .NumRenderTargets = renderPass->numRenderTargets,
                    .RTVFormats = {},
                    .DSVFormat = renderPass->hasDepthStencil ? renderPass->depthStencilViewFormat : DXGI_FORMAT_UNKNOWN,
                    .SampleDesc = dxgiSampleDesc,
                    .NodeMask = mask.Get(),
                    // .CachedPSO = ,
                    .Flags = D3D12_PIPELINE_STATE_FLAG_NONE
                };

                for (uint32 i = 0; i < renderPass->numRenderTargets; i++)
                {
                    graphicsPipelineStateDesc.RTVFormats[i] = renderPass->renderTargetFormats[i];
                }

                D3D12_CHECK(device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&newGraphicsPipelineState->stateObject)));
            }

            graphicsPipelineStateMap.emplace(pipelineStateHash, newGraphicsPipelineState);
            return graphicsPipelineStateMap[pipelineStateHash];
        }

        uint32 CreateD3D12RayTracingPipelineState(const RenderBackendRayTracingPipelineStateDesc* desc, const char* name)
        {
            D3D12RayTracingPipelineStateObject* rayTracingPipelineStateObject = new D3D12RayTracingPipelineStateObject();
            rayTracingPipelineStateObject->desc = *desc;

            D3D12_STATE_OBJECT_TYPE stateObjectType = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

            UINT maxTraceRecursionDepth = desc->maxRayRecursionDepth;
            uint32 numShaders = (uint32)desc->shaders.size();
            uint32 numShaderGroups = (uint32)desc->shaderGroupDescs.size();

            CD3DX12_STATE_OBJECT_DESC rayTracingPipelineStateObjectDesc(stateObjectType);

            // DXIL library
            for (uint32 shaderIndex = 0; shaderIndex < numShaders; shaderIndex++)
            {
                D3D12Shader* shader = GetShader(desc->shaders[shaderIndex]);
                CD3DX12_DXIL_LIBRARY_SUBOBJECT* dxilLibrary = rayTracingPipelineStateObjectDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
                dxilLibrary->SetDXILLibrary(&shader->bytecode);
                dxilLibrary->DefineExport(shader->entryFunctionName.c_str());
            }

            for (uint32 groupIndex = 0; groupIndex < numShaderGroups; groupIndex++)
            {
                if (desc->shaderGroupDescs[groupIndex].type == RenderBackendRayTracingShaderGroupType::Miss)
                {
                    rayTracingPipelineStateObject->numMissShaders++;
                }

                // Hit group
                if (desc->shaderGroupDescs[groupIndex].type == RenderBackendRayTracingShaderGroupType::TrianglesHitGroup)
                {
                    CD3DX12_HIT_GROUP_SUBOBJECT* hitGroup = rayTracingPipelineStateObjectDesc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
                    if (desc->shaderGroupDescs[groupIndex].anyHitShader != RenderBackendRayTracingShaderGroupDesc::ShaderUnused)
                    {
                        D3D12Shader* shader = GetShader(desc->shaders[desc->shaderGroupDescs[groupIndex].anyHitShader]);
                        hitGroup->SetAnyHitShaderImport(shader->entryFunctionName.c_str());
                    }
                    if (desc->shaderGroupDescs[groupIndex].closestHitShader != RenderBackendRayTracingShaderGroupDesc::ShaderUnused)
                    {
                        D3D12Shader* shader = GetShader(desc->shaders[desc->shaderGroupDescs[groupIndex].closestHitShader]);
                        hitGroup->SetClosestHitShaderImport(shader->entryFunctionName.c_str());
                    }
                    if (desc->shaderGroupDescs[groupIndex].intersectionShader != RenderBackendRayTracingShaderGroupDesc::ShaderUnused)
                    {
                        D3D12Shader* shader = GetShader(desc->shaders[desc->shaderGroupDescs[groupIndex].intersectionShader]);
                        hitGroup->SetIntersectionShaderImport(shader->entryFunctionName.c_str());
                    }
                    // todo!!!
                    hitGroup->SetHitGroupExport(L"Any");// (shader->entryFunctionName.c_str());
                    hitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);

                    rayTracingPipelineStateObject->numHitGroups++;
                }
            }

            // Subobject to exports association
            //CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT* subobjectToExportsAssociation = rayTracingPipelineStateObjectDesc.CreateSubobject<CD3DX12_SUBOBJECT_TO_EXPORTS_ASSOCIATION_SUBOBJECT>();
            // subobjectToExportsAssociation->SetSubobjectToAssociate(*localRootSignature);
            // subobjectToExportsAssociation->AddExport(c_raygenShaderName);

            // Global root signature
            CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT* globalRootSignature = rayTracingPipelineStateObjectDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
            globalRootSignature->SetRootSignature(GetID3D12RootSignature());

            // State object config
            if (true) // TODO
            {
                CD3DX12_STATE_OBJECT_CONFIG_SUBOBJECT* stateObjectConfig = rayTracingPipelineStateObjectDesc.CreateSubobject<CD3DX12_STATE_OBJECT_CONFIG_SUBOBJECT>();
                stateObjectConfig->SetFlags(D3D12_STATE_OBJECT_FLAG_ALLOW_STATE_OBJECT_ADDITIONS);
            }

            // Shader config
            CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT* shaderConfig = rayTracingPipelineStateObjectDesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
            UINT maxPayloadSizeInBytes = 128; // TODO
            UINT maxAttributeSizeInBytes = 8; // TODO
            // With payload access qualifiers (PAQs), the MaxPayloadSizeInBytes property of D3D12_RAYTRACING_SHADER_CONFIG is no longer needed.
            shaderConfig->Config(maxPayloadSizeInBytes, maxAttributeSizeInBytes);

            // Pipeline config
            CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT* pipelineConfig = rayTracingPipelineStateObjectDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
            pipelineConfig->Config(maxTraceRecursionDepth);

            //CD3DX12_NODE_MASK_SUBOBJECT* nodeMaskSubobject = rayTracingPipelineStateObjectDesc.CreateSubobject<CD3DX12_NODE_MASK_SUBOBJECT>();
            //nodeMaskSubobject->SetNodeMask(0);

            D3D12_CHECK(GetDXRDevice()->CreateStateObject(rayTracingPipelineStateObjectDesc, IID_PPV_ARGS(&rayTracingPipelineStateObject->stateObject)));

            uint32 index = uint32(rayTracingPipelineStateObjects.size());
            rayTracingPipelineStateObjects.push_back(rayTracingPipelineStateObject);
            return index;
        }

        D3D12RayTracingPipelineStateObject* GetRayTracingPipelineStateObject(RenderBackendRayTracingPipelineStateHandle handle)
        {
            uint32 index = GetRenderBackendHandleRepresentation(handle.GetIndex());
            return rayTracingPipelineStateObjects[index];
        }

        uint32 CreateD3D12ShaderBindingTable(const RenderBackendRayTracingShaderBindingTableDesc* desc, const char* name)
        {
            D3D12RayTracingPipelineStateObject* rayTracingPipelineStateObject = GetRayTracingPipelineStateObject(desc->rayTracingPipelineState);

            uint32 numMissShaders = rayTracingPipelineStateObject->numMissShaders;
            uint32 numHitGroups = rayTracingPipelineStateObject->numHitGroups;
            uint32 numShaderGroups = 1 + numMissShaders + numHitGroups;

            uint32 rayGenGroupStride = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
            uint32 missGroupStride = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
            uint32 hitGroupStride = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;

            uint32 sbtBufferSize = 0;

            sbtBufferSize = rayGenGroupStride;

            uint32 missShaderTableOffset = AlignUp(sbtBufferSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
            sbtBufferSize = missShaderTableOffset + numMissShaders * missGroupStride;

            uint32 hitGroupTableOffset = AlignUp(sbtBufferSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
            sbtBufferSize = hitGroupTableOffset + numHitGroups * hitGroupStride;

            uint32 callableShaderTableOffset = AlignUp(sbtBufferSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
            sbtBufferSize = callableShaderTableOffset + 0;

            RenderBackendBufferDesc sbtBufferDesc = RenderBackendBufferDesc::CreateShaderBindingTable(sbtBufferSize);
            uint32 index = CreateD3D12Buffer(&sbtBufferDesc, nullptr, "SBT");
            D3D12Buffer& sbtBuffer = *buffers[index];

            void* sbtBufferDataT = nullptr;
            sbtBuffer.resource->Map(0, nullptr, &sbtBufferDataT);
            uint8* sbtBufferData = reinterpret_cast<uint8*>(sbtBufferDataT);

            Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
            rayTracingPipelineStateObject->stateObject.As(&stateObjectProperties);

            uint32 numMissShadersTemp = 0;
            uint32 numHitGroupTemp = 0;
            for (uint32 groupIndex = 0; groupIndex < numShaderGroups; groupIndex++)
            {
                if (rayTracingPipelineStateObject->desc.shaderGroupDescs[groupIndex].type == RenderBackendRayTracingShaderGroupType::RayGen)
                {
                    D3D12Shader* shader = GetShader(rayTracingPipelineStateObject->desc.shaders[groupIndex]);
                    const wchar_t* exportName = shader->entryFunctionName.c_str();
                    void* shaderIdentifierPointer = stateObjectProperties->GetShaderIdentifier(exportName);
                    assert(shaderIdentifierPointer != nullptr);
                    memcpy(sbtBufferData, shaderIdentifierPointer, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                }
                if (rayTracingPipelineStateObject->desc.shaderGroupDescs[groupIndex].type == RenderBackendRayTracingShaderGroupType::Miss)
                {
                    D3D12Shader* shader = GetShader(rayTracingPipelineStateObject->desc.shaders[groupIndex]);
                    const wchar_t* exportName = shader->entryFunctionName.c_str();
                    void* shaderIdentifierPointer = stateObjectProperties->GetShaderIdentifier(exportName);
                    assert(shaderIdentifierPointer != nullptr);
                    memcpy(sbtBufferData + missShaderTableOffset + numMissShadersTemp * missGroupStride, shaderIdentifierPointer, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                    numMissShadersTemp++;
                }
                if (rayTracingPipelineStateObject->desc.shaderGroupDescs[groupIndex].type == RenderBackendRayTracingShaderGroupType::TrianglesHitGroup)
                {
                    D3D12Shader* shader = GetShader(rayTracingPipelineStateObject->desc.shaders[groupIndex]);
                    const wchar_t* exportName = L"Any";//shader->entryFunctionName.c_str();
                    void* shaderIdentifierPointer = stateObjectProperties->GetShaderIdentifier(exportName);
                    assert(shaderIdentifierPointer != nullptr);
                    memcpy(sbtBufferData + hitGroupTableOffset + numHitGroupTemp * hitGroupStride, shaderIdentifierPointer, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
                    numHitGroupTemp++;
                }
            }

            sbtBuffer.resource->Unmap(0, nullptr);

            sbtBuffer.shaderBindingTable.rayGenerationShaderRecord.StartAddress = sbtBuffer.resource->GetGPUVirtualAddress();
            sbtBuffer.shaderBindingTable.rayGenerationShaderRecord.SizeInBytes = rayGenGroupStride;

            sbtBuffer.shaderBindingTable.missShaderTable.StartAddress = sbtBuffer.resource->GetGPUVirtualAddress() + missShaderTableOffset;
            sbtBuffer.shaderBindingTable.missShaderTable.SizeInBytes = numMissShaders * missGroupStride;
            sbtBuffer.shaderBindingTable.missShaderTable.StrideInBytes = missGroupStride;

            if (numHitGroups > 0)
            {
                sbtBuffer.shaderBindingTable.hitGroupTable.StartAddress = sbtBuffer.resource->GetGPUVirtualAddress() + hitGroupTableOffset;
                sbtBuffer.shaderBindingTable.hitGroupTable.SizeInBytes = numHitGroups * hitGroupStride;
                sbtBuffer.shaderBindingTable.hitGroupTable.StrideInBytes = hitGroupStride;
            }
            else
            {
                sbtBuffer.shaderBindingTable.hitGroupTable.StartAddress = 0;
                sbtBuffer.shaderBindingTable.hitGroupTable.SizeInBytes = 0;
                sbtBuffer.shaderBindingTable.hitGroupTable.StrideInBytes = 0;
            }

            sbtBuffer.shaderBindingTable.callableShaderTable.StartAddress = 0;// sbtBuffer.resource->GetGPUVirtualAddress() + callableShaderTableOffset;
            sbtBuffer.shaderBindingTable.callableShaderTable.SizeInBytes = 0;
            sbtBuffer.shaderBindingTable.callableShaderTable.StrideInBytes = 0;

            return index;
        }

        std::vector<D3D12TimingQueryHeap*> queryHeaps;

        Microsoft::WRL::ComPtr<D3D12MA::Allocator> allocator;

        struct CopyWorkload
        {
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList7> commandList7;
            Microsoft::WRL::ComPtr<ID3D12Fence> fence;
            D3D12Buffer* uploadBuffer;
        };
        std::vector<CopyWorkload> copyWorkloadFreeList;

        CopyWorkload AllocateCopyWorkload(uint64 bufferSize)
        {
            CopyWorkload workload;

            for (uint32 i = 0; i < copyWorkloadFreeList.size(); i++)
            {
                if ((copyWorkloadFreeList[i].uploadBuffer != nullptr) &&
                    (copyWorkloadFreeList[i].uploadBuffer->size >= bufferSize))
                {
                    // if (copyWorkloadFreeList[i].fence->GetCompletedValue() == 1)
                    {
                        D3D12_CHECK(copyWorkloadFreeList[i].fence->Signal(0));
                        workload = std::move(copyWorkloadFreeList[i]);
                        std::swap(copyWorkloadFreeList[i], copyWorkloadFreeList.back());
                        copyWorkloadFreeList.pop_back();
                        return workload;
                    }
                }
            }

            // @todo use copy queue
            D3D12_CHECK(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&workload.commandAllocator)));
            D3D12_CHECK(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, workload.commandAllocator.Get(), nullptr, IID_PPV_ARGS(&workload.commandList)));
            D3D12_CHECK(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&workload.fence)));

            D3D12_CHECK(workload.commandList->QueryInterface(IID_PPV_ARGS(&workload.commandList7)));

            D3D12_CHECK(workload.commandList->Close());

            RenderBackendBufferDesc uploadBufferDesc = RenderBackendBufferDesc::CreateUpload(bufferSize);
            uint32 uploadBufferIndex = CreateD3D12Buffer(&uploadBufferDesc, nullptr, "CopyWorkload_UploadBuffer");
            workload.uploadBuffer = buffers[uploadBufferIndex];

            return workload;
        }

        void SubmitCopyWorkload(const CopyWorkload& workload)
        {
            workload.commandList->Close();
            ID3D12CommandList* commandlists[] = { workload.commandList.Get() };
            GetCommandQueue(D3D12CommandQueueType::Direct)->GetID3D12CommandQueue()->ExecuteCommandLists(1, commandlists);
            WaitIdle();

            // TODO
            //D3D12_CHECK(GetCommandQueue(D3D12CommandQueueType::Direct)->GetID3D12CommandQueue()->Signal(workload.fence.Get(), 1));
            //D3D12_CHECK(GetCommandQueue(D3D12CommandQueueType::Compute)->GetID3D12CommandQueue()->Wait(workload.fence.Get(), 1));
            //D3D12_CHECK(GetCommandQueue(D3D12CommandQueueType::Copy)->GetID3D12CommandQueue()->Wait(workload.fence.Get(), 1));
            copyWorkloadFreeList.push_back(workload);
        }

        std::vector<D3D12CommandAllocator*> commandAllocators[RenderBackendQueueFamilyCount];
        std::vector<D3D12CommandList*> commandLists[RenderBackendQueueFamilyCount];

        D3D12CommandAllocator* AllocateCommandAllocator(D3D12CommandQueueType queueType)
        {
            D3D12CommandAllocator* commandAllocator = nullptr;

            auto& commandAllocatorPool = commandAllocators[(uint32)queueType];
            if (commandAllocatorPool.empty())
            {
                commandAllocator = new D3D12CommandAllocator();
                D3D12_CHECK(device->CreateCommandAllocator(GetD3D12CommandListType(queueType), IID_PPV_ARGS(&commandAllocator->allocator)));
                commandAllocator->queueType = queueType;
            }
            else
            {
                commandAllocator = commandAllocatorPool.back();
                commandAllocatorPool.pop_back();
            }

            assert(commandAllocator);
            return commandAllocator;
        }

        void ReleaseCommandAllocator(D3D12CommandAllocator* allocator)
        {
            assert(allocator);
            D3D12_CHECK(allocator->GetID3D12CommandAllocator()->Reset());
            auto& commandAllocatorPool = commandAllocators[(uint32)allocator->queueType];
            commandAllocatorPool.push_back(allocator);
        }

        D3D12CommandList* AllocateCommandList(D3D12CommandAllocator* commandAllocator)
        {
            D3D12CommandQueueType queueType = commandAllocator->queueType;

            D3D12CommandList* commandList = nullptr;
            auto& commandListPool = commandLists[(uint32)queueType];
            if (commandListPool.empty())
            {
                commandList = new D3D12CommandList();

                switch (queueType)
                {
                case D3D12CommandQueueType::Direct:
                case D3D12CommandQueueType::Compute:
                {
                    D3D12_CHECK(device->CreateCommandList(
                        0,
                        GetD3D12CommandListType(queueType),
                        commandAllocator->GetID3D12CommandAllocator(),
                        nullptr,
                        IID_PPV_ARGS(&commandList->graphicsCommandList)));
                    commandList->commandList = commandList->graphicsCommandList;
                    D3D12_CHECK(commandList->commandList->QueryInterface(IID_PPV_ARGS(&commandList->graphicsCommandList4)));
                    D3D12_CHECK(commandList->commandList->QueryInterface(IID_PPV_ARGS(&commandList->graphicsCommandList6)));
                    D3D12_CHECK(commandList->commandList->QueryInterface(IID_PPV_ARGS(&commandList->graphicsCommandList7)));
                } break;
                case D3D12CommandQueueType::Copy:
                {
                    D3D12_CHECK(device->CreateCommandList(
                        0,
                        GetD3D12CommandListType(queueType),
                        commandAllocator->GetID3D12CommandAllocator(),
                        nullptr,
                        IID_PPV_ARGS(&commandList->commandList)));
                } break;
                default:
                    std::unreachable();
                    return commandList;
                }
                commandList->queueType = queueType;
                D3D12_CHECK(commandList->commandList->SetName(L"D3D12CommandList"));
            }
            else
            {
                commandList = commandListPool.back();
                commandListPool.pop_back();

                commandList->GetID3D12GraphicsCommandList()->Reset(commandAllocator->GetID3D12CommandAllocator(), nullptr);
            }

            assert(commandList);
            return commandList;
        }

        void ReleaseCommandList(D3D12CommandList* commandList)
        {
            assert(commandList);
            auto& commandListPool = commandLists[(uint32)commandList->queueType];
            commandListPool.push_back(commandList);
        }

        uint32 CreateD3D12QueryHeap(const RenderBackendTimingQueryHeapDesc* desc, const char* name)
        {
            uint32 index = (uint32)queryHeaps.size();
            queryHeaps.push_back(new D3D12TimingQueryHeap());

            D3D12TimingQueryHeap* queryHeap = queryHeaps[index];

            uint32 maxQueryCount = 2 * desc->maxRegions;

            D3D12_QUERY_HEAP_DESC queryHeapDesc = {
                .Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP,
                .Count = maxQueryCount,
                .NodeMask = mask.Get(),
            };
            D3D12_CHECK(device->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&queryHeap->queryHeap)));
            D3D12_CHECK(samplerDescriptorHeap->descriptorHeap->SetName(UTF8ToUTF16(name).c_str()));

            queryHeap->maxQueryCount = maxQueryCount;

            return index;
        }

        struct D3D12DescriptorAllocator
        {
            D3D12Device* device;
            D3D12_DESCRIPTOR_HEAP_DESC desc;
            uint32 descriptorHandleIncrementSize;
            std::vector<Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> descriptorHeaps;
            std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> freeList;

            void Init(D3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT numDescriptorsPerBlock)
            {
                this->device = device;
                desc.NodeMask = device->GetMask().Get();
                desc.Type = type;
                desc.NumDescriptors = numDescriptorsPerBlock;
                desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
                descriptorHandleIncrementSize = device->device->GetDescriptorHandleIncrementSize(type);
            }

            void AllocateBlock()
            {
                descriptorHeaps.emplace_back();
                D3D12_CHECK(device->GetID3D12Device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptorHeaps.back())));
                D3D12_CPU_DESCRIPTOR_HANDLE heapStart = descriptorHeaps.back()->GetCPUDescriptorHandleForHeapStart();
                for (UINT descriptorIndex = 0; descriptorIndex < desc.NumDescriptors; descriptorIndex++)
                {
                    D3D12_CPU_DESCRIPTOR_HANDLE descriptorHandle = heapStart;
                    descriptorHandle.ptr += descriptorIndex * descriptorHandleIncrementSize;
                    freeList.push_back(descriptorHandle);
                }
            }

            D3D12_CPU_DESCRIPTOR_HANDLE Allocate()
            {
                if (freeList.empty())
                {
                    AllocateBlock();
                }
                assert(!freeList.empty());
                D3D12_CPU_DESCRIPTOR_HANDLE handle = freeList.back();
                freeList.pop_back();
                return handle;
            }

            void Release(D3D12_CPU_DESCRIPTOR_HANDLE index)
            {
                freeList.push_back(index);
            }
        };

        D3D12DescriptorAllocator resourceDescriptorAllocator;
        D3D12DescriptorAllocator samplerDescriptorAllocator;
        D3D12DescriptorAllocator rtvDescriptorAllocator;
        D3D12DescriptorAllocator dsvDescriptorAllocator;

        // Bindless
        D3D12DescriptorHeap* resourceDescriptorHeap;
        D3D12DescriptorHeap* samplerDescriptorHeap;

        Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
        ID3D12RootSignature* GetID3D12RootSignature()
        {
            return rootSignature.Get();
        }

        std::vector<int> freeResourceDescriptorIndices;
        int AllocateResourceDescriptorIndex()
        {
            if (!freeResourceDescriptorIndices.empty())
            {
                int index = freeResourceDescriptorIndices.back();
                freeResourceDescriptorIndices.pop_back();
                return index;
            }
            return -1;
        }

        std::vector<int> freeSamplerDescriptorIndices;
        int AllocateSamplerDescriptorIndex()
        {
            if (!freeSamplerDescriptorIndices.empty())
            {
                int index = freeSamplerDescriptorIndices.back();
                freeSamplerDescriptorIndices.pop_back();
                return index;
            }
            return -1;
        }

        //RenderBackendDeviceMask mask;
    };

    class D3D12RenderBackend : public RenderBackend
    {
    public:

        RenderBackendType GetType() const override
        {
            return RenderBackendType::Direct3D12;
        }

        bool Init(const RenderBackendDesc* desc);
        void Exit();
        void Tick() override;
        void CreateRenderDevices(PhysicalDeviceID* physicalDeviceIDs, uint32 numDevices, uint32* outDeviceMasks) override;
        void DestroyRenderDevices() override;
        void FlushRenderDevices() override;
        RenderBackendDeviceContext GetNativeDevice() override;
        RenderBackendSwapChainHandle CreateSwapChain(const RenderBackendSwapChainDesc* desc) override;
        void DestroySwapChain(RenderBackendSwapChainHandle swapChain) override;
        void ResizeSwapChain(RenderBackendSwapChainHandle swapChain, uint32* width, uint32* height) override;
        bool PresentSwapChain(RenderBackendSwapChainHandle swapChain) override;
        RenderBackendTextureHandle GetActiveSwapChainBuffer(RenderBackendSwapChainHandle swapChain) override;
        RenderBackendBufferHandle CreateBuffer(const RenderBackendBufferDesc* desc, const void* data, const char* name) override;
        void ResizeBuffer(RenderBackendBufferHandle buffer, uint64 size) override;
        void MapBuffer(RenderBackendBufferHandle buffer, void** data) override;
        void UnmapBuffer(RenderBackendBufferHandle buffer) override;
        void DestroyBuffer(RenderBackendBufferHandle buffer) override;
        RenderBackendTextureHandle CreateTexture(const RenderBackendTextureDesc* desc, const void* data, const char* name) override;
        void DestroyTexture(RenderBackendTextureHandle texture) override;
        void UploadTexture(RenderBackendTextureHandle handle, const RenderBackendTextureUploadDataDesc& data) override;
        void GetTextureReadbackData(RenderBackendTextureHandle texture, void** data) override;
        RenderBackendTextureViewHandle CreateTextureView(RenderBackendTextureHandle textureHandle, const RenderBackendTextureViewDesc* desc, int32* descriptor) override;
        //RenderBackendTextureSRVHandle CreateTextureSRV(const RenderBackendTextureSRVDesc* desc, const char* name) override;
        //RenderBackendTextureUAVHandle CreateTextureUAV(const RenderBackendTextureUAVDesc* desc, const char* name) override;
        int32 GetTextureSRVBindlessResourceDescriptorIndex(RenderBackendTextureHandle srv) override;
        int32 GetTextureSRVBindlessResourceDescriptorIndex(RenderBackendTextureHandle srv, uint32 mipLevel) override;
        int32 GetTextureUAVBindlessResourceDescriptorIndex(RenderBackendTextureHandle uav, uint32 mipLevel) override;
        int32 GetBufferCBVBindlessResourceDescriptorIndex(RenderBackendBufferHandle uav) override;
        int32 GetBufferSRVBindlessResourceDescriptorIndex(RenderBackendBufferHandle uav) override;
        int32 GetBufferUAVBindlessResourceDescriptorIndex(RenderBackendBufferHandle uav) override;
        int32 GetAccelerationStructureSRVBindlessResourceDescriptorIndex(RenderBackendRayTracingAccelerationStructureHandle accelerationStructure) override;
        RenderBackendSamplerHandle CreateSampler(const RenderBackendSamplerDesc* desc, const char* name) override;
        void DestroySampler(RenderBackendSamplerHandle sampler) override;
        RenderBackendShaderHandle CreateShader(const RenderBackendShaderDesc* desc, const char* name) override;
        void DestroyShader(RenderBackendShaderHandle shader) override;
        RenderBackendTimingQueryHeapHandle CreateTimingQueryHeap(const RenderBackendTimingQueryHeapDesc* desc, const char* name) override;
        void DestroyTimingQueryHeap(RenderBackendTimingQueryHeapHandle timingQueryHeap) override;
        void SubmitCommandLists(RenderBackendCommandList** commandLists, uint32 numCommandLists, RenderBackendSwapChainHandle swapChain) override;
        RenderBackendRayTracingAccelerationStructureHandle CreateRayTracingBottomLevelAccelerationStructure(const RenderBackendRayTracingBottomLevelAccelerationStructureDesc* desc, const char* name) override;
        RenderBackendRayTracingAccelerationStructureHandle CreateRayTracingTopLevelAccelerationStructure(const RenderBackendRayTracingTopLevelAccelerationStructureDesc* desc, const char* name) override;
        RenderBackendRayTracingPipelineStateHandle CreateRayTracingPipelineState(const RenderBackendRayTracingPipelineStateDesc* desc, const char* name) override;
        RenderBackendBufferHandle CreateRayTracingShaderBindingTable(const RenderBackendRayTracingShaderBindingTableDesc* desc, const char* name) override;
        void SetObjectName(RenderBackendTextureHandle handle, const char* name) override
        {
            D3D12Texture* texture = devices[0]->GetTexture(handle);
            texture->GetID3D12Resource()->SetName(D3D12Utils::Widen(name).c_str());
        }
        void SetObjectName(RenderBackendBufferHandle handle, const char* name) override
        {
            D3D12Buffer* buffer = devices[0]->GetBuffer(handle);
            buffer->GetID3D12Resource()->SetName(D3D12Utils::Widen(name).c_str());
        }
        bool IsTearingSupported() const
        {
            return tearingSupported;
        }
        IDXGIFactory6* GetIDXGIFactory()
        {
            return dxgiFactory.Get();
        }
        bool useDebugLayers;
        bool useGPUBasedValidation;
        bool enableHardwareRayTracing;
        D3D12RenderBackendHandleManager handleManager;
    private:
        Microsoft::WRL::ComPtr<IDXGIFactory6> dxgiFactory;
        uint32 numAdapters;
        std::vector<D3D12Adapter*> adapters;
        uint32 numDevices;
        D3D12Device* devices[RenderBackendMaxDeviceCount];
        Microsoft::WRL::ComPtr<ID3D12Device> d3d12Devices[RenderBackendMaxDeviceCount];
        bool tearingSupported;

        HMODULE dxgiLibraryHandle = NULL;
    };

    class D3D12RenderBackendCommandListContext
    {
    public:
        D3D12RenderBackendCommandListContext(D3D12Device* device, D3D12CommandQueueType family, D3D12CommandList* commandList)
            : device(device)
            , queueType(family)
            , commandList(commandList)
            , activeComputePipeline(nullptr)
            , activeGraphicsPipeline(nullptr)
            , activeRayTracingPipeline(nullptr)
            , insideRenderPass(false) {}
        virtual ~D3D12RenderBackendCommandListContext() = default;
        inline D3D12CommandQueueType GetQueueFamily() const { return queueType; }
        inline D3D12CommandList* GetCommandList() const { return commandList; }
        bool IsAsynchronousComputeContext() const
        {
            return queueType == D3D12CommandQueueType::Compute;
        }
        bool CompileRenderBackendCommands(const RenderBackendCommandContainer& container);
        bool CompileRenderBackendCommandsAsynchronous(const RenderBackendCommandContainer& container);
        bool CompileRenderBackendCommand(const RenderBackendCommandCopyBuffer& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandCopyTexture& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandUpdateBuffer& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandUpdateTexture& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandClearBufferUAV& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandClearTextureUAV& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandBarriers& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandBeginTimingQuery& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandEndTimingQuery& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandResolveTimingQueryResults& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandDispatch& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandDispatchIndirect& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandSetViewport& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandSetScissor& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandSetStencilReference& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandBeginRenderPass& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandEndRenderPass& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandDraw& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandDrawIndirect& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandDispatchMesh& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandDispatchMeshIndirect& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandBeginDebugLabel& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandEndDebugLabel& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandBuildRayTracingBottomLevelAccelerationStructure& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandBuildRayTracingTopLevelAccelerationStructure& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandDispatchRays& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandDispatchSuperSampling& command);
    private:
        bool PrepareForDispatch(RenderBackendShaderHandle computeShader, const RenderBackendPushConstantValues& shaderConstants);
        bool PrepareForDraw(RenderBackendShaderHandle vertexShader, RenderBackendShaderHandle pixelShader, const RenderBackendGraphicsPipelineStateDescription& pipelineState, RenderBackendPrimitiveTopology topology, RenderBackendBufferHandle indexBuffer, const RenderBackendPushConstantValues& shaderConstants);
        D3D12Device* device;
        D3D12CommandQueueType queueType;
        D3D12CommandList* commandList;
        D3D12RenderPass activeRenderPass;
        ID3D12PipelineState* activeComputePipeline;
        ID3D12PipelineState* activeGraphicsPipeline;
        ID3D12StateObject* activeRayTracingPipeline;
        bool insideRenderPass;
    };
}