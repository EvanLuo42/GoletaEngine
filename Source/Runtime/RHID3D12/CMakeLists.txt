if(NOT WIN32)
    return()
endif()

goleta_add_module(RHID3D12
    DEPENDENCIES RHI Core
)

target_link_libraries(RHID3D12 PRIVATE
    d3d12
    dxgi
    dxguid
)

if(TARGET D3D12MemoryAllocator)
    target_link_libraries(RHID3D12 PRIVATE D3D12MemoryAllocator)
    target_compile_definitions(RHID3D12 PRIVATE GOLETA_RHID3D12_HAS_D3D12MA=1)
else()
    target_compile_definitions(RHID3D12 PRIVATE GOLETA_RHID3D12_HAS_D3D12MA=0)
    message(WARNING
        "RHID3D12: D3D12MemoryAllocator submodule not available. "
        "Run: git submodule update --init --recursive "
        "(requires a working SSH key for github.com). "
        "Falling back to CreateCommittedResource-only allocation path.")
endif()

target_compile_definitions(RHID3D12 PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
    _WIN32_WINNT=0x0A00
    INITGUID
)

# Tests folder pulls in Slang prebuilt if available. We can't declare a second target for the
# tests in the module CMake (tests are compiled into Engine.Tests), so we expose the needed
# defines through a test-scope interface target and copy shader fixtures to the build output.
if(GOLETA_BUILD_TESTS)
    # Cross-backend harness under RHI/Tests/Common calls forceLinkRhiD3D12Backend(); signal
    # availability via a globally-propagated compile define.
    set_property(GLOBAL APPEND PROPERTY GOLETA_TEST_EXTRA_DEFS GOLETA_RHID3D12_HAS_FORCELINK=1)
    if(TARGET Slang::Slang)
        set_property(GLOBAL APPEND PROPERTY GOLETA_TEST_EXTRA_LIBS Slang::Slang)
        set_property(GLOBAL APPEND PROPERTY GOLETA_TEST_EXTRA_DEFS GOLETA_RHID3D12_HAS_SLANG=1)
    endif()
endif()
