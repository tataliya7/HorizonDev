#include "Streamline.h"
#include "StreamlineUtility.h"

#include <vulkan/vulkan.h>

#include <sl.h>
#include <sl_consts.h>
#include <sl_hooks.h>
#include <sl_version.h>

#include <sl_reflex.h>
#include <sl_dlss.h>
#include <sl_dlss_g.h>
#include <sl_helpers.h>
#include <sl_helpers_vk.h>

namespace Horizon
{
    //static std::wstring GetStreamlineInterposerDLLPath()
    //{
    //    char path[MAX_PATH] = { 0 };
    //    if (GetModuleFileNameA(nullptr, path, sizeof(path)) == 0)
    //    {
    //        return std::wstring();
    //    }
    //    auto basePath = std::filesystem::path(path).parent_path();
    //    auto dllPath = basePath.wstring().append(L"\\sl.interposer.dll");
    //    return dllPath;
    //}

     void StreamlineContext::GetFrameToken(uint32 frameIndex)
     {
         //STREAMLINE_CHECK(slGetNewFrameToken(currentFrameToken, &frameIndex));
     }

     //bool StreamlineContext::ReflexSetOptions(const sl::ReflexOptions& options)
     //{
     //    if (!IsInitialized() || !CheckReflexSupport())
     //    {
     //        return false;
     //    }
     //    STREAMLINE_CHECK(slReflexSetOptions(options));
     //    return true;
     //}

     void StreamlineContext::ReflexSleep(uint32 frameIndex)
     {
         if (CheckReflexSupport())
         {
             sl::FrameToken* currentFrameToken = nullptr;
             STREAMLINE_CHECK(slGetNewFrameToken(currentFrameToken, &frameIndex));
             STREAMLINE_CHECK(slReflexSleep(*currentFrameToken));
         }
     }

     void StreamlineContext::ReflexSetMarkerControllerInputSample(uint32 frameIndex)
     {
         if (CheckReflexSupport())
         {
             sl::FrameToken* currentFrameToken = nullptr;
             STREAMLINE_CHECK(slGetNewFrameToken(currentFrameToken, &frameIndex));
             STREAMLINE_CHECK(slPCLSetMarker(sl::PCLMarker::eControllerInputSample, *currentFrameToken));
         }
     }

     void StreamlineContext::ReflexSetMarkerSimulationStart(uint32 frameIndex)
     {
         if (CheckReflexSupport())
         {
             sl::FrameToken* currentFrameToken = nullptr;
             STREAMLINE_CHECK(slGetNewFrameToken(currentFrameToken, &frameIndex));
             STREAMLINE_CHECK(slPCLSetMarker(sl::PCLMarker::eSimulationStart, *currentFrameToken));
         }
     }

     void StreamlineContext::ReflexSetMarkerSimulationEnd(uint32 frameIndex)
     {
         if (CheckReflexSupport())
         {
             sl::FrameToken* currentFrameToken = nullptr;
             STREAMLINE_CHECK(slGetNewFrameToken(currentFrameToken, &frameIndex));
             STREAMLINE_CHECK(slPCLSetMarker(sl::PCLMarker::eSimulationEnd, *currentFrameToken));
         }
     }

     void StreamlineContext::ReflexSetMarkerRenderSubmitStart(uint32 frameIndex)
     {
         if (CheckReflexSupport())
         {
             sl::FrameToken* currentFrameToken = nullptr;
             STREAMLINE_CHECK(slGetNewFrameToken(currentFrameToken, &frameIndex));
             STREAMLINE_CHECK(slPCLSetMarker(sl::PCLMarker::eRenderSubmitStart, *currentFrameToken));
         }
     }

     void StreamlineContext::ReflexSetMarkerRenderSubmitEnd(uint32 frameIndex)
     {
         if (CheckReflexSupport())
         {
             sl::FrameToken* currentFrameToken = nullptr;
             STREAMLINE_CHECK(slGetNewFrameToken(currentFrameToken, &frameIndex));
             STREAMLINE_CHECK(slPCLSetMarker(sl::PCLMarker::eRenderSubmitEnd, *currentFrameToken));
         }
     }

     void StreamlineContext::ReflexSetMarkerPresentStart(uint32 frameIndex)
     {
         if (CheckReflexSupport())
         {
             sl::FrameToken* currentFrameToken = nullptr;
             STREAMLINE_CHECK(slGetNewFrameToken(currentFrameToken, &frameIndex));
             STREAMLINE_CHECK(slPCLSetMarker(sl::PCLMarker::ePresentStart, *currentFrameToken));
         }
     }

     void StreamlineContext::ReflexSetMarkerPresentEnd(uint32 frameIndex)
     {
         if (CheckReflexSupport())
         {
             sl::FrameToken* currentFrameToken = nullptr;
             STREAMLINE_CHECK(slGetNewFrameToken(currentFrameToken, &frameIndex));
             STREAMLINE_CHECK(slPCLSetMarker(sl::PCLMarker::ePresentEnd, *currentFrameToken));
         }
     }

     void StreamlineContext::ReflexSetMarkerPCLatencyPing(uint32 frameIndex)
     {
         if (CheckReflexSupport())
         {
             sl::FrameToken* currentFrameToken = nullptr;
             STREAMLINE_CHECK(slGetNewFrameToken(currentFrameToken, &frameIndex));
             STREAMLINE_CHECK(slPCLSetMarker(sl::PCLMarker::ePCLatencyPing, *currentFrameToken));
         }
     }

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

    void StreamlineContext::Init()
    {
        std::vector<sl::Feature> features = { sl::kFeatureReflex };

        if (true)
        {
            features.push_back(sl::kFeatureDLSS);
        }

#if !HORIZON_CONFIGURATION_RELEASE
        if (true)
        {
            features.push_back(sl::kFeatureImGUI);
        }
#endif

        sl::Preferences preferences = {};
        preferences.showConsole = true;
        preferences.logLevel = sl::LogLevel::eVerbose; //sl::LogLevel::eDefault;
        preferences.pathsToPlugins = nullptr;
        preferences.numPathsToPlugins = 0;
        preferences.pathToLogsAndData = nullptr;
        preferences.allocateCallback = nullptr;
        preferences.releaseCallback = nullptr;
        preferences.logMessageCallback = StreamlineLogMessageCallback;
        preferences.flags = sl::PreferenceFlags::eDisableCLStateTracking;// | sl::PreferenceFlags::eAllowOTA; // | sl::PreferenceFlags::eUseManualHooking;
        preferences.featuresToLoad = features.data();
        preferences.numFeaturesToLoad = uint32_t(features.size());
        preferences.applicationId = sl::INVALID_UINT;
        preferences.engine = sl::EngineType::eCustom;
        preferences.engineVersion = "Horizon Engine";
        preferences.projectId = "a0f57b54-1daf-4934-90ae-c4035c19df04";
        //if (renderBackend)
        //{
        //    preferences.renderAPI = sl::RenderAPI::eD3D12;
        //}
        //else if ()
        {
            preferences.renderAPI = sl::RenderAPI::eVulkan;
        }
        // else
        // {
        //     LogError(GLogger, std::format("Unsupported API: {}, skipping Streamline initialization."), );
        //     return;
        // }

        sl::Result result = slInit(preferences, sl::kSDKVersion);

        if (result == sl::Result::eOk)
        {
            isInitialized = true;
        }
        else
        {
            //LogError(GLogger, std::format("Failed to initialize Streamline ({}, {})."), result, sl::getResultAsStr(result));
            isInitialized = false;
        }
    }

    void StreamlineContext::Exit()
    {

    }

    void StreamlineContext::Test(RenderBackend* renderBackend)
    {
        if (renderBackend->GetType() == RenderBackendType::Vulkan)
        {
            RenderBackendVulkanInfo renderBackendVulkanInfo = {};
            renderBackend->GetRenderBackendVulkanInfo(&renderBackendVulkanInfo);

            sl::VulkanInfo vulkanInfo = {};
            vulkanInfo.instance = static_cast<VkInstance>(renderBackendVulkanInfo.instance);
            vulkanInfo.device = static_cast<VkDevice>(renderBackendVulkanInfo.device);
            vulkanInfo.physicalDevice = static_cast<VkPhysicalDevice>(renderBackendVulkanInfo.physicalDevice);
            vulkanInfo.computeQueueIndex = renderBackendVulkanInfo.computeQueueIndex;
            vulkanInfo.computeQueueFamily = renderBackendVulkanInfo.computeQueueFamily;
            vulkanInfo.graphicsQueueIndex = renderBackendVulkanInfo.graphicsQueueIndex;
            vulkanInfo.graphicsQueueFamily = renderBackendVulkanInfo.graphicsQueueFamily;
            vulkanInfo.opticalFlowQueueIndex = renderBackendVulkanInfo.opticalFlowQueueIndex;
            vulkanInfo.opticalFlowQueueFamily = renderBackendVulkanInfo.opticalFlowQueueFamily;
            sl::Result slResult = slSetVulkanInfo(vulkanInfo);

            if (true)
            {
                // Set reflex options to a default configuration. This can be changed at runtime in the UI.
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
    }

    // void QueryFeatureRequirements()
    // {
    //     sl::Feature feature =
    //     if (IsStreamlineSupported())
    //     {
    //         sl::FeatureRequirements featureRequirements;
    //         if (SL_SUCCEEDED(slGetFeatureRequirements(feature, featureRequirements)))
    //         {
    //             // Feature is loaded, we can check the requirements
    //             assert(freatureRequirements.flags & sl::FeatureRequirementFlags::eD3D12Supported);
    //             assert(freatureRequirements.flags & sl::FeatureRequirementFlags::eVulkanSupported);
    //         }
    //         else
    //         {
    //             LogError(GLogger, std::format("slGetFeatureRequirements, error code: {}.", int32(result)));
    //         }
    //     }
    // }
}