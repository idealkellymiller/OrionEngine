#pragma once

#include "EngineCore.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <spdlog/fmt/ostr.h>

#include <memory>

namespace Orion
{
    class ORION_API Log 
    {
        public:
            static void Init();

            inline static std::shared_ptr<spdlog::logger>& GetEngineLogger() { return s_EngineLogger; }
            inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
        private:
            static std::shared_ptr<spdlog::logger> s_EngineLogger;
            static std::shared_ptr<spdlog::logger> s_ClientLogger;
    };
}

// core logger macros to allow for easy log msg type outputting
#define ORN_CORE_TRACE(...)   ::Orion::Log::GetEngineLogger()->trace(__VA_ARGS__)
#define ORN_CORE_INFO(...)    ::Orion::Log::GetEngineLogger()->info(__VA_ARGS__)
#define ORN_CORE_WARN(...)    ::Orion::Log::GetEngineLogger()->warn(__VA_ARGS__)
#define ORN_CORE_ERROR(...)   ::Orion::Log::GetEngineLogger()->error(__VA_ARGS__)
#define ORN_CORE_FATAL(...)   ::Orion::Log::GetEngineLogger()->fatal(__VA_ARGS__)

// client logger macros to allow for easy log msg type outputting
#define ORN_TRACE(...)        ::Orion::Log::GetClientLogger()->trace(__VA_ARGS__)
#define ORN_INFO(...)         ::Orion::Log::GetClientLogger()->info(__VA_ARGS__)
#define ORN_WARN(...)         ::Orion::Log::GetClientLogger()->warn(__VA_ARGS__)
#define ORN_ERROR(...)        ::Orion::Log::GetClientLogger()->error(__VA_ARGS__)
#define ORN_FATAL(...)        ::Orion::Log::GetClientLogger()->fatal(__VA_ARGS__)