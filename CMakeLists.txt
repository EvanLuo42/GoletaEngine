cmake_minimum_required(VERSION 3.21)

project("GoletaEngine" LANGUAGES CXX)

option(GOLETA_BUILD_SHARED    "Build engine modules as shared libraries"   ON)
option(GOLETA_BUILD_TESTS     "Build unit tests"                          ON)
option(GOLETA_ENABLE_COVERAGE "Link with /PROFILE for VS Code Coverage"   OFF)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)

# CMake injects /EHsc and /GR into CMAKE_CXX_FLAGS by default. Strip both so
# goleta_compile_options' /EHs-c- /GR- apply without MSVC D9025 "overriding" noise.
# Targets that still need exceptions / RTTI (e.g. FetchContent'd GoogleTest) add them back
# at target scope themselves.
if(MSVC)
    string(REGEX REPLACE "/EH[a-zA-Z-]*" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REGEX REPLACE "/GR[-]?"       "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

# Interface target carrying engine-wide compile / link options. Only goleta modules link it;
# FetchContent'd third-party (GoogleTest, etc.) keeps its own flags to avoid build breakage.
#
# Disabled here:
#   - C++ exceptions (non-deterministic frame-time cost, binary bloat, console SDKs expect off).
#   - RTTI (binary bloat, dynamic_cast discouraged; the engine carries its own TypeHash).
# Enabled here:
#   - Hidden symbol visibility (smaller .so / faster link on ELF/Mach-O).
#   - Per-function/per-data sections + linker dead-strip (drops unused code).
add_library(goleta_compile_options INTERFACE)
if(MSVC)
    target_compile_options(goleta_compile_options INTERFACE
        /EHs-c-
        /GR-
        /Gy
        /Gw
    )
    target_compile_definitions(goleta_compile_options INTERFACE
        _HAS_EXCEPTIONS=0
    )
else()
    target_compile_options(goleta_compile_options INTERFACE
        -fno-exceptions
        -fno-rtti
        -fvisibility=hidden
        -fvisibility-inlines-hidden
        -ffunction-sections
        -fdata-sections
    )
    if(APPLE)
        target_link_options(goleta_compile_options INTERFACE -Wl,-dead_strip)
    else()
        target_link_options(goleta_compile_options INTERFACE -Wl,--gc-sections)
    endif()
endif()

if(GOLETA_ENABLE_COVERAGE AND MSVC)
    add_link_options(/PROFILE)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include(GoletaModule)

if(GOLETA_BUILD_TESTS)
    enable_testing()
    include(GoletaTests)
endif()

add_subdirectory(Source)

if(GOLETA_BUILD_TESTS)
    goleta_create_engine_tests()
endif()
