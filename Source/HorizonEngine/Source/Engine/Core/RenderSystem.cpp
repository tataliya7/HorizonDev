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
        renderBackendType = RenderBackendType::Direct3D12;

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

        shaderLibrary = new ShaderCollection(renderBackend, "../../../Source/HorizonEngine/Shaders");
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
            RenderBackendBufferDescription vertexBufferDesc = RenderBackendBufferDescription::CreateStructured(sizeof(ImDrawVert), 1);
            vertexBuffer[t] = renderBackend->CreateBuffer(&vertexBufferDesc, nullptr, "ImGuiVertexBuffer");

            RenderBackendBufferDescription vertexBufferUploadDesc = RenderBackendBufferDescription::CreateUpload(4);
            vertexBufferUpload[t] = renderBackend->CreateBuffer(&vertexBufferUploadDesc, nullptr, "ImGuiVertexBufferUpload");

            RenderBackendBufferDescription indexBufferDesc = RenderBackendBufferDescription::CreateIndex(sizeof(uint32), 1);
            indexBuffer[t] = renderBackend->CreateBuffer(&indexBufferDesc, nullptr, "ImGuiIndexBuffer");

            RenderBackendBufferDescription indexBufferUploadDesc = RenderBackendBufferDescription::CreateUpload(4);
            indexBufferUpload[t] = renderBackend->CreateBuffer(&indexBufferUploadDesc, nullptr, "ImGuiIndexBufferUpload");

            vertexBufferSize[t] = 4;
            indexBufferSize[t] = 4;
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
        shaderLibrary->HotReload();
        renderGraphResourcePool->Tick();
        renderBackend->Tick();
    }

    RasterizationRenderer* RenderSystem::CreateRenderer()
    {
        return new RasterizationRenderer(renderBackend, renderGraphResourcePool, shaderLibrary, rendererDefaultResources);
    }

    void RenderSystem::RenderSceneView(RasterizationRenderer* renderer, SceneView* sceneView)
    {
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
        int fbWidth = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
        int fbHeight = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
        if (fbWidth <= 0 || fbHeight <= 0)
        {
            return;
        }

        frameInFlightCounter = (frameInFlightCounter + 1) % 3;

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
                    RenderBackendBarrier(vertexBuffer[frameInFlightCounter], RenderBackendBufferSubresourceRange::Whole, RenderBackendResourceState::CopyDst, RenderBackendResourceState::UnorderedAccess)
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
        int fbWidth = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
        int fbHeight = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
        if (fbWidth <= 0 || fbHeight <= 0)
        {
            return;
        }

        RenderBackendViewport viewport(0.0f, 0.0f, (float)fbWidth, (float)fbHeight);
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

        RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::ImGuiVS);
        RenderBackendShaderHandle pixelShader = shaderLibrary->GetShader(ShaderID::ImGuiPS);

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
        // (Because we merged all buffers into a single one, we maintain our own offset into them)
        int globalIndexOffset = 0;
        int globalVertexOffset = 0;
        for (int i = 0; i < drawData->CmdListsCount; i++)
        {
            const ImDrawList* cmdList = drawData->CmdLists[i];
            for (int drawCallIndex = 0; drawCallIndex < cmdList->CmdBuffer.Size; drawCallIndex++)
            {
                //if (drawCallIndex >= 1)
                //{
                //    int a = 0;
                //}
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

                Vector2f scale = Vector2f(2.0f / drawData->DisplaySize.x, -2.0f / drawData->DisplaySize.y);
                Vector2f translate = Vector2f(-1.0f - drawData->DisplayPos.x * scale.x, 1.0f + drawData->DisplayPos.y * scale.y);
                int vertexOffset = pcmd->VtxOffset + globalVertexOffset;

                RenderBackendPushConstantValues shaderConstants = {};
                shaderConstants.BindBufferSRV(0, renderBackend->GetBufferSRVBindlessResourceDescriptorIndex(vertexBuffer[frameInFlightCounter]));
                shaderConstants.BindTextureSRV(1, renderBackend->GetTextureSRVBindlessResourceDescriptorIndex(RenderBackendTextureHandle(pcmd->TextureId)));
                shaderConstants.BindScalar(2, scale.x);
                shaderConstants.BindScalar(3, scale.y);
                shaderConstants.BindScalar(4, translate.x);
                shaderConstants.BindScalar(5, translate.y);
                shaderConstants.BindScalar(6, vertexOffset);

                commandList.DrawIndexed(
                    vertexShader,
                    pixelShader,
                    graphicsPipelineState,
                    shaderConstants,
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
                uiColorAndAlphaTexture = builder.WriteTexture(uiColorAndAlphaTexture, RenderBackendResourceState::RenderTarget);

                builder.SetRenderTargetBinding(0, uiColorAndAlphaTexture, RenderBackendRenderPassLoadOperation::Clear, RenderBackendRenderPassStoreOperation::Store);

                return [=](RenderBackendCommandList& commandList, const RenderGraphResourceRegistry& resourceRegistry)
                {
                    DrawUI(commandList, resourceRegistry.GetRenderBackendTextureHandle(uiColorAndAlphaTexture));
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

                builder.SetRenderTargetBinding(0, displayTexture, RenderBackendRenderPassLoadOperation::Load, RenderBackendRenderPassStoreOperation::Store);

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

                    RenderBackendPushConstantValues shaderConstants = {};
                    shaderConstants.BindTextureSRV(0, resourceRegistry.GetTextureSRVBindlessResourceDescriptorIndex(uiColorAndAlphaTexture));

                    RenderBackendShaderHandle vertexShader = shaderLibrary->GetShader(ShaderID::DrawFullscreenQuadVS);
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
}