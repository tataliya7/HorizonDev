#include "RenderDocPlugin.h"

#include <renderdoc_app.h>

#include <Windows.h>

namespace HE
{
    static RENDERDOC_API_1_1_2* renderDocAPI = nullptr;

    void* LoadRenderDocLibrary(const char* path)
    {
        HMODULE hModule = LoadLibraryA(path);
        if (hModule == NULL)
        {
            // TODO: Log
            return nullptr;
        }

        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(hModule, "RENDERDOC_GetAPI");
        if (RENDERDOC_GetAPI == nullptr)
        {
            // TODO: Log
            FreeLibrary(hModule);
            return nullptr;
        }

        int result = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**)&renderDocAPI);
        if (result != 1)
        {
            // TODO: Log
            FreeLibrary(hModule);
            return nullptr;
        }

        // TODO: Log
        return hModule;
    }

    void RenderDocPluginInit()
    {
        void* renderDocDLL = nullptr;
        renderDocAPI = nullptr;

        renderDocDLL = LoadRenderDocLibrary("C:\\Program Files\\RenderDoc\\renderdoc.dll");
        if (renderDocDLL == nullptr)
        {
            // TODO: Log
        }

        int major = 0;
        int minor = 0;
        int patch = 0;
        renderDocAPI->GetAPIVersion(&major, &minor, &patch);
        // TODO: Log

        renderDocAPI->SetCaptureFilePathTemplate("RenderDocCaptures/capture");

        renderDocAPI->SetFocusToggleKeys(nullptr, 0);
        //renderDocAPI->SetCaptureKeys(nullptr, 0);
        renderDocAPI->MaskOverlayBits(eRENDERDOC_Overlay_None, eRENDERDOC_Overlay_None);
    }

    void StartFrameCapture()
    {
        // To start a frame capture, call StartFrameCapture.
        // You can specify NULL, NULL for the device to capture on if you have only one device and
        // either no windows at all or only one window, and it will capture from that device.
        // See the documentation below for a longer explanation
        renderDocAPI->StartFrameCapture(NULL, NULL);
    }

    void EndFrameCapture(const char* filename)
    {
        // stop the capture
        uint32 result = renderDocAPI->EndFrameCapture(NULL, NULL);
        if (result == 1)
        {
            // SaveRDC(filename);
        }
    }

    void RenderDocPluginTriggerCapture()
    {
        if (!renderDocAPI->IsTargetControlConnected())
        {
            uint32 PID = renderDocAPI->LaunchReplayUI(1, nullptr);
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
            if (!renderDocAPI->IsTargetControlConnected())
            {
                std::string arguments = targetPath.string();
                uint32 PID = renderDocAPI->LaunchReplayUI(1, arguments.c_str());
                if (PID == 0)
                {

                }
            }
        }
    }
}