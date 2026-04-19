goleta_add_module(Core)

# Log.h pulls <spdlog/fmt/fmt.h> for fmt::format_string in the inline template logf(); the
# rest of spdlog is a Private implementation detail of Log.cpp. PUBLIC-link spdlog::spdlog
# so downstream modules picking up Log.h also get the bundled-fmt include path. They still
# should not include spdlog/spdlog.h themselves.
target_link_libraries(Core PUBLIC spdlog::spdlog)
