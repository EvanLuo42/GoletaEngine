/// @file
/// @brief spdlog-backed implementation of the Log facade.

#include "Log.h"

#include <spdlog/async_logger.h>
#include <spdlog/common.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace goleta::log
{
namespace
{

spdlog::level::level_enum toSpd(const Level L) noexcept
{
    switch (L)
    {
    case Level::Trace:
        return spdlog::level::trace;
    case Level::Debug:
        return spdlog::level::debug;
    case Level::Info:
        return spdlog::level::info;
    case Level::Warn:
        return spdlog::level::warn;
    case Level::Error:
        return spdlog::level::err;
    case Level::Critical:
        return spdlog::level::critical;
    case Level::Off:
        return spdlog::level::off;
    }
    return spdlog::level::info;
}

Level fromSpd(spdlog::level::level_enum L) noexcept
{
    switch (L)
    {
    case spdlog::level::trace:
        return Level::Trace;
    case spdlog::level::debug:
        return Level::Debug;
    case spdlog::level::info:
        return Level::Info;
    case spdlog::level::warn:
        return Level::Warn;
    case spdlog::level::err:
        return Level::Error;
    case spdlog::level::critical:
        return Level::Critical;
    default:
        return Level::Off;
    }
}

/// @note Field order matters. C++ destroys members in reverse declaration order, so Pool
///       (declared before Owned) outlives Owned's async_logger destructors. That's what the
///       old "thread pool doesn't exist anymore" bug was about: spdlog's global registry
///       destroyed its pool before the async_loggers still holding weak_ptrs to it. Keeping
///       the pool as our own field makes the ordering explicit.
struct State
{
    std::mutex                                                       Mutex;
    std::vector<spdlog::sink_ptr>                                    Sinks;
    std::shared_ptr<spdlog::details::thread_pool>                    Pool;    ///< Owned; outlives Owned.
    std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> Owned;   ///< Keeps loggers alive.
    std::unordered_map<std::string, Logger>                          Loggers; ///< Facade handles.
    Level                                                            DefaultLevel = Level::Info;
    bool                                                             Async        = false;
    bool                                                             Initialized  = false;
};

State& state()
{
    static State S;
    return S;
}

std::shared_ptr<spdlog::logger> makeLoggerLocked(const std::string_view Name, State& S)
{
    std::shared_ptr<spdlog::logger> Sp;
    if (const bool CanAsync = S.Async && S.Pool != nullptr)
    {
        Sp = std::make_shared<spdlog::async_logger>(std::string{Name}, S.Sinks.begin(), S.Sinks.end(),
                                                    std::weak_ptr<spdlog::details::thread_pool>(S.Pool),
                                                    spdlog::async_overflow_policy::block);
    }
    else
    {
        Sp = std::make_shared<spdlog::logger>(std::string{Name}, S.Sinks.begin(), S.Sinks.end());
    }
    Sp->set_level(toSpd(S.DefaultLevel));
    return Sp;
}

} // namespace

void initialize(const LogConfig& Cfg) noexcept
{
    State& S = state();

    spdlog::set_error_handler(
        [](const std::string& Msg)
        {
            std::fputs("[spdlog] ", stderr);
            std::fputs(Msg.c_str(), stderr);
            std::fputc('\n', stderr);
        });

    std::scoped_lock Lk(S.Mutex);
    S.Sinks.clear();
    S.Sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    if (Cfg.LogFilePath)
    {
        S.Sinks.emplace_back(std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            Cfg.LogFilePath, Cfg.RotatingMaxBytes, Cfg.RotatingMaxFiles));
    }

    S.DefaultLevel = Cfg.DefaultLevel;
    S.Async        = Cfg.Async;
    if (S.Async)
    {
        // One worker thread is plenty for logging throughput; bump if ever needed.
        S.Pool = std::make_shared<spdlog::details::thread_pool>(Cfg.AsyncQueueSize, 1u);
    }
    else
    {
        S.Pool.reset();
    }
    S.Initialized = true;
}

void shutdown() noexcept
{
    State&           S = state();
    std::scoped_lock Lk(S.Mutex);
    // Order matters: destroy loggers first so any async flush-on-destroy still sees a live
    // pool, then drop the pool, then sinks. We deliberately avoid spdlog::shutdown() because
    // we never registered with spdlog's global registry -- it has nothing of ours to clean up.
    S.Loggers.clear();
    S.Owned.clear();
    S.Pool.reset();
    S.Sinks.clear();
    S.Initialized = false;
}

Logger& logger(const std::string_view Name) noexcept
{
    State&           S = state();
    std::scoped_lock Lk(S.Mutex);

    if (const auto It = S.Loggers.find(std::string(Name)); It != S.Loggers.end())
    {
        return It->second;
    }

    Logger L;
    if (!S.Initialized)
    {
        // Auto-bootstrap without initialize(): one stdout color sink.
        S.Sinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        S.Initialized = true;
    }

    const std::string Key{Name};

    std::shared_ptr<spdlog::logger> Sp = makeLoggerLocked(Name, S);

    L.Impl = Sp.get();
    S.Owned.emplace(Key, std::move(Sp));
    auto [Ins, _] = S.Loggers.emplace(Key, L);
    return Ins->second;
}

void Logger::log(const Level Lv, const std::string_view Message) const noexcept
{
    if (!Impl)
        return;
    static_cast<spdlog::logger*>(Impl)->log(toSpd(Lv), Message);
}

void Logger::setLevel(const Level Lv) const noexcept
{
    if (!Impl)
        return;
    static_cast<spdlog::logger*>(Impl)->set_level(toSpd(Lv));
}

Level Logger::level() const noexcept
{
    if (!Impl)
        return Level::Off;
    return fromSpd(static_cast<spdlog::logger*>(Impl)->level());
}

} // namespace goleta::log
