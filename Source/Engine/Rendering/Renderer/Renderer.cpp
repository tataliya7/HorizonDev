#include "Rendering/Renderer/Renderer.h"
#include "Entity/EntityModule.h"
#include "Rendering/RenderGraph/RenderGraph.h"
#include "Rendering/ShaderLibrary.h"
#include "Rendering/ShaderCompiler.h"
#include "Rendering/DxcShaderCompiler/DxcShaderCompiler.h"
#include "Rendering/Renderer/RealTimeRenderer/RealTimeRenderer.h"
#include "Asset/Scene.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>

#include <optick.h>

namespace HE
{
    Renderer* GRenderer = nullptr;

    void Texture2DGenerateMips(ShaderLibrary_Deprecated* shaderLibrary, RenderBackendCommandList& commandList, RenderBackendTextureHandle textureHandle, uint32 width, uint32 height, uint32 numMipLevels)
    {
        if (numMipLevels < 2)
        {
            return;
        }

        RenderBackendShaderHandle graphicsShader = shaderLibrary->GetShaderHandle((uint32)ShaderPipelineID::DownsampleTexture2D_PS);
        for (uint32 mipLevel = 1; mipLevel < numMipLevels; mipLevel++)
        {
            width = width >> 1;
            height = height >> 1;

            RenderBackendViewport viewport(0.0f, 0.0f, (float)width, (float)height);
            commandList.SetViewports(&viewport, 1);

            RenderBackendScissor scissor(0, 0, width, height);
            commandList.SetScissors(&scissor, 1);

            if (mipLevel == 1)
            {
                RenderBackendBarrier transitions[] =
                {
                    RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RENDER_BACKEND_REMAINING_ARRAY_LAYERS), RenderBackendResourceState::Undefined, RenderBackendResourceState::RenderTarget)
                };
                commandList.Transitions(transitions, 1);
            }
            else
            {
                RenderBackendBarrier transitions[] =
                {
                    RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel - 1, 1, 0, RENDER_BACKEND_REMAINING_ARRAY_LAYERS), RenderBackendResourceState::RenderTarget, RenderBackendResourceState::ShaderResource),
                    RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(mipLevel, 1, 0, RENDER_BACKEND_REMAINING_ARRAY_LAYERS), RenderBackendResourceState::Undefined, RenderBackendResourceState::RenderTarget)
                };
                commandList.Transitions(transitions, 2);
            }

            RenderBackendRenderPassInfo renderPass = {
                .colorRenderTargets = { {.texture = textureHandle, .mipLevel = mipLevel, .arrayLayer = 0, .loadOp = RenderBackendRenderTargetLoadOp::DontCare, .storeOp = RenderBackendRenderTargetStoreOp::Store } },
            };
            commandList.BeginRenderPass(renderPass);

            RenderBackendShaderArguments shaderArguments = {};
            shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(textureHandle));
            shaderArguments.PushConstants(0, (float)(mipLevel - 1));
            shaderArguments.PushConstants(1, (float)(width));
            shaderArguments.PushConstants(2, (float)(height));

            RenderBackendGraphicsPipelineState graphicsPipelineState = {};

            commandList.Draw(
                graphicsShader,
                graphicsPipelineState,
                shaderArguments,
                3, 1, 0, 0,
                RenderBackendPrimitiveTopology::TriangleList);

            commandList.EndRenderPass();

            //uint32 dispatchX = Math::CeilDiv(width, 8);
            //uint32 dispatchY = Math::CeilDiv(height, 8);
            //uint32 dispatchZ = 1;

            //commandList.Dispatch(
            //    downsampleTexture2DCS,
            //    shaderArguments,
            //    dispatchX,
            //    dispatchY,
            //    dispatchZ);
        }
        RenderBackendBarrier transition = RenderBackendBarrier(textureHandle, RenderBackendTextureSubresourceRange(numMipLevels - 1, RENDER_BACKEND_REMAINING_MIP_LEVELS, 0, RENDER_BACKEND_REMAINING_ARRAY_LAYERS), RenderBackendResourceState::RenderTarget, RenderBackendResourceState::ShaderResource);
        commandList.Transitions(&transition, 1);
    }

    RealTimeRendererSettings& Renderer::GetRealTimeRendererSettings_Deprecated()
    {
        return ((RealTimeRenderer*)renderPipeline)->settings;
    }

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
        viewMatrix = Math::Inverse(invViewMatrix);
        parameters.viewProjectionMatrix[0] = projectionMatrix * viewMatrix;

        // -X
        rotation = glm::rotate(Quaternion(), Math::DegreesToRadians(90.0), Vector3(0.0, 0.0, 1.0));
        invViewMatrix = Math::Compose(position, rotation * zUpQuat, Vector3(1.0f, 1.0f, 1.0f));
        viewMatrix = Math::Inverse(invViewMatrix);
        parameters.viewProjectionMatrix[1] = projectionMatrix * viewMatrix;

        // +Y
        rotation = Quaternion();
        invViewMatrix = Math::Compose(position, rotation * zUpQuat, Vector3(1.0f, 1.0f, 1.0f));
        viewMatrix = Math::Inverse(invViewMatrix);
        parameters.viewProjectionMatrix[2] = projectionMatrix * viewMatrix;

        // -Y
        rotation = glm::rotate(Quaternion(), Math::DegreesToRadians(180.0), Vector3(0.0, 0.0, 1.0));
        invViewMatrix = Math::Compose(position, rotation * zUpQuat, Vector3(1.0f, 1.0f, 1.0f));
        viewMatrix = Math::Inverse(invViewMatrix);
        parameters.viewProjectionMatrix[3] = projectionMatrix * viewMatrix;

        // +Z
        rotation = glm::rotate(Quaternion(), Math::DegreesToRadians(90.0), Vector3(1.0, 0.0, 0.0));
        invViewMatrix = Math::Compose(position, rotation * zUpQuat, Vector3(1.0f, 1.0f, 1.0f));
        viewMatrix = Math::Inverse(invViewMatrix);
        parameters.viewProjectionMatrix[4] = projectionMatrix * viewMatrix;

        // -Z
        rotation = glm::rotate(Quaternion(), Math::DegreesToRadians(-90.0), Vector3(1.0, 0.0, 0.0));
        invViewMatrix = Math::Compose(position, rotation * zUpQuat, Vector3(1.0f, 1.0f, 1.0f));
        viewMatrix = Math::Inverse(invViewMatrix);
        parameters.viewProjectionMatrix[5] = projectionMatrix * viewMatrix;
    }

    static void SetupShadowCascades(const SceneView& view, const LightComponent& lightComponent, const CameraComponent& camera, CascadedShadowMapShaderParameters& outCascades)
    {
        const Vector3& lightDirection = lightComponent.GetDirection();
        const uint32 numShadowCascades = lightComponent.GetNumDynamicShadowCascades();
        const float maxShadowDistance = lightComponent.GetMaxShadowDistance();

        const float nearClippingPlaneDistance = camera.nearClippingPlane;

        //
     /*   float cascadeNearDistance = ;
        float cascadeFarDistance = ;*/

        uint32 numCascades = lightComponent.GetNumDynamicShadowCascades();

        float cascadeSplits[RendererMaxShadowMapCascadeCount];

        float nearClip = camera.nearClippingPlane;
        float farClip = maxShadowDistance;//camera.farClippingPlane;

        float clipRange = farClip - nearClip;

        float minZ = nearClip;
        float maxZ = nearClip + clipRange;

        float range = maxZ - minZ;
        float ratio = maxZ / minZ;

        // Calculate split plane based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
        for (uint32_t i = 0; i < numCascades; i++)
        {
            float p = (i + 1) / static_cast<float>(numCascades);
            float log = minZ * std::pow(ratio, p);
            float uniform = minZ + range * p;
            float d = lightComponent.GetCascadeSplitLambda() * (log - uniform) + uniform;
            cascadeSplits[i] = (d - nearClip) / clipRange;
        }

        // Calculate orthographic projection matrix for each cascade
        float lastSplitDist = 0.0;
        for (uint32_t cascadeIndex = 0; cascadeIndex < numCascades; cascadeIndex++)
        {
            float splitDist = cascadeSplits[cascadeIndex];

            glm::vec3 frustumCorners[8] = {
                glm::vec3(-1.0f,  1.0f,  1.0f),
                glm::vec3(1.0f,  1.0f,  1.0f),
                glm::vec3(1.0f, -1.0f,  1.0f),
                glm::vec3(-1.0f, -1.0f,  1.0f),
                glm::vec3(-1.0f,  1.0f,  0.0f),
                glm::vec3(1.0f,  1.0f,  0.0f),
                glm::vec3(1.0f, -1.0f,  0.0f),
                glm::vec3(-1.0f, -1.0f,  0.0f),
            };

            // Project frustum corners into world space
            glm::mat4 cameraProjectionMatrix = Math::PerspectiveReverseZ_RH_ZO(Math::DegreesToRadians(camera.fieldOfView), camera.aspectRatio, camera.nearClippingPlane, farClip);
            glm::mat4 cameraInvProjectionMatrix = Math::Inverse(cameraProjectionMatrix);

            glm::mat4 invCam = camera.invViewMatrix * cameraInvProjectionMatrix;
            for (uint32_t i = 0; i < 8; i++)
            {
                glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
                frustumCorners[i] = invCorner / invCorner.w;
            }

            for (uint32_t i = 0; i < 4; i++) {
                glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
                frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
                frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
            }

            // Get frustum center
            glm::vec3 frustumCenter = glm::vec3(0.0f);
            for (uint32_t i = 0; i < 8; i++)
            {
                frustumCenter += frustumCorners[i];
            }
            frustumCenter /= 8.0f;

            float radius = 0.0f;
            for (uint32_t i = 0; i < 8; i++)
            {
                float distance = glm::length(frustumCorners[i] - frustumCenter);
                radius = glm::max(radius, distance);
            }
            radius = std::ceil(radius * 16.0f) / 16.0f;

            glm::vec3 maxExtents = glm::vec3(radius);
            glm::vec3 minExtents = -maxExtents;


            glm::mat viewMatrix = glm::lookAt(frustumCenter - lightDirection * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat projectionMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, maxExtents.z - minExtents.z, 0.0f);

            lastSplitDist = cascadeSplits[cascadeIndex];

            outCascades.viewProjectionMatrix[cascadeIndex] = projectionMatrix * viewMatrix;
            outCascades.splitDepth[cascadeIndex] = (nearClip + splitDist * clipRange) * -1.0f;
            outCascades.numCascades = numCascades;
        }
    }

    Renderer::Renderer()
        : RenderEngine("Horizon Renderer")
    {

    }

    Renderer::~Renderer()
    {

    }

    void Renderer::Init(void* window)
    {
        arena = GArena;
        renderBackend = GRenderBackend;

        assert(GRenderer == nullptr);
        GRenderer = this;

        shaderCompiler = CreateDXCShaderCompiler();
        shaderLibrary = new ShaderLibrary_Deprecated(renderBackend, shaderCompiler, (uint32)ShaderPipelineID::Count, true);
        shaderLibrary->AddIncludeDirectory("../../../Shaders");
        shaderLibrary->AddIncludeDirectory("../../../Shaders/RealTimeRenderer");

        RenderBackendTimingQueryHeapDesc timingQueryHeapDesc(RenderBackendMaxNumTimingQueryRegions);
        timingQueryHeap = renderBackend->CreateTimingQueryHeap(~0u, &timingQueryHeapDesc, "DefaultTimingQueryHeap");
        gpuProfiler = new RenderBackendGPUProfiler(renderBackend);

        ShaderDesc shaderDesc;
        shaderDesc = ShaderDesc::CreateCompute("EquirectangularToCubemap.hsf", "EquirectangularToCubemapCS");
        shaderLibrary->LoadShader((uint32)ShaderPipelineID::EquirectangularToCubemap, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("DownsampleCubemap.hsf", "DownsampleCubemapCS");
        shaderLibrary->LoadShader((uint32)ShaderPipelineID::DownsampleCubemap, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("DownsampleTexture2D.hsf", "DownsampleTexture2DCS");
        shaderLibrary->LoadShader((uint32)ShaderPipelineID::DownsampleTexture2D, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("DownsampleTexture2D_PS.hsf", "DownsampleTexture2D_VS", "DownsampleTexture2D_PS");
        shaderLibrary->LoadShader((uint32)ShaderPipelineID::DownsampleTexture2D_PS, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("ComputeEnvironmentIrradiance.hsf", "ComputeEnvironmentIrradianceCS");
        shaderLibrary->LoadShader((uint32)ShaderPipelineID::ComputeEnvironmentIrradiance, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("ComputeEnvironmentIrradianceSH.hsf", "ComputeEnvironmentIrradianceSHCS");
        shaderLibrary->LoadShader((uint32)ShaderPipelineID::ComputeEnvironmentIrradianceSH, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("FilterEnvironmentMap.hsf", "FilterEnvironmentMapCS");
        shaderLibrary->LoadShader((uint32)ShaderPipelineID::FilterEnvironmentMap, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("GPUFFT.hsf", "SharedMemoryComplexFFTCS");
        shaderLibrary->LoadShader((uint32)ShaderPipelineID::SharedMemoryComplexFFT, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("GPUFFT.hsf", "SharedMemoryComplexIFFTCS");
        shaderLibrary->LoadShader((uint32)ShaderPipelineID::SharedMemoryComplexIFFT, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("GPUFFT.hsf", "SharedMemoryTwoForOneRealFFTCS");
        shaderLibrary->LoadShader((uint32)ShaderPipelineID::SharedMemoryTwoForOneRealFFT, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("GPUFFT.hsf", "SharedMemoryTwoForOneRealIFFTCS");
        shaderLibrary->LoadShader((uint32)ShaderPipelineID::SharedMemoryTwoForOneRealIFFT, shaderDesc);

        shaderDesc = ShaderDesc::CreateCompute("GPUFFT.hsf", "SharedMemoryComplexFFTConvolutionCS");
        shaderLibrary->LoadShader((uint32)ShaderPipelineID::SharedMemoryComplexFFTConvolution, shaderDesc);

        shaderDesc = ShaderDesc::CreateGraphics("RealTimeRenderer/UIColorAndAlpha.hsf", "UIColorAndAlphaVS", "UIColorAndAlphaPS");
        shaderLibrary->LoadShader((uint32)ShaderPipelineID::UIColorAndAlpha, shaderDesc);

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
            std::string filename = "../../../Shaders/ImGui.hsf";
            LoadShaderSourceFromFile("../../../Shaders/ImGui.hsf", source);


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
    }

    void Renderer::Exit()
    {

    }

    void Renderer::AddLight(const SceneView& view, LightComponent& lightComponent, const CameraComponent& camera)
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

    void Renderer::UpdateRenderData(SceneView* view, RenderBackendCommandList* commandList)
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
                lightDataBuffer = renderBackend->CreateBuffer(deviceMask, &lightBufferDesc, nullptr, "LightDataBuffer");
                RenderBackendBufferDesc lightDataUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(RendererMaxLightCount * sizeof(LightShaderParameters));
                lightDataUploadBuffer = renderBackend->CreateBuffer(deviceMask, &lightDataUploadBufferDesc, nullptr, "LightDataUploadBuffer");

                RenderBackendBufferDesc cascadedShadowMapBufferDesc = RenderBackendBufferDesc::CreateByteAddress(RendererMaxCascadedShadowMapCount * sizeof(CascadedShadowMapShaderParameters));
                cascadedShadowMapBuffer = renderBackend->CreateBuffer(deviceMask, &cascadedShadowMapBufferDesc, nullptr, "CascadedShadowMapBuffer");
                RenderBackendBufferDesc cascadedShadowMapUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(RendererMaxCascadedShadowMapCount * sizeof(CascadedShadowMapShaderParameters));
                cascadedShadowMapUploadBuffer = renderBackend->CreateBuffer(deviceMask, &cascadedShadowMapUploadBufferDesc, nullptr, "CascadedShadowMapUploadBuffer");

                RenderBackendBufferDesc cubeShadowMapBufferDesc = RenderBackendBufferDesc::CreateByteAddress(RendererMaxCubeShadowMapCount * sizeof(CubeShadowMapShaderParameters));
                cubeShadowMapBuffer = renderBackend->CreateBuffer(deviceMask, &cubeShadowMapBufferDesc, nullptr, "CubeShadowMapBuffer");
                RenderBackendBufferDesc cubeShadowMapUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(RendererMaxCubeShadowMapCount * sizeof(CubeShadowMapShaderParameters));
                cubeShadowMapUploadBuffer = renderBackend->CreateBuffer(deviceMask, &cubeShadowMapUploadBufferDesc, nullptr, "CubeShadowMapUploadBuffer");
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
                                materialShaderParameters.textures[slot].bindlessTextureIndex = renderBackend->GetTextureSRVDescriptorIndex(deviceMask, material.textures[slot].gpuTexture);
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
                materialUploadBuffer = renderBackend->CreateBuffer(deviceMask, &materialUploadBufferDesc, nullptr, "MaterialUploadBuffer");
                RenderBackendBufferDesc materialBufferDesc = RenderBackendBufferDesc::CreateByteAddress(newMaterialBufferSize);
                materialBuffer = renderBackend->CreateBuffer(deviceMask, &materialBufferDesc, nullptr, "MaterialBuffer");
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
                transforms.push_back(transformComponent.matrix);
                rowMajorTransforms.push_back(Math::Transpose(transformComponent.matrix));

                GeometryShaderParameters geometry;
                geometry.vertexBuffer0 = renderBackend->GetBufferUAVDescriptorIndex(deviceMask, mesh.vertexBuffers[0]);
                geometry.vertexBuffer1 = renderBackend->GetBufferUAVDescriptorIndex(deviceMask, mesh.vertexBuffers[1]);
                geometry.vertexBuffer2 = renderBackend->GetBufferUAVDescriptorIndex(deviceMask, mesh.vertexBuffers[2]);
                geometry.vertexBuffer3 = renderBackend->GetBufferUAVDescriptorIndex(deviceMask, mesh.vertexBuffers[3]);
                geometry.prevVertexBuffer0 = -1;
                geometry.indexBuffer = renderBackend->GetBufferUAVDescriptorIndex(deviceMask, mesh.indexBuffer);
                //geometry.transformBuffer = renderBackend->GetBufferUAVDescriptorIndex(deviceMask, transformBuffer);
                //geometry.previousTransformBuffer = renderBackend->GetBufferUAVDescriptorIndex(deviceMask, previousTransformBuffer);
                geometry.materialIndexBuffer = renderBackend->GetBufferUAVDescriptorIndex(deviceMask, mesh.materialIndexBuffer);
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
                transformUploadBuffer = renderBackend->CreateBuffer(deviceMask, &transformUploadBufferDesc, nullptr, "TransformUploadBuffer");
                RenderBackendBufferDesc transformBufferDesc = RenderBackendBufferDesc::CreateByteAddress(newTransformBufferSize);
                transformBuffer = renderBackend->CreateBuffer(deviceMask, &transformBufferDesc, nullptr, "TransformBuffer");
                RenderBackendBufferDesc transformBufferRowMajorDesc = RenderBackendBufferDesc::CreateUpload(newTransformBufferSize, RenderBackendBufferCreateFlags::AccelerationStruture);
                transformBufferRowMajor = renderBackend->CreateBuffer(deviceMask, &transformBufferRowMajorDesc, nullptr, "TransformBufferRowMajor");
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
                geometry.transformBuffer = renderBackend->GetBufferUAVDescriptorIndex(deviceMask, transformBuffer);
                geometry.previousTransformBuffer = renderBackend->GetBufferUAVDescriptorIndex(deviceMask, previousTransformBuffer);
            }

            uint32 newGeometryBufferSize = numGeometries * sizeof(GeometryShaderParameters);
            if (geometryBuffer == RenderBackendBufferHandle::Null && newGeometryBufferSize > 0)
            {
                RenderBackendBufferDesc geometryUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(newGeometryBufferSize);
                geometryUploadBuffer = renderBackend->CreateBuffer(deviceMask, &geometryUploadBufferDesc, nullptr, "GeometryUploadBuffer");
                RenderBackendBufferDesc geometryBufferDesc = RenderBackendBufferDesc::CreateByteAddress(newGeometryBufferSize);
                geometryBuffer = renderBackend->CreateBuffer(deviceMask, &geometryBufferDesc, nullptr, "GeometryBuffer");
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
                bottomLevelAS = renderBackend->CreateRayTracingBottomLevelAccelerationStructure(deviceMask, &blasDesc, "BLAS");

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
                rayTracingScene = renderBackend->CreateRayTracingTopLevelAccelerationStructure(deviceMask, &tlasDesc, "TLAS");
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
            debugDrawLinesVertexBuffer = renderBackend->CreateBuffer(deviceMask, &bufferDesc, nullptr, "DebugDrawLinesVertexBuffer");

            RenderBackendBufferDesc debugDrawLinesVertexUploadBufferDesc = RenderBackendBufferDesc::CreateUpload(newDebugDrawLinesVertexBufferSize);
            debugDrawLinesVertexUploadBuffer = renderBackend->CreateBuffer(deviceMask, &debugDrawLinesVertexUploadBufferDesc, nullptr, "DebugDrawLinesVertexUploadBuffer");
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

    void Renderer::BeginDrawUI()
    {
        OPTICK_EVENT();

        ImGui::SetCurrentContext(context);
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void Renderer::EndDrawUI()
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

    void Renderer::DrawUI(RenderBackendCommandList& commandList, RenderBackendTextureHandle output)
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
            .colorRenderTargets = { {.texture = output, .mipLevel = 0, .arrayLayer = 0, .loadOp = RenderBackendRenderTargetLoadOp::Clear, .storeOp = RenderBackendRenderTargetStoreOp::Store } },
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

                RenderBackendShaderArguments shaderArguments = {};
                shaderArguments.BindTextureSRV(0, RenderBackendTextureSRVDesc::Create(RenderBackendTextureHandle(pcmd->TextureId)));
                shaderArguments.BindBuffer(1, vertexBuffer[frameInFlightCounter], 0);
                {
                    ImGuiShaderArguments sa = {};
                    sa.vertexOffset = pcmd->VtxOffset + globalVertexOffset;
                    sa.scale = scale;
                    sa.translate = translate;
                    shaderArguments.PushConstantsTest(&sa, sizeof(sa));
                }

                RenderBackendShaderHandle uiShader = shaderLibrary->GetShaderHandle((uint32)ShaderPipelineID::UIColorAndAlpha);
                commandList.DrawIndexed(
                    uiShader,
                    graphicsPipelineState,
                    shaderArguments,
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

    void Renderer::UpdateRayTracingAccelerationStructures(SceneView* view, RenderBackendCommandList* commandList)
    {
        return;
       /* commandList->CopyBuffer(
            instanceUploadBuffer,
            0,
            instanceBuffer,
            0,
            instanceBufferDesc.size);*/

        Scene* scene = view->scene;
        EntityManager* entityManager = scene->GetEntityManager();

        entityManager->GetView<MeshComponent>().each([&](EntityHandle entity, MeshComponent& mesh)
            {
                RayTracingGeometry& rayTracingGeometry = mesh.rayTracingGeometry;
                switch (rayTracingGeometry.state)
                {
                case RayTracingGeometryState::BuildRequired:
                    commandList->BuildRayTracingBottomLevelAccelerationStructure(rayTracingGeometry.blas);
                    break;
                case RayTracingGeometryState::UpdateRequired:
                    commandList->UpdateRayTracingBottomLevelAccelerationStructure(rayTracingGeometry.blas, rayTracingGeometry.blas);
                    break;
                default:
                    break;
                }
                rayTracingGeometry.state = RayTracingGeometryState::UpToDate;
            });

        commandList->BuildRayTracingTopLevelAccelerationStructure(rayTracingScene);

        SetShouldUpdateRayTracingScene(false);
    }

    void Renderer::RenderScene(SceneView* view)
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

        RenderGraph renderGraph(arena, gpuProfiler);
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

        GRenderGraphResourcePool->Tick();
    }
}