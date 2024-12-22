#include "D3D12RenderBackend.h"
#include "D3D12RenderBackendDefinitions.h"
#include "D3D12RenderBackendUtils.h"

#include <optick.h>
#include <pix.h>

#include <wrl/client.h>

//#include <dxgi1_3.h>
//#include <dxgi1_4.h>
//#include <dxgi1_5.h>
#include <dxgi1_6.h>
//#ifdef _DEBUG
#include <dxgidebug.h>
//#endif

#include <d3dcommon.h>
#include <dxgiformat.h>

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

#include "D3D12MemAlloc.h"

#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxguid.lib")

static void D3D12MessageCallback(
    D3D12_MESSAGE_CATEGORY Category,
    D3D12_MESSAGE_SEVERITY Severity,
    D3D12_MESSAGE_ID ID,
    LPCSTR pDescription,
    void* pContext)
{
    switch (Severity)
    {
    case D3D12_MESSAGE_SEVERITY_CORRUPTION:
        Horizon::LogFatal(Horizon::GLogger, std::format("D3D12 Corruption: ID: {}, Description: {}", (int)ID, pDescription));
        break;
    case D3D12_MESSAGE_SEVERITY_ERROR:
        Horizon::LogError(Horizon::GLogger, std::format("D3D12 Error: ID: {}, Description: {}", (int)ID, pDescription));
        break;
    case D3D12_MESSAGE_SEVERITY_WARNING:
        Horizon::LogWarning(Horizon::GLogger, std::format("D3D12 Warning: ID: {}, Description: {}", (int)ID, pDescription));
        break;
    case D3D12_MESSAGE_SEVERITY_INFO:
        Horizon::LogInfo(Horizon::GLogger, std::format("D3D12 Info: ID: {}, Description: {}", (int)ID, pDescription));
        break;
    case D3D12_MESSAGE_SEVERITY_MESSAGE:
        Horizon::LogVerbose(Horizon::GLogger, std::format("D3D12 Message: ID: {}, Description: {}", (int)ID, pDescription));
        break;
    }
}

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
        D3D12_RESOURCE_DESC resourceDesc;
        D3D12_RESOURCE_STATES initialState;
        D3D12_GPU_VIRTUAL_ADDRESS gpuAddress;
        uint64 size;
        void* mappedData;

        D3D12_CPU_DESCRIPTOR_HANDLE descriptor;

        int bindlessResourceDescriptorIndexCBV;
        int bindlessResourceDescriptorIndexSRV;
        int bindlessResourceDescriptorIndexUAV;

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

        uint32 hash;
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
        Microsoft::WRL::ComPtr<ID3D12PipelineState> state;

        D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopologyType;

        ID3D12PipelineState* GetID3D12PipelineState()
        {
            return state.Get();
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

        Microsoft::WRL::ComPtr<ID3D12CommandSignature> dispatchIndirectCommandSignature;
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> drawIndirectCommandSignature;
        Microsoft::WRL::ComPtr<ID3D12CommandSignature> drawIndexedIndirectCommandSignature;
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

            D3D12_RESOURCE_DESC resourceDesc =
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

            D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
            if (EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::Readback))
            {
                initialState = D3D12_RESOURCE_STATE_COPY_DEST;
            }
            else if (EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::Upload))
            {
                initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
            }
            //else if (EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::UniformBuffer))
            //{
            //    initialState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
            //}

            assert(buffer->resource == nullptr);
            assert(buffer->allocation == nullptr);

            D3D12_CHECK(allocator->CreateResource(
                &allocationDesc,
                &resourceDesc,
                initialState,
                nullptr,
                &buffer->allocation,
                IID_PPV_ARGS(&buffer->resource)));
            D3D12_CHECK(buffer->resource->SetName(UTF8ToUTF16(name).c_str()));

            buffer->debugName = name;
            buffer->desc = *desc;
            buffer->resourceDesc = resourceDesc;
            buffer->initialState = initialState;
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
            else if (EnumClassHasFlags(desc->flags, RenderBackendBufferCreateFlags::Upload))
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

            D3D12_CHECK(allocator->CreateResource(
                &allocationDesc,
                &buffer->resourceDesc,
                buffer->initialState,
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

            D3D12_RESOURCE_DESC resourceDesc = {
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

            D3D12MA::ALLOCATION_DESC allocationDesc = {
                .Flags = D3D12MA::ALLOCATION_FLAG_NONE,
                .HeapType = D3D12_HEAP_TYPE_DEFAULT,
                .ExtraHeapFlags = D3D12_HEAP_FLAG_NONE, // TODO
                .CustomPool = nullptr,
                .pPrivateData = nullptr,
            };

            D3D12_RESOURCE_STATES initialResourceState = ConvertToD3D12ResourceState(desc->initialState);

            D3D12_CLEAR_VALUE optimizedClearValue = {};
            optimizedClearValue.Color[0] = desc->clearValue.colorValue.float32[0];
            optimizedClearValue.Color[1] = desc->clearValue.colorValue.float32[1];
            optimizedClearValue.Color[2] = desc->clearValue.colorValue.float32[2];
            optimizedClearValue.Color[3] = desc->clearValue.colorValue.float32[3];
            optimizedClearValue.DepthStencil.Depth = desc->clearValue.depthStencilValue.depth;
            optimizedClearValue.DepthStencil.Stencil = desc->clearValue.depthStencilValue.stencil; // TODO
            optimizedClearValue.Format = resourceDesc.Format;
            bool useClearValue = EnumClassHasFlags(desc->flags, RenderBackendTextureCreateFlags::RenderTarget) || EnumClassHasFlags(desc->flags, RenderBackendTextureCreateFlags::DepthStencil);

            D3D12_CHECK(allocator->CreateResource(
                &allocationDesc,
                &resourceDesc,
                initialResourceState,
                useClearValue ? &optimizedClearValue : nullptr,
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

            // Temp
            texture->debugName = name;

            uint32 numSubresources = desc->arrayLayerCount * std::max(1u, desc->mipLevelCount);
            texture->totalSize = 0;
            texture->footprints.resize(numSubresources);
            texture->rowSizesInBytes.resize(numSubresources);
            texture->numRows.resize(numSubresources);
            device->GetCopyableFootprints(
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

        void InitD3D12BlendDesc(const RenderBackendColorBlendState& blendState, uint32 numRenderTargets, D3D12_BLEND_DESC& dstBlendState)
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

        void InitD3D12RasterizerDesc(const RenderBackendRasterizationState& rasterizationState, D3D12_RASTERIZER_DESC& dstRasterizationState)
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

        void InitD3D12DepthStencilDesc(const RenderBackendDepthStencilState& depthStencilState, D3D12_DEPTH_STENCIL_DESC& dstDepthStencilState)
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

        std::unordered_map<uint64, D3D12GraphicsPipelineState*> graphicsPipelineStateMap;
        std::unordered_map<uint64, D3D12ComputePipelineState*> computePipelineStateMap;

        D3D12ComputePipelineState* FindOrCreateComputePipelineState(D3D12Shader* shader)
        {
            uint32 shaderHash = shader->hash;
            uint64 pipelineStateHash = shaderHash;

            if (computePipelineStateMap.find(pipelineStateHash) != computePipelineStateMap.end())
            {
                return computePipelineStateMap[pipelineStateHash];
            }

            D3D12ComputePipelineState* newComputePipelineState = new D3D12ComputePipelineState();

            D3D12_COMPUTE_PIPELINE_STATE_DESC computePipelineStateDesc = {
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
            const RenderBackendGraphicsPipelineState& pipelineState,
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

                D3D12_CHECK(device2->CreatePipelineState(&streamDesc, IID_PPV_ARGS(&newGraphicsPipelineState->state)));
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

                D3D12_CHECK(device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&newGraphicsPipelineState->state)));
            }

            graphicsPipelineStateMap.emplace(pipelineStateHash, newGraphicsPipelineState);
            return graphicsPipelineStateMap[pipelineStateHash];
        }

        std::vector<D3D12TimingQueryHeap*> queryHeaps;

        Microsoft::WRL::ComPtr<D3D12MA::Allocator> allocator;

        struct CopyWorkload
        {
            Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
            Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
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

            D3D12_CHECK(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&workload.commandAllocator)));
            D3D12_CHECK(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, workload.commandAllocator.Get(), nullptr, IID_PPV_ARGS(&workload.commandList)));
            D3D12_CHECK(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&workload.fence)));

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
            GetCommandQueue(D3D12CommandQueueType::Copy)->GetID3D12CommandQueue()->ExecuteCommandLists(1, commandlists);
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
            return RenderBackendType::D3D12;
        }

        bool Init(const D3D12RenderBackendDesc* desc);
        void Exit();
        void Tick() override;
        void CreateRenderDevices(PhysicalDeviceID* physicalDeviceIDs, uint32 numDevices, uint32* outDeviceMasks) override;
        void DestroyRenderDevices() override;
        void FlushRenderDevices() override;
        RenderBackendDevice GetNativeDevice() override;
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
        D3D12RenderBackendHandleManager handleManager;
    private:
        Microsoft::WRL::ComPtr<IDXGIFactory6> dxgiFactory;
        uint32 numAdapters;
        std::vector<D3D12Adapter*> adapters;
        uint32 numDevices;
        D3D12Device* devices[RenderBackendMaxDeviceCount];
        Microsoft::WRL::ComPtr<ID3D12Device> d3d12Devices[RenderBackendMaxDeviceCount];
        bool tearingSupported;
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
        bool CompileRenderBackendCommands(const RenderBackendCommandContainer& container);
        bool CompileRenderBackendCommand(const RenderBackendCommandCopyBuffer& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandCopyTexture& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandUpdateBuffer& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandUpdateTexture& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandClearBufferUAV& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandClearTextureUAV& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandBarriers& command);
        bool CompileRenderBackendCommand(const RenderBackendCommandTransitions& command);
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
        bool PrepareForDispatch(RenderBackendShaderHandle computeShader, const RenderBackendShaderConstants& shaderConstants);
        bool PrepareForDraw(RenderBackendShaderHandle vertexShader, RenderBackendShaderHandle pixelShader, const RenderBackendGraphicsPipelineState& pipelineState, RenderBackendPrimitiveTopology topology, RenderBackendBufferHandle indexBuffer, const RenderBackendShaderConstants& shaderConstants);
        D3D12Device* device;
        D3D12CommandQueueType queueType;
        D3D12CommandList* commandList;
        D3D12RenderPass activeRenderPass;
        ID3D12PipelineState* activeComputePipeline;
        ID3D12PipelineState* activeGraphicsPipeline;
        ID3D12PipelineState* activeRayTracingPipeline;
        bool insideRenderPass;
    };

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommands(const RenderBackendCommandContainer& container)
    {
#define COMPILE_RENDER_COMMAND(command, RenderBackendCommandStruct)                                        \
        case RenderBackendCommandStruct::Type:                                                             \
        if (!CompileRenderBackendCommand(*reinterpret_cast<const RenderBackendCommandStruct*>(command)))   \
        {                                                                                                  \
            return false;                                                                                  \
        }                                                                                                  \
        break;

        for (uint32 i = 0; i < container.numCommands; i++)
        {
            switch (container.types[i])
            {
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandCopyBuffer);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandCopyTexture);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandUpdateBuffer);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandUpdateTexture);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandClearBufferUAV);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandClearTextureUAV);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandBarriers);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandTransitions);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandBeginTimingQuery);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandEndTimingQuery);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandResolveTimingQueryResults);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandDispatch);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandDispatchIndirect);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandSetViewport);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandSetScissor);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandSetStencilReference);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandBeginRenderPass);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandEndRenderPass);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandDraw);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandDrawIndirect);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandDispatchMesh);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandDispatchMeshIndirect);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandBeginDebugLabel);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandEndDebugLabel);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandBuildRayTracingBottomLevelAccelerationStructure);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandBuildRayTracingTopLevelAccelerationStructure);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandDispatchRays);
                COMPILE_RENDER_COMMAND(container.commands[i], RenderBackendCommandDispatchSuperSampling);
                default: std::unreachable();
            }
        }
#undef COMPILE_RENDER_COMMAND
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandCopyBuffer& command)
    {
        D3D12Buffer* srcBuffer = device->GetBuffer(command.srcBuffer);
        D3D12Buffer* dstBuffer = device->GetBuffer(command.dstBuffer);

        commandList->GetID3D12GraphicsCommandList6()->CopyBufferRegion(dstBuffer->GetID3D12Resource(), command.dstOffset, srcBuffer->GetID3D12Resource(), command.srcOffset, command.bytes);
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandCopyTexture& command)
    {
        D3D12Texture* srcTexture = device->GetTexture(command.srcTexture);
        D3D12Texture* dstTexture = device->GetTexture(command.dstTexture);

        D3D12_TEXTURE_COPY_LOCATION dstLocation = {
            .pResource = dstTexture->GetID3D12Resource(),
            .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            .SubresourceIndex = D3D12CalcSubresource(command.dstSubresourceLayers.mipLevel, command.dstSubresourceLayers.firstLayer, 0, 1, command.dstSubresourceLayers.arrayLayers)
        };

        D3D12_TEXTURE_COPY_LOCATION srcLocation = {
            .pResource = srcTexture->GetID3D12Resource(),
            .Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
            .SubresourceIndex = D3D12CalcSubresource(command.srcSubresourceLayers.mipLevel, command.srcSubresourceLayers.firstLayer, 0, 1, command.srcSubresourceLayers.arrayLayers)
        };

        D3D12_BOX srcBox = {
            .left   = (UINT)command.srcOffset.x,
            .top    = (UINT)command.srcOffset.y,
            .front  = (UINT)command.srcOffset.z,
            .right  = (UINT)command.srcOffset.x + command.extent.width,
            .bottom = (UINT)command.srcOffset.y + command.extent.height,
            .back   = (UINT)command.srcOffset.z + command.extent.depth
        };

        commandList->GetID3D12GraphicsCommandList6()->CopyTextureRegion(&dstLocation, command.dstOffset.x, command.dstOffset.y, command.dstOffset.z, &srcLocation, &srcBox);
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandUpdateBuffer& command)
    {
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandUpdateTexture& command)
    {
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandClearBufferUAV& command)
    {
        D3D12Buffer* buffer = device->GetBuffer(command.buffer);

        const UINT clearValue[4] =
        {
            command.data,
            command.data,
            command.data,
            command.data
        };

        D3D12_GPU_DESCRIPTOR_HANDLE viewGPUHandle = device->resourceDescriptorHeap->gpuDescriptorHandle;
        viewGPUHandle.ptr += buffer->bindlessResourceDescriptorIndexUAV * device->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CPU_DESCRIPTOR_HANDLE viewCPUHandle = buffer->descriptor;

        commandList->GetID3D12GraphicsCommandList()->ClearUnorderedAccessViewUint(viewGPUHandle, viewCPUHandle, buffer->GetID3D12Resource(), clearValue, 0, nullptr);

        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandClearTextureUAV& command)
    {
        D3D12Texture* texture = device->GetTexture(command.uav.texture);

        const UINT clearValue[4] =
        {
            command.clearValue.colorValue.uint32[0],
            command.clearValue.colorValue.uint32[1],
            command.clearValue.colorValue.uint32[2],
            command.clearValue.colorValue.uint32[3]
        };

        if (!command.clearValue.test)
        {
            D3D12UnorderedAccessView* uav = texture->GetUnorderedAccessView(command.uav.mipLevel);

            D3D12_GPU_DESCRIPTOR_HANDLE viewGPUHandle = device->resourceDescriptorHeap->gpuDescriptorHandle;
            viewGPUHandle.ptr += uav->bindlessIndex * device->device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            D3D12_CPU_DESCRIPTOR_HANDLE viewCPUHandle = uav->descriptor;

            commandList->GetID3D12GraphicsCommandList()->ClearUnorderedAccessViewUint(viewGPUHandle, viewCPUHandle, texture->GetID3D12Resource(), clearValue, 0, nullptr);
        }
        else
        {
            const FLOAT clearRGBA[4] =
            {
                command.clearValue.colorValue.float32[0],
                command.clearValue.colorValue.float32[1],
                command.clearValue.colorValue.float32[2],
                command.clearValue.colorValue.float32[3]
            };

            D3D12RenderTargetView* rtv = texture->GetRenderTargetView(0);
            D3D12_CPU_DESCRIPTOR_HANDLE viewCPUHandle = rtv->descriptor;
            D3D12_RECT rect =
            {
                .left = 0,
                .top = 0,
                .right = LONG(texture->width),
                .bottom = LONG(texture->height)
            };
            commandList->GetID3D12GraphicsCommandList()->ClearRenderTargetView(viewCPUHandle, clearRGBA, 1, &rect);
        }

        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandBarriers& command)
    {
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandTransitions& command)
    {
#if 1
        struct D3D12DiscardResourceDesc
        {
            ID3D12Resource* resource = nullptr;
            D3D12_DISCARD_REGION region = {};

            std::string debug;
        };
        std::vector<D3D12DiscardResourceDesc> resourcesToDiscard;

        std::vector<D3D12_RESOURCE_BARRIER> barriers;
        for (uint32 i = 0; i < command.transitionCount; i++)
        {
            const RenderBackendBarrier& transition = command.transitions[i];

            // Before and after states must be different.
            bool isTransitionBarrier = transition.stateBefore != transition.stateAfter;

            if ((transition.stateBefore == RenderBackendResourceState::UnorderedAccess) && (transition.stateAfter == RenderBackendResourceState::UnorderedAccess))
            {
                isTransitionBarrier = false;
            }
            else
            {
                assert(transition.stateBefore != transition.stateAfter);
            }

            if (isTransitionBarrier)
            {
                switch (transition.type)
                {
                case RenderBackendBarrier::Type::Texture:
                {
                    D3D12Texture* texture = device->GetTexture(transition.texture);
                    uint32 firstLevel = transition.textureRange.firstLevel;
                    uint32 firstLayer = transition.textureRange.firstLayer;
                    uint32 planeSlice = 0;
                    uint32 mipLevels = (transition.textureRange.mipLevels == RenderBackendTextureSubresourceRange::RemainingMipLevels) ? (texture->mipLevels - firstLevel) : (transition.textureRange.mipLevels);
                    uint32 arraySlices = (transition.textureRange.arrayLayers == RenderBackendTextureSubresourceRange::RemainingArrayLayers) ? (texture->arraySize - firstLayer) : (transition.textureRange.arrayLayers);

                    // LogVerbose(GLogger, std::format("Processing Texture State Transition: {}, initial state: {}, state before: {}, state after: {}, firstLevel: {}, mipLevels: {}, firstLayer: {}, arraySlices: {}",
                    //     texture->debugName,
                    //     int(texture->initialState),
                    //     int(transition.stateBefore),
                    //     int(transition.stateAfter),
                    //     firstLevel,
                    //     mipLevels,
                    //     firstLayer,
                    //     arraySlices));

                    if (transition.textureRange.IsAll())
                    {
                        D3D12_RESOURCE_BARRIER barrier = {};
                        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                        barrier.Transition.pResource = texture->GetID3D12Resource();
                        barrier.Transition.StateBefore = (transition.stateBefore != RenderBackendResourceState::Undefined) ? ConvertToD3D12ResourceState(transition.stateBefore) : ConvertToD3D12ResourceState(texture->initialState);
                        barrier.Transition.StateAfter = ConvertToD3D12ResourceState(transition.stateAfter);
                        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

                        // Before and after states must be different.
                        // TODO: better way?
                        if (barrier.Transition.StateBefore != barrier.Transition.StateAfter)
                        {
                            barriers.emplace_back(barrier);
                        }
                    }
                    else
                    {
                        for (uint32 mipSlice = firstLevel; mipSlice < firstLevel + mipLevels; mipSlice++)
                        {
                            for (uint32 arraySlice = firstLayer; arraySlice < firstLayer + arraySlices; arraySlice++)
                            {
                                D3D12_RESOURCE_BARRIER barrier = {};
                                barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                                barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                                barrier.Transition.pResource = texture->GetID3D12Resource();
                                barrier.Transition.StateBefore = (transition.stateBefore != RenderBackendResourceState::Undefined) ? ConvertToD3D12ResourceState(transition.stateBefore) : ConvertToD3D12ResourceState(texture->initialState);
                                barrier.Transition.StateAfter = ConvertToD3D12ResourceState(transition.stateAfter);
                                barrier.Transition.Subresource = D3D12CalcSubresource(mipSlice, arraySlice, planeSlice, texture->mipLevels, texture->arraySize);

                                // Before and after states must be different.
                                // TODO: better way?
                                if (barrier.Transition.StateBefore != barrier.Transition.StateAfter)
                                {
                                    barriers.emplace_back(barrier);
                                }
                            }
                        }
                    }

                    if ((transition.stateBefore == RenderBackendResourceState::Undefined) && !texture->isSwapChainBuffer)
                    {
                        D3D12DiscardResourceDesc& discard = resourcesToDiscard.emplace_back();
                        discard.resource = texture->GetID3D12Resource();
                        discard.region.FirstSubresource = D3D12CalcSubresource(firstLevel, firstLayer, planeSlice, texture->mipLevels, texture->arraySize);
                        discard.region.NumSubresources = GetNumSubresources(device->GetID3D12Device(), mipLevels, arraySlices, texture->format);
                        discard.region.NumRects = 0;
                        discard.region.pRects = nullptr;
                        discard.debug = texture->debugName;
                    }
                } break;
                case RenderBackendBarrier::Type::Buffer:
                {
                    D3D12Buffer* buffer = device->GetBuffer(transition.buffer);

                    D3D12_RESOURCE_BARRIER& barrier = barriers.emplace_back();
                    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
                    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                    barrier.Transition.pResource = buffer->GetID3D12Resource();
                    barrier.Transition.StateBefore = ConvertToD3D12ResourceState(transition.stateBefore);
                    barrier.Transition.StateAfter = ConvertToD3D12ResourceState(transition.stateAfter);
                    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

                    /*if (transition.stateBefore == RenderBackendResourceState::Undefined)
                    {
                        D3D12DiscardResourceDesc& discard = resourcesToDiscard.emplace_back();
                        discard.resource = buffer->GetID3D12Resource();
                    }*/
                } break;
                }
            }
            else
            {
                switch (transition.type)
                {
                case RenderBackendBarrier::Type::Texture:
                {
                    D3D12Texture* texture = device->GetTexture(transition.texture);

                    D3D12_RESOURCE_BARRIER& barrier = barriers.emplace_back();
                    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
                    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                    barrier.UAV.pResource = texture->GetID3D12Resource();
                } break;
                case RenderBackendBarrier::Type::Buffer:
                {
                    D3D12Buffer* buffer = device->GetBuffer(transition.buffer);

                    D3D12_RESOURCE_BARRIER& barrier = barriers.emplace_back();
                    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
                    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
                    barrier.UAV.pResource = buffer->GetID3D12Resource();
                } break;
                }
            }
        }

        if (!barriers.empty())
        {
            commandList->GetID3D12GraphicsCommandList6()->ResourceBarrier((UINT)barriers.size(), barriers.data());
        }

        // https://www.asawicki.info/news_1724_initializing_dx12_textures_after_allocation_and_aliasing
        if (!resourcesToDiscard.empty())
        {
            for (D3D12DiscardResourceDesc& discard : resourcesToDiscard)
            {
                commandList->GetID3D12GraphicsCommandList6()->DiscardResource(discard.resource, (discard.region.NumSubresources > 0) ? &discard.region : nullptr);
            }
        }
#else
        std::vector<D3D12_GLOBAL_BARRIER> globalBarriers;
        std::vector<D3D12_TEXTURE_BARRIER> textureBarriers;
        std::vector<D3D12_BUFFER_BARRIER> bufferBarriers;
        for (uint32 i = 0; i < command.numTransitions; i++)
        {
            const auto& transition = command.transitions[i];
            assert(transition.stateBefore != transition.stateAfter);

            switch (transition.type)
            {
            case RenderBackendBarrier::Type::Texture:
            {
                D3D12Texture* texture = device->GetTexture(transition.texture);
                D3D12_TEXTURE_BARRIER& textureBarrier = textureBarriers.emplace_back();

                D3D12_BARRIER_SUBRESOURCE_RANGE subresourceRange =
                {
                    .IndexOrFirstMipLevel = transition.textureRange.firstLevel,
                    .NumMipLevels = (transition.textureRange.mipLevelCount == RenderBackendTextureSubresourceRange::RemainingMipLevels) ? (texture->mipLevelCount - transition.textureRange.firstLevel) : transition.textureRange.mipLevelCount,
                    .FirstArraySlice = transition.textureRange.firstLayer,
                    .NumArraySlices = (transition.textureRange.arrayLayerCount == RenderBackendTextureSubresourceRange::RemainingArrayLayers) ? (texture->arrayLayerCount - transition.textureRange.firstLayer) : transition.textureRange.arrayLayerCount,
                    .FirstPlane = 0,
                    .NumPlanes = 1
                };
                textureBarrier.SyncBefore = D3D12_BARRIER_SYNC_ALL; // TODO
                textureBarrier.SyncAfter = D3D12_BARRIER_SYNC_ALL; // TODO
                textureBarrier.AccessBefore = D3D12_BARRIER_ACCESS_COMMON; // TODO
                textureBarrier.AccessAfter = D3D12_BARRIER_ACCESS_COMMON; // TODO
                textureBarrier.LayoutBefore = ConvertToD3D12BarrierLayout(transition.stateBefore);
                textureBarrier.LayoutAfter = ConvertToD3D12BarrierLayout(transition.stateAfter);
                textureBarrier.pResource = texture->GetID3D12Resource();
                textureBarrier.Subresources = subresourceRange;
                textureBarrier.Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE; // TODO
            } break;
            case RenderBackendBarrier::Type::Buffer:
            {

            } break;
            }
        }

        std::vector<D3D12_BARRIER_GROUP> barrierGroups;
        if (!globalBarriers.empty())
        {
            D3D12_BARRIER_GROUP& group = barrierGroups.emplace_back();
            group.Type = D3D12_BARRIER_TYPE_GLOBAL;
            group.NumBarriers = (UINT32)globalBarriers.size();
            group.pGlobalBarriers = globalBarriers.data();
        }
        if (!textureBarriers.empty())
        {
            D3D12_BARRIER_GROUP& group = barrierGroups.emplace_back();
            group.Type = D3D12_BARRIER_TYPE_TEXTURE;
            group.NumBarriers = (UINT32)textureBarriers.size();
            group.pTextureBarriers = textureBarriers.data();
        }
        if (!bufferBarriers.empty())
        {
            D3D12_BARRIER_GROUP& group = barrierGroups.emplace_back();
            group.Type = D3D12_BARRIER_TYPE_BUFFER;
            group.NumBarriers = (UINT32)bufferBarriers.size();
            group.pBufferBarriers = bufferBarriers.data();
        }

        if (!barrierGroups.empty())
        {
            commandList->GetID3D12GraphicsCommandList7()->Barrier((UINT)barrierGroups.size(), barrierGroups.data());
        }
#endif
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandBeginTimingQuery& command)
    {
        const auto& timingQueryHeap = device->GetTimingQueryHeap(command.timingQueryHeap);

        uint32 queryIndex = command.region * 2 + 0;
        assert(queryIndex < timingQueryHeap->maxQueryCount);

        commandList->GetID3D12GraphicsCommandList6()->EndQuery(timingQueryHeap->GetID3D12QueryHeap(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex);
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandEndTimingQuery& command)
    {
        const auto& timingQueryHeap = device->GetTimingQueryHeap(command.timingQueryHeap);

        uint32 queryIndex = command.region * 2 + 1;
        assert(queryIndex < timingQueryHeap->maxQueryCount);

        commandList->GetID3D12GraphicsCommandList6()->EndQuery(timingQueryHeap->GetID3D12QueryHeap(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex);
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandResolveTimingQueryResults& command)
    {
        OPTICK_EVENT();

        const auto& timingQueryHeap = device->GetTimingQueryHeap(command.timingQueryHeap);
        const auto& buffer = device->GetBuffer(command.buffer);

        uint32 queryStart = 2 * command.regionStart;
        uint32 queryCount = 2 * command.regionCount;

        commandList->GetID3D12GraphicsCommandList6()->ResolveQueryData(
            timingQueryHeap->GetID3D12QueryHeap(),
            D3D12_QUERY_TYPE_TIMESTAMP,
            queryStart,
            queryCount,
            buffer->GetID3D12Resource(),
            command.offset);
        return true;
    }

    bool D3D12RenderBackendCommandListContext::PrepareForDispatch(RenderBackendShaderHandle computeShader, const RenderBackendShaderConstants& shaderConstants)
    {
        D3D12Shader* d3d12Shader = device->GetShader(computeShader);
        D3D12ComputePipelineState* pipelineState = device->FindOrCreateComputePipelineState(d3d12Shader);
        if (pipelineState->GetID3D12PipelineState() != activeComputePipeline)
        {
            ID3D12RootSignature* rootSignature = device->GetID3D12RootSignature();

            commandList->GetID3D12GraphicsCommandList6()->SetPipelineState(pipelineState->GetID3D12PipelineState());
            commandList->GetID3D12GraphicsCommandList6()->SetComputeRootSignature(rootSignature);

            activeComputePipeline = pipelineState->GetID3D12PipelineState();
        }

        if (RenderBackendPushConstantsBytes > 0)
        {
            const void* pushConstantsData = &shaderConstants.data;
            commandList->GetID3D12GraphicsCommandList6()->SetComputeRoot32BitConstants(
                0, // TODO
                RenderBackendPushConstantsBytes / 4,
                pushConstantsData,
                0);
        }

        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDispatch& command)
    {
        OPTICK_EVENT();

        if (!PrepareForDispatch(command.computeShader, command.shaderConstants))
        {
            return false;
        }
        commandList->GetID3D12GraphicsCommandList6()->Dispatch(command.threadGroupCountX, command.threadGroupCountY, command.threadGroupCountZ);
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDispatchIndirect& command)
    {
        OPTICK_EVENT();

        if (!PrepareForDispatch(command.computeShader, command.shaderConstants))
        {
            return false;
        }
        commandList->GetID3D12GraphicsCommandList6()->ExecuteIndirect(device->GetDispatchIndirectCommandSignature(), 1, device->GetBuffer(command.argumentBuffer)->GetID3D12Resource(), command.argumentBufferOffset, nullptr, 0);
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandBuildRayTracingBottomLevelAccelerationStructure& command)
    {
        OPTICK_EVENT();

        const D3D12RayTracingAccelerationStructure* srcBLAS = command.srcBLAS ? device->GetRayTracingAccelerationStructure(command.srcBLAS) : nullptr;
        const D3D12RayTracingAccelerationStructure* dstBLAS = device->GetRayTracingAccelerationStructure(command.dstBLAS);

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        if (srcBLAS)
        {
            assert(buildFlags | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE);
            buildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
        }

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInputs = {};
        buildInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        buildInputs.Flags = buildFlags;
        buildInputs.NumDescs = uint32(dstBLAS->geometries.size());
        buildInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        buildInputs.pGeometryDescs = dstBLAS->geometries.data();

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
        buildDesc.DestAccelerationStructureData = dstBLAS->accelerationStructureBuffer->GetGPUVirtualAddress();
        buildDesc.Inputs = buildInputs;
        buildDesc.SourceAccelerationStructureData = srcBLAS ? srcBLAS->accelerationStructureBuffer->GetGPUVirtualAddress() : D3D12_GPU_VIRTUAL_ADDRESS(0);
        buildDesc.ScratchAccelerationStructureData = dstBLAS->scratchBuffer->GetGPUVirtualAddress();

        commandList->GetID3D12GraphicsCommandList4()->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandBuildRayTracingTopLevelAccelerationStructure& command)
    {
        OPTICK_EVENT();

        const D3D12RayTracingAccelerationStructure* srcTLAS = command.srcTLAS ? device->GetRayTracingAccelerationStructure(command.srcTLAS) : nullptr;
        const D3D12RayTracingAccelerationStructure* dstTLAS = device->GetRayTracingAccelerationStructure(command.dstTLAS);

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
        if (srcTLAS)
        {
            assert(buildFlags | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE);
            buildFlags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
        }

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS buildInputs = {};
        buildInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        buildInputs.Flags = buildFlags;
        buildInputs.NumDescs = dstTLAS->numInstances;
        buildInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
        buildInputs.InstanceDescs = dstTLAS->instanceBuffer->GetGPUVirtualAddress();

        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
        buildDesc.DestAccelerationStructureData = dstTLAS->accelerationStructureBuffer->GetGPUVirtualAddress();
        buildDesc.Inputs = buildInputs;
        buildDesc.SourceAccelerationStructureData = srcTLAS ? srcTLAS->accelerationStructureBuffer->GetGPUVirtualAddress() : D3D12_GPU_VIRTUAL_ADDRESS(0);
        buildDesc.ScratchAccelerationStructureData = dstTLAS->scratchBuffer->GetGPUVirtualAddress();

        commandList->GetID3D12GraphicsCommandList4()->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDispatchRays& command)
    {
        OPTICK_EVENT();

        D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = {};

        commandList->GetID3D12GraphicsCommandList7()->DispatchRays(&dispatchRaysDesc);
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandSetViewport& command)
    {
        D3D12_VIEWPORT viewports[RenderBackendMaxViewportCount];
        for (uint32 i = 0; i < command.viewportCount; i++)
        {
            viewports[i] =
            {
                .TopLeftX = command.viewports[i].x,
                .TopLeftY = command.viewports[i].y,
                .Width    = command.viewports[i].width,
                .Height   = command.viewports[i].height,
                .MinDepth = command.viewports[i].minDepth,
                .MaxDepth = command.viewports[i].maxDepth
            };
        }

        commandList->GetID3D12GraphicsCommandList6()->RSSetViewports(command.viewportCount, viewports);
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandSetScissor& command)
    {
        D3D12_RECT scissors[RenderBackendMaxViewportCount];
        for (uint32 i = 0; i < command.scissorCount; i++)
        {
            scissors[i] =
            {
                .left   = command.scissors[i].left,
                .top    = command.scissors[i].top,
                .right  = command.scissors[i].left + (int32)command.scissors[i].width,
                .bottom = command.scissors[i].top + (int32)command.scissors[i].height
            };
        }

        commandList->GetID3D12GraphicsCommandList6()->RSSetScissorRects(command.scissorCount, scissors);
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandSetStencilReference& command)
    {
        commandList->GetID3D12GraphicsCommandList6()->OMSetStencilRef(command.stencilReference);
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandBeginRenderPass& command)
    {
#if D3D12_RENDER_BACKEND_USE_RENDER_PASS

        // TODO: optimize this, to avoid recreate PSOs.
        memset(&activeRenderPass, 0, sizeof(D3D12RenderPass));

        activeRenderPass.numRenderTargets = 0;
        activeRenderPass.hasDepthStencil = false;

        UINT numRenderTargets = 0;
        bool hasDepthStencil = false;

        D3D12_RENDER_PASS_RENDER_TARGET_DESC renderTargetDescs[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
        D3D12_RENDER_PASS_DEPTH_STENCIL_DESC depthStencilDesc = {};

        for (uint32 index = 0; index < RenderBackendMaxRenderTargetCount; index++)
        {
            const auto& renderTarget = command.renderPassInfo.renderTargets[index];
            // TODO: remove this
            if (!renderTarget.texture)
            {
                continue;
            }

            D3D12Texture* texture  = device->GetTexture(renderTarget.texture);

            D3D12_RENDER_PASS_RENDER_TARGET_DESC& renderTargetDesc = renderTargetDescs[numRenderTargets];
            renderTargetDesc.cpuDescriptor = texture->GetRenderTargetView(renderTarget.mipLevel)->descriptor;
            renderTargetDesc.BeginningAccess.Type = ConvertToD3D12RenderPassBeginningAccessType(renderTarget.loadOp);
            renderTargetDesc.BeginningAccess.Clear.ClearValue = texture->clearValue;
            renderTargetDesc.EndingAccess.Type = ConvertToD3D12RenderPassEndingAccessType(renderTarget.storeOp);

            numRenderTargets++;
            activeRenderPass.numRenderTargets = numRenderTargets;
            activeRenderPass.renderTargetFormats[numRenderTargets - 1] = texture->format;
        }

        if (command.renderPassInfo.depthStencil.texture)
        {
            const auto& depthStencil = command.renderPassInfo.depthStencil;

            D3D12Texture* texture = device->GetTexture(depthStencil.texture);

            depthStencilDesc.cpuDescriptor = texture->GetDepthStencilView(GetDepthStencilViewIndex(depthStencil.depthStencilAccessType))->descriptor;
            depthStencilDesc.DepthBeginningAccess.Type = ConvertToD3D12RenderPassBeginningAccessType(depthStencil.depthLoadOp);
            depthStencilDesc.DepthBeginningAccess.Clear.ClearValue = texture->clearValue;
            depthStencilDesc.DepthEndingAccess.Type = ConvertToD3D12RenderPassEndingAccessType(depthStencil.depthStoreOp);
            depthStencilDesc.StencilBeginningAccess.Type = ConvertToD3D12RenderPassBeginningAccessType(depthStencil.stencilLoadOp);
            depthStencilDesc.StencilEndingAccess.Type = ConvertToD3D12RenderPassEndingAccessType(depthStencil.stencilStoreOp);

            hasDepthStencil = true;
            activeRenderPass.hasDepthStencil = true;
            activeRenderPass.depthStencilViewFormat = texture->format;
        }

        D3D12_RENDER_PASS_FLAGS flags = D3D12_RENDER_PASS_FLAG_NONE;
        if (command.renderPassInfo.depthStencil.depthStencilAccessType == RenderBackendDepthStencilAccessType::DepthReadOnly_StencilNoAccess ||
            command.renderPassInfo.depthStencil.depthStencilAccessType == RenderBackendDepthStencilAccessType::DepthReadOnly_StencilWrite ||
            command.renderPassInfo.depthStencil.depthStencilAccessType == RenderBackendDepthStencilAccessType::DepthReadOnly_StencilReadOnly)
        {
            flags |= D3D12_RENDER_PASS_FLAG_BIND_READ_ONLY_DEPTH;
        }
        if (command.renderPassInfo.depthStencil.depthStencilAccessType == RenderBackendDepthStencilAccessType::DepthNoAccess_StencilReadOnly ||
            command.renderPassInfo.depthStencil.depthStencilAccessType == RenderBackendDepthStencilAccessType::DepthWrite_StencilReadOnly ||
            command.renderPassInfo.depthStencil.depthStencilAccessType == RenderBackendDepthStencilAccessType::DepthReadOnly_StencilReadOnly)
        {
            flags |= D3D12_RENDER_PASS_FLAG_BIND_READ_ONLY_STENCIL;
        }

        if (command.renderPassInfo.allowUAVWrites)
        {
            flags |= D3D12_RENDER_PASS_FLAG_ALLOW_UAV_WRITES;
        }

        commandList->GetID3D12GraphicsCommandList6()->BeginRenderPass(numRenderTargets, renderTargetDescs, hasDepthStencil ? &depthStencilDesc : nullptr, flags);
#else
        activeRenderPass.numRenderTargets = 0;
        activeRenderPass.hasDepthStencil = false;

        UINT numRenderTargets = 0;
        bool hasDepthStencil = false;

        D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViews[D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
        D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = {};

        for (uint32 index = 0; index < RenderBackendMaxRenderTargetCount; index++)
        {
            const auto& renderTarget = command.renderPassInfo.renderTargets[index];
            // TODO: remove this
            if (!renderTarget.texture)
            {
                continue;
            }

            D3D12Texture* texture  = device->GetTexture(renderTarget.texture);

            renderTargetViews[index] = texture->GetRenderTargetView(renderTarget.mipLevel)->descriptor;

            numRenderTargets++;
            activeRenderPass.numRenderTargets = numRenderTargets;
            activeRenderPass.renderTargetFormats[numRenderTargets - 1] = texture->format;

            if (renderTarget.loadOp == RenderBackendRenderPassBeginningAccessType::Clear)
            {
                D3D12_RECT clearRect = {};
                clearRect.left = 0;
                clearRect.top = 0;
                clearRect.right = texture->width - 1;
                clearRect.bottom = texture->height - 1;

                commandList->GetID3D12GraphicsCommandList6()->ClearRenderTargetView(renderTargetViews[index], texture->clearValue.Color, 1, &clearRect);
            }
        }

        if (command.renderPassInfo.depthStencil.texture)
        {
            const auto& depthStencil = command.renderPassInfo.depthStencil;

            D3D12Texture* texture = device->GetTexture(depthStencil.texture);

            depthStencilView = texture->GetDepthStencilView(GetDepthStencilViewIndex(depthStencil.depthStencilAccessType))->descriptor;

            hasDepthStencil = true;
            activeRenderPass.hasDepthStencil = true;
            activeRenderPass.depthStencilViewFormat = texture->format;

            D3D12_CLEAR_FLAGS clearFlags = D3D12_CLEAR_FLAGS(0);

            if (depthStencil.depthLoadOp == RenderBackendRenderPassBeginningAccessType::Clear)
            {
                clearFlags |= D3D12_CLEAR_FLAG_DEPTH;
            }

            if (depthStencil.stencilLoadOp == RenderBackendRenderPassBeginningAccessType::Clear)
            {
                clearFlags |= D3D12_CLEAR_FLAG_STENCIL;
            }

            if (clearFlags != 0)
            {
                D3D12_RECT clearRect = {};
                clearRect.left = 0;
                clearRect.top = 0;
                clearRect.right = texture->width - 1;
                clearRect.bottom = texture->height - 1;

                commandList->GetID3D12GraphicsCommandList6()->ClearDepthStencilView(depthStencilView, clearFlags, texture->clearValue.DepthStencil.Depth, texture->clearValue.DepthStencil.Stencil, 1, &clearRect);
            }
        }

        commandList->GetID3D12GraphicsCommandList6()->OMSetRenderTargets(numRenderTargets, renderTargetViews, FALSE, hasDepthStencil ? &depthStencilView : nullptr);
#endif

        insideRenderPass = true;

        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandEndRenderPass& command)
    {
#if D3D12_RENDER_BACKEND_USE_RENDER_PASS
        commandList->GetID3D12GraphicsCommandList6()->EndRenderPass();
#endif
        insideRenderPass = false;
        return true;
    }

    bool D3D12RenderBackendCommandListContext::PrepareForDraw(RenderBackendShaderHandle vertexShader, RenderBackendShaderHandle pixelShader, const RenderBackendGraphicsPipelineState& pipelineStateDesc, RenderBackendPrimitiveTopology topology, RenderBackendBufferHandle indexBuffer, const RenderBackendShaderConstants& shaderConstants)
    {
        assert(insideRenderPass);

        D3D12GraphicsPipelineState* pipelineState = device->FindOrCreateGraphicsPipelineState(device->GetShader(vertexShader), device->GetShader(pixelShader), nullptr, nullptr, pipelineStateDesc, &activeRenderPass, topology);
        if (pipelineState->GetID3D12PipelineState() != activeGraphicsPipeline)
        {
            ID3D12RootSignature* rootSignature = device->rootSignature.Get();

            commandList->GetID3D12GraphicsCommandList6()->SetPipelineState(pipelineState->GetID3D12PipelineState());
            commandList->GetID3D12GraphicsCommandList6()->SetGraphicsRootSignature(rootSignature);

            D3D_PRIMITIVE_TOPOLOGY primitiveTopology = ConvertToD3DPrimitiveTopology(topology);
            commandList->GetID3D12GraphicsCommandList6()->IASetPrimitiveTopology(primitiveTopology);

            activeComputePipeline = pipelineState->GetID3D12PipelineState();
        }

        if (indexBuffer)
        {
            const D3D12Buffer* buffer = device->GetBuffer(indexBuffer);
            D3D12_INDEX_BUFFER_VIEW indexBufferView = {
                .BufferLocation = buffer->gpuAddress,
                .SizeInBytes = (UINT)buffer->size,
                .Format = DXGI_FORMAT_R32_UINT,
            };
            commandList->GetID3D12GraphicsCommandList6()->IASetIndexBuffer(&indexBufferView);
        }

        if (RenderBackendPushConstantsBytes > 0)
        {
            const void* pushConstantsData = &shaderConstants.data;
            commandList->GetID3D12GraphicsCommandList6()->SetGraphicsRoot32BitConstants(
                0, // TODO
                RenderBackendPushConstantsBytes / 4,
                pushConstantsData,
                0);
        }

        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDraw& command)
    {
        OPTICK_EVENT();

        if (!PrepareForDraw(command.vertexShader, command.pixelShader, command.pipelineState, command.topology, command.indexBuffer, command.shaderConstants))
        {
            return false;
        }
        if (!command.indexBuffer)
        {
            commandList->GetID3D12GraphicsCommandList6()->DrawInstanced(
                command.draw.vertexCount,
                command.draw.instanceCount,
                command.draw.firstVertex,
                command.draw.firstInstance);
        }
        else
        {
            commandList->GetID3D12GraphicsCommandList6()->DrawIndexedInstanced(
                command.drawIndexed.indexCount,
                command.drawIndexed.instanceCount,
                command.drawIndexed.firstIndex,
                command.drawIndexed.vertexOffset,
                command.drawIndexed.firstInstance);
        }
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDrawIndirect& command)
    {
        if (!PrepareForDraw(command.vertexShader, command.pixelShader, command.pipelineState, command.topology, command.indexBuffer, command.shaderConstants))
        {
            return false;
        }
        if (!command.indexBuffer)
        {
            commandList->GetID3D12GraphicsCommandList6()->ExecuteIndirect(
                device->GetDrawIndirectCommandSignature(),
                command.drawCount,
                device->GetBuffer(command.argumentBuffer)->GetID3D12Resource(),
                command.argumentBufferOffset,
                nullptr,
                0);
        }
        else
        {
            commandList->GetID3D12GraphicsCommandList6()->ExecuteIndirect(
                device->GetDrawIndexedIndirectCommandSignature(),
                command.drawCount,
                device->GetBuffer(command.argumentBuffer)->GetID3D12Resource(),
                command.argumentBufferOffset,
                nullptr,
                0);
        }
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDispatchMesh& command)
    {
        // if (!PrepareForMeshShading(command.amplificationShader, command.meshShader, command.pixelShader, command.pipelineState, command.topology, command.shaderConstants))
        // {
        //     return false;
        // }
        // commandList->GetID3D12GraphicsCommandList6()->DispatchMesh(
        //     command.threadGroupCountX,
        //     command.threadGroupCountY,
        //     command.threadGroupCountZ);
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDispatchMeshIndirect& command)
    {
        // if (!PrepareForMeshShading(command.amplificationShader, command.meshShader, command.pixelShader, command.pipelineState, command.topology, command.shaderConstants))
        // {
        //     return false;
        // }
        // commandList->GetID3D12GraphicsCommandList6()->ExecuteIndirect(
        //     device->GetDispatchMeshIndirectCommandSignature(),
        //     command.numDraws,
        //     device->GetBuffer(command.argumentBuffer)->GetID3D12Resource(),
        //     command.argumentBufferOffset,
        //     nullptr,
        //     0);
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandBeginDebugLabel& command)
    {
        PIXBeginEvent(commandList->GetID3D12GraphicsCommandList6(), 0xFF000000, UTF8ToUTF16(command.labelName).c_str());
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandEndDebugLabel& command)
    {
        PIXEndEvent(commandList->GetID3D12GraphicsCommandList6());
        return true;
    }

    bool D3D12RenderBackendCommandListContext::CompileRenderBackendCommand(const RenderBackendCommandDispatchSuperSampling& command)
    {
        OPTICK_EVENT();

        D3D12Texture* outputTexture = device->GetTexture(command.output);
        D3D12Texture* colorTexture = device->GetTexture(command.color);
        D3D12Texture* depthTexture = device->GetTexture(command.depth);
        D3D12Texture* motionVectorTexture = device->GetTexture(command.motionVectors);
        D3D12Texture* exposureTexture = device->GetTexture(command.exposure);

        auto GetRenderBackendTextureResourceD3D12 = [](D3D12Texture* texture, D3D12_RESOURCE_STATES state)
        {
            RenderBackendTextureResource textureResource = {};
            textureResource.texture = texture->resource.Get();
            textureResource.info = nullptr;
            textureResource.memory = nullptr;
            textureResource.view = nullptr;
            textureResource.width = texture->width;
            textureResource.height = texture->height;
            textureResource.mipLevels = texture->mipLevels;
            textureResource.arrayLayers = texture->arraySize;
            textureResource.format = texture->format;
            textureResource.state = state;
            textureResource.flags = 0;
            textureResource.usage = 0;
            return textureResource;
        };

        RenderBackendTextureResource output = GetRenderBackendTextureResourceD3D12(outputTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        RenderBackendTextureResource color = GetRenderBackendTextureResourceD3D12(colorTexture, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        RenderBackendTextureResource depth = GetRenderBackendTextureResourceD3D12(depthTexture, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        RenderBackendTextureResource motionVectors = GetRenderBackendTextureResourceD3D12(motionVectorTexture, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        RenderBackendTextureResource exposure = GetRenderBackendTextureResourceD3D12(exposureTexture, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        bool succeed = command.callback(static_cast<void*>(commandList->commandList.Get()), command.context, output, color, depth, motionVectors, exposure);

        // TODO: restore pipeline state
        ID3D12DescriptorHeap* descriptorHeaps[] =
        {
            device->resourceDescriptorHeap->GetID3D12DescriptorHeap(),
            device->samplerDescriptorHeap->GetID3D12DescriptorHeap(),
        };
        commandList->GetID3D12GraphicsCommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

        return true;
    }

// Whether to set redist parameters by exporting constant data via well known symbols.
// This feature requires Window 10 version 1909 (19H2) or newer.
// For more details about D3D12 Redistributable, please refer to DirectX-Specs.
#define EXPORT_D3D12_REDIST_CONSTANT_DATA (_WIN64 && 1)

#if EXPORT_D3D12_REDIST_CONSTANT_DATA
// D3D12SDKVersion declares the SDK version of the D3D12 redistributable that the Application is targeting.
extern "C" { _declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }
// D3D12SDKPath is a UTF-8 string that declares that D3D12Core.dll, D3D12SDKLayers.dll, and other D3D12 redist binaries are located in the subfolder D3D12 relative to the exe.
extern "C" { _declspec(dllexport) extern const char* D3D12SDKPath = /*u8*/".\\D3D12\\"; }
#endif

    bool D3D12RenderBackend::Init(const D3D12RenderBackendDesc* desc)
    {
        if (false)
        {
#if !HORIZON_CONFIGURATION_RELEASE
            const UUID experimentalFeatures[] =
            {
                D3D12ExperimentalShaderModels
            };
            if (SUCCEEDED(D3D12EnableExperimentalFeatures(_countof(experimentalFeatures), experimentalFeatures, nullptr, nullptr)))
            {
                LogInfo(GLogger, std::format("Agility SDK not found."));
            }
#else
            LogWarning(GLogger, std::format("Try to enable experimental features in the release build, ignored."));
#endif
        }

        DWORD dxgiFactoryFlags = 0;

        useDebugLayers = desc->useDebugLayers;
        useGPUBasedValidation = desc->useGPUBasedValidation;

        if (useDebugLayers)
        {
            Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
            if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))))
            {
                debugInterface->EnableDebugLayer();
                LogInfo(GLogger, std::format("D3D12 debug validation layer enabled"));
                if (useGPUBasedValidation)
                {
                    Microsoft::WRL::ComPtr<ID3D12Debug1> debugInterface1;
                    if (SUCCEEDED(debugInterface->QueryInterface(IID_PPV_ARGS(&debugInterface1))))
                    {
                        debugInterface1->SetEnableGPUBasedValidation(true);
                        LogInfo(GLogger, std::format("D3D12 GPU based debug validation layer enabled"));
                    }
                    else
                    {
                        LogWarning(GLogger, std::format("Unable to enable D3D12 GPU based debug validation layer"));
                    }
                }

                Microsoft::WRL::ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
                if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
                {
                    dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

                    dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
                    dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true);
                    dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);

                    DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
                    {
                        80 /* IDXGISwapChain::GetContainingOutput: The swap chain's adapter does not control the output on which the swap chain's window resides. */,
                    };
                    DXGI_INFO_QUEUE_FILTER filter = {};
                    filter.DenyList.NumIDs = _countof(hide);
                    filter.DenyList.pIDList = hide;
                    dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
                }
            }
            else
            {
                LogWarning(GLogger, std::format("Unable to enable D3D12 debug validation layer"));
            }
        }

        HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory));
        if (FAILED(hr))
        {
            LogWarning(GLogger, std::format("Error: CreateDXGIFactory2 failed!"));
            return false;
        }

        Microsoft::WRL::ComPtr<IDXGIFactory5> dxgiFactory5;
        if (SUCCEEDED(dxgiFactory->QueryInterface(IID_PPV_ARGS(&dxgiFactory5))))
        {
            BOOL allowTearing = FALSE;
            if (SUCCEEDED(dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
            {
                tearingSupported = (allowTearing != FALSE) ? true : false;
            }
        }

        Microsoft::WRL::ComPtr<IDXGIAdapter1> currentAdapter;
        for (uint32 adapterIndex = 0; dxgiFactory->EnumAdapters1(adapterIndex, &currentAdapter) != DXGI_ERROR_NOT_FOUND; adapterIndex++)
        {
            if (currentAdapter)
            {
                {
                    DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
                    D3D12_CHECK(currentAdapter->GetDesc1(&dxgiAdapterDesc));

                    LogInfo(GLogger,
                        std::format(L"Found D3D12 Adapter {}: {}",
                        adapterIndex,
                        dxgiAdapterDesc.Description));

                    LogInfo(GLogger,
                        std::format(L"Adapter has {}MB of dedicated video memory, {}MB of dedicated system memory, and {}MB of shared system memory",
                        (uint32)(dxgiAdapterDesc.DedicatedVideoMemory  / (1024 * 1024)),
                        (uint32)(dxgiAdapterDesc.DedicatedSystemMemory / (1024 * 1024)),
                        (uint32)(dxgiAdapterDesc.SharedSystemMemory    / (1024 * 1024))));

                    D3D12Adapter* newAdapter = new D3D12Adapter();
                    newAdapter->adapterIndex = adapterIndex;
                    newAdapter->desc = dxgiAdapterDesc;
                    newAdapter->dxgiAdapter = currentAdapter;

                    adapters.push_back(newAdapter);
                    numAdapters++;
                }
            }
        }

        return true;
    }

    void D3D12RenderBackend::Exit()
    {
        DestroyRenderDevices();
    }

    void D3D12RenderBackend::Tick()
    {
        for (uint32 deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            D3D12Device* device = devices[deviceIndex];
            device->Tick();
        }
    }

    void D3D12RenderBackend::CreateRenderDevices(PhysicalDeviceID* physicalDeviceIDs, uint32 numDevices, uint32* outDeviceMasks)
    {
        // for (uint32 i = 0; i < numAdapters; i++)
        {
            D3D12Adapter* adapter = adapters[0];
            devices[0] = new D3D12Device(this);
            D3D12Device* device = devices[0];
            d3d12Devices[0] = device->device;
            if (device->Init(this, adapter))
            {

            }
            this->numDevices++;
        }
    }

    void D3D12RenderBackend::DestroyRenderDevices()
    {
        D3D12Device* device = devices[0];
        device->Exit();
        delete device;

        Microsoft::WRL::ComPtr<ID3D12Device> d3d12Device = d3d12Devices[0];

        if (d3d12Device && useDebugLayers)
        {
            Microsoft::WRL::ComPtr<ID3D12DebugDevice> debugDevice;
            if (SUCCEEDED(device->device->QueryInterface(IID_PPV_ARGS(&debugDevice))))
            {
                D3D12_RLDO_FLAGS rldoFlags = D3D12_RLDO_DETAIL;
                D3D12_CHECK(debugDevice->ReportLiveDeviceObjects(rldoFlags));
            }
        }
    }

    void D3D12RenderBackend::FlushRenderDevices()
    {
        D3D12Device* device = devices[0];

        device->WaitIdle();
    }

    RenderBackendDevice D3D12RenderBackend::GetNativeDevice()
    {
        RenderBackendDevice d = {};
        d.device = devices[0]->device.Get();
        d.physicalDevice = nullptr;
        return d;
    }

    uint32 D3D12Device::CreateD3D12SwapChain(const RenderBackendSwapChainDesc* desc)
    {
        D3D12SwapChain* swapChain = new D3D12SwapChain();

        ID3D12CommandQueue* commandQueue = GetCommandQueue(D3D12CommandQueueType::Direct)->GetID3D12CommandQueue();
        HWND windowHandle = (HWND)desc->windowHandle;

        UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        if (backend->IsTearingSupported())
        {
            swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = desc->width;
        swapChainDesc.Height = desc->height;
        swapChainDesc.Format = ConvertToDXGIFormat(desc->format);
        swapChainDesc.Stereo = false;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
        swapChainDesc.BufferCount = desc->numBuffers;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
        swapChainDesc.Flags = swapChainFlags;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreenDesc = {};
        fullscreenDesc.Windowed = true;

        D3D12_CHECK(backend->GetIDXGIFactory()->CreateSwapChainForHwnd(
            commandQueue,
            windowHandle,
            &swapChainDesc,
            &fullscreenDesc,
            nullptr,
            &swapChain->dxgiSwapChain1));

        swapChain->width = swapChainDesc.Width;
        swapChain->height = swapChainDesc.Height;
        swapChain->format = swapChainDesc.Format;
        swapChain->numBuffers = swapChainDesc.BufferCount;
        swapChain->vsyncEnabled = desc->vsync;
        swapChain->windowed = fullscreenDesc.Windowed;

        if (swapChain->dxgiSwapChain1)
        {
            D3D12_CHECK(swapChain->dxgiSwapChain1->QueryInterface(IID_PPV_ARGS(&swapChain->dxgiSwapChain2)));
            D3D12_CHECK(swapChain->dxgiSwapChain1->QueryInterface(IID_PPV_ARGS(&swapChain->dxgiSwapChain3)));
            D3D12_CHECK(swapChain->dxgiSwapChain1->QueryInterface(IID_PPV_ARGS(&swapChain->dxgiSwapChain4)));
        }

        DXGI_COLOR_SPACE_TYPE colorSpace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;

        UINT colorSpaceSupport = 0;
        if (SUCCEEDED(swapChain->dxgiSwapChain4->CheckColorSpaceSupport(colorSpace, &colorSpaceSupport)))
        {
            if (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT)
            {
                if (SUCCEEDED(swapChain->dxgiSwapChain4->SetColorSpace1(colorSpace)))
                {
                    switch (colorSpace)
                    {
                    default:
                    case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
                        break;
                    case DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709:
                        break;
                    case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
                        break;
                    }
                }
            }
        }

        for (uint32 i = 0; i < desc->numBuffers; i++)
        {
            D3D12Texture* texture = new D3D12Texture();
            texture->width = swapChainDesc.Width;
            texture->height = swapChainDesc.Height;
            texture->format = swapChainDesc.Format;
            texture->arraySize = 1;
            texture->mipLevels = 1;
            texture->allocation = nullptr;
            texture->isSwapChainBuffer = true;
            D3D12_CHECK(swapChain->dxgiSwapChain4->GetBuffer(i, IID_PPV_ARGS(&texture->resource)));
            D3D12_CHECK(texture->resource->SetName(L"SwapChainBuffer"));

            texture->debugName = "SwapChainBuffer"; // Temp

            uint32 textureIndex = AllocateTexture();
            textures[textureIndex] = texture;
            swapChain->buffers[i] = backend->handleManager.Allocate<RenderBackendTextureHandle>(~0u);
            SetRenderBackendHandleRepresentation(swapChain->buffers[i].GetIndex(), textureIndex);

            D3D12_CHECK(device->CreateFence(1, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&swapChain->frameFences[i])));
        }

        uint32 index = (uint32)swapChains.size();
        swapChains.emplace_back(swapChain);
        return index;
    }

    RenderBackendSwapChainHandle D3D12RenderBackend::CreateSwapChain(const RenderBackendSwapChainDesc* desc)
    {
        D3D12Device* device = devices[0];

        uint32 index = device->CreateD3D12SwapChain(desc);

        RenderBackendSwapChainHandle handle = handleManager.Allocate<RenderBackendSwapChainHandle>();
        device->SetRenderBackendHandleRepresentation(handle.GetIndex(), index);

        return handle;
    }

    void D3D12RenderBackend::DestroySwapChain(RenderBackendSwapChainHandle handle)
    {

    }

    void D3D12RenderBackend::ResizeSwapChain(RenderBackendSwapChainHandle handle, uint32* width, uint32* height)
    {
        D3D12Device* device = devices[0];
        D3D12SwapChain* swapChain = device->GetSwapChain(handle);

        device->WaitIdle();

        swapChain->width = *width;
        swapChain->height = *height;

        UINT swapChainFlags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        if (device->backend->IsTearingSupported())
        {
            swapChainFlags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
        }

        for (uint32 i = 0; i < swapChain->numBuffers; i++)
        {
            D3D12Texture* texture = device->GetTexture(swapChain->buffers[i]);
            texture->resource = nullptr;
        }

        D3D12_CHECK(swapChain->GetIDXGISwapChain4()->ResizeBuffers(
            swapChain->numBuffers,
            swapChain->width,
            swapChain->height,
            swapChain->format,
            swapChainFlags));

        for (uint32 i = 0; i < swapChain->numBuffers; i++)
        {
            D3D12Texture* texture = device->GetTexture(swapChain->buffers[i]);
            texture->width = swapChain->width;
            texture->height = swapChain->height;
            D3D12_CHECK(swapChain->dxgiSwapChain4->GetBuffer(i, IID_PPV_ARGS(&texture->resource)));
            D3D12_CHECK(texture->resource->SetName(L"SwapChainBuffer"));
        }
    }

    bool D3D12RenderBackend::PresentSwapChain(RenderBackendSwapChainHandle handle)
    {
        OPTICK_EVENT();

        D3D12Device* device = devices[0];
        D3D12SwapChain* swapChain = device->GetSwapChain(handle);

        const UINT presentSyncInterval = swapChain->vsyncEnabled ? 1 : 0;

        UINT presentFlags = 0;
        if (IsTearingSupported() && swapChain->windowed && presentSyncInterval == 0)
        {
            presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
        }

        HRESULT hr = swapChain->GetIDXGISwapChain4()->Present(presentSyncInterval, presentFlags);
        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            return false;
        }

        UINT lastPresentCount = 0;
        if (SUCCEEDED(swapChain->GetIDXGISwapChain4()->GetLastPresentCount(&lastPresentCount)))
        {
        	//presentCounter = lastPresentCount;
        }
        else
        {
        	//presentCounter++;
        }

        // TODO: refactor this
        {
            uint32 bufferIndex = swapChain->GetCurrentBackBufferIndex();
            ID3D12Fence* fence = swapChain->GetFrameFence(bufferIndex);
            if (fence->GetCompletedValue() < 1)
            {
                // If hEvent is a null handle, then this API will not return until the specified fence value(s) have been reached.
                hr = fence->SetEventOnCompletion(1, NULL);
                assert(SUCCEEDED(hr));
            }
            hr = fence->Signal(0);
            assert(SUCCEEDED(hr));
        }

        return true;
    }

    RenderBackendTextureHandle D3D12RenderBackend::GetActiveSwapChainBuffer(RenderBackendSwapChainHandle handle)
    {
        D3D12Device* device = devices[0];
        D3D12SwapChain* swapChain = device->GetSwapChain(handle);
        return swapChain->buffers[swapChain->GetCurrentBackBufferIndex()];
    }

    RenderBackendBufferHandle D3D12RenderBackend::CreateBuffer(const RenderBackendBufferDesc* desc, const void* data, const char* name)
    {
        RenderBackendBufferHandle handle = handleManager.Allocate<RenderBackendBufferHandle>();
        D3D12Device* device = devices[0];
        uint32 index = device->CreateD3D12Buffer(desc, data, name);
        device->SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        return handle;
    }

    void D3D12RenderBackend::ResizeBuffer(RenderBackendBufferHandle handle, uint64 size)
    {
        D3D12Device* device = devices[0];
        uint32 index = 0;
        if (device->TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
        {
            device->ResizeD3D12Buffer(index, size);
        }
    }

    void D3D12RenderBackend::MapBuffer(RenderBackendBufferHandle handle, void** data)
    {
        D3D12Device* device = devices[0];
        D3D12Buffer* buffer = device->GetBuffer(handle);

        buffer->resource->Map(0, nullptr, data);
        //*data = buffer->mappedData;
    }

    void D3D12RenderBackend::UnmapBuffer(RenderBackendBufferHandle handle)
    {
        D3D12Device* device = devices[0];
        D3D12Buffer* buffer = device->GetBuffer(handle);

        buffer->resource->Unmap(0, nullptr);
    }

    void D3D12RenderBackend::DestroyBuffer(RenderBackendBufferHandle handle)
    {

    }

    RenderBackendTextureHandle D3D12RenderBackend::CreateTexture(const RenderBackendTextureDesc* desc, const void* data, const char* name)
    {
        RenderBackendTextureHandle handle = handleManager.Allocate<RenderBackendTextureHandle>();
        D3D12Device* device = devices[0];
        uint32 index = device->CreateD3D12Texture(desc, data, name);
        device->SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        return handle;
    }

    void D3D12RenderBackend::DestroyTexture(RenderBackendTextureHandle texture)
    {

    }

    void D3D12RenderBackend::UploadTexture(RenderBackendTextureHandle handle, const RenderBackendTextureUploadDataDesc& data)
    {

    }

    void D3D12RenderBackend::GetTextureReadbackData(RenderBackendTextureHandle texture, void** data)
    {

    }

    //RenderBackendTextureSRVHandle D3D12RenderBackend::CreateTextureSRV(const RenderBackendTextureSRVDesc* desc, const char* name)
    //{
    //    return RenderBackendTextureSRVHandle::Null;
    //}

    //RenderBackendTextureUAVHandle D3D12RenderBackend::CreateTextureUAV(const RenderBackendTextureUAVDesc* desc, const char* name)
    //{
    //    return RenderBackendTextureUAVHandle::Null;
    //}

    int32 D3D12RenderBackend::GetTextureSRVBindlessResourceDescriptorIndex(RenderBackendTextureHandle handle)
    {
        D3D12Device* device = devices[0];
        uint32 textureIndex = 0;
        if (!device->TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &textureIndex))
        {
            return 0;
        }
        D3D12Texture* texture = device->textures[textureIndex];
        return texture->shaderResourceView->bindlessIndex;
    }

    int32 D3D12RenderBackend::GetTextureSRVBindlessResourceDescriptorIndex(RenderBackendTextureHandle handle, uint32 mipLevel)
    {
        D3D12Device* device = devices[0];
        uint32 textureIndex = 0;
        if (!device->TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &textureIndex))
        {
            return 0;
        }
        D3D12Texture* texture = device->textures[textureIndex];
        return texture->shaderResourceViews[mipLevel]->bindlessIndex;
    }

    int32 D3D12RenderBackend::GetTextureUAVBindlessResourceDescriptorIndex(RenderBackendTextureHandle handle, uint32 mipLevel)
    {
        D3D12Device* device = devices[0];
        uint32 textureIndex = 0;
        if (!device->TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &textureIndex))
        {
            return 0;
        }
        D3D12Texture* texture = device->textures[textureIndex];
        return texture->GetUnorderedAccessView(mipLevel)->bindlessIndex;
    }

    int32 D3D12RenderBackend::GetBufferCBVBindlessResourceDescriptorIndex(RenderBackendBufferHandle handle)
    {
        D3D12Device* device = devices[0];
        uint32 bufferIndex = 0;
        if (!device->TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &bufferIndex))
        {
            return 0;
        }
        D3D12Buffer* buffer = device->buffers[bufferIndex];
        return buffer->bindlessResourceDescriptorIndexCBV;
    }

    int32 D3D12RenderBackend::GetBufferSRVBindlessResourceDescriptorIndex(RenderBackendBufferHandle handle)
    {
        D3D12Device* device = devices[0];
        uint32 bufferIndex = 0;
        if (!device->TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &bufferIndex))
        {
            return 0;
        }
        D3D12Buffer* buffer = device->buffers[bufferIndex];
        return buffer->bindlessResourceDescriptorIndexSRV;
    }

    int32 D3D12RenderBackend::GetBufferUAVBindlessResourceDescriptorIndex(RenderBackendBufferHandle handle)
    {
        D3D12Device* device = devices[0];
        uint32 bufferIndex = 0;
        if (!device->TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &bufferIndex))
        {
            return 0;
        }
        D3D12Buffer* buffer = device->buffers[bufferIndex];
        return buffer->bindlessResourceDescriptorIndexUAV;
    }

    int32 D3D12RenderBackend::GetAccelerationStructureSRVBindlessResourceDescriptorIndex(RenderBackendRayTracingAccelerationStructureHandle handle)
    {
        D3D12Device* device = devices[0];
        uint32 index = 0;
        if (!device->TryGetRenderBackendHandleRepresentation(handle.GetIndex(), &index))
        {
            return 0;
        }
        D3D12RayTracingAccelerationStructure* accelerationStructure = device->accelerationStructures[index];
        return accelerationStructure->bindlessIndex;
    }

    RenderBackendSamplerHandle D3D12RenderBackend::CreateSampler(const RenderBackendSamplerDesc* desc, const char* name)
    {
        RenderBackendSamplerHandle handle = handleManager.Allocate<RenderBackendSamplerHandle>();
        D3D12Device* device = devices[0];
        uint32 index = device->CreateD3D12Sampler(desc, name);
        device->SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        return handle;
    }

    void D3D12RenderBackend::DestroySampler(RenderBackendSamplerHandle sampler)
    {

    }

    RenderBackendShaderHandle D3D12RenderBackend::CreateShader(const RenderBackendShaderDesc* desc, const char* name)
    {
        RenderBackendShaderHandle handle = handleManager.Allocate<RenderBackendShaderHandle>();
        D3D12Device* device = devices[0];
        uint32 index = device->CreateD3D12Shader(desc, name);
        device->SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        return handle;
    }

    void D3D12RenderBackend::DestroyShader(RenderBackendShaderHandle shader)
    {

    }

    RenderBackendTimingQueryHeapHandle D3D12RenderBackend::CreateTimingQueryHeap(const RenderBackendTimingQueryHeapDesc* desc, const char* name)
    {
        RenderBackendTimingQueryHeapHandle handle = handleManager.Allocate<RenderBackendTimingQueryHeapHandle>();
        D3D12Device* device = devices[0];
        uint32 index = device->CreateD3D12QueryHeap(desc, name);
        device->SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        return handle;
    }

    void D3D12RenderBackend::DestroyTimingQueryHeap(RenderBackendTimingQueryHeapHandle timingQueryHeap)
    {

    }

    void D3D12RenderBackend::SubmitCommandLists(RenderBackendCommandList** commandLists, uint32 numCommandLists, RenderBackendSwapChainHandle swapChainHandle)
    {
        OPTICK_EVENT();

        if (!commandLists || numCommandLists == 0)
        {
            return;
        }

        D3D12Device* device = devices[0];

        D3D12CommandQueueType queueType = D3D12CommandQueueType::Direct;

        D3D12CommandAllocator* commandAllocator = device->AllocateCommandAllocator(queueType);
        D3D12CommandList* commandList = device->AllocateCommandList(commandAllocator);

        ID3D12GraphicsCommandList6* graphicsCommandList6 = commandList->GetID3D12GraphicsCommandList6();

        ID3D12DescriptorHeap* descriptorHeaps[] = {
            device->resourceDescriptorHeap->GetID3D12DescriptorHeap(),
            device->samplerDescriptorHeap->GetID3D12DescriptorHeap(),
        };
        graphicsCommandList6->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

        for (uint32 i = 0; i < numCommandLists; i++)
        {
            D3D12RenderBackendCommandListContext context(device, queueType, commandList);
            if (!context.CompileRenderBackendCommands(*commandLists[i]->GetCommandContainer()))
            {
                // TODO
            }
        }

        graphicsCommandList6->Close();

        D3D12CommandQueue* commandQueue = device->GetCommandQueue(queueType);

        D3D12SubmissionWorkload* workload = new D3D12SubmissionWorkload();

        workload->commandQueue = commandQueue;
        workload->commandListsToExecute.push_back(commandList);
        workload->commandAllocatorsToRelease.push_back(commandAllocator);

        if (swapChainHandle)
        {
            D3D12SwapChain* swapChain = device->GetSwapChain(swapChainHandle);
            uint32 currentBackBufferIndex = swapChain->GetCurrentBackBufferIndex();
            ID3D12Fence* frameFence = swapChain->GetFrameFence(currentBackBufferIndex);

            workload->fencesToWait.push_back(D3D12SubmissionWorkload::Fence(frameFence, 0));
            workload->fencesToSignal.push_back(D3D12SubmissionWorkload::Fence(frameFence, 1));
        }

        for (auto& [fence, value] : workload->fencesToWait)
        {
            D3D12_CHECK(commandQueue->GetID3D12CommandQueue()->Wait(fence.Get(), value));
        }

        const uint32 numCommandListsToExecute = (uint32)workload->commandListsToExecute.size();
        if (numCommandListsToExecute > 0)
        {
            std::vector<ID3D12CommandList*> d3d12CommandLists;

            for (D3D12CommandList* commandList : workload->commandListsToExecute)
            {
                d3d12CommandLists.push_back(commandList->GetID3D12CommandList());
            }

            commandQueue->GetID3D12CommandQueue()->ExecuteCommandLists(numCommandListsToExecute, d3d12CommandLists.data());

            for (D3D12CommandList* commandList : workload->commandListsToExecute)
            {
                device->ReleaseCommandList(commandList);
            }
        }

        uint64 fenceValue = commandQueue->SignalFence();
        workload->completionFenceValue = fenceValue;

        for (auto& [fence, value] : workload->fencesToSignal)
        {
            D3D12_CHECK(commandQueue->GetID3D12CommandQueue()->Signal(fence.Get(), value));
        }

        device->workloads.push_back(workload);
    }

    RenderBackendRayTracingAccelerationStructureHandle D3D12RenderBackend::CreateRayTracingBottomLevelAccelerationStructure(const RenderBackendRayTracingBottomLevelAccelerationStructureDesc* desc, const char* name)
    {
        RenderBackendRayTracingAccelerationStructureHandle handle = handleManager.Allocate<RenderBackendRayTracingAccelerationStructureHandle>();
        D3D12Device* device = devices[0];
        uint32 index = device->CreateD3D12RayTracingBottomLevelAccelerationStructure(desc, name);
        device->SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        return handle;
    }

    RenderBackendRayTracingAccelerationStructureHandle D3D12RenderBackend::CreateRayTracingTopLevelAccelerationStructure(const RenderBackendRayTracingTopLevelAccelerationStructureDesc* desc, const char* name)
    {
        RenderBackendRayTracingAccelerationStructureHandle handle = handleManager.Allocate<RenderBackendRayTracingAccelerationStructureHandle>();
        D3D12Device* device = devices[0];
        uint32 index = device->CreateD3D12RayTracingTopLevelAccelerationStructure(desc, name);
        device->SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        return handle;
    }

    RenderBackendRayTracingPipelineStateHandle D3D12RenderBackend::CreateRayTracingPipelineState(const RenderBackendRayTracingPipelineStateDesc* desc, const char* name)
    {
        return RenderBackendRayTracingPipelineStateHandle::Null;
    }

    RenderBackendBufferHandle D3D12RenderBackend::CreateRayTracingShaderBindingTable(const RenderBackendRayTracingShaderBindingTableDesc* desc, const char* name)
    {
        return RenderBackendBufferHandle::Null;
    }

    bool D3D12Device::Init(D3D12RenderBackend* backend, D3D12Adapter* adapter)
    {
        D3D_FEATURE_LEVEL minimumFeatureLevel = D3D_FEATURE_LEVEL_12_2; // aka DirectX 12 Ultimate

        HRESULT hr = D3D12CreateDevice(adapter->GetIDXGIAdapter(), minimumFeatureLevel, IID_PPV_ARGS(&device));
        if (FAILED(hr))
        {
            LogError(GLogger, std::format("Error: Failed to create D3D12 device."));
            return false;
        }

        //mask = RenderBackendDeviceMask(0);
        mask =
        {
            .mask = 0U
        };

        CD3DX12FeatureSupport features;
        D3D12_CHECK(features.Init(device.Get()));

        if (backend->useDebugLayers)
        {
            ID3D12InfoQueue1* d3d12InfoQueue;
            if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&d3d12InfoQueue))))
            {
                d3d12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
                d3d12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
                d3d12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
                d3d12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_INFO, true);
                d3d12InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_MESSAGE, true);

                std::vector<D3D12_MESSAGE_SEVERITY> enabledSeverities;
                enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_CORRUPTION);
                enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_ERROR);
                enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_WARNING);
                enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_INFO);
                enabledSeverities.push_back(D3D12_MESSAGE_SEVERITY_MESSAGE);

                std::vector<D3D12_MESSAGE_ID> disabledMessages;
                //disabledMessages.push_back(D3D12_MESSAGE_ID_DRAW_EMPTY_SCISSOR_RECTANGLE);
                //disabledMessages.push_back(D3D12_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS);
                //disabledMessages.push_back(D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE);
                //disabledMessages.push_back(D3D12_MESSAGE_ID_CREATEPIPELINELIBRARY_DRIVERVERSIONMISMATCH);
                //disabledMessages.push_back(D3D12_MESSAGE_ID_CREATEPIPELINELIBRARY_ADAPTERVERSIONMISMATCH);
                //disabledMessages.push_back(D3D12_MESSAGE_ID_LOADPIPELINE_NAMENOTFOUND);

                D3D12_INFO_QUEUE_FILTER filter = {};
                filter.AllowList.NumSeverities = static_cast<UINT>(enabledSeverities.size());
                filter.AllowList.pSeverityList = enabledSeverities.data();
                filter.DenyList.NumIDs = static_cast<UINT>(disabledMessages.size());
                filter.DenyList.pIDList = disabledMessages.data();
                D3D12_CHECK(d3d12InfoQueue->AddStorageFilterEntries(&filter));

                DWORD messageCallbackCookie = 0;
                if (SUCCEEDED(d3d12InfoQueue->RegisterMessageCallback(D3D12MessageCallback, D3D12_MESSAGE_CALLBACK_IGNORE_FILTERS, nullptr, &messageCallbackCookie)))
                {

                }
                else
                {
                    assert(false);
                }

                d3d12InfoQueue->Release();
            }
        }

        D3D_FEATURE_LEVEL maxSupportedFeatureLevel = minimumFeatureLevel;

        constexpr D3D_FEATURE_LEVEL allFeatureLevels[] =
        {
            D3D_FEATURE_LEVEL_12_2,
            D3D_FEATURE_LEVEL_12_1,
            D3D_FEATURE_LEVEL_12_0,
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1,
            D3D_FEATURE_LEVEL_1_0_CORE,
            D3D_FEATURE_LEVEL_1_0_GENERIC
        };

        D3D12_FEATURE_DATA_FEATURE_LEVELS featureSupportData =
        {
            .NumFeatureLevels = _countof(allFeatureLevels),
            .pFeatureLevelsRequested = allFeatureLevels,
        };

        if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &featureSupportData, sizeof(featureSupportData))))
        {
            maxSupportedFeatureLevel = featureSupportData.MaxSupportedFeatureLevel;
        }

        LogInfo(GLogger, std::format(L"Max supported feature level {}.", GetD3DFeatureLevelWCHAR(maxSupportedFeatureLevel)));


        D3D12_FEATURE_DATA_SHADER_MODEL featureDataShaderModel =
        {
            .HighestShaderModel = D3D_SHADER_MODEL_NONE
        };

        constexpr D3D_SHADER_MODEL allShaderModels[] =
        {
            D3D_SHADER_MODEL_6_9,
            D3D_SHADER_MODEL_6_8,
            D3D_SHADER_MODEL_6_7,
            D3D_SHADER_MODEL_6_6,
            //D3D_SHADER_MODEL_6_5,
            //D3D_SHADER_MODEL_6_4,
            //D3D_SHADER_MODEL_6_3,
            //D3D_SHADER_MODEL_6_2,
            //D3D_SHADER_MODEL_6_1,
            //D3D_SHADER_MODEL_6_0,
            //D3D_SHADER_MODEL_5_1,
        };

        for (const D3D_SHADER_MODEL shaderModel : allShaderModels)
        {
            featureDataShaderModel.HighestShaderModel = shaderModel;
            if (SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &featureDataShaderModel, sizeof(featureDataShaderModel))))
            {
                break;
            }
        }

        if (featureDataShaderModel.HighestShaderModel == D3D_SHADER_MODEL_NONE)
        {
            // TODO
        }

        LogInfo(GLogger, std::format(L"Max supported shader model {}.", GetD3DShaderModelWCHAR(featureDataShaderModel.HighestShaderModel)));

        D3D12_FEATURE_DATA_D3D12_OPTIONS options = {};
        D3D12_CHECK(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &options, sizeof(options)));

        // See: https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_DynamicResources.html
        // ResourceDescriptorHeap/SamplerDescriptorHeap must be supported on devices that support both D3D12_RESOURCE_BINDING_TIER_3 and D3D_SHADER_MODEL_6_6
        if (options.ResourceBindingTier >= D3D12_RESOURCE_BINDING_TIER_3 && featureDataShaderModel.HighestShaderModel >= D3D_SHADER_MODEL_6_6)
        {
            LogInfo(GLogger, std::format("Bindless resources are supported."));
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
        device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));

        if (options5.RaytracingTier == D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
        {
            LogInfo(GLogger, std::format("DirectX Raytracing is not supported."));
        }
        else
        {
            LogInfo(GLogger, std::format("DirectX Raytracing is supported. Tier: {}.", (int)options5.RaytracingTier));

            device->QueryInterface(IID_PPV_ARGS(&device5));
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};
        device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &options7, sizeof(options7));

        if (options7.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
        {
            LogInfo(GLogger, std::format("Mesh shader is not supported."));
        }
        else
        {
            LogInfo(GLogger, std::format("Mesh shader is supported. Tier: {}.", (int)options7.MeshShaderTier));
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS9 options9 = {};
        device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS9, &options9, sizeof(options9));

        D3D12_FEATURE_DATA_D3D12_OPTIONS11 options11 = {};
        device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS11, &options11, sizeof(options11));

        D3D12_FEATURE_DATA_D3D12_OPTIONS18 options18 = {};
        device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS18, &options18, sizeof(options18));

        if (options18.RenderPassesValid != TRUE)
        {
            LogInfo(GLogger, std::format("Render Pass is not supported."));
        }
        else
        {
            LogInfo(GLogger, std::format("Render Pass is supported. Tier: {}.", GetD3D12RenderPassesTierName(options5.RenderPassesTier)));
        }

        D3D12_FEATURE_DATA_D3D12_OPTIONS21 options21 = {};
        device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS21, &options21, sizeof(options21));

        if (options21.WorkGraphsTier == D3D12_WORK_GRAPHS_TIER_NOT_SUPPORTED)
        {
            LogInfo(GLogger, std::format("Work graphs are not supported."));
        }
        else
        {
            LogInfo(GLogger, std::format("Work graphs are supported. Tier: {}.", (int)options21.WorkGraphsTier));
        }

        for (uint32 i = 0; i < (uint32)D3D12CommandQueueType::Count; i++)
        {
            commandQueues[i] = nullptr;
        }

        {
            D3D12CommandQueueType queueType = D3D12CommandQueueType::Direct;

            D3D12CommandQueue* commandQueue = new D3D12CommandQueue();
            commandQueue->queueType = queueType;

            D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {
                .Type = GetD3D12CommandListType(queueType),
                .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
                .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
                .NodeMask = mask.Get(),
            };
            D3D12_CHECK(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue->queue)));
            D3D12_CHECK(commandQueue->queue->SetName(L"DirectCommandQueue"));

            D3D12_CHECK(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&commandQueue->fence)));
            commandQueue->lastSignaledValue = 0;

            commandQueues[(uint32)queueType] = commandQueue;
        }
        {
            D3D12CommandQueueType queueType = D3D12CommandQueueType::Compute;

            D3D12CommandQueue* commandQueue = new D3D12CommandQueue();
            commandQueue->queueType = queueType;

            D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {
                .Type = GetD3D12CommandListType(queueType),
                .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
                .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
                .NodeMask = mask.Get(),
            };
            D3D12_CHECK(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue->queue)));
            D3D12_CHECK(commandQueue->queue->SetName(L"ComputeCommandQueue"));

            D3D12_CHECK(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&commandQueue->fence)));
            commandQueue->lastSignaledValue = 0;

            commandQueues[(uint32)queueType] = commandQueue;
        }
        {
            D3D12CommandQueueType queueType = D3D12CommandQueueType::Copy;

            D3D12CommandQueue* commandQueue = new D3D12CommandQueue();
            commandQueue->queueType = queueType;

            D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {
                .Type = GetD3D12CommandListType(queueType),
                .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
                .Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
                .NodeMask = mask.Get(),
            };
            D3D12_CHECK(device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue->queue)));
            D3D12_CHECK(commandQueue->queue->SetName(L"CopyCommandQueue"));

            D3D12_CHECK(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&commandQueue->fence)));
            commandQueue->lastSignaledValue = 0;

            commandQueues[(uint32)queueType] = commandQueue;
        }

        // Create command signatures
        {
            D3D12_INDIRECT_ARGUMENT_DESC dispatchIndirectArgumentDesc;
            dispatchIndirectArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH;

            D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc =
            {
                .ByteStride = sizeof(RenderBackendDispatchIndirectArguments),
                .NumArgumentDescs = 1,
                .pArgumentDescs = &dispatchIndirectArgumentDesc,
            };
            D3D12_CHECK(device->CreateCommandSignature(&commandSignatureDesc, nullptr, IID_PPV_ARGS(&dispatchIndirectCommandSignature)));
        }
        {
            D3D12_INDIRECT_ARGUMENT_DESC drawIndirectArgumentDesc;
            drawIndirectArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW;

            D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc =
            {
                .ByteStride = sizeof(RenderBackendDrawIndirectArguments),
                .NumArgumentDescs = 1,
                .pArgumentDescs = &drawIndirectArgumentDesc,
            };
            D3D12_CHECK(device->CreateCommandSignature(&commandSignatureDesc, nullptr, IID_PPV_ARGS(&drawIndirectCommandSignature)));
        }
        {
            D3D12_INDIRECT_ARGUMENT_DESC drawIndexedIndirectArgumentDesc;
            drawIndexedIndirectArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

            D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc =
            {
                .ByteStride = sizeof(RenderBackendDrawIndexedIndirectArguments),
                .NumArgumentDescs = 1,
                .pArgumentDescs = &drawIndexedIndirectArgumentDesc,
            };
            D3D12_CHECK(device->CreateCommandSignature(&commandSignatureDesc, nullptr, IID_PPV_ARGS(&drawIndexedIndirectCommandSignature)));
        }
        {
            D3D12_INDIRECT_ARGUMENT_DESC dispatchMeshIndirectArgumentDesc;
            dispatchMeshIndirectArgumentDesc.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH;

            D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc =
            {
                .ByteStride = sizeof(RenderBackendDispatchMeshIndirectArguments),
                .NumArgumentDescs = 1,
                .pArgumentDescs = &dispatchMeshIndirectArgumentDesc,
            };
            D3D12_CHECK(device->CreateCommandSignature(&commandSignatureDesc, nullptr, IID_PPV_ARGS(&dispatchMeshIndirectCommandSignature)));
        }

        resourceDescriptorAllocator.Init(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8192);
        samplerDescriptorAllocator.Init(this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 256);
        rtvDescriptorAllocator.Init(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1024);
        dsvDescriptorAllocator.Init(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 128);

        // Resource descriptor heap
        {
            resourceDescriptorHeap = new D3D12DescriptorHeap();

            D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
                .NumDescriptors = 1000000, // tier 1 limit
                .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                .NodeMask = mask.Get(),
            };
            D3D12_CHECK(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&resourceDescriptorHeap->descriptorHeap)));
            D3D12_CHECK(resourceDescriptorHeap->descriptorHeap->SetName(L"BindlessResourceDescriptorHeap"));

            resourceDescriptorHeap->cpuDescriptorHandle = resourceDescriptorHeap->GetID3D12DescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
            resourceDescriptorHeap->gpuDescriptorHandle = resourceDescriptorHeap->GetID3D12DescriptorHeap()->GetGPUDescriptorHandleForHeapStart();

            for (int i = 0; i < D3D12_RENDER_BACKEND_BINDLESS_MAX_NUM_RESOURCE_DESCRIPTOERS; i++)
            {
                freeResourceDescriptorIndices.push_back(D3D12_RENDER_BACKEND_BINDLESS_MAX_NUM_RESOURCE_DESCRIPTOERS - i - 1);
            }
        }

        // Sampler descriptor heap
        {
            samplerDescriptorHeap = new D3D12DescriptorHeap();

            D3D12_DESCRIPTOR_HEAP_DESC heapDesc =
            {
                .Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
                .NumDescriptors = 2048, // tier 1 limit
                .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
                .NodeMask = mask.Get(),
            };
            D3D12_CHECK(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&samplerDescriptorHeap->descriptorHeap)));
            D3D12_CHECK(samplerDescriptorHeap->descriptorHeap->SetName(L"BindlessSamplerDescriptorHeap"));

            samplerDescriptorHeap->cpuDescriptorHandle = samplerDescriptorHeap->GetID3D12DescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
            samplerDescriptorHeap->gpuDescriptorHandle = samplerDescriptorHeap->GetID3D12DescriptorHeap()->GetGPUDescriptorHandleForHeapStart();

            for (int i = 0; i < D3D12_RENDER_BACKEND_BINDLESS_MAX_NUM_SAMPLER_DESCRIPTOERS; i++)
            {
                freeSamplerDescriptorIndices.push_back(D3D12_RENDER_BACKEND_BINDLESS_MAX_NUM_SAMPLER_DESCRIPTOERS - i - 1);
            }
        }

        CD3DX12_DESCRIPTOR_RANGE1 descriptorRanges[2] = {};
        descriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, -1, 0, 1, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);
        descriptorRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, -1, 0, 2, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);

        CD3DX12_ROOT_PARAMETER1 rootParameters[2] = {};
        rootParameters[0].InitAsConstants(RenderBackendPushConstantsBytes / 4, 999, 0, D3D12_SHADER_VISIBILITY_ALL);
        rootParameters[1].InitAsDescriptorTable(2, &descriptorRanges[0], D3D12_SHADER_VISIBILITY_ALL);

        D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;
        rootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED;
        rootSignatureFlags |= D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED;

        D3D12_STATIC_SAMPLER_DESC staticSamplers[8] = {};
        {
            RenderBackendSamplerDesc desc = RenderBackendSamplerDesc::CreatePointWarp(0.0f, -FLT_MAX, FLT_MAX, 1);
            staticSamplers[0].Filter = ConvertToD3D12Filter(desc.filter);
            staticSamplers[0].AddressU = ConvertToD3D12TextureAddressMode(desc.addressModeU);
            staticSamplers[0].AddressV = ConvertToD3D12TextureAddressMode(desc.addressModeV);
            staticSamplers[0].AddressW = ConvertToD3D12TextureAddressMode(desc.addressModeW);
            staticSamplers[0].MipLODBias = desc.mipLodBias;
            staticSamplers[0].MaxAnisotropy = desc.maxAnisotropy;
            staticSamplers[0].ComparisonFunc = ConvertToD3D12ComparisonFunc(desc.compareOp);
            staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            staticSamplers[0].MinLOD = desc.minLod;
            staticSamplers[0].MaxLOD = desc.maxLod;
            staticSamplers[0].ShaderRegister = 100;
            staticSamplers[0].RegisterSpace = 0;
            staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        }
        {
            RenderBackendSamplerDesc desc = RenderBackendSamplerDesc::CreatePointClamp(0.0f, -FLT_MAX, FLT_MAX, 1);
            staticSamplers[1].Filter = ConvertToD3D12Filter(desc.filter);
            staticSamplers[1].AddressU = ConvertToD3D12TextureAddressMode(desc.addressModeU);
            staticSamplers[1].AddressV = ConvertToD3D12TextureAddressMode(desc.addressModeV);
            staticSamplers[1].AddressW = ConvertToD3D12TextureAddressMode(desc.addressModeW);
            staticSamplers[1].MipLODBias = desc.mipLodBias;
            staticSamplers[1].MaxAnisotropy = desc.maxAnisotropy;
            staticSamplers[1].ComparisonFunc = ConvertToD3D12ComparisonFunc(desc.compareOp);
            staticSamplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            staticSamplers[1].MinLOD = desc.minLod;
            staticSamplers[1].MaxLOD = desc.maxLod;
            staticSamplers[1].ShaderRegister = 101;
            staticSamplers[1].RegisterSpace = 0;
            staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        }
        {
            RenderBackendSamplerDesc desc = RenderBackendSamplerDesc::CreatePointBorder(0.0f, -FLT_MAX, FLT_MAX, 1);
            staticSamplers[2].Filter = ConvertToD3D12Filter(desc.filter);
            staticSamplers[2].AddressU = ConvertToD3D12TextureAddressMode(desc.addressModeU);
            staticSamplers[2].AddressV = ConvertToD3D12TextureAddressMode(desc.addressModeV);
            staticSamplers[2].AddressW = ConvertToD3D12TextureAddressMode(desc.addressModeW);
            staticSamplers[2].MipLODBias = desc.mipLodBias;
            staticSamplers[2].MaxAnisotropy = desc.maxAnisotropy;
            staticSamplers[2].ComparisonFunc = ConvertToD3D12ComparisonFunc(desc.compareOp);
            staticSamplers[2].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            staticSamplers[2].MinLOD = desc.minLod;
            staticSamplers[2].MaxLOD = desc.maxLod;
            staticSamplers[2].ShaderRegister = 102;
            staticSamplers[2].RegisterSpace = 0;
            staticSamplers[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        }
        {
            RenderBackendSamplerDesc desc = RenderBackendSamplerDesc::CreateLinearWarp(0.0f, -FLT_MAX, FLT_MAX, 1);
            staticSamplers[3].Filter = ConvertToD3D12Filter(desc.filter);
            staticSamplers[3].AddressU = ConvertToD3D12TextureAddressMode(desc.addressModeU);
            staticSamplers[3].AddressV = ConvertToD3D12TextureAddressMode(desc.addressModeV);
            staticSamplers[3].AddressW = ConvertToD3D12TextureAddressMode(desc.addressModeW);
            staticSamplers[3].MipLODBias = desc.mipLodBias;
            staticSamplers[3].MaxAnisotropy = desc.maxAnisotropy;
            staticSamplers[3].ComparisonFunc = ConvertToD3D12ComparisonFunc(desc.compareOp);
            staticSamplers[3].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            staticSamplers[3].MinLOD = desc.minLod;
            staticSamplers[3].MaxLOD = desc.maxLod;
            staticSamplers[3].ShaderRegister = 103;
            staticSamplers[3].RegisterSpace = 0;
            staticSamplers[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        }
        {
            RenderBackendSamplerDesc desc = RenderBackendSamplerDesc::CreateLinearClamp(0.0f, -FLT_MAX, FLT_MAX, 1);
            staticSamplers[4].Filter = ConvertToD3D12Filter(desc.filter);
            staticSamplers[4].AddressU = ConvertToD3D12TextureAddressMode(desc.addressModeU);
            staticSamplers[4].AddressV = ConvertToD3D12TextureAddressMode(desc.addressModeV);
            staticSamplers[4].AddressW = ConvertToD3D12TextureAddressMode(desc.addressModeW);
            staticSamplers[4].MipLODBias = desc.mipLodBias;
            staticSamplers[4].MaxAnisotropy = desc.maxAnisotropy;
            staticSamplers[4].ComparisonFunc = ConvertToD3D12ComparisonFunc(desc.compareOp);
            staticSamplers[4].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            staticSamplers[4].MinLOD = desc.minLod;
            staticSamplers[4].MaxLOD = desc.maxLod;
            staticSamplers[4].ShaderRegister = 104;
            staticSamplers[4].RegisterSpace = 0;
            staticSamplers[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        }
        {
            RenderBackendSamplerDesc desc = RenderBackendSamplerDesc::CreateLinearBorder(0.0f, -FLT_MAX, FLT_MAX, 1);
            staticSamplers[5].Filter = ConvertToD3D12Filter(desc.filter);
            staticSamplers[5].AddressU = ConvertToD3D12TextureAddressMode(desc.addressModeU);
            staticSamplers[5].AddressV = ConvertToD3D12TextureAddressMode(desc.addressModeV);
            staticSamplers[5].AddressW = ConvertToD3D12TextureAddressMode(desc.addressModeW);
            staticSamplers[5].MipLODBias = desc.mipLodBias;
            staticSamplers[5].MaxAnisotropy = desc.maxAnisotropy;
            staticSamplers[5].ComparisonFunc = ConvertToD3D12ComparisonFunc(desc.compareOp);
            staticSamplers[5].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            staticSamplers[5].MinLOD = desc.minLod;
            staticSamplers[5].MaxLOD = desc.maxLod;
            staticSamplers[5].ShaderRegister = 105;
            staticSamplers[5].RegisterSpace = 0;
            staticSamplers[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        }
        {
            RenderBackendSamplerDesc desc = RenderBackendSamplerDesc::CreateComparisonLinearClamp(0.0f, -FLT_MAX, FLT_MAX, 0, RenderBackendCompareOp::Greater);
            staticSamplers[6].Filter = ConvertToD3D12Filter(desc.filter);
            staticSamplers[6].AddressU = ConvertToD3D12TextureAddressMode(desc.addressModeU);
            staticSamplers[6].AddressV = ConvertToD3D12TextureAddressMode(desc.addressModeV);
            staticSamplers[6].AddressW = ConvertToD3D12TextureAddressMode(desc.addressModeW);
            staticSamplers[6].MipLODBias = desc.mipLodBias;
            staticSamplers[6].MaxAnisotropy = desc.maxAnisotropy;
            staticSamplers[6].ComparisonFunc = ConvertToD3D12ComparisonFunc(desc.compareOp);
            staticSamplers[6].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            staticSamplers[6].MinLOD = desc.minLod;
            staticSamplers[6].MaxLOD = desc.maxLod;
            staticSamplers[6].ShaderRegister = 106;
            staticSamplers[6].RegisterSpace = 0;
            staticSamplers[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        }
        {
            RenderBackendSamplerDesc desc = RenderBackendSamplerDesc::CreateComparisonLinearClamp(0.0f, -FLT_MAX, FLT_MAX, 1, RenderBackendCompareOp::Less);
            staticSamplers[7].Filter = ConvertToD3D12Filter(desc.filter);
            staticSamplers[7].AddressU = ConvertToD3D12TextureAddressMode(desc.addressModeU);
            staticSamplers[7].AddressV = ConvertToD3D12TextureAddressMode(desc.addressModeV);
            staticSamplers[7].AddressW = ConvertToD3D12TextureAddressMode(desc.addressModeW);
            staticSamplers[7].MipLODBias = desc.mipLodBias;
            staticSamplers[7].MaxAnisotropy = desc.maxAnisotropy;
            staticSamplers[7].ComparisonFunc = ConvertToD3D12ComparisonFunc(desc.compareOp);
            staticSamplers[7].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
            staticSamplers[7].MinLOD = desc.minLod;
            staticSamplers[7].MaxLOD = desc.maxLod;
            staticSamplers[7].ShaderRegister = 107;
            staticSamplers[7].RegisterSpace = 0;
            staticSamplers[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        }

        CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignatureDesc;
        versionedRootSignatureDesc.Init_1_1(_countof(rootParameters), rootParameters, _countof(staticSamplers), staticSamplers, rootSignatureFlags);

        ID3DBlob* serializedRootSignature;
        D3D12_CHECK(D3D12SerializeVersionedRootSignature(&versionedRootSignatureDesc, &serializedRootSignature, nullptr));

        D3D12_CHECK(device->CreateRootSignature(
            0,
            serializedRootSignature->GetBufferPointer(), serializedRootSignature->GetBufferSize(),
            IID_PPV_ARGS(&rootSignature)));
        D3D12_CHECK(rootSignature->SetName(L"BindlessRootSignature"));

        D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
        allocatorDesc.pDevice = device.Get();
        allocatorDesc.pAdapter = adapter->GetIDXGIAdapter();
        //allocatorDesc.PreferredBlockSize = 256 * 1024 * 1024;
        //allocatorDesc.Flags |= D3D12MA::ALLOCATOR_FLAG_ALWAYS_COMMITTED;
        allocatorDesc.Flags = D3D12MA::ALLOCATOR_FLAGS(D3D12MA::ALLOCATOR_FLAG_DEFAULT_POOLS_NOT_ZEROED | D3D12MA::ALLOCATOR_FLAG_MSAA_TEXTURES_ALWAYS_COMMITTED);

        D3D12_CHECK(D3D12MA::CreateAllocator(&allocatorDesc, &allocator));

        return true;
    }

    void D3D12Device::Exit()
    {
        WaitIdle();

        delete resourceDescriptorHeap;
        delete samplerDescriptorHeap;

        for (uint32 index = 0; index < (uint32)D3D12CommandQueueType::Count; index++)
        {
            if (commandQueues[(uint32)index])
            {
                delete commandQueues[(uint32)index];
            }
        }
    }

    RenderBackend* RenderBackendCreateD3D12(const D3D12RenderBackendDesc* desc)
    {
        D3D12RenderBackend* d3d12Backend = new D3D12RenderBackend();
        if (!d3d12Backend->Init(desc))
        {
            delete d3d12Backend;
            return nullptr;
        }
        return d3d12Backend;
    }

    void RenderBackendDestroyD3D12(RenderBackend* backend)
    {
        D3D12RenderBackend* d3d12Backend = (D3D12RenderBackend*)backend;
        d3d12Backend->Exit();
        delete d3d12Backend;
    }
}