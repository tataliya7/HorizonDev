#include "RenderSystem.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>

#include <optick.h>

namespace Horizon
{
    RenderSystem::RenderSystem()
    {

    }

    RenderSystem::~RenderSystem()
    {

    }

    void RenderSystem::Init()
    {
        // TODO: make it configurable
        RenderBackendType renderBackendType = RenderBackendType::Vulkan;
        bool enableDebugLayers = true;
        bool enableHardwareRayTracing = false;

        if (renderBackendType == RenderBackendType::Vulkan)
        {
            int flags = VULKAN_RENDER_BACKEND_CREATE_FLAGS_SURFACE;
            if (enableDebugLayers)
            {
                flags |= VULKAN_RENDER_BACKEND_CREATE_FLAGS_VALIDATION_LAYERS;
            }
            if (enableHardwareRayTracing)
            {
                flags |= VULKAN_RENDER_BACKEND_CREATE_FLAGS_RAY_TRACING;
            }
            renderBackend = RenderBackendCreateVulkan(flags);
        }
        else if (renderBackendType == RenderBackendType::D3D12)
        {
            D3D12RenderBackendDesc d3d12RenderBackendDesc = {
                .useDebugLayers = enableDebugLayers,
                .useGPUBasedValidation = enableDebugLayers,
            };
            renderBackend = RenderBackendCreateD3D12(&d3d12RenderBackendDesc);
        }
        else
        {
            LogError(GLogger, std::format("Unknown RenderBackendType!"));
        }

        // Currently, multiple devices are not supported.
        uint32 primaryDeviceMask = 0;
        uint32 physicalDeviceID = 0;
        renderBackend->CreateRenderDevices(&physicalDeviceID, 1, &primaryDeviceMask);

        shaderLibrary = new ShaderLibrary(renderBackend, "../../../Source/HorizonEngine/Shaders");
        LoadAllShaders_Deprecated(shaderLibrary);

        renderGraphResourcePool = new RenderGraphResourcePool(renderBackend);

        gpuProfiler = new RenderBackendGPUProfiler(renderBackend);

        RenderBackendCommandList* commandList = new RenderBackendCommandList(GArena);
        rendererDefaultResources = new RendererDefaultResources(renderBackend, renderGraphResourcePool, shaderLibrary);
        rendererDefaultResources->Initialize(*commandList);
        renderBackend->SubmitCommandLists(&commandList, 1, RenderBackendSwapChainHandle::Null);

        renderBackend->FlushRenderDevices();

        for (uint32 t = 0; t < 3; t++)
        {
            {
                RenderBackendBufferDesc vertexBufferDesc = RenderBackendBufferDesc::CreateByteAddress(4);
                vertexBuffer[t] = renderBackend->CreateBuffer(&vertexBufferDesc, nullptr, "ImGuiVertexBuffer");

                RenderBackendBufferDesc vertexBufferUploadDesc = RenderBackendBufferDesc::CreateUpload(4);
                vertexBufferUpload[t] = renderBackend->CreateBuffer(&vertexBufferUploadDesc, nullptr, "ImGuiVertexBufferUpload");

                vertexBufferSize[t] = 4;
            }
            {
                RenderBackendBufferDesc indexBufferDesc = RenderBackendBufferDesc::CreateIndex(sizeof(uint32), 1);
                indexBuffer[t] = renderBackend->CreateBuffer(&indexBufferDesc, nullptr, "ImGuiIndexBuffer");

                RenderBackendBufferDesc indexBufferUploadDesc = RenderBackendBufferDesc::CreateUpload(4);
                indexBufferUpload[t] = renderBackend->CreateBuffer(&indexBufferUploadDesc, nullptr, "ImGuiIndexBufferUpload");

                indexBufferSize[t] = 4;
            }
            {
                RenderBackendBufferDesc drawDataBufferDesc = RenderBackendBufferDesc::CreateByteAddress(4);
                drawDataBuffer[t] = renderBackend->CreateBuffer(&drawDataBufferDesc, nullptr, "ImGuiDrawDataBuffer");

                RenderBackendBufferDesc drawDataBufferUploadDesc = RenderBackendBufferDesc::CreateUpload(4);
                drawDataBufferUpload[t] = renderBackend->CreateBuffer(&drawDataBufferUploadDesc, nullptr, "ImGuiDrawDataBufferUpload");

                drawDataBufferSize[t] = 4;
            }
            {
                RenderBackendBufferDesc drawIndexedIndirectCommandBufferDesc = RenderBackendBufferDesc::CreateIndirectArguments(4);
                drawIndexedIndirectCommandBuffer[t] = renderBackend->CreateBuffer(&drawIndexedIndirectCommandBufferDesc, nullptr, "ImGuiDrawIndexedIndirectCommandBuffer");

                RenderBackendBufferDesc drawIndexedIndirectCommandBufferUploadDesc = RenderBackendBufferDesc::CreateUpload(4);
                drawIndexedIndirectCommandBufferUpload[t] = renderBackend->CreateBuffer(&drawIndexedIndirectCommandBufferUploadDesc, nullptr, "ImGuiDrawIndexedIndirectCommandBufferUpload");

                drawIndexedIndirectCommandBufferSize[t] = 4;
            }
        }
    }

    void RenderSystem::Exit()
    {
        RenderBackendType renderBackendType = renderBackend->GetType();
        if (renderBackendType == RenderBackendType::Vulkan)
        {
            RenderBackendDestroyVulkan(renderBackend);
        }
        else if (renderBackendType == RenderBackendType::D3D12)
        {
            RenderBackendDestroyD3D12(renderBackend);
        }
    }

    void RenderSystem::Tick(float deltaTimeInSeconds)
    {
        shaderLibrary->HotReload();
        renderGraphResourcePool->Tick();



    }

    RealTimeRenderer* RenderSystem::CreateRenderer()
    {
        return new RealTimeRenderer(renderBackend, renderGraphResourcePool, shaderLibrary, rendererDefaultResources);
    }

    void RenderSystem::RenderSceneView(RealTimeRenderer* renderer, SceneView* sceneView)
    {
        RenderBackendCommandList* commandListUpload = new RenderBackendCommandList(GArena);
        UpdateImGuiData(commandListUpload);
        renderBackend->SubmitCommandLists(&commandListUpload, 1, RenderBackendSwapChainHandle::Null);
        delete commandListUpload;

        RenderBackendCommandList* commandList = new RenderBackendCommandList(GArena);

        sceneView->scene->UpdateGPUScene(commandList);

        gpuProfiler->BeginFrame(commandList);

        uint32 frameTimingQueryRegion = gpuProfiler->BeginRegion(commandList, "GPU Frametime");

        renderer->InitializeSceneView(sceneView);

        RenderGraph renderGraph(GArena, renderGraphResourcePool, gpuProfiler);

        renderer->Render(renderGraph);

        RenderUserInterface(renderGraph, *sceneView);

        renderGraph.Execute(*commandList);

        gpuProfiler->EndRegion(frameTimingQueryRegion, commandList);
        gpuProfiler->EndFrame(commandList);

        renderBackend->SubmitCommandLists(&commandList, 1, RenderBackendSwapChainHandle::Null);

        delete commandList;
    }

    void RenderSystem::BeginDrawUI(ImGuiContext* context)
    {
        OPTICK_EVENT();

        ImGui::SetCurrentContext(context);
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    struct ImGuiDrawData
    {
        int textureID;
        int vertexOffset;
        int scissorOffset;
        int scissorSize;
        Vector2 scale;
        Vector2 translate;
    };

    void RenderSystem::EndDrawUI()
    {
        OPTICK_EVENT();

        ImGui::EndFrame();
        ImGui::Render();

        ImDrawData* drawData = ImGui::GetDrawData();
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fbWidth = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
        int fbHeight = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
        if (fbWidth <= 0 || fbHeight <= 0)
        {
            return;
        }

        // Will project scissor/clipping rectangles into framebuffer space
        ImVec2 clipOffset = drawData->DisplayPos;         // (0,0) unless using multi-viewports
        ImVec2 clipScale = drawData->FramebufferScale;    // (1,1) unless using retina display which are often (2,2)

        totalDrawCommandCount = 0;
        std::vector<ImGuiDrawData> drawDataArray;
        std::vector<RenderBackendDrawIndexedIndirectArguments> drawIndexedIndirectArguments;

        // (Because we merged all buffers into a single one, we maintain our own offset into them)
        int globalIndexOffset = 0;
        int globalVertexOffset = 0;
        for (int i = 0; i < drawData->CmdListsCount; i++)
        {
            const ImDrawList* cmdList = drawData->CmdLists[i];
            for (int drawCallIndex = 0; drawCallIndex < cmdList->CmdBuffer.Size; drawCallIndex++)
            {
                const ImDrawCmd* pcmd = &cmdList->CmdBuffer[drawCallIndex];

                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clipMin((pcmd->ClipRect.x - clipOffset.x) * clipScale.x, (pcmd->ClipRect.y - clipOffset.y) * clipScale.y);
                ImVec2 clipMax((pcmd->ClipRect.z - clipOffset.x) * clipScale.x, (pcmd->ClipRect.w - clipOffset.y) * clipScale.y);

                // Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
                if (clipMin.x < 0.0f) { clipMin.x = 0.0f; }
                if (clipMin.y < 0.0f) { clipMin.y = 0.0f; }
                if (clipMax.x > fbWidth) { clipMax.x = (float)fbWidth; }
                if (clipMax.y > fbHeight) { clipMax.y = (float)fbHeight; }
                if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
                {
                    continue;
                }

                int textureID = renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(RenderBackendTextureHandle(pcmd->TextureId));
                int vertexOffset = pcmd->VtxOffset + globalVertexOffset;
                Vector2 scale = Vector2(2.0f / drawData->DisplaySize.x, -2.0f / drawData->DisplaySize.y);
                Vector2 translate = Vector2(-1.0f - drawData->DisplayPos.x * scale.x, 1.0f + drawData->DisplayPos.y * scale.y);

                int32 clipMin_x_int = int32(clipMin.x);
                int32 clipMin_y_int = int32(clipMin.y);
                int32 scissor_w_int = int32(clipMax.x - clipMin.x);
                int32 scissor_h_int = int32(clipMax.y - clipMin.y);

                ImGuiDrawData& drawCommandData = drawDataArray.emplace_back();
                drawCommandData.textureID = textureID;
                drawCommandData.vertexOffset = vertexOffset;
                drawCommandData.scissorOffset = ((clipMin_x_int & 0xFFFF) << 16) | (clipMin_y_int & 0xFFFF);
                drawCommandData.scissorSize = ((scissor_w_int & 0xFFFF) << 16) | (scissor_h_int & 0xFFFF);
                drawCommandData.scale = scale;
                drawCommandData.translate = translate;

                RenderBackendDrawIndexedIndirectArguments& arguments = drawIndexedIndirectArguments.emplace_back();
                arguments.numIndices = pcmd->ElemCount;
                arguments.numInstances = 1;
                arguments.firstIndex = pcmd->IdxOffset + globalIndexOffset;
                arguments.vertexOffset = 0;
                arguments.firstInstance = totalDrawCommandCount;

                totalDrawCommandCount++;
            }
            globalIndexOffset += cmdList->IdxBuffer.Size;
            globalVertexOffset += cmdList->VtxBuffer.Size;
        }

        frameInFlightCounter = (frameInFlightCounter + 1) % 3;

        currentVertexBufferDataSize[frameInFlightCounter] = 0;
        currentIndexBufferDataSize[frameInFlightCounter] = 0;
        currentDrawDataBufferDataSize[frameInFlightCounter] = 0;
        currentDrawIndexedIndirectCommandBufferDataSize[frameInFlightCounter] = 0;
        if (drawData->TotalVtxCount > 0)
        {
            // Create or reserve the vertex/index buffers
            currentVertexBufferDataSize[frameInFlightCounter] = drawData->TotalVtxCount * sizeof(ImDrawVert);
            if (vertexBufferSize[frameInFlightCounter] < currentVertexBufferDataSize[frameInFlightCounter])
            {
                renderBackend->ResizeBuffer(vertexBuffer[frameInFlightCounter], currentVertexBufferDataSize[frameInFlightCounter]);
                renderBackend->ResizeBuffer(vertexBufferUpload[frameInFlightCounter], currentVertexBufferDataSize[frameInFlightCounter]);
                vertexBufferSize[frameInFlightCounter] = currentVertexBufferDataSize[frameInFlightCounter];
            }

            currentIndexBufferDataSize[frameInFlightCounter] = drawData->TotalIdxCount * sizeof(ImDrawIdx);
            if (indexBufferSize[frameInFlightCounter] < currentIndexBufferDataSize[frameInFlightCounter])
            {
                renderBackend->ResizeBuffer(indexBuffer[frameInFlightCounter], currentIndexBufferDataSize[frameInFlightCounter]);
                renderBackend->ResizeBuffer(indexBufferUpload[frameInFlightCounter], currentIndexBufferDataSize[frameInFlightCounter]);
                indexBufferSize[frameInFlightCounter] = currentIndexBufferDataSize[frameInFlightCounter];
            }

            currentDrawDataBufferDataSize[frameInFlightCounter] = totalDrawCommandCount * sizeof(ImGuiDrawData);
            if (drawDataBufferSize[frameInFlightCounter] < currentDrawDataBufferDataSize[frameInFlightCounter])
            {
                renderBackend->ResizeBuffer(drawDataBuffer[frameInFlightCounter], currentDrawDataBufferDataSize[frameInFlightCounter]);
                renderBackend->ResizeBuffer(drawDataBufferUpload[frameInFlightCounter], currentDrawDataBufferDataSize[frameInFlightCounter]);
                drawDataBufferSize[frameInFlightCounter] = currentDrawDataBufferDataSize[frameInFlightCounter];
            }

            currentDrawIndexedIndirectCommandBufferDataSize[frameInFlightCounter] = totalDrawCommandCount * sizeof(RenderBackendDrawIndexedIndirectArguments);
            if (drawIndexedIndirectCommandBufferSize[frameInFlightCounter] < currentDrawIndexedIndirectCommandBufferDataSize[frameInFlightCounter])
            {
                renderBackend->ResizeBuffer(drawIndexedIndirectCommandBuffer[frameInFlightCounter], currentDrawIndexedIndirectCommandBufferDataSize[frameInFlightCounter]);
                renderBackend->ResizeBuffer(drawIndexedIndirectCommandBufferUpload[frameInFlightCounter], currentDrawIndexedIndirectCommandBufferDataSize[frameInFlightCounter]);
                drawIndexedIndirectCommandBufferSize[frameInFlightCounter] = currentDrawIndexedIndirectCommandBufferDataSize[frameInFlightCounter];
            }

            uint32 vertexOffset = 0;
            uint32 indexOffset = 0;

            void* vertexBufferDataPtr;
            renderBackend->MapBuffer(vertexBufferUpload[frameInFlightCounter], &vertexBufferDataPtr);

            void* indexBufferDataPtr;
            renderBackend->MapBuffer(indexBufferUpload[frameInFlightCounter], &indexBufferDataPtr);

            ImDrawVert* vtx_dst = (ImDrawVert*)vertexBufferDataPtr;
            ImDrawIdx* idx_dst = (ImDrawIdx*)indexBufferDataPtr;
            for (int i = 0; i < drawData->CmdListsCount; i++)
            {
                const ImDrawList* cmdList = drawData->CmdLists[i];
                memcpy(vtx_dst + vertexOffset, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
                memcpy(idx_dst + indexOffset, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
                vertexOffset += cmdList->VtxBuffer.Size;
                indexOffset += cmdList->IdxBuffer.Size;
            }
            renderBackend->UnmapBuffer(vertexBufferUpload[frameInFlightCounter]);
            renderBackend->UnmapBuffer(indexBufferUpload[frameInFlightCounter]);

            renderBackend->UpdateBuffer(drawDataBufferUpload[frameInFlightCounter], 0, drawDataArray.data(), currentDrawDataBufferDataSize[frameInFlightCounter]);
            renderBackend->UpdateBuffer(drawIndexedIndirectCommandBufferUpload[frameInFlightCounter], 0, drawIndexedIndirectArguments.data(), currentDrawIndexedIndirectCommandBufferDataSize[frameInFlightCounter]);
        }
    }

    void RenderSystem::UpdateImGuiData(RenderBackendCommandList* commandList)
    {
        // Update vertex buffer and index buffer for ImGui
        {
            if (currentVertexBufferDataSize[frameInFlightCounter] > 0)
            {
                RenderBackendBarrier barrier1[] =
                {
                    RenderBackendBarrier(vertexBuffer[frameInFlightCounter], RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::Undefined, RenderBackendResourceState::CopyDst)
                };
                commandList->Transitions(barrier1, 1);
                commandList->CopyBuffer(
                    vertexBufferUpload[frameInFlightCounter],
                    0,
                    vertexBuffer[frameInFlightCounter],
                    0,
                    currentVertexBufferDataSize[frameInFlightCounter]);
                RenderBackendBarrier barrier2[] =
                {
                    RenderBackendBarrier(vertexBuffer[frameInFlightCounter], RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::ShaderResource)
                };
                commandList->Transitions(barrier2, 1);
            }

            if (currentIndexBufferDataSize[frameInFlightCounter] > 0)
            {
                RenderBackendBarrier barrier1[] =
                {
                    RenderBackendBarrier(indexBuffer[frameInFlightCounter], RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::Undefined, RenderBackendResourceState::CopyDst)
                };
                commandList->Transitions(barrier1, 1);
                commandList->CopyBuffer(
                    indexBufferUpload[frameInFlightCounter],
                    0,
                    indexBuffer[frameInFlightCounter],
                    0,
                    currentIndexBufferDataSize[frameInFlightCounter]);
                RenderBackendBarrier barrier2[] =
                {
                    RenderBackendBarrier(indexBuffer[frameInFlightCounter], RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::IndexBuffer)
                };
                commandList->Transitions(barrier2, 1);
            }

            if (currentDrawDataBufferDataSize[frameInFlightCounter] > 0)
            {
                RenderBackendBarrier barrier1[] =
                {
                    RenderBackendBarrier(drawDataBuffer[frameInFlightCounter], RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::Undefined, RenderBackendResourceState::CopyDst)
                };
                commandList->Transitions(barrier1, 1);
                commandList->CopyBuffer(
                    drawDataBufferUpload[frameInFlightCounter],
                    0,
                    drawDataBuffer[frameInFlightCounter],
                    0,
                    currentDrawDataBufferDataSize[frameInFlightCounter]);
                RenderBackendBarrier barrier2[] =
                {
                    RenderBackendBarrier(drawDataBuffer[frameInFlightCounter], RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::ShaderResource)
                };
                commandList->Transitions(barrier2, 1);
            }

            if (currentDrawIndexedIndirectCommandBufferDataSize[frameInFlightCounter] > 0)
            {
                RenderBackendBarrier barrier1[] =
                {
                    RenderBackendBarrier(drawIndexedIndirectCommandBuffer[frameInFlightCounter], RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::Undefined, RenderBackendResourceState::CopyDst)
                };
                commandList->Transitions(barrier1, 1);
                commandList->CopyBuffer(
                    drawIndexedIndirectCommandBufferUpload[frameInFlightCounter],
                    0,
                    drawIndexedIndirectCommandBuffer[frameInFlightCounter],
                    0,
                    currentDrawIndexedIndirectCommandBufferDataSize[frameInFlightCounter]);
                RenderBackendBarrier barrier2[] =
                {
                    RenderBackendBarrier(drawIndexedIndirectCommandBuffer[frameInFlightCounter], RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::IndirectArgument)
                };
                commandList->Transitions(barrier2, 1);
            }
        }
    }

    void RenderSystem::DrawUI(RenderBackendCommandList& commandList, RenderBackendTextureHandle output)
    {
        ImDrawData* drawData = ImGui::GetDrawData();
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fbWidth = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
        int fbHeight = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
        if (fbWidth <= 0 || fbHeight <= 0)
        {
            return;
        }

        if (totalDrawCommandCount == 0)
        {
            return;
        }

        RenderBackendViewport viewport(0.0f, 0.0f, (float)fbWidth, (float)fbHeight);
        commandList.SetViewports(&viewport, 1);

        RenderBackendScissor scissor(0, 0, fbWidth, fbHeight);
        commandList.SetScissors(&scissor, 1);

        RenderBackendRenderPassInfo renderPass = {
            .renderTargets = { {.texture = output, .mipLevel = 0, .arrayLayer = 0, .loadOp = RenderBackendRenderPassBeginningAccessType::Clear, .storeOp = RenderBackendRenderPassEndingAccessType::Preserve } },
        };
        commandList.BeginRenderPass(renderPass);

        RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::ImGuiVS);
        RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::ImGuiPS);

        RenderBackendGraphicsPipelineState graphicsPipelineState = {};
        graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
        graphicsPipelineState.depthStencilState.depthTestEnable = false;
        graphicsPipelineState.depthStencilState.depthWriteEnable = false;
        graphicsPipelineState.colorBlendState.targetBlends[0].blendEnable = true;
        graphicsPipelineState.colorBlendState.targetBlends[0].srcColorBlendFactor = RenderBackendBlendFactor::SrcAlpha;
        graphicsPipelineState.colorBlendState.targetBlends[0].dstColorBlendFactor = RenderBackendBlendFactor::OneMinusSrcAlpha;
        graphicsPipelineState.colorBlendState.targetBlends[0].colorBlendOp = RenderBackendBlendOp::Add;
        graphicsPipelineState.colorBlendState.targetBlends[0].srcAlphaBlendFactor = RenderBackendBlendFactor::One;
        graphicsPipelineState.colorBlendState.targetBlends[0].dstAlphaBlendFactor = RenderBackendBlendFactor::OneMinusSrcAlpha;
        graphicsPipelineState.colorBlendState.targetBlends[0].alphaBlendOp = RenderBackendBlendOp::Add;
        graphicsPipelineState.colorBlendState.targetBlends[0].writeMask = RenderBackendColorComponentFlags::RGBA;

        RenderBackendShaderConstants shaderConstants = {};
        shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(vertexBuffer[frameInFlightCounter]));
        shaderConstants.BindBufferSRV(1, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(drawDataBuffer[frameInFlightCounter]));

        commandList.DrawIndexedIndirect(
            vertexShader,
            pixelShader,
            graphicsPipelineState,
            shaderConstants,
            indexBuffer[frameInFlightCounter],
            drawIndexedIndirectCommandBuffer[frameInFlightCounter],
            0,
            totalDrawCommandCount,
            RenderBackendPrimitiveTopology::TriangleList);

        commandList.EndRenderPass();
    }

    void RenderSystem::RenderUserInterface(RenderGraph& renderGraph, const SceneView& view)
    {
        RenderGraphTextureDesc uiColorAndAlphaTextureDesc = RenderGraphTextureDesc::Create2D(
            view.displayWidth,
            view.displayHeight,
            RenderBackendTextureFormat::R8G8B8A8Unorm,
            RenderBackendTextureCreateFlags::ShaderResource | RenderBackendTextureCreateFlags::RenderTarget,
            RenderBackendTextureClearValue::CreateColorValueFloat4(0.0f, 0.0f, 0.0f, 0.0f));
        RenderGraphTextureHandle uiColorAndAlphaTexture = renderGraph.CreateTexture(uiColorAndAlphaTextureDesc, "UIColorAndAlphaTexture");

        renderGraph.AddPass(
            std::format("UIColorAndAlpha (Graphics, {}x{})", view.displayWidth, view.displayHeight),
            RenderGraphPassFlags::Graphics | RenderGraphPassFlags::SkipRenderPass,
            [&](RenderGraphBuilder& builder)
            {
                uiColorAndAlphaTexture = builder.WriteTexture(uiColorAndAlphaTexture, RenderBackendResourceState::RenderTarget);

                builder.BindRenderTarget(0, uiColorAndAlphaTexture, RenderBackendRenderPassBeginningAccessType::Clear, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    DrawUI(commandList, registry.GetRenderBackendTextureHandle(uiColorAndAlphaTexture));
                };
            });

        RenderGraphTextureHandle displayTexture = renderGraph.ImportExternalTexture(view.targetTexture, "TargetTexture");

        renderGraph.AddPass(
            std::format("GUIComposition (Graphics, {}x{})", view.displayWidth, view.displayHeight),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                uiColorAndAlphaTexture = builder.ReadTexture(uiColorAndAlphaTexture, RenderBackendResourceState::ShaderResource);
                displayTexture = builder.WriteTexture(displayTexture, RenderBackendResourceState::RenderTarget);

                builder.BindRenderTarget(0, displayTexture, RenderBackendRenderPassBeginningAccessType::Preserve, RenderBackendRenderPassEndingAccessType::Preserve);

                return [=](RenderGraphRegistry& registry, RenderBackendCommandList& commandList)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, float(view.displayWidth), float(view.displayHeight));
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, view.displayWidth, view.displayHeight);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                    graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                    graphicsPipelineState.depthStencilState.depthTestEnable = false;
                    graphicsPipelineState.depthStencilState.depthWriteEnable = false;
                    graphicsPipelineState.colorBlendState.targetBlends[0].blendEnable = true;
                    graphicsPipelineState.colorBlendState.targetBlends[0].srcColorBlendFactor = RenderBackendBlendFactor::SrcAlpha;
                    graphicsPipelineState.colorBlendState.targetBlends[0].dstColorBlendFactor = RenderBackendBlendFactor::OneMinusSrcAlpha;
                    graphicsPipelineState.colorBlendState.targetBlends[0].colorBlendOp = RenderBackendBlendOp::Add;
                    graphicsPipelineState.colorBlendState.targetBlends[0].srcAlphaBlendFactor = RenderBackendBlendFactor::One;
                    graphicsPipelineState.colorBlendState.targetBlends[0].dstAlphaBlendFactor = RenderBackendBlendFactor::OneMinusSrcAlpha;
                    graphicsPipelineState.colorBlendState.targetBlends[0].alphaBlendOp = RenderBackendBlendOp::Add;
                    graphicsPipelineState.colorBlendState.targetBlends[0].writeMask = RenderBackendColorComponentFlags::RGBA;

                    RenderBackendShaderConstants shaderConstants = {};
                    shaderConstants.BindTextureSRV(0, registry.GetTextureSRVBindlessResourceDescriptorIndex(uiColorAndAlphaTexture));

                    RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::FullScreenQuadVS);
                    RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::GUICompositionPS);

                    commandList.Draw(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        shaderConstants,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }
#if 0
    static void SetupCubeShadowMapParameters(const LightComponent& lightComponent, CubeShadowMapShaderParameters& parameters)
    {
        float nearClippingPlane = 0.1f;
        float farClippingPlane = std::max(1.0f, lightComponent.radius);
        Vector3 position = lightComponent.position;

        Matrix4x4 viewMatrix;
        Matrix4x4 invViewMatrix;
        Matrix4x4 projectionMatrix;
        Quaternion rotation;
        static Quaternion zUpQuat = glm::rotate(Quaternion(), Math::DegreesToRadians(90.0), Vector3(1.0, 0.0, 0.0));
        projectionMatrix = Math::PerspectiveReverseZ_RH_ZO(M_HALF_PI, 1.0f, nearClippingPlane, farClippingPlane);

        // +X
        rotation = glm::rotate(Quaternion(), Math::DegreesToRadians(-90.0), Vector3(0.0, 0.0, 1.0));
        invViewMatrix = Math::Compose(position, rotation * zUpQuat, Vector3(1.0f, 1.0f, 1.0f));
        viewMatrix = Math::InverseMatrix(invViewMatrix);
        parameters.viewProjectionMatrix[0] = projectionMatrix * viewMatrix;

        // -X
        rotation = glm::rotate(Quaternion(), Math::DegreesToRadians(90.0), Vector3(0.0, 0.0, 1.0));
        invViewMatrix = Math::Compose(position, rotation * zUpQuat, Vector3(1.0f, 1.0f, 1.0f));
        viewMatrix = Math::InverseMatrix(invViewMatrix);
        parameters.viewProjectionMatrix[1] = projectionMatrix * viewMatrix;

        // +Y
        rotation = Quaternion();
        invViewMatrix = Math::Compose(position, rotation * zUpQuat, Vector3(1.0f, 1.0f, 1.0f));
        viewMatrix = Math::InverseMatrix(invViewMatrix);
        parameters.viewProjectionMatrix[2] = projectionMatrix * viewMatrix;

        // -Y
        rotation = glm::rotate(Quaternion(), Math::DegreesToRadians(180.0), Vector3(0.0, 0.0, 1.0));
        invViewMatrix = Math::Compose(position, rotation * zUpQuat, Vector3(1.0f, 1.0f, 1.0f));
        viewMatrix = Math::InverseMatrix(invViewMatrix);
        parameters.viewProjectionMatrix[3] = projectionMatrix * viewMatrix;

        // +Z
        rotation = glm::rotate(Quaternion(), Math::DegreesToRadians(90.0), Vector3(1.0, 0.0, 0.0));
        invViewMatrix = Math::Compose(position, rotation * zUpQuat, Vector3(1.0f, 1.0f, 1.0f));
        viewMatrix = Math::InverseMatrix(invViewMatrix);
        parameters.viewProjectionMatrix[4] = projectionMatrix * viewMatrix;

        // -Z
        rotation = glm::rotate(Quaternion(), Math::DegreesToRadians(-90.0), Vector3(1.0, 0.0, 0.0));
        invViewMatrix = Math::Compose(position, rotation * zUpQuat, Vector3(1.0f, 1.0f, 1.0f));
        viewMatrix = Math::InverseMatrix(invViewMatrix);
        parameters.viewProjectionMatrix[5] = projectionMatrix * viewMatrix;
    }

    RenderSystem::RenderSystem()
        : EngineSubsystem("Horizon RenderSystem")
    {

    }

    RenderSystem::~RenderSystem()
    {

    }

    void RenderSystem::Init(void* window)
    {
        arena = GArena;
        renderBackend = GRenderBackend;

        assert(GRenderer == nullptr);
        GRenderer = this;

        // TODO: Remove this
        shaderLibrary = new ShaderLibrary(renderBackend, shaderCompiler, ShaderID::Count, true);
        shaderLibrary->AddIncludeDirectory("../../../Shaders");
        shaderLibrary->AddIncludeDirectory("../../../Shaders/RealTimeRenderer");
        shaderLibrary->AddIncludeDirectory("../../../Shaders/RealTimeRenderer/SubsurfaceScattering");
        shaderLibrary->AddIncludeDirectory("../../../Shaders/RealTimeRenderer/SurfelGI");
        shaderLibrary->AddIncludeDirectory("../../../Shaders/RealTimeRenderer/PostProcessing");

        shaderCompiler = CreateDXCShaderCompiler();
        shaderLibrary = new ShaderLibrary(renderBackend, shaderCompiler, (uint32)ShaderPipelineID::Count, true);
        shaderLibrary->AddIncludeDirectory("../../../Shaders");
        shaderLibrary->AddIncludeDirectory("../../../Shaders/RealTimeRenderer");

        RenderBackendTimingQueryHeapDesc timingQueryHeapDesc(RenderBackendGPUProfiler::MaxTimingQueryRegionCount);
        timingQueryHeap = renderBackend->CreateTimingQueryHeap(&timingQueryHeapDesc, "DefaultTimingQueryHeap");
        gpuProfiler = new RenderBackendGPUProfiler(renderBackend);

        InitializeDefaultResources();

        renderPipeline = new RealTimeRenderer(renderBackend, shaderCompiler, this);

        // Init UI
        {
            IMGUI_CHECKVERSION();
            context = ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            IM_ASSERT(io.BackendRendererUserData == NULL && "Already initialized a renderer backend!");
            io.BackendRendererName = "Horizon Engine";
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
            io.IniFilename = "../../../Assets/Test/imgui.ini";
            io.IniSavingRate = 600;

            //ImGui::LoadIniSettingsFromDisk("../../../Assets/Test/imgui.ini");

            ImGui::StyleColorsDark();
            ImGuiStyle& style = ImGui::GetStyle();

            // light style from Pacôme Danhiez (user itamago) https://github.com/ocornut/imgui/pull/511#issuecomment-175719267
            style.WindowRounding = 2.0f;
            style.ScrollbarRounding = 3.0f;
            style.GrabRounding = 2.0f;
            style.AntiAliasedLines = true;
            style.AntiAliasedFill = true;
            style.WindowRounding = 2;
            style.ChildRounding = 2;
            style.ScrollbarSize = 16;
            style.ScrollbarRounding = 3;
            style.GrabRounding = 2;
            style.ItemSpacing.x = 10;
            style.ItemSpacing.y = 4;
            style.IndentSpacing = 22;
            style.FramePadding.x = 6;
            style.FramePadding.y = 4;
            style.Alpha = 1.0f;
            style.FrameRounding = 3.0f;
            style.TabBorderSize = 0.0f;

            style.Colors[ImGuiCol_Text] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
            style.Colors[ImGuiCol_WindowBg] = ImVec4(0.94f, 0.94f, 0.94f, 0.94f);
            style.Colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
            style.Colors[ImGuiCol_PopupBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
            style.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.39f);
            style.Colors[ImGuiCol_BorderShadow] = ImVec4(1.00f, 1.00f, 1.00f, 0.10f);
            style.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.94f);
            style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
            style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            style.Colors[ImGuiCol_TitleBg] = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
            style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
            style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
            style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
            style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.69f, 0.69f, 0.69f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
            style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
            //style.Colors[ImGuiCol_ComboBg] = ImVec4(0.86f, 0.86f, 0.86f, 0.99f);
            style.Colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
            style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
            style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
            style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
            style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_Separator] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
            style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
            style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
            style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
            style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
            style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
            style.Colors[ImGuiCol_Tab] = ImVec4(0.59f, 0.59f, 0.59f, 1.0f);
            style.Colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
            style.Colors[ImGuiCol_TabActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
            style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.59f, 0.59f, 0.59f, 1.0f);
            style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
            style.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
            style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
            style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
            style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

            ImGui_ImplGlfw_InitForOther((GLFWwindow*)window, true);

            // Upload Fonts
            {
                if (true)
                {
                    io.FontDefault = io.Fonts->AddFontFromFileTTF("../../../Assets/Fonts/OpenSans/OpenSans-Regular.ttf", 26.0f);
                }
                else
                {
                    io.FontDefault = io.Fonts->AddFontFromFileTTF("../../../Assets/Fonts/FZHTJW.TTF", 20.0f, NULL, io.Fonts->GetGlyphRangesChineseFull());
                }
                unsigned char* pixels;
                int width, height;
                io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
                RenderBackendTextureDesc defaultFontTextureDesc = RenderBackendTextureDesc::Create2D(
                    width,
                    height,
                    RenderBackendTextureFormat::RGBA8Unorm,
                    RenderBackendTextureCreateFlags::ShaderResource);
                defaultFontTexture = renderBackend->CreateTexture(~0u, &defaultFontTextureDesc, pixels, "DefaultFont");
                io.Fonts->SetTexID(defaultFontTexture.ToUnit64());
            }

            std::vector<uint8> source;
            std::vector<std::wstring> includeDirs_w;
            std::vector<std::wstring> defines;
            includeDirs_w.push_back(HE_TEXT("../../../Shaders"));
            std::string filename = "../../../Shaders/ImGui.hsm";
            LoadShaderSourceFromFile("../../../Shaders/ImGui.hsm", source);

            std::vector<const char*> includeDirs;
            includeDirs.push_back("../../../Shaders");

            ShadingLanguage shadingLanguage = (GRenderBackend->GetType() == RenderBackendType::Vulkan) ? ShadingLanguage::SPIRV : ShadingLanguage::DXIL;

            RenderBackendShaderDesc imguiShaderDesc;

            ShaderSource shaderSource;
            shaderSource.filename = filename.c_str();
            shaderSource.sourceData = source.data();
            shaderSource.sourceSize = source.size();
            shaderSource.entryPoint = "ImGuiVS";
            shaderSource.stage = ShaderStage::Vertex;
            shaderSource.defines = nullptr;
            shaderSource.numDefines = 0;
            shaderSource.includeDirectories = includeDirs.data();
            shaderSource.numIncludeDirectories = 1;

            ShaderCompilerSettings shaderCompilerSettings;
            ShaderCompilerOutput shaderCompilerOutput;

            shaderCompiler->CompileShader(shaderCompilerSettings, shaderSource, shadingLanguage, &shaderCompilerOutput);

            imguiShaderDesc.stages[(uint32)RenderBackendShaderStage::Vertex].data = shaderCompilerOutput.blob.GetData();
            imguiShaderDesc.stages[(uint32)RenderBackendShaderStage::Vertex].size = shaderCompilerOutput.blob.GetSize();
            imguiShaderDesc.entryPoints[(uint32)RenderBackendShaderStage::Vertex] = shaderSource.entryPoint;

            ShaderCompilerOutput shaderCompilerOutput2;
            shaderSource.filename = filename.c_str();
            shaderSource.sourceData = source.data();
            shaderSource.sourceSize = source.size();
            shaderSource.entryPoint = "ImGuiPS";
            shaderSource.stage = ShaderStage::Pixel;
            shaderSource.defines = nullptr;
            shaderSource.numDefines = 0;
            shaderSource.includeDirectories = includeDirs.data();
            shaderSource.numIncludeDirectories = 1;

            shaderCompiler->CompileShader(shaderCompilerSettings, shaderSource, shadingLanguage, &shaderCompilerOutput2);

            imguiShaderDesc.stages[(uint32)RenderBackendShaderStage::Pixel].data = shaderCompilerOutput2.blob.GetData();
            imguiShaderDesc.stages[(uint32)RenderBackendShaderStage::Pixel].size = shaderCompilerOutput2.blob.GetSize();
            imguiShaderDesc.entryPoints[(uint32)RenderBackendShaderStage::Pixel] = shaderSource.entryPoint;

            imguiShader = renderBackend->CreateShader(~0u, &imguiShaderDesc, "ImGuiPS");

            for (uint32 t = 0; t < 3; t++)
            {
                RenderBackendBufferDesc vertexBufferDesc = RenderBackendBufferDesc::CreateByteAddress(4);
                vertexBuffer[t] = renderBackend->CreateBuffer(~0u, &vertexBufferDesc, nullptr, "ImGuiVertexBuffer");

                RenderBackendBufferDesc vertexBufferUploadDesc = RenderBackendBufferDesc::CreateUpload(4);
                vertexBufferUpload[t] = renderBackend->CreateBuffer(~0u, &vertexBufferUploadDesc, nullptr, "ImGuiVertexBufferUpload");

                RenderBackendBufferDesc indexBufferDesc = RenderBackendBufferDesc::CreateIndex(sizeof(uint32), 1);
                indexBuffer[t] = renderBackend->CreateBuffer(~0u, &indexBufferDesc, nullptr, "ImGuiIndexBuffer");

                RenderBackendBufferDesc indexBufferUploadDesc = RenderBackendBufferDesc::CreateUpload(4);
                indexBufferUpload[t] = renderBackend->CreateBuffer(~0u, &indexBufferUploadDesc, nullptr, "ImGuiIndexBufferUpload");

                vertexBufferSize[t] = 4;
                indexBufferSize[t] = 4;
            }
        }

        renderBackend->FlushRenderDevices();
    }

    void RenderSystem::Exit()
    {
        ReleaseDefaultResources();
    }

    void RenderSystem::Tick()
    {
        renderGraphResourcePool->Tick();
    }

    void RenderSystem::AddLight(const SceneView& view, LightComponent& lightComponent, const CameraComponent& camera)
    {
        // Early out if light has no power
        if (lightComponent.luminousIntensity <= SMALL_NUMBER)
        {
            return;
        }

        if (numLights >= RendererMaxLightCount)
        {
            LogWarning(GLogger, std::format("Too many lights in the scene !!!"));
            return;
        }

        bool isSkyAtmosphereLight = false;

        if (!skyAtmosphereLight && (lightComponent.type == LightComponent::LightType::Directional))
        {
            skyAtmosphereLight = &lightComponent;
            isSkyAtmosphereLight = true;
        }

        // Setup LightShaderParameters
        LightShaderParameters& light = lightData[numLights];
        light.color = lightComponent.GetPhysicalLightColor();
        light.position = lightComponent.position;
        light.radius = lightComponent.radius;
        light.type = (uint32)lightComponent.type;
        light.forwardVec = lightComponent.forwardVec;
        light.rightVec = lightComponent.rightVec;
        light.upVec = lightComponent.upVec;
        light.shadowMapIndex = -1;

        LightInfo& info = lightInfo[numLights];
        info.component = &lightComponent;

        if (lightComponent.CastShadows())
        {
            if (lightComponent.UseRayTracingShadows())
            {
                // TODO

            }
            else
            {
                if (lightComponent.type == LightComponent::LightType::Directional)
                {
                    // Setup CascadedShadowMapShaderParameters
                    CascadedShadowMapShaderParameters& shadowCascades = cascadedShadowMapData[numCascadedShadowMaps];
                    SetupShadowCascades(view, lightComponent, camera, shadowCascades);

                    cascadedShadowMapIndexToLightIndex[numCascadedShadowMaps] = numLights;
                    numCascadedShadowMaps++;
                }
                else if (lightComponent.type == LightComponent::LightType::Point)
                {
                    CubeShadowMapShaderParameters& cubeShadowMapParameters = cubeShadowMapData[numCubeShadowMaps];
                    SetupCubeShadowMapParameters(lightComponent, cubeShadowMapParameters);

                    cubeShadowMapIndexToLightIndex[numCubeShadowMaps] = numLights;
                    light.shadowMapIndex = numCubeShadowMaps;
                    numCubeShadowMaps++;
                }
            }
        }

        numLights++;
    }

    void RenderSystem::UpdateRenderData(SceneView* view, RenderBackendCommandList* commandList)
    {
        OPTICK_EVENT();

        Scene* scene = view->scene;
        const CameraComponent& camera = view->camera;

        uint32 regionID = gpuProfiler->BeginRegion(commandList, "UpdateRenderData");

        EntityManager* entityManager = scene->GetEntityManager();

        uint32 deviceMask = ~0u;

        // UpdateLightData
        {
            numLights = 0;
            numCascadedShadowMaps = 0;
            numCubeShadowMaps = 0;

            skyAtmosphereLight = nullptr;

            entityManager->GetView<LightComponent>().each([&](EntityHandle entity, LightComponent& light)
            {
                AddLight(*view, light, camera);
            });

            if (!lightDataBuffer)
            {
                RenderBackendBufferDesc lightBufferDesc = RenderBackendBufferDesc::CreateByteAddress(RendererMaxLightCount * sizeof(LightShaderParameters));
                lightDataBuffer = renderBackend->CreateBuffer(&lightBufferDesc, nullptr, "LightDataBuffer");
                RenderBackendBufferDesc lightDataUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(RendererMaxLightCount * sizeof(LightShaderParameters));
                lightDataUploadBuffer = renderBackend->CreateBuffer(&lightDataUploadBufferDesc, nullptr, "LightDataUploadBuffer");

                RenderBackendBufferDesc cascadedShadowMapBufferDesc = RenderBackendBufferDesc::CreateByteAddress(RendererMaxCascadedShadowMapCount * sizeof(CascadedShadowMapShaderParameters));
                cascadedShadowMapBuffer = renderBackend->CreateBuffer(&cascadedShadowMapBufferDesc, nullptr, "CascadedShadowMapBuffer");
                RenderBackendBufferDesc cascadedShadowMapUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(RendererMaxCascadedShadowMapCount * sizeof(CascadedShadowMapShaderParameters));
                cascadedShadowMapUploadBuffer = renderBackend->CreateBuffer(&cascadedShadowMapUploadBufferDesc, nullptr, "CascadedShadowMapUploadBuffer");

                RenderBackendBufferDesc cubeShadowMapBufferDesc = RenderBackendBufferDesc::CreateByteAddress(RendererMaxCubeShadowMapCount * sizeof(CubeShadowMapShaderParameters));
                cubeShadowMapBuffer = renderBackend->CreateBuffer(&cubeShadowMapBufferDesc, nullptr, "CubeShadowMapBuffer");
                RenderBackendBufferDesc cubeShadowMapUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(RendererMaxCubeShadowMapCount * sizeof(CubeShadowMapShaderParameters));
                cubeShadowMapUploadBuffer = renderBackend->CreateBuffer(&cubeShadowMapUploadBufferDesc, nullptr, "CubeShadowMapUploadBuffer");
            }

            if (numLights > 0)
            {
                renderBackend->UpdateBuffer(lightDataUploadBuffer, 0, &lightData, numLights * sizeof(LightShaderParameters));
                commandList->CopyBuffer(
                    lightDataUploadBuffer,
                    0,
                    lightDataBuffer,
                    0,
                    numLights * sizeof(LightShaderParameters));
            }

            if (numCascadedShadowMaps > 0)
            {
                renderBackend->UpdateBuffer(cascadedShadowMapUploadBuffer, 0, &cascadedShadowMapData, numCascadedShadowMaps * sizeof(CascadedShadowMapShaderParameters));
                commandList->CopyBuffer(
                    cascadedShadowMapUploadBuffer,
                    0,
                    cascadedShadowMapBuffer,
                    0,
                    numCascadedShadowMaps * sizeof(CascadedShadowMapShaderParameters));
            }

            if (numCubeShadowMaps > 0)
            {
                renderBackend->UpdateBuffer(cubeShadowMapUploadBuffer, 0, &cubeShadowMapData, numCubeShadowMaps * sizeof(CubeShadowMapShaderParameters));
                commandList->CopyBuffer(
                    cubeShadowMapUploadBuffer,
                    0,
                    cubeShadowMapBuffer,
                    0,
                    numCubeShadowMaps * sizeof(CubeShadowMapShaderParameters));
            }

            entityManager->GetView<EnvironmentLightComponent>().each([&](EntityHandle entity, EnvironmentLightComponent& skyLight)
            {
                if (skyLight.IsDirty())
                {
                    UpdateSkyLight(skyLight);

                    environmentMap = skyLight.environmentMap;
                    irradianceEnvironmentMap = skyLight.irradianceEnvironmentMap;
                    irradianceEnvironmentMapSH = skyLight.irradianceEnvironmentMapSH;
                    filteredEnvironmentMap = skyLight.filteredEnvironmentMap;
                }
            });
        }

        // UpdateMaterialData
        {
            numMaterials = 0;
            materials.clear();

            MaterialShaderParameters defaultMaterial;
            defaultMaterial.baseColor = Vector4(1, 0, 0, 1);
            defaultMaterial.metallic = 0.0f;
            defaultMaterial.roughness = 1.0f;
            defaultMaterial.specular = 0.5f;
            defaultMaterial.specularTint = 1.0f;
            defaultMaterial.emission = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
            defaultMaterial.emissionStrength = 1.0f;
            defaultMaterial.sssSurfaceAlbedo = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
            defaultMaterial.sssMFP = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
            defaultMaterial.secondRoughness = 0.5f;
            defaultMaterial.lobeMix = 0.0f;
            defaultMaterial.flags = 0;
            materials.emplace_back(defaultMaterial);

            int meshCount = 0;
            entityManager->GetView<MeshComponent>().each([&](EntityHandle entity, MeshComponent& mesh)
            {
                meshCount++;
                if (true)
                {
                    mesh.materialBufferOffset = int(materials.size() * sizeof(MaterialShaderParameters));
                    for (auto& material : mesh.materials)
                    {
                        MaterialShaderParameters materialShaderParameters;
                        materialShaderParameters.baseColor = material.baseColor;
                        materialShaderParameters.metallic = material.metallic;
                        materialShaderParameters.roughness = material.roughness;
                        materialShaderParameters.specular = material.specular;
                        materialShaderParameters.specularTint = material.specularTint;
                        materialShaderParameters.emission = material.emission;
                        materialShaderParameters.emissionStrength = material.emissionStrength;
                        materialShaderParameters.sssSurfaceAlbedo = material.sssSurfaceAlbedo;
                        materialShaderParameters.sssMFP = material.sssSurfaceAlbedo;
                        materialShaderParameters.secondRoughness = material.secondRoughness;
                        materialShaderParameters.lobeMix = material.lobeMix;
                        materialShaderParameters.flags = 0;
                        if (material.useMetallicRoughnessWorkflow)
                        {
                            materialShaderParameters.flags |= MATERIAL_FLAGS_BIT_USE_METALLIC_ROUGHNESS_WORKFLOW;
                        }
                        for (uint32 slot = 0; slot < RendererMaxMaterialTextureSlotCount; slot++)
                        {
                            if (material.textures[slot].used)
                            {
                                materialShaderParameters.textures[slot].bindlessTextureIndex = renderBackend->GetTextureSRVDescriptorIndex(material.textures[slot].gpuTexture);
                            }
                        }
                        materials.emplace_back(materialShaderParameters);
                    }
                }
            });
            numMaterials = (uint32)materials.size();

            uint64 newMaterialBufferSize = numMaterials * sizeof(MaterialShaderParameters);
            if (materialBuffer == RenderBackendBufferHandle::Null && newMaterialBufferSize > 0)
            {
                RenderBackendBufferDesc materialUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(newMaterialBufferSize);
                materialUploadBuffer = renderBackend->CreateBuffer(&materialUploadBufferDesc, nullptr, "MaterialUploadBuffer");
                RenderBackendBufferDesc materialBufferDesc = RenderBackendBufferDesc::CreateByteAddress(newMaterialBufferSize);
                materialBuffer = renderBackend->CreateBuffer(&materialBufferDesc, nullptr, "MaterialBuffer");
                materialBufferSize = newMaterialBufferSize;
            }
            else if (materialBufferSize < newMaterialBufferSize)
            {
                renderBackend->ResizeBuffer(materialUploadBuffer, newMaterialBufferSize);
                renderBackend->ResizeBuffer(materialBuffer, newMaterialBufferSize);
                materialBufferSize = newMaterialBufferSize;
            }

            if (materialBuffer && numMaterials > 0)
            {
                renderBackend->UpdateBuffer(materialUploadBuffer, 0, materials.data(), materialBufferSize);
                commandList->CopyBuffer(
                    materialUploadBuffer,
                    0,
                    materialBuffer,
                    0,
                    materialBufferSize);
                RenderBackendBarrier barrier[] =
                {
                    RenderBackendBarrier(materialBuffer, RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::UnorderedAccess)
                };
                commandList->Transitions(barrier, 1);
            }
        }

        // UpdateMeshData
        {
            geometries.clear();
            drawList.clear();
            transforms.clear();
            rowMajorTransforms.clear();

            std::vector<RenderBackendRayTracingGeometryDesc> geometryDescs;

            entityManager->GetView<MeshComponent>().each([&](EntityHandle entity, MeshComponent& mesh)
            {
                const TransformComponent& transformComponent = entityManager->GetComponent<TransformComponent>(entity);

                int transformIndex = (int)transforms.size();
                transforms.push_back(transformComponent.localToWorldMatrix);
                rowMajorTransforms.push_back(Math::Transpose(transformComponent.localToWorldMatrix));

                GeometryShaderParameters geometry;
                geometry.vertexBuffer0 = renderBackend->GetBufferUAVDescriptorIndex(mesh.vertexBuffers[0]);
                geometry.vertexBuffer1 = renderBackend->GetBufferUAVDescriptorIndex(mesh.vertexBuffers[1]);
                geometry.vertexBuffer2 = renderBackend->GetBufferUAVDescriptorIndex(mesh.vertexBuffers[2]);
                geometry.vertexBuffer3 = renderBackend->GetBufferUAVDescriptorIndex(mesh.vertexBuffers[3]);
                geometry.prevVertexBuffer0 = -1;
                geometry.indexBuffer = renderBackend->GetBufferUAVDescriptorIndex(mesh.indexBuffer);
                //geometry.transformBuffer = renderBackend->GetBufferUAVDescriptorIndex(transformBuffer);
                //geometry.previousTransformBuffer = renderBackend->GetBufferUAVDescriptorIndex(previousTransformBuffer);
                geometry.materialIndexBuffer = renderBackend->GetBufferUAVDescriptorIndex(mesh.materialIndexBuffer);
                geometry.materialBufferOffset = mesh.materialBufferOffset;
                geometry.transformIndex = transformIndex;
                geometry.previousTransformIndex = (mesh.updateCounter + 1 == updateCounter) ? mesh.previousTransformIndex : -1;
                geometry.baseVertex = 0;
                geometry.vertexCount = mesh.numVertices;
                geometry.baseIndex = 0;
                geometry.indexCount = mesh.numIndices;
                geometry.boundsMin = mesh.boundsMin;
                geometry.boundsMax = mesh.boundsMax;
                geometries.emplace_back(geometry);

                DrawCallInfo drawCall;
                drawCall.vertexBuffers[0] = mesh.vertexBuffers[0];
                drawCall.vertexBuffers[1] = mesh.vertexBuffers[1];
                drawCall.vertexBuffers[2] = mesh.vertexBuffers[2];
                drawCall.vertexBuffers[3] = mesh.vertexBuffers[3];
                drawCall.indexBuffer = mesh.indexBuffer;
                drawCall.numVertices = mesh.numVertices;
                drawCall.numIndices = mesh.numIndices;
                drawCall.firstIndex = 0;
                drawCall.geometryIndex = (uint32)geometries.size() - 1;
                drawList.push_back(drawCall);

                mesh.updateCounter = updateCounter;
                mesh.previousTransformIndex = transformIndex;

                if (ShouldUpdateRayTracingScene())
                {
                    RenderBackendRayTracingGeometryDesc geometryDesc = {
                        .type = RenderBackendRayTracingGeometryType::Triangles,
                        .flags = RenderBackendRayTracingGeometryFlags::Opaque,
                        .triangleDesc = {
                            .numIndices = geometry.indexCount,
                            .numVertices = geometry.vertexCount,
                            .vertexStride = 3 * sizeof(float),
                            .vertexBuffer = mesh.vertexBuffers[0],
                            .vertexOffset = 0,
                            .indexBuffer = mesh.indexBuffer,
                            .indexOffset = geometry.baseIndex * sizeof(uint32),
                            //.transformBuffer = mesh.transformBufferRowMajor,
                            .transformOffset = geometry.transformIndex * 16 * sizeof(float),
                        }
                    };
                    geometryDescs.emplace_back(geometryDesc);
                }
            });
            numTransforms = (uint32)transforms.size();
            numGeometries = (uint32)geometries.size();

            previousTransformBuffer = transformBuffer;

            uint32 newTransformBufferSize = numTransforms * sizeof(Matrix4x4);
            if (transformBuffer == RenderBackendBufferHandle::Null && newTransformBufferSize > 0)
            {
                RenderBackendBufferDesc transformUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(newTransformBufferSize);
                transformUploadBuffer = renderBackend->CreateBuffer(&transformUploadBufferDesc, nullptr, "TransformUploadBuffer");
                RenderBackendBufferDesc transformBufferDesc = RenderBackendBufferDesc::CreateByteAddress(newTransformBufferSize);
                transformBuffer = renderBackend->CreateBuffer(&transformBufferDesc, nullptr, "TransformBuffer");
                RenderBackendBufferDesc transformBufferRowMajorDesc = RenderBackendBufferDesc::CreateUpload(newTransformBufferSize, RenderBackendBufferCreateFlags::AccelerationStruture);
                transformBufferRowMajor = renderBackend->CreateBuffer(&transformBufferRowMajorDesc, nullptr, "TransformBufferRowMajor");
                transformBufferSize = newTransformBufferSize;
            }
            else if (transformBufferSize < newTransformBufferSize)
            {
                renderBackend->ResizeBuffer(transformUploadBuffer, newTransformBufferSize);
                renderBackend->ResizeBuffer(transformBuffer, newTransformBufferSize);
                transformBufferSize = newTransformBufferSize;
            }

            if (transformBuffer && numTransforms > 0)
            {
                renderBackend->UpdateBuffer(transformUploadBuffer, 0, transforms.data(), transformBufferSize);
                renderBackend->UpdateBuffer(transformBufferRowMajor, 0, rowMajorTransforms.data(), transformBufferSize);
                commandList->CopyBuffer(
                    transformUploadBuffer,
                    0,
                    transformBuffer,
                    0,
                    transformBufferSize);
                RenderBackendBarrier barrier[] =
                {
                    RenderBackendBarrier(transformBuffer, RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::UnorderedAccess)
                };
                commandList->Transitions(barrier, 1);
            }

            for (auto& geometryDesc : geometryDescs)
            {
                geometryDesc.triangleDesc.transformBuffer = transformBufferRowMajor;
            }

            for (GeometryShaderParameters& geometry : geometries)
            {
                geometry.transformBuffer = renderBackend->GetBufferUAVDescriptorIndex(transformBuffer);
                geometry.previousTransformBuffer = renderBackend->GetBufferUAVDescriptorIndex(previousTransformBuffer);
            }

            uint32 newGeometryBufferSize = numGeometries * sizeof(GeometryShaderParameters);
            if (geometryBuffer == RenderBackendBufferHandle::Null && newGeometryBufferSize > 0)
            {
                RenderBackendBufferDesc geometryUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(newGeometryBufferSize);
                geometryUploadBuffer = renderBackend->CreateBuffer(&geometryUploadBufferDesc, nullptr, "GeometryUploadBuffer");
                RenderBackendBufferDesc geometryBufferDesc = RenderBackendBufferDesc::CreateByteAddress(newGeometryBufferSize);
                geometryBuffer = renderBackend->CreateBuffer(&geometryBufferDesc, nullptr, "GeometryBuffer");
                geometryBufferSize = newGeometryBufferSize;
            }
            else if (geometryBufferSize < newGeometryBufferSize)
            {
                renderBackend->ResizeBuffer(geometryUploadBuffer, newGeometryBufferSize);
                renderBackend->ResizeBuffer(geometryBuffer, newGeometryBufferSize);
                geometryBufferSize = newGeometryBufferSize;
            }

            if (geometryBuffer && numGeometries > 0)
            {
                renderBackend->UpdateBuffer(geometryUploadBuffer, 0, geometries.data(), geometryBufferSize);
                commandList->CopyBuffer(
                    geometryUploadBuffer,
                    0,
                    geometryBuffer,
                    0,
                    geometryBufferSize);
                RenderBackendBarrier barrier[] =
                {
                    RenderBackendBarrier(geometryBuffer, RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::UnorderedAccess)
                };
                commandList->Transitions(barrier, 1);
            }

            if (ShouldUpdateRayTracingScene() && !rayTracingScene)
            {
                RenderBackendRayTracingBottomLevelAccelerationDesc blasDesc = {
                    .buildFlags = RenderBackendRayTracingAccelerationStructureBuildFlags::PreferFastTrace,
                    .numGeometries = (uint32)geometryDescs.size(),
                    .geometryDescs = geometryDescs.data(),
                };
                bottomLevelAS = renderBackend->CreateRayTracingBottomLevelAccelerationStructure(&blasDesc, "BLAS");

                RenderBackendRayTracingInstance geometryInstance = {
                    .transformMatrix = Matrix4x4(1.0),
                    .instanceID = 0,
                    .instanceMask = 0xff,
                    .instanceContributionToHitGroupIndex = 0,
                    .flags = RenderBackendRayTracingInstanceFlags::TriangleFacingCullDisable,
                    .blas = bottomLevelAS,
                };

                RenderBackendRayTracingTopLevelAccelerationDesc tlasDesc = {
                    .buildFlags = RenderBackendRayTracingAccelerationStructureBuildFlags::PreferFastTrace,
                    .geometryFlags = RenderBackendRayTracingGeometryFlags::Opaque,
                    .numInstances = 1,
                    .instances = &geometryInstance,
                };
                rayTracingScene = renderBackend->CreateRayTracingTopLevelAccelerationStructure(&tlasDesc, "TLAS");
            }
        }

        // Debug draw
        debugDrawLinesVertices.clear();

#if DEBUG
        entityManager->GetView<CameraComponent>().each([&](EntityHandle entity, CameraComponent& component)
        {
            // Calculate camera far plane corners
            float distance = component.farClippingPlane;
            float halfFovRad = Math::DegreesToRadians(component.fieldOfView) * 0.5f;
            float uLen = distance * Math::Tan(halfFovRad);
            float rLen = uLen * component.aspectRatio;
            Vector3 farCenterPoint = component.position + distance * component.forwardVec;
            Vector3 u = uLen * component.upVec;
            Vector3 r = rLen * component.rightVec;

            Vector3 corners[4];
            corners[0] = farCenterPoint - u - r; // left-bottom
            corners[1] = farCenterPoint - u + r; // right-bottom
            corners[2] = farCenterPoint + u - r; // left-up
            corners[3] = farCenterPoint + u + r; // right-up

            DrawLine(component.position, corners[0], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 0);
            DrawLine(component.position, corners[1], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 0);
            DrawLine(component.position, corners[2], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 0);
            DrawLine(component.position, corners[3], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 0);

            DrawLine(corners[0], corners[1], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 0);
            DrawLine(corners[1], corners[3], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 0);
            DrawLine(corners[2], corners[3], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 0);
            DrawLine(corners[2], corners[0], Vector4(1.0f, 0.0f, 0.0f, 1.0f), 0);
        });
#endif

        /*
        entityManager->GetView<MeshComponent>().each([&](EntityHandle entity, MeshComponent& mesh)
        {
            if (true)
            {
                for (const auto& element : mesh.elements)
                {
                    DrawWireBox(mesh.transformData[element.transformIndex], { element.boundsMin, element.boundsMax }, Vector4(1.0f, 0.0f, 0.0f, 1.0f), 0);
                }
            }
        });*/

        uint32 newDebugDrawLinesVertexBufferSize = (uint32)debugDrawLinesVertices.size() * sizeof(Vector3);
        if (debugDrawLinesVertexBuffer == RenderBackendBufferHandle::Null && newDebugDrawLinesVertexBufferSize > 0)
        {
            debugDrawLinesVertexBufferSize = newDebugDrawLinesVertexBufferSize;

            RenderBackendBufferDesc bufferDesc = RenderBackendBufferDesc::CreateByteAddress(newDebugDrawLinesVertexBufferSize);
            debugDrawLinesVertexBuffer = renderBackend->CreateBuffer(&bufferDesc, nullptr, "DebugDrawLinesVertexBuffer");

            RenderBackendBufferDesc debugDrawLinesVertexUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(newDebugDrawLinesVertexBufferSize);
            debugDrawLinesVertexUploadBuffer = renderBackend->CreateBuffer(&debugDrawLinesVertexUploadBufferDesc, nullptr, "DebugDrawLinesVertexUploadBuffer");
        }
        else if (debugDrawLinesVertexBufferSize < newDebugDrawLinesVertexBufferSize)
        {
            renderBackend->ResizeBuffer(debugDrawLinesVertexBuffer, newDebugDrawLinesVertexBufferSize);
            renderBackend->ResizeBuffer(debugDrawLinesVertexUploadBuffer, newDebugDrawLinesVertexBufferSize);

            debugDrawLinesVertexBuffer = newDebugDrawLinesVertexBufferSize;
        }

        if (debugDrawLinesVertexBuffer && debugDrawLinesVertexBufferSize > 0)
        {
            renderBackend->UpdateBuffer(debugDrawLinesVertexUploadBuffer, 0, debugDrawLinesVertices.data(), debugDrawLinesVertexBufferSize);
            commandList->CopyBuffer(
                debugDrawLinesVertexUploadBuffer,
                0,
                debugDrawLinesVertexBuffer,
                0,
                debugDrawLinesVertexBufferSize);
        }

        // Update vertex buffer and index buffer for ImGui
        {
            if (currentVertexBufferDataSize[frameInFlightCounter] > 0)
            {
                RenderBackendBarrier barrier1[] =
                {
                    RenderBackendBarrier(vertexBuffer[frameInFlightCounter], RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::Undefined, RenderBackendResourceState::CopyDst)
                };
                commandList->Transitions(barrier1, 1);
                commandList->CopyBuffer(
                    vertexBufferUpload[frameInFlightCounter],
                    0,
                    vertexBuffer[frameInFlightCounter],
                    0,
                    currentVertexBufferDataSize[frameInFlightCounter]);
                RenderBackendBarrier barrier2[] =
                {
                    RenderBackendBarrier(vertexBuffer[frameInFlightCounter], RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::UnorderedAccess)
                };
                commandList->Transitions(barrier2, 1);
            }

            if (currentIndexBufferDataSize[frameInFlightCounter] > 0)
            {
                RenderBackendBarrier barrier1[] =
                {
                    RenderBackendBarrier(vertexBuffer[frameInFlightCounter], RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::Undefined, RenderBackendResourceState::CopyDst)
                };
                commandList->Transitions(barrier1, 1);
                commandList->CopyBuffer(
                    indexBufferUpload[frameInFlightCounter],
                    0,
                    indexBuffer[frameInFlightCounter],
                    0,
                    currentIndexBufferDataSize[frameInFlightCounter]);
                RenderBackendBarrier barrier2[] =
                {
                    RenderBackendBarrier(indexBuffer[frameInFlightCounter], RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::IndexBuffer)
                };
                commandList->Transitions(barrier2, 1);
            }
        }

        /*if (ShouldUpdateRayTracingScene())
        {
            UpdateRayTracingAccelerationStructures(view, commandList);
        }*/

        gpuProfiler->EndRegion(regionID);

        updateCounter++;
    }

    void RenderSystem::DrawUI(RenderBackendCommandList& commandList, RenderBackendTextureHandle output)
    {
        ImDrawData* drawData = ImGui::GetDrawData();
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fbWidth = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
        int fbHeight = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
        if (fbWidth <= 0 || fbHeight <= 0)
        {
            return;
        }

        RenderBackendViewport viewport(0.0f, 0.0f, (float)fbWidth, (float)fbHeight);
        commandList.SetViewports(&viewport, 1);

        RenderBackendRenderPassInfo renderPass = {
            .renderTargets = { {.texture = output, .mipLevel = 0, .arrayLayer = 0, .loadOp = RenderBackendRenderPassBeginningAccessType::Clear, .storeOp = RenderBackendRenderPassEndingAccessType::Preserve } },
        };
        commandList.BeginRenderPass(renderPass);

        // Will project scissor/clipping rectangles into framebuffer space
        ImVec2 clipOffset = drawData->DisplayPos;         // (0,0) unless using multi-viewports
        ImVec2 clipScale = drawData->FramebufferScale;    // (1,1) unless using retina display which are often (2,2)

        // Render command lists
        // (Because we merged all buffers into a single one, we maintain our own offset into them)
        int globalIndexOffset = 0;
        int globalVertexOffset = 0;
        for (int i = 0; i < drawData->CmdListsCount; i++)
        {
            const ImDrawList* cmdList = drawData->CmdLists[i];
            for (int drawCallIndex = 0; drawCallIndex < cmdList->CmdBuffer.Size; drawCallIndex++)
            {
                const ImDrawCmd* pcmd = &cmdList->CmdBuffer[drawCallIndex];

                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clipMin((pcmd->ClipRect.x - clipOffset.x) * clipScale.x, (pcmd->ClipRect.y - clipOffset.y) * clipScale.y);
                ImVec2 clipMax((pcmd->ClipRect.z - clipOffset.x) * clipScale.x, (pcmd->ClipRect.w - clipOffset.y) * clipScale.y);

                // Clamp to viewport as vkCmdSetScissor() won't accept values that are off bounds
                if (clipMin.x < 0.0f) { clipMin.x = 0.0f; }
                if (clipMin.y < 0.0f) { clipMin.y = 0.0f; }
                if (clipMax.x > fbWidth) { clipMax.x = (float)fbWidth; }
                if (clipMax.y > fbHeight) { clipMax.y = (float)fbHeight; }
                if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
                {
                    continue;
                }

                RenderBackendScissor scissor((int32)(clipMin.x), (int32)(clipMin.y), (uint32)(clipMax.x - clipMin.x), (uint32)(clipMax.y - clipMin.y));
                commandList.SetScissors(&scissor, 1);

                Vector2 scale = Vector2(2.0f / drawData->DisplaySize.x, -2.0f / drawData->DisplaySize.y);
                Vector2 translate = Vector2(-1.0f - drawData->DisplayPos.x * scale.x, 1.0f + drawData->DisplayPos.y * scale.y);

                RenderBackendGraphicsPipelineState graphicsPipelineState = {};
                graphicsPipelineState.rasterizationState.cullMode = RenderBackendRasterizationCullMode::None;
                graphicsPipelineState.depthStencilState.depthTestEnable = false;
                graphicsPipelineState.depthStencilState.depthWriteEnable = false;
                graphicsPipelineState.colorBlendState.targetBlends[0].blendEnable = true;
                graphicsPipelineState.colorBlendState.targetBlends[0].srcColorBlendFactor = RenderBackendBlendFactor::SrcAlpha;
                graphicsPipelineState.colorBlendState.targetBlends[0].dstColorBlendFactor = RenderBackendBlendFactor::OneMinusSrcAlpha;
                graphicsPipelineState.colorBlendState.targetBlends[0].colorBlendOp = RenderBackendBlendOp::Add;
                graphicsPipelineState.colorBlendState.targetBlends[0].srcAlphaBlendFactor = RenderBackendBlendFactor::One;
                graphicsPipelineState.colorBlendState.targetBlends[0].dstAlphaBlendFactor = RenderBackendBlendFactor::OneMinusSrcAlpha;
                graphicsPipelineState.colorBlendState.targetBlends[0].alphaBlendOp = RenderBackendBlendOp::Add;
                graphicsPipelineState.colorBlendState.targetBlends[0].colorWriteMask = RenderBackendColorComponentFlags::RGBA;

                struct ImGuiShaderArguments
                {
                    Vector2 scale;
                    Vector2 translate;
                    int vertexOffset;
                };

                RenderBackendShaderConstants shaderConstants = {};
                shaderConstants.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(RenderBackendTextureHandle(pcmd->TextureId)));
                shaderConstants.BindBuffer(1, vertexBuffer[frameInFlightCounter], 0);
                {
                    ImGuiShaderArguments sa = {};
                    sa.vertexOffset = pcmd->VtxOffset + globalVertexOffset;
                    sa.scale = scale;
                    sa.translate = translate;
                    shaderConstants.PushConstantsTest(&sa, sizeof(sa));
                }

                RenderBackendShaderHandle uiShader = shaderLibrary->GetShaderHandle((uint32)ShaderPipelineID::UIColorAndAlpha);
                commandList.DrawIndexed(
                    uiShader,
                    graphicsPipelineState,
                    shaderConstants,
                    indexBuffer[frameInFlightCounter],
                    pcmd->ElemCount,
                    1,
                    pcmd->IdxOffset + globalIndexOffset,
                    pcmd->VtxOffset + globalVertexOffset,
                    0,
                    RenderBackendPrimitiveTopology::TriangleList);
            }
            globalIndexOffset += cmdList->IdxBuffer.Size;
            globalVertexOffset += cmdList->VtxBuffer.Size;
        }

        commandList.EndRenderPass();
    }

    void RenderSystem::RenderScene(SceneView* view)
    {
        shaderLibrary->HotReload();

        RenderBackendCommandList* uploadCommandList = new RenderBackendCommandList(arena);

        gpuProfiler->BeginFrame(uploadCommandList);
        uint32 frameTimingQueryRegion = gpuProfiler->BeginRegion(uploadCommandList, "GPU Frametime");

        view->scene->GetEntityManager()->GetView<SkyAtmosphereComponent>().each([&](EntityHandle entity, SkyAtmosphereComponent& skyAtmosphere)
            {
                if (true)
                {
                    skyAtmosphereComponent = &skyAtmosphere;
                }
            });

        ((RealTimeRenderer*)renderPipeline)->UpdatePerFrameData(*view, uploadCommandList);
        UpdateRenderData(view, uploadCommandList);

        RenderGraph renderGraph(arena, renderGraphResourcePool, gpuProfiler);
        renderPipeline->SetupRenderGraph(renderGraph, *view);

        RenderGraphExecuteContext renderGraphExecuteContext;
        renderGraphExecuteContext.renderBackend = renderBackend;

        renderGraph.Execute(&renderGraphExecuteContext);

        gpuProfiler->EndRegion(frameTimingQueryRegion, renderGraphExecuteContext.commandLists.back());
        gpuProfiler->EndFrame(renderGraphExecuteContext.commandLists.back());

        /*uint32 numCommandLists = (uint32)renderGraphExecuteContext.commandLists.size();
        RenderBackendCommandList** commandLists = renderGraphExecuteContext.commandLists.data();*/

        std::vector<RenderBackendCommandList*> commandLists;
        commandLists.push_back(uploadCommandList);
        for (RenderBackendCommandList* commandList : renderGraphExecuteContext.commandLists)
        {
            commandLists.push_back(commandList);
        }
        //renderBackend->SubmitCommandLists(commandLists.data(), commandLists.size(), RenderBackendSwapChainHandle::Null);

        //RenderBackendCommandList* uploadCommandList = new RenderBackendCommandList(arena);
        renderBackend->SubmitCommandLists(commandLists.data(), (uint32)commandLists.size(), view->swapChain);

        delete uploadCommandList;

        renderGraphResourcePool->Tick();
    }
#endif
}