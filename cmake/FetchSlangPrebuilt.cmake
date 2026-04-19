# FetchSlangPrebuilt.cmake
#
# Download a prebuilt Slang release from github.com/shader-slang/slang and expose an imported
# target Slang::Slang. By default the latest release is used; pin with GOLETA_SLANG_VERSION for
# reproducible CI (e.g. "v2024.14.1").
#
# Only loaded when GOLETA_BUILD_TESTS=ON; RHID3D12 itself has no Slang dependency.

function(goleta_fetch_slang_prebuilt)
    if(TARGET Slang::Slang)
        return()
    endif()

    set(_version "${GOLETA_SLANG_VERSION}")
    if(NOT _version OR _version STREQUAL "LATEST")
        set(_api_url "https://api.github.com/repos/shader-slang/slang/releases/latest")
        set(_manifest "${CMAKE_BINARY_DIR}/_deps/slang-release.json")
        file(DOWNLOAD "${_api_url}" "${_manifest}" TLS_VERIFY ON STATUS _status)
        list(GET _status 0 _rc)
        if(NOT _rc EQUAL 0)
            message(WARNING "Slang prebuilt: failed to query GitHub API: ${_status}")
            return()
        endif()
        file(READ "${_manifest}" _json)
        string(JSON _version GET "${_json}" tag_name)
    endif()

    if(NOT _version)
        message(WARNING "Slang prebuilt: could not resolve release tag; skipping.")
        return()
    endif()

    string(REGEX REPLACE "^v" "" _version_num "${_version}")

    # Platform/architecture asset selection.
    if(WIN32)
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(_asset "slang-${_version_num}-windows-x86_64.zip")
        else()
            set(_asset "slang-${_version_num}-windows-x86.zip")
        endif()
    elseif(APPLE)
        set(_asset "slang-${_version_num}-macos-aarch64.tar.gz")
    else()
        set(_asset "slang-${_version_num}-linux-x86_64.tar.gz")
    endif()

    set(_url "https://github.com/shader-slang/slang/releases/download/${_version}/${_asset}")
    set(_archive "${CMAKE_BINARY_DIR}/_deps/${_asset}")
    set(_extract_dir "${CMAKE_BINARY_DIR}/_deps/slang-prebuilt")

    if(NOT EXISTS "${_archive}")
        message(STATUS "Fetching Slang prebuilt ${_version} from ${_url}")
        file(DOWNLOAD "${_url}" "${_archive}" TLS_VERIFY ON SHOW_PROGRESS STATUS _dl_status)
        list(GET _dl_status 0 _dl_rc)
        if(NOT _dl_rc EQUAL 0)
            message(WARNING "Slang prebuilt: download failed: ${_dl_status}; D3D12 tests will be skipped.")
            return()
        endif()
    endif()

    if(NOT EXISTS "${_extract_dir}/include/slang.h")
        file(MAKE_DIRECTORY "${_extract_dir}")
        file(ARCHIVE_EXTRACT INPUT "${_archive}" DESTINATION "${_extract_dir}")
    endif()

    # The layout after extraction is either {extract_dir}/slang-<version>/... or
    # {extract_dir}/... directly depending on the archive. Resolve once.
    file(GLOB _root_candidates LIST_DIRECTORIES true "${_extract_dir}/*")
    set(_slang_root "${_extract_dir}")
    foreach(_cand IN LISTS _root_candidates)
        if(EXISTS "${_cand}/include/slang.h")
            set(_slang_root "${_cand}")
            break()
        endif()
    endforeach()
    if(NOT EXISTS "${_slang_root}/include/slang.h")
        message(WARNING "Slang prebuilt: include/slang.h not found under ${_slang_root}; skipping.")
        return()
    endif()

    # Locate the import library (Windows) / shared object (Linux/macOS).
    if(WIN32)
        file(GLOB_RECURSE _slang_lib "${_slang_root}/lib/*slang.lib")
        file(GLOB_RECURSE _slang_dll "${_slang_root}/bin/*slang.dll")
    elseif(APPLE)
        file(GLOB_RECURSE _slang_dll "${_slang_root}/lib/libslang.dylib")
        set(_slang_lib "${_slang_dll}")
    else()
        file(GLOB_RECURSE _slang_dll "${_slang_root}/lib/libslang.so")
        set(_slang_lib "${_slang_dll}")
    endif()
    list(LENGTH _slang_dll _nrd)
    if(_nrd EQUAL 0)
        message(WARNING "Slang prebuilt: runtime library not located under ${_slang_root}; skipping.")
        return()
    endif()
    list(GET _slang_dll 0 _slang_runtime)
    list(GET _slang_lib 0 _slang_import)

    add_library(Slang::Slang SHARED IMPORTED GLOBAL)
    set_target_properties(Slang::Slang PROPERTIES
        IMPORTED_LOCATION "${_slang_runtime}"
        INTERFACE_INCLUDE_DIRECTORIES "${_slang_root}/include")
    if(WIN32)
        set_target_properties(Slang::Slang PROPERTIES IMPORTED_IMPLIB "${_slang_import}")
    endif()

    set(GOLETA_SLANG_BIN_DIR "${_slang_root}/bin" CACHE INTERNAL "Slang prebuilt bin directory")
    set(GOLETA_SLANG_RESOLVED_VERSION "${_version}" CACHE INTERNAL "Resolved Slang release version")

    # Slang pass-through to DXIL requires dxcompiler.dll + dxil.dll from the Windows SDK.
    # Without them, compile to DXIL fails with 'failed to load downstream compiler dxc'.
    if(WIN32)
        set(_wsdk_bin "")
        if(DEFINED ENV{WindowsSdkBinPath} AND DEFINED ENV{WindowsSDKVersion})
            set(_wsdk_bin "$ENV{WindowsSdkBinPath}$ENV{WindowsSDKVersion}x64")
        elseif(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION)
            set(_wsdk_bin "$ENV{ProgramFiles\(x86\)}/Windows Kits/10/bin/${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}/x64")
        else()
            file(GLOB _wsdk_candidates "C:/Program Files (x86)/Windows Kits/10/bin/10.*/x64")
            if(_wsdk_candidates)
                list(SORT _wsdk_candidates)
                list(REVERSE _wsdk_candidates)
                list(GET _wsdk_candidates 0 _wsdk_bin)
            endif()
        endif()
        if(EXISTS "${_wsdk_bin}/dxcompiler.dll" AND EXISTS "${_wsdk_bin}/dxil.dll")
            file(COPY "${_wsdk_bin}/dxcompiler.dll" "${_wsdk_bin}/dxil.dll"
                 DESTINATION "${GOLETA_SLANG_BIN_DIR}")
            message(STATUS "Slang prebuilt: staged dxcompiler.dll + dxil.dll from ${_wsdk_bin}")
        else()
            message(WARNING "Slang prebuilt: dxcompiler.dll / dxil.dll not found under '${_wsdk_bin}'; "
                            "DXIL compilation will fail at test runtime.")
        endif()
    endif()
endfunction()
