#include "Log/Log.h"

namespace Orion
{
    std::shared_ptr<spdlog::logger> Log::s_EngineLogger;
    std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

    void Log::Init()
    {
        // set logging format for both loggers
        // format: [TIMESTAMP] <logger_name>: <msg>
        spdlog::set_pattern("%^[%T] %n: %v%$");
        
        s_EngineLogger = spdlog::stdout_color_mt("ORION");
        s_EngineLogger->set_level(spdlog::level::trace);

        s_ClientLogger = spdlog::stdout_color_mt("APP");
        s_ClientLogger->set_level(spdlog::level::trace);
    }
}