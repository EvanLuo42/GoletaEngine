include(GenerateExportHeader)

# goleta_add_module(<ModuleName>
#   [DEPENDENCIES <dep1> <dep2> ...]
# )
#
# Expects the calling CMakeLists.txt to be at Source/Runtime/<ModuleName>/
# with subdirectories Public/, Internal/, Private/ (all optional).
#
# Public/   -> exported API; include path and contents both visible downstream.
# Internal/ -> also on the PUBLIC include path so inline chains in Public/
#              headers resolve, but documented as implementation detail --
#              downstream code should not include Internal/ headers directly
#              and is not installed with the module.
# Private/  -> sources and headers only visible to this module's TUs.
#
function(goleta_add_module MODULE_NAME)
    cmake_parse_arguments(ARG "" "" "DEPENDENCIES" ${ARGN})

    if(GOLETA_BUILD_SHARED)
        set(_lib_type SHARED)
    else()
        set(_lib_type STATIC)
    endif()

    file(GLOB_RECURSE _public_sources
        "${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Public/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Public/*.cpp"
    )
    file(GLOB_RECURSE _internal_sources
        "${CMAKE_CURRENT_SOURCE_DIR}/Internal/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Internal/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Internal/*.inl"
        "${CMAKE_CURRENT_SOURCE_DIR}/Internal/*.cpp"
    )
    file(GLOB_RECURSE _private_sources
        "${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/Private/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp"
    )

    add_library(${MODULE_NAME} ${_lib_type}
        ${_public_sources}
        ${_internal_sources}
        ${_private_sources}
    )

    target_include_directories(${MODULE_NAME}
        PUBLIC
            "${CMAKE_CURRENT_SOURCE_DIR}/Public"
            "${CMAKE_CURRENT_SOURCE_DIR}/Internal"
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/Private"
    )

    string(TOUPPER "${MODULE_NAME}" _upper_name)
    generate_export_header(${MODULE_NAME}
        EXPORT_MACRO_NAME "${_upper_name}_API"
        EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/Public/${MODULE_NAME}Export.h"
        STATIC_DEFINE "${_upper_name}_STATIC"
    )

    if(NOT GOLETA_BUILD_SHARED)
        target_compile_definitions(${MODULE_NAME}
            PUBLIC "${_upper_name}_STATIC"
        )
    endif()

    if(ARG_DEPENDENCIES)
        target_link_libraries(${MODULE_NAME} PUBLIC ${ARG_DEPENDENCIES})
    endif()

    target_compile_features(${MODULE_NAME} PUBLIC cxx_std_20)

    install(TARGETS ${MODULE_NAME}
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/Public/"
        DESTINATION "include"
        FILES_MATCHING
            PATTERN "*.h"
            PATTERN "*.hpp"
    )

    set_property(GLOBAL APPEND PROPERTY GOLETA_MODULES ${MODULE_NAME})

    if(GOLETA_BUILD_TESTS)
        goleta_collect_module_tests(${MODULE_NAME})
    endif()
endfunction()
