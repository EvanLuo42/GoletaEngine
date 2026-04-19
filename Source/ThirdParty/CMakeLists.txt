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

# D3D12 Memory Allocator (AMD) — suballocates ID3D12Resource behind a small
# allocator surface. Loaded only on Windows and only when the submodule has
# been initialised. RHID3D12 falls back to committed resources otherwise.
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/D3D12MemoryAllocator/CMakeLists.txt")
    set(D3D12MA_BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
    set(D3D12MA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    add_subdirectory(D3D12MemoryAllocator EXCLUDE_FROM_ALL)
endif()

# enkiTS -- task scheduler used by RenderGraph for parallel pass recording.
# Forced build settings: no C interface, no examples, static-linked.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/enkiTS/CMakeLists.txt")
    set(ENKITS_BUILD_C_INTERFACE OFF CACHE BOOL "" FORCE)
    set(ENKITS_BUILD_EXAMPLES    OFF CACHE BOOL "" FORCE)
    set(ENKITS_BUILD_SHARED      OFF CACHE BOOL "" FORCE)
    set(ENKITS_INSTALL           OFF CACHE BOOL "" FORCE)
    add_subdirectory(enkiTS EXCLUDE_FROM_ALL)
endif()
