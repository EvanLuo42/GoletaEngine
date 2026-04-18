cmake_minimum_required(VERSION 3.21)

project("GoletaEngine" LANGUAGES CXX)

option(GOLETA_BUILD_SHARED    "Build engine modules as shared libraries"   ON)
option(GOLETA_BUILD_TESTS     "Build unit tests"                          ON)
option(GOLETA_ENABLE_COVERAGE "Link with /PROFILE for VS Code Coverage"   OFF)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS OFF)

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
