include(FetchContent)
include(GoogleTest)

set(_gtest_saved_shared ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
FetchContent_MakeAvailable(googletest)

set(BUILD_SHARED_LIBS ${_gtest_saved_shared} CACHE BOOL "" FORCE)

# goleta_collect_module_tests(<ModuleName>)
#
# Called from goleta_add_module. Scans <module>/Tests/ and appends any .cpp
# sources to the global GOLETA_TEST_SOURCES property, plus the Tests/ dir to
# GOLETA_TEST_INCLUDE_DIRS. The sources are wired into a single Engine.Tests
# executable later by goleta_create_engine_tests().
function(goleta_collect_module_tests MODULE_NAME)
    set(_tests_dir "${CMAKE_CURRENT_SOURCE_DIR}/Tests")
    if(NOT EXISTS "${_tests_dir}")
        return()
    endif()
    file(GLOB_RECURSE _sources CONFIGURE_DEPENDS
        "${_tests_dir}/*.cpp"
        "${_tests_dir}/*.h"
        "${_tests_dir}/*.hpp"
    )
    if(NOT _sources)
        return()
    endif()
    set_property(GLOBAL APPEND PROPERTY GOLETA_TEST_SOURCES ${_sources})
    set_property(GLOBAL APPEND PROPERTY GOLETA_TEST_INCLUDE_DIRS ${_tests_dir})
endfunction()

# goleta_create_engine_tests()
#
# Call once, after all modules have been added. Pulls together every module's
# collected test sources into a single Engine.Tests executable linked against
# all registered modules.
function(goleta_create_engine_tests)
    get_property(_sources GLOBAL PROPERTY GOLETA_TEST_SOURCES)
    if(NOT _sources)
        return()
    endif()
    get_property(_include_dirs GLOBAL PROPERTY GOLETA_TEST_INCLUDE_DIRS)
    get_property(_modules      GLOBAL PROPERTY GOLETA_MODULES)

    set(_target Engine.Tests)
    add_executable(${_target} ${_sources})
    target_link_libraries(${_target} PRIVATE
        ${_modules}
        GTest::gtest
        GTest::gtest_main
    )
    target_include_directories(${_target} PRIVATE ${_include_dirs})
    target_compile_features(${_target} PRIVATE cxx_std_20)
    set_target_properties(${_target} PROPERTIES FOLDER "Tests")

    if(WIN32)
        add_custom_command(TARGET ${_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_RUNTIME_DLLS:${_target}>
                    $<TARGET_FILE_DIR:${_target}>
            COMMAND_EXPAND_LISTS
        )
    endif()

    gtest_discover_tests(${_target})
endfunction()
