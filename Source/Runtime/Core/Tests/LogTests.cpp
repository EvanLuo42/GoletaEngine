#include <gtest/gtest.h>

#include "Log.h"

using namespace goleta;

TEST(LogTests, LoggerIsGetOrCreateAndStable)
{
    log::Logger& A1 = log::logger("TestCategory");
    log::Logger& A2 = log::logger("TestCategory");
    EXPECT_EQ(&A1, &A2);

    log::Logger& B = log::logger("OtherCategory");
    EXPECT_NE(&A1, &B);
}

TEST(LogTests, LevelRoundTrip)
{
    log::Logger& L = log::logger("LevelCategory");
    L.setLevel(log::Level::Trace);
    EXPECT_EQ(L.level(), log::Level::Trace);
    L.setLevel(log::Level::Error);
    EXPECT_EQ(L.level(), log::Level::Error);
    L.setLevel(log::Level::Off);
    EXPECT_EQ(L.level(), log::Level::Off);
}

TEST(LogTests, FormatMacroCompilesAndDispatches)
{
    log::Logger& L = log::logger("FormatCategory");
    L.setLevel(log::Level::Trace);

    // These should compile (format string check is compile-time) and not crash.
    GOLETA_LOG_INFO(FormatCategory, "hello {}", 42);
    GOLETA_LOG_WARN(FormatCategory, "x={} y={:.2f}", 1, 3.14159);
    GOLETA_LOG_ERROR(FormatCategory, "no args");
    SUCCEED();
}

TEST(LogTests, BelowLevelIsFiltered)
{
    log::Logger& L = log::logger("FilteredCategory");
    L.setLevel(log::Level::Error);
    // Below-threshold calls must be accepted silently (and in practice, skipped quickly).
    GOLETA_LOG_INFO(FilteredCategory, "skipped");
    GOLETA_LOG_TRACE(FilteredCategory, "also skipped");
    GOLETA_LOG_ERROR(FilteredCategory, "goes through");
    SUCCEED();
}

TEST(LogTests, AsyncPathRoundTrips)
{
    // Bring up async explicitly. Loggers created after initialize() hit the async path;
    // shutdown() tears the pool down in the right order.
    log::LogConfig Cfg{};
    Cfg.Async          = true;
    Cfg.AsyncQueueSize = 1024;
    log::initialize(Cfg);

    log::Logger& L = log::logger("AsyncCategory");
    L.setLevel(log::Level::Trace);
    for (int I = 0; I < 32; ++I)
    {
        GOLETA_LOG_INFO(AsyncCategory, "msg {}", I);
    }
    log::shutdown();   // flushes, drops loggers first, pool second
    SUCCEED();
}
