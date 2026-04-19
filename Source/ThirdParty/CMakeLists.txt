# spdlog -- engine-wide logging.
# Forced build settings:
#   SPDLOG_NO_EXCEPTIONS   : engine-wide -fno-exceptions; spdlog routes errors through a
#                            set_error_handler() callback instead of throwing.
#   SPDLOG_FMT_EXTERNAL    : use spdlog's bundled fmt to avoid version drift with anything
#                            else in the tree.
#   SPDLOG_USE_STD_FORMAT  : stay on fmt; std::format is slower and more compiler-sensitive.
#   SPDLOG_BUILD_SHARED    : static link, one fewer runtime DLL dep.
set(SPDLOG_NO_EXCEPTIONS  ON  CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL   OFF CACHE BOOL "" FORCE)
set(SPDLOG_USE_STD_FORMAT OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED   OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL        OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE  OFF CACHE BOOL "" FORCE)

add_subdirectory(spdlog EXCLUDE_FROM_ALL)
