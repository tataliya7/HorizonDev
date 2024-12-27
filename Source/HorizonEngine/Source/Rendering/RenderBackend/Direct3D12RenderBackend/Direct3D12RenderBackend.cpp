#include "Direct3D12RenderBackend.h"
#include "Direct3D12RenderBackendDefinitions.h"
#include "Direct3D12RenderBackendUtility.h"
#include "Direct3D12RenderBackendPrivate.h"

#include <optick.h>
#include <pix.h>

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

                    // LogVerbose(GLogger, std::format("Processing Texture State Transition: {}, initial stateObject: {}, stateObject before: {}, stateObject after: {}, firstLevel: {}, mipLevels: {}, firstLayer: {}, arraySlices: {}",
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

        D3D12RayTracingPipelineStateObject* rayTracingPipelineStateObject = device->GetRayTracingPipelineStateObject(command.pipelineStateObject);
        if (activeRayTracingPipeline != rayTracingPipelineStateObject->GetID3D12StateObject())
        {
            commandList->GetID3D12GraphicsCommandList4()->SetPipelineState1(rayTracingPipelineStateObject->GetID3D12StateObject());
            activeRayTracingPipeline = rayTracingPipelineStateObject->GetID3D12StateObject();
        }

        if (RenderBackendPushConstantsBytes > 0)
        {
            const void* pushConstantsData = &command.shaderConstants.data;
            commandList->GetID3D12GraphicsCommandList4()->SetComputeRoot32BitConstants(
                0, // TODO
                RenderBackendPushConstantsBytes / 4,
                pushConstantsData,
                0);
        }

        D3D12Buffer* sbtBuffer = device->GetBuffer(command.shaderBindingTable);

        D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = {};
        dispatchRaysDesc.RayGenerationShaderRecord = sbtBuffer->shaderBindingTable.rayGenerationShaderRecord;
        dispatchRaysDesc.MissShaderTable = sbtBuffer->shaderBindingTable.missShaderTable;
        dispatchRaysDesc.HitGroupTable = sbtBuffer->shaderBindingTable.hitGroupTable;
        dispatchRaysDesc.CallableShaderTable = sbtBuffer->shaderBindingTable.callableShaderTable;
        dispatchRaysDesc.Width = command.width;
        dispatchRaysDesc.Height = command.height;
        dispatchRaysDesc.Depth = command.depth;

        commandList->GetID3D12GraphicsCommandList4()->DispatchRays(&dispatchRaysDesc);
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
        // if (!PrepareForMeshShading(command.amplificationShader, command.meshShader, command.pixelShader, command.pipelineStateObject, command.topology, command.shaderConstants))
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
        // if (!PrepareForMeshShading(command.amplificationShader, command.meshShader, command.pixelShader, command.pipelineStateObject, command.topology, command.shaderConstants))
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

        // TODO: restore pipeline stateObject
        ID3D12DescriptorHeap* descriptorHeaps[] =
        {
            device->resourceDescriptorHeap->GetID3D12DescriptorHeap(),
            device->samplerDescriptorHeap->GetID3D12DescriptorHeap(),
        };
        commandList->GetID3D12GraphicsCommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

        commandList->GetID3D12GraphicsCommandList6()->SetGraphicsRootSignature(device->GetID3D12RootSignature());
        commandList->GetID3D12GraphicsCommandList6()->SetComputeRootSignature(device->GetID3D12RootSignature());
        if (device->backend->enableRayTracingSupport)
        {
            commandList->GetID3D12GraphicsCommandList4()->SetComputeRootSignature(device->GetID3D12RootSignature());
        }

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

        enableRayTracingSupport = desc->useHardwareRayTracing;

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

        commandList->GetID3D12GraphicsCommandList6()->SetGraphicsRootSignature(device->GetID3D12RootSignature());
        commandList->GetID3D12GraphicsCommandList6()->SetComputeRootSignature(device->GetID3D12RootSignature());
        if (device->backend->enableRayTracingSupport)
        {
            commandList->GetID3D12GraphicsCommandList4()->SetComputeRootSignature(device->GetID3D12RootSignature());
        }

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
        RenderBackendRayTracingPipelineStateHandle handle = handleManager.Allocate<RenderBackendRayTracingPipelineStateHandle>();
        D3D12Device* device = devices[0];
        uint32 index = device->CreateD3D12RayTracingPipelineState(desc, name);
        device->SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        return handle;
    }

    RenderBackendBufferHandle D3D12RenderBackend::CreateRayTracingShaderBindingTable(const RenderBackendRayTracingShaderBindingTableDesc* desc, const char* name)
    {
        RenderBackendBufferHandle handle = handleManager.Allocate<RenderBackendBufferHandle>();
        D3D12Device* device = devices[0];
        uint32 index = device->CreateD3D12ShaderBindingTable(desc, name);
        device->SetRenderBackendHandleRepresentation(handle.GetIndex(), index);
        return handle;
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