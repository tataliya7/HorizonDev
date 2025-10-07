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
        enableHardwareRayTracing = false;
        renderBackendType = RenderBackendType::Vulkan;

#if HORIZON_CONFIGURATION_RELEASE
        enableDebugLayer = false;
#endif

        std::vector<RenderBackendFeature> renderBackendFeatures = {};

        RenderBackendDesc renderBackendDesc =
        {
            .type = renderBackendType,
            .applicationName = "Horizon Demo",
            .applicationVersion = 0,
            .engineName = "Horizon Engine",
            .engineVersion = 0,
            .enableDebugLayer = enableDebugLayer,
            .features = renderBackendFeatures.data(),
            .featureCount = uint32(renderBackendFeatures.size())
        };

        renderBackend = RenderBackendCreateInstance(&renderBackendDesc);

        if (!renderBackend)
        {
            LogError(GLogger, std::format("Failed to create render backend instance."));
            return;
        }

        // Currently, multiple devices are not supported.
        uint32 primaryDeviceMask = 0;
        uint32 physicalDeviceID = 0;
        renderBackend->CreateRenderDevices(&physicalDeviceID, 1, &primaryDeviceMask);

        shaderRepository = new ShaderRepository(renderBackend, "../../../Source/HorizonEngine/Shaders");
        LoadAllShaders_Deprecated(shaderRepository);

        renderGraphResourcePool = new RenderGraphResourcePool(renderBackend);

        gpuProfiler = new RenderBackendGPUProfiler(renderBackend);

        RenderBackendCommandList* commandList = new RenderBackendCommandList(GArena);
        rendererDefaultResources = new RendererDefaultResources(renderBackend, renderGraphResourcePool, shaderRepository);
        rendererDefaultResources->Initialize(*commandList);
        renderBackend->SubmitCommandLists(&commandList, 1, RenderBackendSwapChainHandle::Null);

        renderBackend->FlushRenderDevices();

        for (uint32 t = 0; t < maxFramesInFlight; t++)
        {
            RenderBackendBufferDescription vertexBufferDesc = RenderBackendBufferDescription::CreateStructured(sizeof(ImDrawVert), 10000);
            vertexBuffer[t] = renderBackend->CreateBuffer(&vertexBufferDesc, nullptr, "ImGuiVertexBuffer");

            RenderBackendBufferDescription vertexBufferUploadDesc = RenderBackendBufferDescription::CreateUpload(vertexBufferDesc.size);
            vertexBufferUpload[t] = renderBackend->CreateBuffer(&vertexBufferUploadDesc, nullptr, "ImGuiVertexBufferUpload");

            RenderBackendBufferDescription indexBufferDesc = RenderBackendBufferDescription::CreateIndex(sizeof(uint32), 10000);
            indexBuffer[t] = renderBackend->CreateBuffer(&indexBufferDesc, nullptr, "ImGuiIndexBuffer");

            RenderBackendBufferDescription indexBufferUploadDesc = RenderBackendBufferDescription::CreateUpload(indexBufferDesc.size);
            indexBufferUpload[t] = renderBackend->CreateBuffer(&indexBufferUploadDesc, nullptr, "ImGuiIndexBufferUpload");

            vertexBufferSize[t] = vertexBufferDesc.size;
            indexBufferSize[t] = indexBufferDesc.size;
        }
    }

    void RenderSystem::Exit()
    {
        RenderBackendType renderBackendType = renderBackend->GetType();
        if (renderBackendType == RenderBackendType::Vulkan)
        {
            RenderBackendDestroyVulkan(renderBackend);
        }
        else if (renderBackendType == RenderBackendType::Direct3D12)
        {
            RenderBackendDestroyDirect3D12(renderBackend);
        }
    }

    void RenderSystem::Tick(float deltaTimeInSeconds)
    {
        OPTICK_EVENT();

        shaderRepository->HotReload();
        renderGraphResourcePool->Tick();
        renderBackend->Tick();
    }

    SceneRenderer* RenderSystem::CreateSceneRenderer(SceneView* sceneView)
    {
        assert(sceneView);

        SceneRenderer* sceneRenderer = nullptr;

        RenderMode renderMode = sceneView->GetRenderSettings().renderMode;

        if ((renderMode == RenderMode::RasterRendering) || (renderMode == RenderMode::HybridRendering))
        {
            sceneRenderer = new RasterizationRenderer(renderBackend, renderGraphResourcePool, shaderRepository, rendererDefaultResources);
        }
        else if ((renderMode == RenderMode::RealTimePathTracing) || (renderMode == RenderMode::ReferencePathTracing))
        {
            sceneRenderer = new PathTracingRenderer(renderBackend, renderGraphResourcePool, shaderRepository, rendererDefaultResources);
        }
        else
        {
            std::unreachable();
        }

        return sceneRenderer;
    }

    // void DestroySceneRenderer(SceneRenderer* sceneRenderer)
    // {
    //     assert(sceneRenderer != nullptr);
    //
    //     RenderBackend* renderBackend = sceneRenderer->GetRenderBackend();
    //
    //     renderBackend->FlushRenderDevices();
    //
    //     delete sceneRenderer;
    // }

    // SceneRenderer* RenderSystem::CreateRenderer()
    // {
    //
    //     return new RasterizationRenderer(renderBackend, renderGraphResourcePool, shaderRepository, rendererDefaultResources);
    // }

    void RenderSystem::RenderSceneView(SceneRenderer* renderer, SceneView* sceneView)
    {
        OPTICK_EVENT();

        RenderBackendCommandList* commandList = new RenderBackendCommandList(GArena);

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

    void RenderSystem::EndDrawUI()
    {
        OPTICK_EVENT();

        ImGui::EndFrame();
        ImGui::Render();

        ImDrawData* drawData = ImGui::GetDrawData();
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fbWidth = static_cast<int>(drawData->DisplaySize.x * drawData->FramebufferScale.x);
        int fbHeight = static_cast<int>(drawData->DisplaySize.y * drawData->FramebufferScale.y);
        if (fbWidth <= 0 || fbHeight <= 0)
        {
            return;
        }

        frameInFlightCounter = (frameInFlightCounter + 1) % maxFramesInFlight;

        // @todo Underlying buffer
        currentVertexBufferDataSize[frameInFlightCounter] = 0;
        currentIndexBufferDataSize[frameInFlightCounter] = 0;
        if (drawData->TotalVtxCount > 0)
        {
            // Create or reserve the vertex/index buffers
            currentVertexBufferDataSize[frameInFlightCounter] = drawData->TotalVtxCount * sizeof(ImDrawVert);
            currentIndexBufferDataSize[frameInFlightCounter] = drawData->TotalIdxCount * sizeof(ImDrawIdx);
            if (vertexBufferSize[frameInFlightCounter] < currentVertexBufferDataSize[frameInFlightCounter])
            {
                renderBackend->ResizeBuffer(vertexBuffer[frameInFlightCounter], currentVertexBufferDataSize[frameInFlightCounter]);
                renderBackend->ResizeBuffer(vertexBufferUpload[frameInFlightCounter], currentVertexBufferDataSize[frameInFlightCounter]);
                vertexBufferSize[frameInFlightCounter] = currentVertexBufferDataSize[frameInFlightCounter];
            }
            if (indexBufferSize[frameInFlightCounter] < currentIndexBufferDataSize[frameInFlightCounter])
            {
                renderBackend->ResizeBuffer(indexBuffer[frameInFlightCounter], currentIndexBufferDataSize[frameInFlightCounter]);
                renderBackend->ResizeBuffer(indexBufferUpload[frameInFlightCounter], currentIndexBufferDataSize[frameInFlightCounter]);
                indexBufferSize[frameInFlightCounter] = currentIndexBufferDataSize[frameInFlightCounter];
            }
            uint32 vertexOffset = 0;
            uint32 indexOffset = 0;

            void* vertexBufferDataPtr;
            void* indexBufferDataPtr;
            renderBackend->MapBuffer(vertexBufferUpload[frameInFlightCounter], &vertexBufferDataPtr);
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
        }
    }

    void RenderSystem::UpdateImGuiData(RenderBackendCommandList* commandList)
    {
        RenderBackendBarrier barrier[] =
        {
            RenderBackendBarrier()
        };
        commandList->Barriers(barrier, 1);

        // Update vertex buffer and index buffer for ImGui
        {
            if (currentVertexBufferDataSize[frameInFlightCounter] > 0)
            {
                RenderBackendBarrier barrier1[] =
                {
                    RenderBackendBarrier(vertexBuffer[frameInFlightCounter], RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::Undefined, RenderBackendResourceState::CopyDst)
                };
                commandList->Barriers(barrier1, 1);
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
                commandList->Barriers(barrier2, 1);
            }

            if (currentIndexBufferDataSize[frameInFlightCounter] > 0)
            {
                RenderBackendBarrier barrier1[] =
                {
                    RenderBackendBarrier(indexBuffer[frameInFlightCounter], RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::Undefined, RenderBackendResourceState::CopyDst)
                };
                commandList->Barriers(barrier1, 1);
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
                commandList->Barriers(barrier2, 1);
            }
        }
    }

    void RenderSystem::DrawUI(RenderBackendCommandList& commandList, RenderBackendTextureHandle output)
    {
        ImDrawData* drawData = ImGui::GetDrawData();
        // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
        int fbWidth = static_cast<int>(drawData->DisplaySize.x * drawData->FramebufferScale.x);
        int fbHeight = static_cast<int>(drawData->DisplaySize.y * drawData->FramebufferScale.y);
        if (fbWidth <= 0 || fbHeight <= 0)
        {
            return;
        }

        RenderBackendViewport viewport(0.0f, 0.0f, static_cast<float>(fbWidth), static_cast<float>(fbHeight));
        commandList.SetViewports(&viewport, 1);

        RenderBackendRenderPassInfo renderPass =
        {
            .renderTargets =
            {
                {
                    .texture = output,
                    .mipLevel = 0,
                    .loadOperation = RenderBackendRenderPassLoadOperation::Clear,
                    .storeOperation = RenderBackendRenderPassStoreOperation::Store
                }
            },
        };
        commandList.BeginRenderPass(renderPass);

        // Will project scissor/clipping rectangles into framebuffer space
        ImVec2 clipOffset = drawData->DisplayPos;         // (0,0) unless using multi-viewports
        ImVec2 clipScale = drawData->FramebufferScale;    // (1,1) unless using retina display which are often (2,2)

        RenderBackendShaderHandle vertexShader = shaderRepository->GetShader(ShaderID::ImGuiVS);
        RenderBackendShaderHandle pixelShader = shaderRepository->GetShader(ShaderID::ImGuiPS);

        RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
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

        // Render command lists
        // Because we merged all buffers into a single one, we maintain our own offset into them
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
                clipMin.x = std::clamp(clipMin.x, 0.0f, static_cast<float>(fbWidth));
                clipMin.y = std::clamp(clipMin.y, 0.0f, static_cast<float>(fbHeight));

                if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
                {
                    continue;
                }

                RenderBackendScissor scissor(static_cast<int32>(clipMin.x), static_cast<int32>(clipMin.y), static_cast<uint32>(clipMax.x - clipMin.x), static_cast<uint32>(clipMax.y - clipMin.y));
                commandList.SetScissors(&scissor, 1);

                Vector2f scale = Vector2f(2.0f / drawData->DisplaySize.x, -2.0f / drawData->DisplaySize.y);
                Vector2f translate = Vector2f(-1.0f - drawData->DisplayPos.x * scale.x, 1.0f + drawData->DisplayPos.y * scale.y);
                int vertexOffset = pcmd->VtxOffset + globalVertexOffset;

                RenderBackendPushConstantValues pushConstantValues = {};
                pushConstantValues.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(vertexBuffer[frameInFlightCounter]));
                pushConstantValues.BindTextureSRV(1, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(RenderBackendTextureHandle(pcmd->TextureId)));
                pushConstantValues.OverrideShaderConstantValue(2, scale.x);
                pushConstantValues.OverrideShaderConstantValue(3, scale.y);
                pushConstantValues.OverrideShaderConstantValue(4, translate.x);
                pushConstantValues.OverrideShaderConstantValue(5, translate.y);
                pushConstantValues.OverrideShaderConstantValue(6, vertexOffset);

                commandList.DrawIndexed(
                    vertexShader,
                    pixelShader,
                    graphicsPipelineState,
                    pushConstantValues,
                    indexBuffer[frameInFlightCounter],
                    pcmd->ElemCount,
                    1,
                    pcmd->IdxOffset + globalIndexOffset,
                    0,//pcmd->VtxOffset + globalVertexOffset,
                    0,
                    RenderBackendPrimitiveTopology::TriangleList);
            }

            globalIndexOffset += cmdList->IdxBuffer.Size;
            globalVertexOffset += cmdList->VtxBuffer.Size;
        }

        commandList.EndRenderPass();
    }

    void RenderSystem::RenderUserInterface(RenderGraph& renderGraph, const SceneView& view)
    {
        RenderGraphTextureDescription uiColorAndAlphaTextureDesc = RenderGraphTextureDescription::Create2D(
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
                builder.SetRenderTargetBinding(0, uiColorAndAlphaTexture, RenderBackendRenderPassLoadOperation::Clear, RenderBackendRenderPassStoreOperation::Store);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    DrawUI(commandList, resourceRegistry.GetRenderBackendTextureHandle(uiColorAndAlphaTexture));
                };
            });

        RenderGraphTextureHandle displayTexture = renderGraph.ImportExternalTexture(view.displayTexture, "DisplayTexture");
        bool offscreen = view.displayTexture != view.targetTexture;

        renderGraph.AddPass(
            std::format("GUIComposition (Graphics, {}x{})", view.displayWidth, view.displayHeight),
            RenderGraphPassFlags::Graphics,
            [&](RenderGraphBuilder& builder)
            {
                builder.SetBindlessResourceSRV(0, uiColorAndAlphaTexture);

                builder.SetRenderTargetBinding(0, displayTexture, offscreen ? RenderBackendRenderPassLoadOperation::Clear : RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);

                RenderBackendShaderHandle vertexShader = shaderRepository->GetShader(ShaderID::DrawFullscreenQuadVS);
                RenderBackendShaderHandle pixelShader = shaderRepository->GetShader(ShaderID::GUICompositionPS);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    RenderBackendViewport viewport(0.0f, 0.0f, float(view.displayWidth), float(view.displayHeight));
                    commandList.SetViewports(&viewport, 1);

                    RenderBackendScissor scissor(0, 0, view.displayWidth, view.displayHeight);
                    commandList.SetScissors(&scissor, 1);

                    RenderBackendGraphicsPipelineStateDescription graphicsPipelineState = {};
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

                    RenderBackendPushConstantValues pushConstantValues = resourceRegistry.GetPushConstantValues();

                    commandList.Draw(
                        vertexShader,
                        pixelShader,
                        graphicsPipelineState,
                        pushConstantValues,
                        3, 1, 0, 0,
                        RenderBackendPrimitiveTopology::TriangleList);
                };
            });
    }
}