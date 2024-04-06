#pragma once

#include "Core/CoreCommon.h"

namespace HE
{
    class Logger
    {
    public:
        Logger();
        ~Logger();
    };

    extern Logger* GLogger;

    extern bool CreateConsoleLogger_Deprecated();
    extern void LogVerbose(Logger* logger, std::wstring_view message);
    extern void LogInfo(Logger* logger, std::wstring_view message);
    extern void LogWarning(Logger* logger, std::wstring_view message);
    extern void LogError(Logger* logger, std::wstring_view message);
    extern void LogFatal(Logger* logger, std::wstring_view message);
    extern void LogVerbose(Logger* logger, std::string_view message);
    extern void LogInfo(Logger* logger, std::string_view message);
    extern void LogWarning(Logger* logger, std::string_view message);
    extern void LogError(Logger* logger, std::string_view message);
    extern void LogFatal(Logger* logger, std::string_view message);
}