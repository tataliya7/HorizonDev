#include "Streamline.h"

#include <assert.h>
#include <filesystem>

//#include <Windows.h>

#define STREAMLINE_CEHCK(f) assert(f == sl::Result::eOk)

namespace Streamline
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

    void StreamlineContext::GetNewFrameToken()
    {
        STREAMLINE_CEHCK(slGetNewFrameToken(currentFrameToken, nullptr));
    }

    bool StreamlineContext::ReflexSetOptions(const sl::ReflexOptions& options)
    {
        if (!IsInitialized() || !CheckReflexSupport())
        {
            return false;
        }
        STREAMLINE_CEHCK(slReflexSetOptions(options));
        return true;
    }

    void StreamlineContext::ReflexSleep()
    {
        if (CheckReflexSupport())
        {
            STREAMLINE_CEHCK(slReflexSleep(*currentFrameToken));
        }
    }

    void StreamlineContext::ReflexSetMarkerInputSample()
    {
        if (CheckReflexSupport())
        {
            STREAMLINE_CEHCK(slReflexSetMarker(sl::ReflexMarker::eInputSample, *currentFrameToken));
        }
    }

    void StreamlineContext::ReflexSetMarkerSimulationStart()
    {
        if (CheckReflexSupport())
        {
            STREAMLINE_CEHCK(slReflexSetMarker(sl::ReflexMarker::eSimulationStart, *currentFrameToken));
        }
    }

    void StreamlineContext::ReflexSetMarkerSimulationEnd()
    {
        if (CheckReflexSupport())
        {
            STREAMLINE_CEHCK(slReflexSetMarker(sl::ReflexMarker::eSimulationEnd, *currentFrameToken));
        }
    }

    void StreamlineContext::ReflexSetMarkerRenderSubmitStart()
    {
        if (CheckReflexSupport())
        {
            STREAMLINE_CEHCK(slReflexSetMarker(sl::ReflexMarker::eRenderSubmitStart, *currentFrameToken));
        }
    }

    void StreamlineContext::ReflexSetMarkerRenderSubmitEnd()
    {
        if (CheckReflexSupport())
        {
            STREAMLINE_CEHCK(slReflexSetMarker(sl::ReflexMarker::eRenderSubmitEnd, *currentFrameToken));
        }
    }

    void StreamlineContext::ReflexSetMarkerPresentStart()
    {
        if (CheckReflexSupport())
        {
            STREAMLINE_CEHCK(slReflexSetMarker(sl::ReflexMarker::ePresentStart, *currentFrameToken));
        }
    }

    void StreamlineContext::ReflexSetMarkerPresentEnd()
    {
        if (CheckReflexSupport())
        {
            STREAMLINE_CEHCK(slReflexSetMarker(sl::ReflexMarker::ePresentEnd, *currentFrameToken));
        }
    }

    void StreamlineContext::ReflexSetMarkerPCLatencyPing()
    {
        if (CheckReflexSupport())
        {
            STREAMLINE_CEHCK(slReflexSetMarker(sl::ReflexMarker::ePCLatencyPing, *currentFrameToken));
        }
    }
}
