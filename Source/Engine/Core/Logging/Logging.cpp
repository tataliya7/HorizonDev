#include "Core/Logging/Logging.h"

#include <stdio.h>
#include <stdarg.h>

#define SPDLOG_USE_STD_FORMAT
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace HE
{
    enum
    {
        MassageSize = 1024
    };

    Logger* GLogger = nullptr;
    std::shared_ptr<spdlog::logger> gLogger = nullptr;

    bool CreateConsoleLogger_Deprecated()
    {
        std::string logsDirectory = "Logs";
        if (!std::filesystem::exists(logsDirectory))
        {
            std::filesystem::create_directories(logsDirectory);
        }

        std::vector<spdlog::sink_ptr> sinks =
        {
            std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
            std::make_shared<spdlog::sinks::basic_file_sink_mt>("Logs/Horizon.log", true),
        };

        sinks[0]->set_pattern("%^[%Y-%m-%d %T][%n][%l]%v%$");
        sinks[1]->set_pattern("..");

        auto colorSink = static_cast<spdlog::sinks::stdout_color_sink_mt*>(sinks[0].get());
        colorSink->set_color(spdlog::level::trace, FOREGROUND_BLUE);
        colorSink->set_color(spdlog::level::info, std::numeric_limits<uint16_t>::max());
        colorSink->set_color(spdlog::level::warn, FOREGROUND_RED | FOREGROUND_GREEN);
        colorSink->set_color(spdlog::level::err, FOREGROUND_RED);

        gLogger = std::make_shared<spdlog::logger>("Console", begin(sinks), end(sinks));
        gLogger->set_level(spdlog::level::trace);
        spdlog::register_logger(gLogger);

        GLogger = (Logger*)gLogger.get();

        return true;
    }

    void LogVerbose(Logger* logger, std::wstring_view message)
    {
        SPDLOG_LOGGER_CALL((spdlog::logger*)logger, spdlog::level::level_enum::debug, message);
    }

    void LogInfo(Logger* logger, std::wstring_view message)
    {
        SPDLOG_LOGGER_CALL((spdlog::logger*)logger, spdlog::level::level_enum::info, message);
    }

    void LogWarning(Logger* logger, std::wstring_view message)
    {
        SPDLOG_LOGGER_CALL((spdlog::logger*)logger, spdlog::level::level_enum::warn, message);
    }

    void LogError(Logger* logger, std::wstring_view message)
    {
        SPDLOG_LOGGER_CALL((spdlog::logger*)logger, spdlog::level::level_enum::err, message);
    }

    void LogFatal(Logger* logger, std::wstring_view message)
    {
        SPDLOG_LOGGER_CALL((spdlog::logger*)logger, spdlog::level::level_enum::critical, message);
    }

    void LogVerbose(Logger* logger, std::string_view message)
    {
        SPDLOG_LOGGER_CALL((spdlog::logger*)logger, spdlog::level::level_enum::debug, message);
    }

    void LogInfo(Logger* logger, std::string_view message)
    {
        SPDLOG_LOGGER_CALL((spdlog::logger*)logger, spdlog::level::level_enum::info, message);
    }

    void LogWarning(Logger* logger, std::string_view message)
    {
        SPDLOG_LOGGER_CALL((spdlog::logger*)logger, spdlog::level::level_enum::warn, message);
    }

    void LogError(Logger* logger, std::string_view message)
    {
        SPDLOG_LOGGER_CALL((spdlog::logger*)logger, spdlog::level::level_enum::err, message);
    }

    void LogFatal(Logger* logger, std::string_view message)
    {
        SPDLOG_LOGGER_CALL((spdlog::logger*)logger, spdlog::level::level_enum::critical, message);
    }
}