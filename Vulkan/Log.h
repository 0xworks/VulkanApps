#pragma once

#include "Core.h"

#include "spdlog/spdlog.h"

// On Windows, spdlog.h brings in Windows.h, which in turn defines some things which clash with the Vulkan SDK and with our code.
// undef them here.
#undef MemoryBarrier
#undef CreateWindow

namespace Vulkan {

class Log {
public:

   static void Init();

   static spdlog::logger& GetCoreLogger();
   static spdlog::logger& GetAppLogger();

private:
   static std::shared_ptr<spdlog::logger> s_loggerCore;
   static std::shared_ptr<spdlog::logger> s_loggerApp;
};

}

#define CORE_LOG_TRACE(...)        ::Vulkan::Log::GetCoreLogger().trace(__VA_ARGS__)
#define CORE_LOG_INFO(...)         ::Vulkan::Log::GetCoreLogger().info(__VA_ARGS__)
#define CORE_LOG_WARN(...)         ::Vulkan::Log::GetCoreLogger().warn(__VA_ARGS__)
#define CORE_LOG_ERROR(...)        ::Vulkan::Log::GetCoreLogger().error(__VA_ARGS__)
#define CORE_LOG_FATAL(...)        ::Vulkan::Log::GetCoreLogger().critical(__VA_ARGS__)

#define LOG_TRACE(...)        ::Vulkan::Log::GetAppLogger().trace(__VA_ARGS__)
#define LOG_INFO(...)         ::Vulkan::Log::GetAppLogger().info(__VA_ARGS__)
#define LOG_WARN(...)         ::Vulkan::Log::GetAppLogger().warn(__VA_ARGS__)
#define LOG_ERROR(...)        ::Vulkan::Log::GetAppLogger().error(__VA_ARGS__)
#define LOG_FATAL(...)        ::Vulkan::Log::GetAppLogger().critical(__VA_ARGS__)
