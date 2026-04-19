#pragma once

/// @file
/// @brief Logging facade. Backend is spdlog, but spdlog types never appear in public headers
///        -- only Logger, Level, and the format entry points. Build with compile-time format
///        string checking via {fmt}.

#include <spdlog/fmt/fmt.h>

#include <cstdint>
#include <iterator>
#include <string_view>
#include <utility>
// downstream code. Do not include other spdlog headers outside
// of Core's Private/.

#include "CoreExport.h"

namespace goleta::log
{

/// @brief Stepped severity. Aligned with spdlog's levels but insulated from its enum values.
enum class Level : uint8_t
{
    Trace = 0,
    Debug,
    Info,
    Warn,
    Error,
    Critical,
    Off,
};

/// @brief Named category. Obtained by name via logger("SomeCategory"); cached by the facade
///        for the process lifetime, so caching the reference in a static is safe.
class CORE_API Logger
{
public:
    /// @brief Write a single line. The string is copied into the sink; safe to discard after.
    void log(Level Lv, std::string_view Message) const noexcept;

    void  setLevel(Level Lv) const noexcept;
    Level level() const noexcept;

private:
    friend CORE_API Logger& logger(std::string_view Name) noexcept;
    void*                   Impl = nullptr; ///< spdlog::logger* behind the scenes.
};

/// @brief Get-or-create a logger for the given category name.
CORE_API Logger& logger(std::string_view Name) noexcept;

/// @brief One-time startup tuning. All fields may be left default.
struct LogConfig
{
    const char* LogFilePath      = nullptr; ///< Null = stdout only; otherwise rotating file sink.
    uint32_t    RotatingMaxBytes = 16u * 1024u * 1024u;
    uint32_t    RotatingMaxFiles = 5;
    bool        Async            = true;
    uint32_t    AsyncQueueSize   = 8192; ///< Power of two.
    Level       DefaultLevel     = Level::Info;
};

CORE_API void initialize(const LogConfig& Cfg) noexcept;
CORE_API void shutdown() noexcept;

/// @brief Format and dispatch. Format string is checked at compile time via fmt.
template <class... Args>
void logf(Logger& L, Level Lv, fmt::format_string<Args...> Fmt, Args&&... A)
{
    if (static_cast<uint8_t>(Lv) < static_cast<uint8_t>(L.level()))
    {
        return;
    }
    fmt::memory_buffer Buf;
    fmt::format_to(std::back_inserter(Buf), Fmt, std::forward<Args>(A)...);
    L.log(Lv, std::string_view{Buf.data(), Buf.size()});
}

} // namespace goleta::log

#define GOLETA_LOG_TRACE(Category, ...) \
    ::goleta::log::logf(::goleta::log::logger(#Category), ::goleta::log::Level::Trace, __VA_ARGS__)
#define GOLETA_LOG_DEBUG(Category, ...) \
    ::goleta::log::logf(::goleta::log::logger(#Category), ::goleta::log::Level::Debug, __VA_ARGS__)
#define GOLETA_LOG_INFO(Category, ...) \
    ::goleta::log::logf(::goleta::log::logger(#Category), ::goleta::log::Level::Info, __VA_ARGS__)
#define GOLETA_LOG_WARN(Category, ...) \
    ::goleta::log::logf(::goleta::log::logger(#Category), ::goleta::log::Level::Warn, __VA_ARGS__)
#define GOLETA_LOG_ERROR(Category, ...) \
    ::goleta::log::logf(::goleta::log::logger(#Category), ::goleta::log::Level::Error, __VA_ARGS__)
#define GOLETA_LOG_CRITICAL(Category, ...) \
    ::goleta::log::logf(::goleta::log::logger(#Category), ::goleta::log::Level::Critical, __VA_ARGS__)
