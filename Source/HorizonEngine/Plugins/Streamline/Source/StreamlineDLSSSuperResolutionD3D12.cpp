#include <sl.h>
#include <sl_consts.h>
#include <sl_hooks.h>
#include <sl_version.h>
#include <sl_reflex.h>
#include <sl_dlss.h>
#include <sl_dlss_g.h>
#include <sl_helpers.h>

static void StreamlineLogMessageCallback(sl::LogType type, const char* msg)
{
    switch (type)
    {
    case sl::LogType::eError:
        Horizon::LogError(Horizon::GLogger, std::format("Streamline: {}", msg));
        break;
    case sl::LogType::eWarn:
        Horizon::LogWarning(Horizon::GLogger, std::format("Streamline: {}", msg));
        break;
    case sl::LogType::eInfo:
        Horizon::LogInfo(Horizon::GLogger, std::format("Streamline: {}", msg));
        break;
        break;
    }
}

static void myAPIErrorCallback(const sl::APIError& e)
{
    // Handle error, use e.hres with DirectX and e.vkRes on Vulkan
    printf("HRESULT %d\n", e.hres);
};

namespace Streamline
{

    {
        sl::Feature streamlineFeatures[] = { sl::kFeatureReflex, sl::kFeatureDLSS, sl::kFeatureDLSS_G };

        sl::Preferences pref = {};
        pref.showConsole = true;
        pref.logLevel = sl::LogLevel::eDefault;
        pref.pathsToPlugins = nullptr;
        pref.numPathsToPlugins = 0;
        pref.pathToLogsAndData = nullptr;
        pref.allocateCallback = nullptr;
        pref.releaseCallback = nullptr;
        pref.logMessageCallback = StreamlineLogMessageCallback;
        pref.flags = sl::PreferenceFlags::eDisableCLStateTracking;// | sl::PreferenceFlags::eAllowOTA;
        pref.featuresToLoad = streamlineFeatures;
        pref.numFeaturesToLoad = _countof(streamlineFeatures);
        pref.applicationId = sl::INVALID_UINT;
        pref.engine = sl::EngineType::eCustom;
        pref.engineVersion = "Horizon Engine";
        pref.projectId = "a0f57b54-1daf-4934-90ae-c4035c19df04";
        pref.renderAPI = sl::RenderAPI::eD3D12;

        sl::Result result;
        if (SL_FAILED(result, slInit(pref, sl::kSDKVersion)))
        {
            LogInfo(GLogger, std::format("slInit, error code: {}.", (int32)result));
        }

        sl::ReflexState state = {};
        if (SL_FAILED(result, slReflexGetState(state)))
        {
            LogInfo(GLogger, std::format("slReflexGetState, error code: {}", (int32)result));
        }
    }
        {
            sl::Result slResult = slSetD3DDevice(device.Get());
            if (slResult == sl::Result::eOk)
            {
                // Set reflex consts to a default config. This can be changed at runtime in the UI.
                sl::ReflexOptions reflexOptions = {};
                reflexOptions.mode = sl::ReflexMode::eLowLatency;
                reflexOptions.frameLimitUs = 0;
                reflexOptions.useMarkersToOptimize = true;
                //reflexOptions.virtualKey = VK_F13;
                if (SL_FAILED(result, slReflexSetOptions(reflexOptions)))
                {
                    LogError(GLogger, std::format("slReflexSetOptions, error code: {}", (int32)result));
                }
            }
        }
    {
        #if HE_ENBALE_STREAMLINE_SUPPORT
        uint32 targetWidth = command.targetWidth;
        uint32 targetHeight = command.targetHeight;

        sl::ViewportHandle viewport = 0;

        sl::Result result;

        static bool first = true;
        if (first)
        {
            device->WaitIdle();
            slFreeResources(sl::kFeatureDLSS_G, viewport);
            first = false;
        }

        sl::DLSSGOptions options{};
        // These are populated based on user selection in the UI
        options.mode = sl::DLSSGMode::eOn;
        options.onErrorCallback = myAPIErrorCallback;

        // IMPORTANT: Note that we are using IDENTICAL viewport as when tagging our resources
        result = slDLSSGSetOptions(viewport, options);
        if (result != sl::Result::eOk)
        {
            printf("slDLSSGSetOptions\n");
            // Handle error here, check the logs
        }

        sl::DLSSGState state = {};
        result = slDLSSGGetState(viewport, state, &options);
        if (result != sl::Result::eOk)
        {
            printf("slDLSSGGetState\n");
        }

        sl::ReflexState reflexState = {};
        result = slReflexGetState(reflexState);
        if (result != sl::Result::eOk)
        {
            printf("slReflexGetState\n");
        }

        D3D12Texture* outputD3D12Texture = device->GetTexture(command.output);
        D3D12Texture* colorD3D12Texture = device->GetTexture(command.color);
        D3D12Texture* depthD3D12Texture = device->GetTexture(command.depth);
        D3D12Texture* motionVectorsD3D12Texture = device->GetTexture(command.motionVectors);

        sl::Resource output = sl::Resource(sl::ResourceType::eTex2d, outputD3D12Texture->resource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
        sl::Resource color = sl::Resource(sl::ResourceType::eTex2d, colorD3D12Texture->resource.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        sl::Resource depth = sl::Resource(sl::ResourceType::eTex2d, depthD3D12Texture->resource.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        sl::Resource motionVectors = sl::Resource(sl::ResourceType::eTex2d, motionVectorsD3D12Texture->resource.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

        sl::Extent renderExtent = {};
        renderExtent.top = 0;
        renderExtent.left = 0;
        renderExtent.width = colorD3D12Texture->width;
        renderExtent.height = colorD3D12Texture->height;

        sl::Extent targetExtent = {};
        targetExtent.top = 0;
        targetExtent.left = 0;
        targetExtent.width = outputD3D12Texture->width;
        targetExtent.height = outputD3D12Texture->height;

        {
            sl::ResourceTag outputTag = sl::ResourceTag(&output, sl::kBufferTypeScalingOutputColor, sl::ResourceLifecycle::eValidUntilPresent, &targetExtent);
            sl::ResourceTag colorTag = sl::ResourceTag(&color, sl::kBufferTypeScalingInputColor, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent);
            sl::ResourceTag depthTag = sl::ResourceTag(&depth, sl::kBufferTypeDepth, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent);
            sl::ResourceTag motionVectorsTag = sl::ResourceTag(&motionVectors, sl::kBufferTypeMotionVectors, sl::ResourceLifecycle::eValidUntilPresent, &renderExtent);
            sl::ResourceTag tags[] = { outputTag, colorTag, depthTag, motionVectorsTag };
            slSetTag(viewport, tags, _countof(tags), commandList->GetID3D12GraphicsCommandList());
        }

        {
            //sl::ResourceTag hudLessColor = sl::ResourceTag(nullptr, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent, nullptr);
            /*sl::ResourceTag hudLessColor = sl::ResourceTag(&output, sl::kBufferTypeHUDLessColor, sl::ResourceLifecycle::eValidUntilPresent, &targetExtent);
            sl::ResourceTag tags[] = { hudLessColor };
            slSetTag(viewport, tags, _countof(tags), commandList->GetID3D12GraphicsCommandList());*/
        }

        {
            //sl::ResourceTag uiColorAndAlpha = sl::ResourceTag(nullptr, sl::kBufferTypeUIColorAndAlpha, sl::ResourceLifecycle::eValidUntilPresent, nullptr);
            ////sl::ResourceTag uiColorAndAlpha = sl::ResourceTag(&output, sl::kBufferTypeUIColorAndAlpha, sl::ResourceLifecycle::eValidUntilPresent, &targetExtent);
            //sl::ResourceTag tags[] = { uiColorAndAlpha };
            //slSetTag(viewport, tags, _countof(tags), commandList->GetID3D12GraphicsCommandList());
        }

        //{
        //    // Note: here `precisionInfo` refers to the transform needed to be applied to the buffer values to convert from a low-precision format (e.g. 8-bits) to a high-precision format (e.g. 16-bits). Refer to
        //    sl::ResourceTag bidirectionalDistortionTag = sl::ResourceTag(nullptr, sl::kBufferTypeBidirectionalDistortionField, sl::ResourceLifecycle::eValidUntilPresent, &fullExtent, &precisionInfo); // valid all the time
        //    sl::Resource inputs[] = { bidirectionalDistortionTag };
        //    slSetTag(viewport, inputs, _countof(inputs), cmdList);
        //}

        sl::FrameToken* frameToken = nullptr;
        if (SL_FAILED(result, slGetNewFrameToken(frameToken, &command.frameIndex)))
        {
            LogError(GLogger, std::format("slGetNewFrameToken, error code: {}", (int32)result));
            return false;
        }

        // Inform SL that DLSS should be injected at this point for the given viewport
        const sl::BaseStructure* inputs[] = { &viewport };
        if (SL_FAILED(result, slEvaluateFeature(sl::kFeatureDLSS, *frameToken, inputs, _countof(inputs), commandList->GetID3D12GraphicsCommandList6())))
        {
            LogError(GLogger, std::format("slEvaluateFeature, error code: {}", (int32)result));
            return false;
        }

        // TODO: set descriptor heaps back
        ID3D12DescriptorHeap* descriptorHeaps[] = {
            device->resourceDescriptorHeap->GetID3D12DescriptorHeap(),
            device->samplerDescriptorHeap->GetID3D12DescriptorHeap(),
        };
        commandList->GetID3D12GraphicsCommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
#endif
    }
}