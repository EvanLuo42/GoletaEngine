goleta_add_module(RHI
    DEPENDENCIES Core
)

# Cross-backend test suite ships its .slang fixtures here; the Engine.Tests POST_BUILD step
# picks them up via the GOLETA_TEST_SHADER_DIRS global property and stages them into
# ${exe_dir}/TestShaders/.
if(GOLETA_BUILD_TESTS)
    set(_rhi_shader_src "${CMAKE_CURRENT_SOURCE_DIR}/Tests/Shaders")
    if(EXISTS "${_rhi_shader_src}")
        set_property(GLOBAL APPEND PROPERTY GOLETA_TEST_SHADER_DIRS "${_rhi_shader_src}")
    endif()
endif()
