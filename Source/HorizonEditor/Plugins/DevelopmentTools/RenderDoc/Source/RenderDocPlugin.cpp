#include "RenderDocPlugin.h"

#include <renderdoc_app.h>

namespace Horizon
{
    static RENDERDOC_API_1_1_2* renderdocAPI = nullptr;

    void RenderDocPluginInit()
    {
        ConfigurationParser configuration = ConfigurationParser::ParseFile("../../../Source/HorizonEditor/Plugins/DevelopmentTools/RenderDoc/Config/RenderDocPluginSettings.toml");

        std::string renderDocPath = configuration.Get("RenderDocPluginSettings.RenderBackendType").AsStringOr("");

        OSLibraryHandle renderdocLibrary = OSLoadLibrary(renderDocPath.c_str());
        if (renderdocLibrary == NULL)
        {
            LogWarning(GLogger, std::format("Failed to load rendedoc library."));
            return;
        }

        pRENDERDOC_GetAPI RENDERDOC_GetAPI = reinterpret_cast<pRENDERDOC_GetAPI>(OSGetSymbolAddressFromLibrary(renderdocLibrary, "RENDERDOC_GetAPI"));
        if (RENDERDOC_GetAPI == nullptr)
        {
            LogWarning(GLogger, std::format("Failed to obtain address of RENDERDOC_GetAPI from library: {}.", renderDocPath));
            OSFreeLibrary(renderdocLibrary);
            return;
        }

        int result = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, reinterpret_cast<void**>(&renderdocAPI));
        if (result != 1)
        {
            LogWarning(GLogger, std::format("Failed to get renderdoc API, minimum supported API version: eRENDERDOC_API_Version_1_1_2."));
            OSFreeLibrary(renderdocLibrary);
            return;
        }

        int major = 0;
        int minor = 0;
        int patch = 0;
        renderdocAPI->GetAPIVersion(&major, &minor, &patch);

        // Disable the overlay.
        renderdocAPI->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);

        renderdocAPI->SetCaptureFilePathTemplate("RenderDocCaptures/capture");
        renderdocAPI->SetFocusToggleKeys(nullptr, 0);
        //renderdocAPI->SetCaptureKeys(nullptr, 0);
        renderdocAPI->SetCaptureOptionU32(eRENDERDOC_Option_DebugOutputMute, 0);

        LogInfo(GLogger, std::format("Found renderdoc library, API version: {}.{}.{}.", major, minor, patch));
    }

    void StartFrameCapture()
    {
        // To start a frame capture, call StartFrameCapture.
        // You can specify NULL, NULL for the device to capture on if you have only one device and
        // either no windows at all or only one window, and it will capture from that device.
        // See the documentation below for a longer explanation
        renderdocAPI->StartFrameCapture(NULL, NULL);
    }

    void EndFrameCapture(const char* filename)
    {
        // Stop the capture.
        uint32 result = renderdocAPI->EndFrameCapture(NULL, NULL);
        if (result == 1)
        {
            // SaveRDC(filename);
        }
    }

    void RenderDocPluginTriggerCapture()
    {
        if (!renderdocAPI->IsTargetControlConnected())
        {
            uint32 PID = renderdocAPI->LaunchReplayUI(1, nullptr);
            if (PID == 0)
            {

            }
        }
    }

    void SaveRDC(const char* filename)
    {
        std::filesystem::path targetPath = filename;
        if (!targetPath.empty())
        {
            if (std::filesystem::exists(targetPath))
            {
                std::filesystem::remove(targetPath);
            }

            if (std::filesystem::create_directories(targetPath.parent_path()))
            {

            }
            else
            {
                // TODO: Log warning
            }
        }

        bool launchRenderDoc = true;
        if (launchRenderDoc)
        {
            if (!renderdocAPI->IsTargetControlConnected())
            {
                std::string arguments = targetPath.string();
                uint32 PID = renderdocAPI->LaunchReplayUI(1, arguments.c_str());
                if (PID == 0)
                {

                }
            }
        }
    }
}